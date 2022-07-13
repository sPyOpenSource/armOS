/*
 * unix_io.c --- This is the Unix (well, really POSIX) implementation
 * 	of the I/O manager.
 *
 * Implements a one-block write-through cache.
 *
 * Includes support for Windows NT support under Cygwin. 
 *
 * Copyright (C) 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001,
 * 	2002 by Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "partition.h"
#include "device.h"
#include "vfs.h"
#include "ext2.h"
#include "ext2fs.h"

/*
 * For checking structure magic numbers...
 */

#define EXT2_CHECK_MAGIC(struct, code) \
	  if ((struct)->magic != (code)) return (code)

struct unix_cache {
	char		*buf;
	unsigned long	block;
	int		access_time;
	int		dirty:1;
	int		in_use:1;
};

#define CACHE_SIZE 8
#define WRITE_DIRECT_SIZE 4	/* Must be smaller than CACHE_SIZE */
#define READ_DIRECT_SIZE 4	/* Should be smaller than CACHE_SIZE */

struct unix_private_data {
	int	magic;
	VFS::Device *dev;
	int	flags;
	int	access_time;
	struct unix_cache cache[CACHE_SIZE];
};

static errcode_t unix_open(VFS::Device *dev, io_channel *channel);
static errcode_t unix_close(io_channel channel);
static errcode_t unix_set_blksize(io_channel channel, int blksize);
static errcode_t unix_read_blk(io_channel channel, unsigned long block,
			       int count, void *data);
static errcode_t unix_write_blk(io_channel channel, unsigned long block,
				int count, const void *data);
static errcode_t unix_flush(io_channel channel);
static errcode_t unix_write_byte(io_channel channel, unsigned long offset,
				int size, const void *data);

static void reuse_cache(io_channel channel, struct unix_private_data *data,
		 struct unix_cache *cache, unsigned long block);

static struct struct_io_manager struct_unix_manager = {
	EXT2_ET_MAGIC_IO_MANAGER,
	"Unix I/O Manager",
	unix_open,
	unix_close,
	unix_set_blksize,
	unix_read_blk,
	unix_write_blk,
	unix_flush,
	0
};

io_manager unix_io_manager = &struct_unix_manager;

/*
 * Here are the raw I/O functions
 */
static errcode_t raw_read_blk(io_channel channel,
			      struct unix_private_data *data,
			      unsigned long block,
			      int count, void *buf)
{
	errcode_t	retval;
	u32		size;
	u64		location;
	int		actual = 0;
        int		ret;

        size = (count < 0) ? -count : count * channel->block_size;
	location = block * channel->block_size;
	if ((ret = data->dev->lseek(location, SEEK_SET)) != (s64)location) {
		retval = (ret < 0) ? -ret : EXT2_ET_LLSEEK_FAILED;
		goto error_out;
	}
	actual = data->dev->read(buf, size);
	if (actual != (s32)size) {
		if (actual < 0)
			actual = 0;
		retval = EXT2_ET_SHORT_READ;
		goto error_out;
	}
	return 0;
	
error_out:
	memset((char *) buf+actual, 0, size-actual);
	if (channel->read_error)
		retval = (channel->read_error)(channel, block, count, buf,
					       size, actual, retval);
	return retval;
}

static errcode_t raw_write_blk(io_channel channel,
			       struct unix_private_data *data,
			       unsigned long block,
			       int count, const void *buf)
{
	u32		size;
	u64		location;
	int		actual = 0;
	errcode_t	retval;
        s64		ret;

	if (count == 1)
		size = channel->block_size;
	else {
		if (count < 0)
			size = -count;
		else
			size = count * channel->block_size;
	}

	location = (u64) block * channel->block_size;
	if ((ret = data->dev->lseek(location, SEEK_SET)) != (s64)location) {
		retval = (ret < 0) ? -ret : EXT2_ET_LLSEEK_FAILED;
		goto error_out;
	}
	
	actual = data->dev->write(buf, size);
	if (actual != (s32)size) {
		retval = EXT2_ET_SHORT_WRITE;
		goto error_out;
	}
	return 0;
	
error_out:
	if (channel->write_error)
		retval = (channel->write_error)(channel, block, count, buf,
						size, actual, retval);
	return retval;
}


