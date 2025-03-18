#ifndef BACKENDINFO_H
#define BACKENDINFO_H

#include <QObject>

class SyncBackend;

struct BackendInfo {
    SyncBackend *backend;
    int priority;
    bool syncOnOpen;
};

#endif // BACKENDINFO_H
