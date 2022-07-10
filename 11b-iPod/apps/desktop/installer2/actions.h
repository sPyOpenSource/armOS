/* ipl/installer/actions.h -*- C++ -*- Copyright (c) 2006 Joshua Oreman
 * Distributed under the GPL. See COPYING for details.
 */

#ifndef ACTIONS_H
#define ACTIONS_H

#include "installer.h"
#include "packages.h"
#include "libtar/libtar.h"
#include "rawpod/device.h"
#include <QThread>
#include <QList>
#include <QHttp>
#include <QFile>

struct image;
typedef struct image fw_image_t;

class ActionOutlet : public InstallerPage
{
    Q_OBJECT

public:
    ActionOutlet (Installer *wiz = 0)
        : InstallerPage (wiz)
    {}

public slots:
    virtual void setTaskDescription (QString str) = 0;
    virtual void setCurrentAction (QString str) = 0;
    virtual void setTotalProgress (int tp) = 0;
    virtual void setCurrentProgress (int cp) = 0;
    virtual void fatalError (QString str) = 0;
};

template <class F, class Arg>
class RunnerThread : public QThread 
{
public:
    RunnerThread (F func, Arg arg)
        : _fn (func), _arg (arg)
    {}

protected:
    virtual void run() {
        (*_fn)(_arg);
    }

    F _fn;
    Arg _arg;
};

class Action : public QThread
{
    Q_OBJECT

public:
    Action() : QThread() {}

    virtual void start (ActionOutlet *outlet) {
        _outlet = outlet;
        connect (this, SIGNAL(setTaskDescription(QString)), _outlet, SLOT(setTaskDescription(QString)));
        connect (this, SIGNAL(setCurrentAction(QString)), _outlet, SLOT(setCurrentAction(QString)));
        connect (this, SIGNAL(setTotalProgress(int)), _outlet, SLOT(setTotalProgress(int)));
        connect (this, SIGNAL(setCurrentProgress(int)), _outlet, SLOT(setCurrentProgress(int)));
        connect (this, SIGNAL(fatalError(QString)), _outlet, SLOT(fatalError(QString)));
        QThread::start();
    }
    
signals:
    void setTaskDescription (QString str);
    void setCurrentAction (QString str);
    void setTotalProgress (int tp);
    void setCurrentProgress (int cp);
    void fatalError (QString str);

protected:
    ActionOutlet *_outlet;
};

class BackupAction : public Action
{
public:
    BackupAction (VFS::Device *device, QString bkppath)
        : _dev (device), _path (bkppath)
    {}

protected:
    virtual void run();
    
    VFS::Device *_dev;
    QString _path;
};

class RestoreBackupAction : public Action
{
public:
    RestoreBackupAction (int device, QString bkppath)
        : _dev (device), _path (bkppath)
    {}

protected:
    virtual void run();
    
    int _dev;
    QString _path;
};

class HeuristicUninstallAction : public Action
{
public:
    HeuristicUninstallAction (int device) 
        : _dev (device)
    {}
    
protected:
    virtual void run();
    
    int _dev;
};

class PartitionAction : public Action
{
    Q_OBJECT

public:
    PartitionAction (VFS::Device *device, int oldpart, int newpart, Partition::Type newtype, int newsize)
        : _dev (device), _oldnr (oldpart), _newnr (newpart),
          _newtype (newtype), _newsize (newsize)
    {}

signals:
    void doDeviceSetup (PartitionTable *t);

protected:
    virtual void run();
    
    VFS::Device *_dev;
    int _oldnr, _newnr;
    Partition::Type _newtype;
    int _newsize;
};

class FormatAction : public Action
{
public:
    FormatAction (int part, int (*mkfs)(VFS::Device *), const char *str)
        : _part (part), _mkfsfunc (mkfs), _str (str)
    {}

protected:
    virtual void run();
    
    int _part;
    int (*_mkfsfunc)(VFS::Device *);
    const char *_str;
};

class DelayAction : public Action
{
public:
    DelayAction (int nsec)
        : _sec (nsec)
    {}

protected:
    virtual void run();
    
    int _sec;
};

class PackageAction : public Action
{
public:
    PackageAction (Package& pkg, QString label)
        : _pkg (pkg), _label (label)
    {}

protected:
    Package& _pkg;
    QString _label;
};

class PackageDownloadAction : public PackageAction
{
    Q_OBJECT

public:
    PackageDownloadAction (Package& pkg, QString label)
        : PackageAction (pkg, label)
    {}

protected slots:
    void httpSendProgress (int done, int total); 
    void httpReadProgress (int done, int total);
    void httpStateChanged (int state);
    void httpRequestFinished (int req, bool err);
    void httpDone (bool err);
    void httpResponseHeaderReceived (const QHttpResponseHeader& resp);

protected:
    virtual void run();

    QHttp *http;
    QString host;
    int hostReq, headReq, getReq;
    int contentLength;
    bool complete;
    QFile *out;
};

class PackageInstallAction : public PackageAction
{
public:
    PackageInstallAction (Package& pkg, QString label)
        : PackageAction (pkg, label)
    {}

protected:
    virtual void run();
    static void update_progress_shim (TAR *t) {
        PackageInstallAction *self = (PackageInstallAction *)t->data;
        if (self) self->update_progress (t);
    }
    void update_progress (TAR *t);
};

class PackageRemoveAction : public PackageAction
{
public:
    PackageRemoveAction (Package& pkg, QString label, PkgTreeWidgetItem *twi = 0)
        : PackageAction (pkg, label), _twi (twi)
    {}

protected:
    virtual void run();

    PkgTreeWidgetItem *_twi;
};

class ChangeLoaderAction : public Action
{
public:
    ChangeLoaderAction (LoaderType oldL, LoaderType newL) : _old (oldL), _new (newL) {}

protected:
    virtual void run();
    LoaderType _old, _new;
};

class FirmwareRecreateAction : public Action
{
public:
    FirmwareRecreateAction() : fw_file (0) {}
    ~FirmwareRecreateAction() { if (fw_file) delete fw_file; }

protected:
    static void rethread_shim (FirmwareRecreateAction *self) {
        self->run_sub();
    }

    virtual void run();
    void run_sub();

    char *fw_file;
};

class CacheFlushAction : public Action
{
public:
    CacheFlushAction (BlockCache *cdev)
        : _cdev (cdev)
    {}
    
protected:
    virtual void run();

    BlockCache *_cdev;
};

class FSSetupAction : public Action 
{
    Q_OBJECT

signals:
    void doFilesystemSetup();

protected:
    virtual void run() {
        connect (this, SIGNAL(doFilesystemSetup()), installer, SLOT(setupFilesystems()));
        emit doFilesystemSetup();
    }
};

extern QList<Action*> *PendingActions;

#endif