/*
 * Here we implement the cache functions
 */

/* Allocate the cache buffers */
static errcode_t alloc_cache(io_channel channel,
			     struct unix_private_data *data)
{
	errcode_t		retval;
	struct unix_cache	*cache;
	int			i;
	
	data->access_time = 0;
	for (i=0, cache = data->cache; i < CACHE_SIZE; i++, cache++) {
		cache->block = 0;
		cache->access_time = 0;
		cache->dirty = 0;
		cache->in_use = 0;
		if ((retval = ext2fs_get_mem(channel->block_size,
					     &cache->buf)))
			return retval;
	}
	return 0;
}

/* Free the cache buffers */
static void free_cache(struct unix_private_data *data)
{
	struct unix_cache	*cache;
	int			i;
	
	data->access_time = 0;
	for (i=0, cache = data->cache; i < CACHE_SIZE; i++, cache++) {
		cache->block = 0;
		cache->access_time = 0;
		cache->dirty = 0;
		cache->in_use = 0;
		if (cache->buf)
			ext2fs_free_mem(&cache->buf);
		cache->buf = 0;
	}
}

#ifndef NO_IO_CACHE
/*
 * Try to find a block in the cache.  If the block is not found, and
 * eldest is a non-zero pointer, then fill in eldest with the cache
 * entry to that should be reused.
 */
static struct unix_cache *find_cached_block(struct unix_private_data *data,
					    unsigned long block,
					    struct unix_cache **eldest)
{
	struct unix_cache	*cache, *unused_cache, *oldest_cache;
	int			i;
	
	unused_cache = oldest_cache = 0;
	for (i=0, cache = data->cache; i < CACHE_SIZE; i++, cache++) {
		if (!cache->in_use) {
			if (!unused_cache)
				unused_cache = cache;
			continue;
		}
		if (cache->block == block) {
			cache->access_time = ++data->access_time;
			return cache;
		}
		if (!oldest_cache ||
		    (cache->access_time < oldest_cache->access_time))
			oldest_cache = cache;
	}
	if (eldest)
		*eldest = (unused_cache) ? unused_cache : oldest_cache;
	return 0;
}

/*
 * Reuse a particular cache entry for another block.
 */
static void reuse_cache(io_channel channel, struct unix_private_data *data,
		 struct unix_cache *cache, unsigned long block)
{
	if (cache->dirty && cache->in_use)
		raw_write_blk(channel, data, cache->block, 1, cache->buf);

	cache->in_use = 1;
	cache->dirty = 0;
	cache->block = block;
	cache->access_time = ++data->access_time;
}

/*
 * Flush all of the blocks in the cache
 */
static errcode_t flush_cached_blocks(io_channel channel,
				     struct unix_private_data *data,
				     int invalidate)

{
	struct unix_cache	*cache;
	errcode_t		retval, retval2;
	int			i;
	
	retval2 = 0;
	for (i=0, cache = data->cache; i < CACHE_SIZE; i++, cache++) {
		if (!cache->in_use)
			continue;
		
		if (invalidate)
			cache->in_use = 0;
		
		if (!cache->dirty)
			continue;
		
		retval = raw_write_blk(channel, data,
				       cache->block, 1, cache->buf);
		if (retval)
			retval2 = retval;
		else
			cache->dirty = 0;
	}
	return retval2;
}
#endif /* NO_IO_CACHE */

