/* ipl/installer/firmware.cc -*- C++ -*- Copyright (c) 2006 Joshua Oreman
 * Distributed under the GPL. See COPYING for details.
 */

#include "installer.h"
#include "actions.h"
#include "panes.h"
#include "rawpod/partition.h"
#include "rawpod/vfs.h"
#include "rawpod/device.h"
#include "rawpod/fat32.h"
#include "rawpod/ext2.h"
#include <setjmp.h>
#include "make_fw2.h"

#include <QMessageBox>

#define FATAL_T(str) do { emit fatalError(str); while(1); } while(0)
#define FATAL(str) FATAL_T(tr(str))

int rawpod_open (fw_fileops *fo, const char *name, int writing) 
{
    VFS::File *fh = 0;
    if (name[0] == '<') {
        int devnr = 0, part = 0;
        devnr = atoi (name + 1);
        if (strchr (name, ',')) {
            part = atoi (strrchr (name, ',') + 1);
        }
        VFS::Device *dev;
        if (devnr == iPodLocation) {
            switch (part) {
            case 0:
                dev = iPodDevice;
                break;
            case 1:
                dev = iPodFirmwarePartitionDevice;
                break;
            case 2:
                dev = iPodMusicPartitionDevice;
                break;
            case 3:
                dev = iPodLinuxPartitionDevice;
                break;
            default:
                dev = setup_partition (devnr, part);
            }
        } else if (part) {
            dev = setup_partition (devnr, part);
        } else {
            dev = new LocalRawDevice (devnr);
        }

        const char *filepart = strrchr (name, '>');
        if (filepart && filepart[1] == '/') {
            filepart++;
            while (*filepart == '/') filepart++;
            if (*filepart) {
                VFS::Filesystem *fs;
                if (part == 2) {
                    fs = iPodMusicPartitionFS;
                } else if (part == 3 || part == 4) {
                    fs = iPodLinuxPartitionFS; 
                } else {
                    fs = 0;
                    fh = new VFS::ErrorFile (EINVAL);
                }
                if (fs) {
                    int err;
                    if ((err = fs->init()) < 0) {
                        fh = new VFS::ErrorFile (-err);
                    } else {
                        fh = fs->open (filepart, writing? (O_WRONLY|O_CREAT|O_TRUNC) : O_RDONLY);
                    }
                }
            } else {
                fh = new VFS::ErrorFile (EINVAL);
            }
        } else {
            fh = new VFS::DeviceFile (dev);
        }
    } else {
        fh = new LocalFile (name, writing? OPEN_WRITE|OPEN_CREATE : OPEN_READ);
    }

    if (fh->error()) {
        errno = fh->error();
        return -fh->error();
    }
    fo->data = fh;
    return 0;
}

void rawpod_close (fw_fileops *fo) { ((VFS::File *)fo->data)->close(); delete (VFS::File *)fo->data; }
int rawpod_read (fw_fileops *fo, void *buf, int len) { return ((VFS::File *)fo->data)->read (buf, len); }
int rawpod_write (fw_fileops *fo, const void *buf, int len) { return ((VFS::File *)fo->data)->write (buf, len); }
int rawpod_lseek (fw_fileops *fo, s64 off, int whence) {
    if (((VFS::File *)fo->data)->lseek (off, whence) < 0)
        return -1;
    return 0;
}
s64 rawpod_tell (fw_fileops *fo) { return ((VFS::File *)fo->data)->lseek (0, SEEK_CUR); }

const char *make_device_name (int devnr, int partnr = 0, const char *filename = 0) 
{
    static char buf[256];
    if (partnr) sprintf (buf, "<%d,%d>", devnr, partnr);
    else sprintf (buf, "<%d>", devnr);
    if (filename) sprintf (buf + strlen (buf), "/%s", filename);
    return buf;
}

