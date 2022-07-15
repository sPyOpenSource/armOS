/* kernel/vfs.h -*- C++ -*- Copyright (c) 2005 Joshua Oreman
 * XenOS, of which this file is a part, is licensed under the
 * GNU General Public License. See the file COPYING in the
 * source distribution for details.
 */

#ifndef _VFS_H_
#define _VFS_H_

#ifdef __CYGWIN32__
#define WIN32
#endif

#define _FILE_OFFSET_BITS 64
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include "errors.h"
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifndef WIN32
#include <sys/param.h>
#endif
#undef RAWPOD_BIG_ENDIAN
#undef FIGURED_OUT_ENDIAN
#ifdef __BYTE_ORDER
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define FIGURED_OUT_ENDIAN
# else
#  if __BYTE_ORDER == __BIG_ENDIAN
#   define RAWPOD_BIG_ENDIAN
#   define FIGURED_OUT_ENDIAN
#  endif
# endif
#endif
#ifndef FIGURED_OUT_ENDIAN
# if defined(i386) || defined(__i386__) || defined(_M_IX86) || defined(vax) || defined(__alpha)
#  define FIGURED_OUT_ENDIAN
# elif (defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)) || defined(__BIG_ENDIAN__) || defined (__ppc__) || defined(__powerpc__)
#  define RAWPOD_BIG_ENDIAN
#  define FIGURED_OUT_ENDIAN
# endif
#endif 
#ifndef FIGURED_OUT_ENDIAN
# error Could not figure out endian.
#endif

#ifndef DONT_REDEFINE_OPEN_CONSTANTS
#undef O_RDONLY
#undef O_WRONLY
#undef O_RDWR
#undef O_CREAT
#undef O_APPEND
#undef O_EXCL
#undef O_TRUNC
#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR   02
#define O_CREAT  04
#define O_APPEND 010
#define O_EXCL   020
#define O_TRUNC  040
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long ulong;
typedef unsigned long long u64;
typedef   signed char s8;
typedef   signed short s16;
typedef   signed int s32;
typedef   signed long slong;
typedef   signed long long s64;

#ifdef __APPLE__
typedef off_t loff_t;
#endif

#ifdef st_atime
#define ST_XTIME_ARE_MACROS
#endif
#undef st_atime
#undef st_ctime
#undef st_mtime

struct my_stat 
{
    u32           st_dev;      /* device */
    u32           st_ino;      /* inode */
    u32           st_mode;     /* protection */
    u32           st_nlink;    /* number of hard links */
    u16           st_uid;      /* user ID of owner */
    u16           st_gid;      /* group ID of owner */
    u32           st_rdev;     /* device type (if inode device) */
    u64           st_size;     /* total size, in bytes */
    u32           st_blksize;  /* blocksize for filesystem I/O */
    u32           st_blocks;   /* number of blocks allocated */
    u32           st_atime;    /* time of last access */
    u32           st_mtime;    /* time of last modification */
    u32           st_ctime;    /* time of last status change */    
};

#ifndef S_IFMT
#define S_IFMT     0170000
#define S_IFSOCK   0140000
#define S_IFLNK    0120000
#define S_IFREG    0100000
#define S_IFBLK    0060000
#define S_IFDIR    0040000
#define S_IFCHR    0020000
#define S_IFIFO    0010000
#define S_ISUID    0004000
#define S_ISGID    0002000
#define S_ISVTX    0001000
#define S_IRWXU    00700
#define S_IRUSR    00400
#define S_IWUSR    00200
#define S_IXUSR    00100
#define S_IRWXG    00070
#define S_IRGRP    00040
#define S_IWGRP    00020
#define S_IXGRP    00010
#define S_IRWXO    00007
#define S_IROTH    00004
#define S_IWOTH    00002
#define S_IXOTH    00001

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#endif

namespace VFS
{
    class Device 
    {
    public:
	Device() {}
	virtual ~Device() {}
	
	virtual int read (void *buf, int n) = 0;
	virtual int write (const void *buf, int n) = 0;
	virtual s64 lseek (s64 off, int whence) { (void)off, (void)whence; return -1; }
    };

    class BlockDevice : public Device
    {
    public:
        BlockDevice()
            : Device(), _pos (0), _blocksize (512), _blocksize_bits (512), _blocks (0)
        {}
        virtual ~BlockDevice() {}

