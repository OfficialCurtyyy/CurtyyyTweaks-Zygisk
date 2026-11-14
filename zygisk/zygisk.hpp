#pragma once
#include <jni.h>
#include <cstdint>
#include <type_traits>

#define ZYGISK_API_VERSION 2

namespace zygisk {

struct Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;

class ModuleBase {
public:
    virtual void onLoad(Api *api, JNIEnv *env) {}
    virtual void preAppSpecialize(AppSpecializeArgs *args) {}
    virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
    virtual void preServerSpecialize(ServerSpecializeArgs *args) {}
    virtual void postServerSpecialize(const ServerSpecializeArgs *args) {}
    virtual ~ModuleBase() = default;
};

struct AppSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jobjectArray &rlimits;
    jint &mount_external;
    jstring &se_info;
    jstring &nice_name;
    jstring &instruction_set;
    jstring &app_data_dir;

    jintArray *fds_to_ignore = nullptr;
    jboolean *is_child_zygote = nullptr;
    jboolean *is_top_app = nullptr;
    jobjectArray *pkg_data_info_list = nullptr;
    jobjectArray *whitelisted_data_info_list = nullptr;
    jboolean *mount_data_dirs = nullptr;
    jboolean *mount_storage_dirs = nullptr;

    AppSpecializeArgs() = delete;
};

struct ServerSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jlong &permitted_capabilities;
    jlong &effective_capabilities;

    ServerSpecializeArgs() = delete;
};

enum Option : int {
    FORCE_DENYLIST_UNMOUNT = 0,
    DLCLOSE_MODULE_LIBRARY = 1,
    OPTION_MAX
};

struct Api {
    virtual void setOption(Option opt) = 0;
    virtual int connectCompanion() = 0;
};

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(clazz)                                            \
    static_assert(std::is_base_of<zygisk::ModuleBase, clazz>::value,             \
                  "clazz must subclass zygisk::ModuleBase");                     \
    extern "C" __attribute__((visibility("default"))) __attribute__((used))      \
    void *zygisk_module_entry() {                                                \
        static clazz instance;                                                   \
        return reinterpret_cast<void *>(&instance);                              \
}