void FirmwareRecreateAction::run_sub() 
{
    emit setCurrentAction (tr ("Loading the existing images from the iPod..."));

    int jr;
    if ((jr = setjmp (fw_error_out)) != 0) {
        FATAL_T (tr ("Unable to modify the firmware (error %1)").arg (jr));
    }

    fw_clear_ignore();
    fw_add_ignore ("aupd");
    fw_add_ignore ("hibe");
    
    struct my_stat st;
    int updating = (Mode == Update || Mode == ChangeLoader);

    switch (iPodLoader) {
    case Loader1Linux:
        fw_load_all (fw_file, updating? "osos" : "osos1");
        if (iPodLinuxPartitionFS->stat ("/loader.bin", &st) >= 0 || !updating)
            fw_load_binary (make_device_name (iPodLocation, 3, "loader.bin"), "osos@");
        if (iPodLinuxPartitionFS->stat ("/linux.bin", &st) >= 0 || !updating)
            fw_load_binary (make_device_name (iPodLocation, 3, "linux.bin"), "osos0");
        break;
    case Loader1Apple:
        fw_load_all (fw_file, updating? "osos" : "osos0");
        if (iPodLinuxPartitionFS->stat ("/loader.bin", &st) >= 0 || !updating)
            fw_load_binary (make_device_name (iPodLocation, 3, "loader.bin"), "osos@");
        if (iPodLinuxPartitionFS->stat ("/linux.bin", &st) >= 0 || !updating)
            fw_load_binary (make_device_name (iPodLocation, 3, "linux.bin"), "osos1");
        break;
    case Loader2:
        fw_load_all (fw_file, updating? "osos" : "osos0");
        if (iPodLinuxPartitionFS->stat ("/loader.bin", &st) >= 0 || !updating)
            fw_load_binary (make_device_name (iPodLocation, 3, "loader.bin"), "osos@");
        if (iPodLinuxPartitionFS->stat ("/linux.bin", &st) >= 0 || !updating)
            fw_load_binary (make_device_name (iPodLocation, 3, "linux.bin"), "linx");
        break;
    case UnknownLoader:
        fw_file = strdup (make_device_name (iPodLocation, 1));
        fw_load_all (fw_file, "osos");
        fw_image_info *osos_inf;
        // Old stuff?
        if ((osos_inf = fw_find_image ("osos")) &&
            !memcmp ((char *)osos_inf->memblock + osos_inf->header.entryOffset + 0x100, "!ATAsoso", 8)) {
            // Old Loader1 or Loader2 - set the parent flag
            osos_inf->header.isparent = 1;
            fw_create_dump (fw_file);
            fw_clear();
            fw_load_all (fw_file, "osos");
        }

        // Is it an L1?
        if (fw_find_image ("osos@") && !fw_find_image ("linx")) {
            if (iPodLinuxPartitionFS->stat ("/loader.bin", &st) >= 0 || !updating)
                fw_load_binary (make_device_name (iPodLocation, 3, "loader.bin"), "osos@");

            // Which is the default?
            if (fw_find_image ("osos0") && !memcmp (fw_find_image ("osos0")->memblock, "\xfe\x1f\x00\xea", 4)) {
                // Linux default
                if (iPodLinuxPartitionFS->stat ("/linux.bin", &st) >= 0 || !updating)
                    fw_load_binary (make_device_name (iPodLocation, 3, "linux.bin"), "osos0");
                iPodLoader = Loader1Apple;
            } else if (fw_find_image ("osos1") && !memcmp (fw_find_image ("osos1")->memblock, "\xfe\x1f\x00\xea", 4)) {
                // Linux secondary
                if (iPodLinuxPartitionFS->stat ("/linux.bin", &st) >= 0 || !updating)
                    fw_load_binary (make_device_name (iPodLocation, 3, "linux.bin"), "osos1");
                iPodLoader = Loader1Linux;
            } else {
                // Non-Linuxed Loader2, probably
                if (iPodLinuxPartitionFS->stat ("/linux.bin", &st) >= 0 || !updating)
                    fw_load_binary (make_device_name (iPodLocation, 3, "linux.bin"), "linx");
                iPodLoader = Loader2;
            }
        } else if (fw_find_image ("linx")) {
            // L2
            if (iPodLinuxPartitionFS->stat ("/loader.bin", &st) >= 0 || !updating)
                fw_load_binary (make_device_name (iPodLocation, 3, "loader.bin"), "osos@");
            if (iPodLinuxPartitionFS->stat ("/linux.bin", &st) >= 0 || !updating)
                fw_load_binary (make_device_name (iPodLocation, 3, "linux.bin"), "linx");
            iPodLoader = Loader2;
        } else {
            // Something odd
            FATAL ("You've apparently installed iPodLinux manually, and I can't figure out what type of "
                   "firmware layout your iPod has. Sorry, but I can't continue.");
        }
        break;
    }

    emit setCurrentAction (tr ("Writing the new firmware..."));

    fw_create_dump (make_device_name (iPodLocation, 1));

    emit setCurrentAction (tr ("Writing firmware information to the ext2 partition..."));
    
    VFS::File *fh = iPodLinuxPartitionFS->open ("/etc/loadertype", O_WRONLY | O_CREAT | O_TRUNC);
    switch (iPodLoader) {
    case Loader1Apple:
        fh->write ("Apple1", 5);
        break;
    case Loader1Linux:
        fh->write ("Linux1", 5);
        break;
    case Loader2:
        fh->write ("2Loader", 7);
        break;
    case UnknownLoader:
        fh->write ("?Unknown", 8);
        break;
    }
    fh->write ("\n", 1);
    fh->close();
    delete fh;
    
    emit setCurrentAction (tr ("Done."));
}