        virtual int read (void *buf, int n);
        virtual int write (const void *buf, int n);
        virtual s64 lseek (s64 off, int whence);
        u64 size() { return _blocks; }
        int blocksize() { return _blocksize; }
        int blocksizeBits() { return _blocksize_bits; }

    protected:
        virtual int doRead (void *buf, u64 sec) = 0;
        virtual int doWrite (const void *buf, u64 sec) = 0;

        void setBlocksize (int bsz) {
            int bit = 1;
            _blocksize = bsz;
            _blocksize_bits = 0;
            while (!(_blocksize & bit)) {
                _blocksize_bits++;
                bit <<= 1;
            }
            if (_blocksize & ~(1 << _blocksize_bits)) {
                printf ("Unaligned blocksize, expect trouble\n");
            }
        }
        void setSize (u64 blocks) { _blocks = blocks; }

    protected:
        u64 _pos;
        int _blocksize, _blocksize_bits;
        u64 _blocks;
    };

    class File
    {
    public:
	File() {}
	virtual ~File() {}
	
	virtual int read (void *buf, int n) = 0;
	virtual int write (const void *buf, int n) = 0;
	virtual s64 lseek (s64 off, int whence) { (void)off, (void)whence; return -ESPIPE; }
        virtual int error() { return 0; }
        virtual int chown (int uid, int gid = -1) { (void)uid, (void)gid; return -EPERM; }
        virtual int chmod (int mode) { (void)mode; return -EPERM; }
        virtual int truncate() { return -EROFS; }
        virtual int stat (struct my_stat *st) { (void)st; return -ENOSYS; }

        virtual int close() { return 0; }
    };

    class DeviceFile : public File 
    {
    public:
        DeviceFile (Device *dev) : _dev (dev) { _dev->lseek (0, SEEK_SET); }
        virtual ~DeviceFile() {}

        virtual int read (void *buf, int n) { return _dev->read (buf, n); }
        virtual int write (const void *buf, int n) { return _dev->write (buf, n); }
        virtual s64 lseek (s64 off, int whence) { return _dev->lseek (off, whence); }
        virtual int error() { return 0; }
        virtual int close() { return 0; }

    protected:
        Device *_dev;
    };

    class LoopbackDevice : public Device 
    {
    public:
        LoopbackDevice (File *f) : _f (f) {}
        virtual ~LoopbackDevice() { _f->close(); }

        virtual int read (void *buf, int n) { return _f->read (buf, n); }
        virtual int write (const void *buf, int n) { return _f->write (buf, n); }
        virtual s64 lseek (s64 off, int whence) { return _f->lseek (off, whence); }

    protected:
        File *_f;
    };

    class ErrorFile : public File
    {
    public:
        ErrorFile (int err) { _err = err; if (_err < 0) _err = -_err; }
        int read (void *buf, int n) { (void)buf, (void)n; return -EBADF; }
        int write (const void *buf, int n) { (void)buf, (void)n; return -EBADF; }
        int error() { return _err; }

    private:
        int _err;
    };

    class BlockFile : public File
    {
    public:
        BlockFile (int blocksize, int bits) 
            : _blocksize (blocksize), _blocksize_bits (bits), _pos (0), _size (0)
        {}

        virtual ~BlockFile() {}

	virtual int read (void *buf, int n);
	virtual int write (const void *buf, int n);
	virtual s64 lseek (s64 off, int whence);

    protected:
        virtual int readblock (void *buf, u32 block) = 0;
        virtual int writeblock (void *buf, u32 block) = 0;

        int _blocksize, _blocksize_bits;
        u64 _pos, _size;
    };

    struct dirent 
    {
	u32 d_ino;
	char d_name[256];
    };

    class Dir
    {
    public:
	Dir() {}
	virtual ~Dir() {}

	virtual int readdir (struct dirent *buf) = 0;
	virtual int close() { return 0; }
        virtual int error() { return 0; }
    };

    class ErrorDir : public Dir
    {
    public:
        ErrorDir (int err) { _err = err; if (_err < 0) _err = -_err; }
        int readdir (struct dirent *de) { (void)de; return -EBADF; }
        int error() { return _err; }

    private:
        int _err;
    };

    class Filesystem
    {
    public:
	Filesystem (Device *d)
	    : _device (d)
	{}
	virtual ~Filesystem() {}

