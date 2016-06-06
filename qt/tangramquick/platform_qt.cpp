#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>

#include "urlWorker.h"
#include "platform_qt.h"

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <QCoreApplication>
#include <QObject>
#include <QWindow>

#define NUM_WORKERS 3

static std::string s_resourceRoot;

static UrlWorker s_Workers[NUM_WORKERS];
static std::list<std::unique_ptr<UrlTask>> s_urlTaskQueue;

static QObject *s_quickItem = NULL;

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void processNetworkQueue() {
    // attach workers to NetWorkerData
    auto taskItr = s_urlTaskQueue.begin();
    for(auto& worker : s_Workers) {
        if(taskItr == s_urlTaskQueue.end()) {
            break;
        }
        if(worker.isAvailable()) {
            worker.perform(std::move(*taskItr));
            taskItr = s_urlTaskQueue.erase(taskItr);
        }
    }
}

void requestRender() {
    if (s_quickItem)
        QCoreApplication::postEvent(s_quickItem, new QEvent(TANGRAM_REQ_RENDER_EVENT_TYPE));
}

void setContinuousRendering(bool _isContinuous)
{
    if (s_quickItem)
        s_quickItem->setProperty("continuousRendering", _isContinuous);
}

bool isContinuousRendering()
{
    if (s_quickItem)
        return s_quickItem->property("continuousRendering").toBool();
    return false;
}

std::string setResourceRoot(const char* _path) {

    std::string dir(_path);

    s_resourceRoot = std::string(dirname(&dir[0])) + '/';

    std::string base(_path);

    return std::string(basename(&base[0]));

}

std::string resolvePath(const char* _path, PathType _type) {

    switch (_type) {
    case PathType::absolute:
    case PathType::internal:
        return std::string(_path);
    case PathType::resource:
        return s_resourceRoot + _path;
    }
    return "";
}

std::string stringFromFile(const char* _path, PathType _type) {

    unsigned int length = 0;
    unsigned char* bytes = bytesFromFile(_path, _type, &length);

    std::string out(reinterpret_cast<char*>(bytes), length);
    free(bytes);

    return out;
}

unsigned char* bytesFromFile(const char* _path, PathType _type, unsigned int* _size) {

    std::string path = resolvePath(_path, _type);

    std::ifstream resource(path.c_str(), std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", path.c_str());
        *_size = 0;
        return nullptr;
    }

    *_size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*) malloc(sizeof(char) * (*_size));

    resource.read(cdata, *_size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);
}

// No system fonts implementation (yet!)
std::string systemFontPath(const std::string& _name, const std::string& _weight,
                           const std::string& _face) {
    return "";
}

// No system fonts fallback implementation (yet!)
std::string systemFontFallbackPath(int _importance, int _weightHint) {
    return "";
}

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {
    std::unique_ptr<UrlTask> task(new UrlTask(_url, _callback));
    for(auto& worker : s_Workers) {
        if(worker.isAvailable()) {
            worker.perform(std::move(task));
            return true;
        }
    }
    s_urlTaskQueue.push_back(std::move(task));
    return true;

    /*if (widget) {
        widget->startUrlRequest(_url, _calback);
    }*/
}

void cancelUrlRequest(const std::string& _url) {

    // Only clear this request if a worker has not started operating on it!!
    // otherwise it gets too convoluted with curl!
    auto itr = s_urlTaskQueue.begin();
    while(itr != s_urlTaskQueue.end()) {
        if((*itr)->url == _url) {
            itr = s_urlTaskQueue.erase(itr);
        } else {
            itr++;
        }
    }
}

void finishUrlRequests() {
    for(auto& worker : s_Workers) {
        worker.join();
    }
}

void setCurrentThreadPriority(int priority){
    int tid = syscall(SYS_gettid);
    //int  p1 = getpriority(PRIO_PROCESS, tid);

    setpriority(PRIO_PROCESS, tid, priority);

    //int  p2 = getpriority(PRIO_PROCESS, tid);
    //logMsg("set niceness: %d -> %d\n", p1, p2);
}

void registerItem(QObject *quickItem)
{
    if (!s_quickItem)
        s_quickItem = quickItem;
}

void initGLExtensions()
{
}