static errcode_t unix_open(VFS::Device *dev, io_channel *channel)
{
	io_channel	io = NULL;
	struct unix_private_data *data = NULL;
	errcode_t	retval;
        const char *name = "<rawpod device>";

	if (dev == 0)
		return EXT2_ET_BAD_DEVICE_NAME;
	retval = ext2fs_get_mem(sizeof(struct struct_io_channel), &io);
	if (retval)
		return retval;
	memset(io, 0, sizeof(struct struct_io_channel));
	io->magic = EXT2_ET_MAGIC_IO_CHANNEL;
	retval = ext2fs_get_mem(sizeof(struct unix_private_data), &data);
	if (retval)
		goto cleanup;

	io->manager = unix_io_manager;
	retval = ext2fs_get_mem(strlen(name)+1, &io->name);
	if (retval)
		goto cleanup;

	strcpy(io->name, name);
	io->private_data = data;
	io->block_size = 1024;
	io->read_error = 0;
	io->write_error = 0;
	io->refcount = 1;

	memset(data, 0, sizeof(struct unix_private_data));
	data->magic = EXT2_ET_MAGIC_UNIX_IO_CHANNEL;

	if ((retval = alloc_cache(io, data)))
		goto cleanup;
	
	data->dev = dev;
	*channel = io;
	return 0;

cleanup:
	if (data) {
		free_cache(data);
		ext2fs_free_mem(&data);
	}
	if (io)
		ext2fs_free_mem(&io);
	return retval;
}

static errcode_t unix_close(io_channel channel)
{
	struct unix_private_data *data;
	errcode_t	retval = 0;

	EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);
	data = (struct unix_private_data *) channel->private_data;
	EXT2_CHECK_MAGIC(data, EXT2_ET_MAGIC_UNIX_IO_CHANNEL);

	if (--channel->refcount > 0)
		return 0;

#ifndef NO_IO_CACHE
	retval = flush_cached_blocks(channel, data, 0);
#endif

        delete data->dev;
	free_cache(data);

	ext2fs_free_mem(&channel->private_data);
	if (channel->name)
		ext2fs_free_mem(&channel->name);
	ext2fs_free_mem(&channel);
	return retval;
}

static errcode_t unix_set_blksize(io_channel channel, int blksize)
{
	struct unix_private_data *data;
	errcode_t		retval;

	EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);
	data = (struct unix_private_data *) channel->private_data;
	EXT2_CHECK_MAGIC(data, EXT2_ET_MAGIC_UNIX_IO_CHANNEL);

	if (channel->block_size != blksize) {
#ifndef NO_IO_CACHE
		if ((retval = flush_cached_blocks(channel, data, 0)))
			return retval;
#endif
		
		channel->block_size = blksize;
		free_cache(data);
		if ((retval = alloc_cache(channel, data)))
			return retval;
	}
	return 0;
}


static errcode_t unix_read_blk(io_channel channel, unsigned long block,
			       int count, void *buf)
{
	struct unix_private_data *data;
	struct unix_cache *cache, *reuse[READ_DIRECT_SIZE];
	errcode_t	retval;
	char		*cp;
	int		i, j;

	EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);
	data = (struct unix_private_data *) channel->private_data;
	EXT2_CHECK_MAGIC(data, EXT2_ET_MAGIC_UNIX_IO_CHANNEL);

#ifdef NO_IO_CACHE
	return raw_read_blk(channel, data, block, count, buf);