        // Init function - called to actually init the fs
        // Not in constructor so we can return error codes.
        virtual int init() = 0;
	
        // Sense function - returns 1 if dev contains this fs
        static int probe (Device *dev) { (void)dev; return 0; };
        
	// Opening functions - mandatory
	virtual File *open (const char *path, int flags) = 0;
	virtual Dir *opendir (const char *path) = 0;

	// Creating/destroying functions - optional.
        // EPERM = "Filesystem does not support blah-blah."
	virtual int mkdir (const char *path) { (void)path; return -EPERM; }
	virtual int rmdir (const char *path) { (void)path; return -EPERM; }
	virtual int unlink (const char *path) { (void)path; return -EPERM; }

	// Other functions - optional
        virtual int rename (const char *oldpath, const char *newpath) { (void)oldpath, (void)newpath; return -EROFS; }
	virtual int link (const char *oldpath, const char *newpath) { (void)oldpath, (void)newpath; return -EPERM; }
	virtual int symlink (const char *dest, const char *path) { (void)dest, (void)path; return -EPERM; }
        virtual int readlink (const char *path, char *buf, int len) { (void)path, (void)buf, (void)len; return -EINVAL; }
        virtual int chmod (const char *dest, int mode) { (void)dest, (void)mode; return -EPERM; }
        virtual int chown (const char *dest, int uid, int gid) { (void)dest, (void)uid, (void)gid; return -EPERM; }
        virtual int stat (const char *path, struct my_stat *st) {
            File *fp = open (path, O_RDONLY);
            int err = 0;
            if (fp->error())
                err = -fp->error();
            else
                err = fp->stat (st);
            fp->close();
            delete fp;
            return err;
        }
        virtual int lstat (const char *path, struct my_stat *st) {
            return stat (path, st);
        }

    protected:
	Device *_device;
    };

    class MountedFilesystem : public Filesystem
    {
    public:
        MountedFilesystem (const char *root)
            : Filesystem (0), _root (root)
        {}
        virtual ~MountedFilesystem() {}

        virtual int init() { return 0; }

        virtual File *open (const char *path, int flags);
        virtual Dir *opendir (const char *path);
        
#ifndef WIN32
        virtual int mkdir (const char *path) { if (::mkdir (_resolve (path), 0755) < 0) return -errno; return 0; }
        virtual int rmdir (const char *path) { if (::rmdir (_resolve (path)) < 0) return -errno; return 0; }
        virtual int unlink (const char *path) { if (::unlink (_resolve (path)) < 0) return -errno; return 0; }

        virtual int rename (const char *Old, const char *New)
        { if (::rename (_resolve (Old), _resolve (New)) < 0) return -errno; return 0; }

        virtual int link (const char *Old, const char *New)
        { if (::link (_resolve (Old), _resolve (New)) < 0) return -errno; return 0; }

        virtual int symlink (const char *Old, const char *New)
        { if (::symlink (Old, _resolve (New)) < 0) return -errno; return 0; }

        virtual int readlink (const char *path, char *buf, int len)
        { if (::readlink (_resolve (path), buf, len) < 0) return -errno; return 0; }

        virtual int chmod (const char *dest, int mode)
        { if (::chmod (_resolve (dest), mode) < 0) return -errno; return 0; }

        virtual int chown (const char *dest, int uid, int gid)
        { if (::chown (_resolve (dest), uid, gid) < 0) return -errno; return 0; }

        virtual int stat (const char *path, struct my_stat *st) {
            struct stat s;
            if (::stat (_resolve (path), &s) < 0) return -errno;
            convert_stat (&s, st);
            return 0;
        }

        virtual int lstat (const char *path, struct my_stat *st) {
            struct stat s;
            if (::lstat (_resolve (path), &s) < 0) return -errno;
            convert_stat (&s, st);
            return 0;
        }
#endif

    protected:
        const char *_root;
        struct QuickConcatenator 
        {
            char *data;
            QuickConcatenator (const char *first, const char *second) {
                data = (char *)malloc (strlen (first) + strlen (second) + 1);
                strcpy (data, first);
                strcat (data, second);
            }
            ~QuickConcatenator() { free (data); }
            operator const char*() { return data; }
        };
        QuickConcatenator _resolve (const char *path) { return QuickConcatenator (_root, path); }
#ifndef WIN32
        void convert_stat (struct stat *s, struct my_stat *st);  
#endif
    };
};

#endif
