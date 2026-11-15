/* SPDX-License-Identifier: MIT */
/* Copyright (C) 2021-2023 topjohnwu, John Wu, and KernelSU contributors */

#pragma once

#include <cstdint>
#include <jni.h>
#include <sys/system_properties.h>

namespace zygisk {

struct Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;

class ModuleBase {
public:
    inline virtual ~ModuleBase() {}
    inline virtual void onLoad(Api *api, JNIEnv *env) {}
    inline virtual void preAppSpecialize(AppSpecializeArgs *args) {}
    inline virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
    inline virtual void preServerSpecialize(ServerSpecializeArgs *args) {}
    inline virtual void postServerSpecialize(const ServerSpecializeArgs *args) {}
};

enum Option : int {
    FORCE_DENYLIST_UNMOUNT = 0,
    DLCLOSE_MODULE_LIBRARY = 1,
};

enum StateFlag : uint32_t {
    PROCESS_GRANTED_ROOT = (1u << 0),
    PROCESS_ON_DENYLIST  = (1u << 1),
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

    AppSpecializeArgs(
        jint &uid, jint &gid, jintArray &gids, jint &runtime_flags,
        jobjectArray &rlimits, jint &mount_external, jstring &se_info,
        jstring &nice_name, jstring &instruction_set, jstring &app_data_dir)
        : uid(uid), gid(gid), gids(gids), runtime_flags(runtime_flags),
          rlimits(rlimits), mount_external(mount_external), se_info(se_info),
          nice_name(nice_name), instruction_set(instruction_set),
          app_data_dir(app_data_dir) {}
};

struct ServerSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jlong &permitted_capabilities;
    jlong &effective_capabilities;

    ServerSpecializeArgs(
        jint &uid, jint &gid, jintArray &gids, jint &runtime_flags,
        jlong &permitted_capabilities, jlong &effective_capabilities)
        : uid(uid), gid(gid), gids(gids), runtime_flags(runtime_flags),
          permitted_capabilities(permitted_capabilities),
          effective_capabilities(effective_capabilities) {}
};

struct internal_module_abi {
    long api_version;
    ModuleBase *_this;
    void (*preAppSpecialize)(ModuleBase *, AppSpecializeArgs *);
    void (*postAppSpecialize)(ModuleBase *, const AppSpecializeArgs *);
    void (*preServerSpecialize)(ModuleBase *, ServerSpecializeArgs *);
    void (*postServerSpecialize)(ModuleBase *, const ServerSpecializeArgs *);
    internal_module_abi(ModuleBase *m) : _this(m) {
        api_version = 5;
        preAppSpecialize = [](ModuleBase *self, AppSpecializeArgs *a){ self->preAppSpecialize(a); };
        postAppSpecialize = [](ModuleBase *self, const AppSpecializeArgs *a){ self->postAppSpecialize(a); };
        preServerSpecialize = [](ModuleBase *self, ServerSpecializeArgs *a){ self->preServerSpecialize(a); };
        postServerSpecialize = [](ModuleBase *self, const ServerSpecializeArgs *a){ self->postServerSpecialize(a); };
    }
};

struct Api {
#define call(method) ((ab)->method)

    void setOption(Option opt) {
        call(setOption)(ab, this, opt);
    }
    int getModuleDir() {
        return call(getModuleDir)(ab, this);
    }
    uint32_t getFlags() {
        return call(getFlags)(ab, this);
    }

#undef call

private:
    struct abi_table *ab;
    void *impl;
    friend struct internal_module_abi;
    Api(struct abi_table *t, void *i) : ab(t), impl(i) {}

    struct abi_table {
        void (*setOption)(struct abi_table *, Api *, Option);
        int (*getModuleDir)(struct abi_table *, Api *);
        uint32_t (*getFlags)(struct abi_table *, Api *);
    };
};

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(clazz) \
void zygisk_module_entry(zygisk::internal_module_abi **abi) { \
    static auto m = new clazz(); \
    static auto a = new zygisk::internal_module_abi(m); \
    *abi = a; \
}