#else
	/*
	 * If we're doing an odd-sized read or a very large read,
	 * flush out the cache and then do a direct read.
	 */
	if (count < 0 || count > WRITE_DIRECT_SIZE) {
		if ((retval = flush_cached_blocks(channel, data, 0)))
			return retval;
		return raw_read_blk(channel, data, block, count, buf);
	}

	cp = (char *)buf;
	while (count > 0) {
		/* If it's in the cache, use it! */
		if ((cache = find_cached_block(data, block, &reuse[0]))) {
#ifdef DEBUG
			printf("Using cached block %d\n", block);
#endif
			memcpy(cp, cache->buf, channel->block_size);
			count--;
			block++;
			cp += channel->block_size;
			continue;
		}
		/*
		 * Find the number of uncached blocks so we can do a
		 * single read request
		 */
		for (i=1; i < count; i++)
			if (find_cached_block(data, block+i, &reuse[i]))
				break;
#ifdef DEBUG
		printf("Reading %d blocks starting at %d\n", i, block);
#endif
		if ((retval = raw_read_blk(channel, data, block, i, cp)))
			return retval;
		
		/* Save the results in the cache */
		for (j=0; j < i; j++) {
			count--;
			cache = reuse[j];
			reuse_cache(channel, data, cache, block++);
			memcpy(cache->buf, cp, channel->block_size);
			cp += channel->block_size;
		}
	}
	return 0;
#endif /* NO_IO_CACHE */
}

static errcode_t unix_write_blk(io_channel channel, unsigned long block,
				int count, const void *buf)
{
	struct unix_private_data *data;
	struct unix_cache *cache, *reuse;
	errcode_t	retval = 0;
	const char	*cp;
	int		writethrough;

	EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);
	data = (struct unix_private_data *) channel->private_data;
	EXT2_CHECK_MAGIC(data, EXT2_ET_MAGIC_UNIX_IO_CHANNEL);

#ifdef NO_IO_CACHE
	return raw_write_blk(channel, data, block, count, buf);
#else	
	/*
	 * If we're doing an odd-sized write or a very large write,
	 * flush out the cache completely and then do a direct write.
	 */
	if (count < 0 || count > WRITE_DIRECT_SIZE) {
		if ((retval = flush_cached_blocks(channel, data, 1)))
			return retval;
		return raw_write_blk(channel, data, block, count, buf);
	}

	/*
	 * For a moderate-sized multi-block write, first force a write
	 * if we're in write-through cache mode, and then fill the
	 * cache with the blocks.
	 */
	writethrough = channel->flags & CHANNEL_FLAGS_WRITETHROUGH;
	if (writethrough)
		retval = raw_write_blk(channel, data, block, count, buf);
	
	cp = (const char *)buf;
	while (count > 0) {
		cache = find_cached_block(data, block, &reuse);
		if (!cache) {
			cache = reuse;
			reuse_cache(channel, data, cache, block);
		}
		memcpy(cache->buf, cp, channel->block_size);
		cache->dirty = !writethrough;
		count--;
		block++;
		cp += channel->block_size;
	}
	return retval;
#endif /* NO_IO_CACHE */
}

static errcode_t unix_write_byte(io_channel channel, unsigned long offset,
				 int size, const void *buf)
{
	struct unix_private_data *data;
	errcode_t	retval = 0;
	ssize_t		actual;

	EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);
	data = (struct unix_private_data *) channel->private_data;
	EXT2_CHECK_MAGIC(data, EXT2_ET_MAGIC_UNIX_IO_CHANNEL);

#ifndef NO_IO_CACHE
	/*
	 * Flush out the cache completely
	 */
	if ((retval = flush_cached_blocks(channel, data, 1)))
		return retval;
#endif

	if (data->dev->lseek(offset, SEEK_SET) < 0)
		return EXT2_ET_LLSEEK_FAILED;
	
	actual = data->dev->write(buf, size);
	if (actual != size)
		return EXT2_ET_SHORT_WRITE;

	return 0;
}

/*
 * Flush data buffers to disk.  
 */
static errcode_t unix_flush(io_channel channel)
{
	struct unix_private_data *data;
	errcode_t retval = 0;
	
	EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);
	data = (struct unix_private_data *) channel->private_data;
	EXT2_CHECK_MAGIC(data, EXT2_ET_MAGIC_UNIX_IO_CHANNEL);

#ifndef NO_IO_CACHE
	retval = flush_cached_blocks(channel, data, 0);
#endif
	return retval;
}