static void setup_makefw() 
{
    fw_default_fileops.open = rawpod_open;
    fw_default_fileops.close = rawpod_close;
    fw_default_fileops.read = rawpod_read;
    fw_default_fileops.write = rawpod_write;
    fw_default_fileops.lseek = rawpod_lseek;
    fw_default_fileops.tell = rawpod_tell;

    fw_test_endian();
    fw_set_options (FW_LOAD_IMAGES_TO_MEMORY | FW_QUIET | (iPodLoader == Loader2? FW_LOADER2 : FW_LOADER1));
    fw_set_generation (iPodVersion);
    fw_clear();
}

void FirmwareRecreateAction::run() 
{
    setup_makefw();
    
    if (iPodDoBackup)
        fw_file = strdup (iPodBackupLocation.toAscii().data());
    else
        fw_file = strdup (make_device_name (iPodLocation, 1));

    emit setTaskDescription (tr ("Patching the iPod's firmware."));
    emit setTotalProgress (0);
    emit setCurrentProgress (0);

    int prog = 0;

    RunnerThread <void(*)(FirmwareRecreateAction*), FirmwareRecreateAction*> *rthr =
        new RunnerThread <void(*)(FirmwareRecreateAction*), FirmwareRecreateAction*> (rethread_shim, this);
    rthr->start();
    while (rthr->isRunning()) {
        emit setCurrentProgress (prog++);
        usleep (300000);
    }

    delete rthr;
}

void ChangeLoaderAction::run() 
{
    if (_old == _new) return;

    emit setTaskDescription (tr ("Rejiggering the loader..."));
    emit setTotalProgress (0);
    emit setCurrentProgress (0);

    setup_makefw();

    int jr;
    if ((jr = setjmp (fw_error_out)) != 0) {
        FATAL_T (tr ("Unable to rejigger the loader (error %1)").arg (jr));
    }

    fw_clear_ignore();
    fw_add_ignore ("aupd");
    fw_add_ignore ("hibe");
    fw_load_all (iPodDoBackup? iPodBackupLocation.toAscii().data() :
                 make_device_name (iPodLocation, 1), "osos");
    if (_old != Loader2 && _new != Loader2) {
        // osos0 <-> osos1
        fw_rename_image ("osos0", "osos1");
    } else if (_old == Loader2) {
        // linx -> osos1, then swap osos0 and osos1 if necessary
        fw_rename_image ("linx", "osos1");
        if (_new == Loader1Linux)
            fw_rename_image ("osos1", "osos0");
    } else {
        // osos0 or osos1 -> linx, then swap osos0 and osos1 if necessary
        fw_rename_image ((_old == Loader1Linux)? "osos0" : "osos1", "linx");
        if (_old == Loader1Linux)
            fw_rename_image ("osos1", "osos0");
    }
    fw_create_dump (make_device_name (iPodLocation, 1));
}

void HeuristicUninstallAction::run() 
{
    emit fatalError ("Heuristic uninstall is not yet implemented. Use iTunes.");
}
