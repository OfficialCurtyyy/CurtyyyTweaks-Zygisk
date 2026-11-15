#pragma once
#include <jni.h>

namespace zygisk {

enum Option : int {
    FORCE_DENYLIST_UNMOUNT = 0,
    DLCLOSE_MODULE_LIBRARY = 1
};

struct AppSpecializeArgs {
    jint uid;
    jint gid;
    jintArray gids;
    jint runtime_flags;
    jobjectArray rlimits;
    jint mount_external;
    jstring se_info;
    jstring nice_name;
    jstring instruction_set;
    jstring app_data_dir;
};

struct ServerSpecializeArgs {
    jint uid;
    jint gid;
    jintArray gids;
    jint runtime_flags;
    jlong permitted_capabilities;
    jlong effective_capabilities;
};

struct Api {
    virtual void setOption(Option opt) = 0;
    virtual int connectCompanion() = 0;
    virtual ~Api() {}
};

class ModuleBase {
public:
    virtual void onLoad(Api *api, JNIEnv *env) {}
    virtual void preAppSpecialize(AppSpecializeArgs *args) {}
    virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
    virtual void preServerSpecialize(ServerSpecializeArgs *args) {}
    virtual void postServerSpecialize(const ServerSpecializeArgs *args) {}
    virtual ~ModuleBase() {}
};

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(clazz) \
    extern "C" __attribute__((visibility("default"), used)) \
    void *zygisk_module_entry() { \
        static clazz instance; \
        return (void *)&instance; \
}
