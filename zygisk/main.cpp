#include <jni.h>
#include <string>
#include <vector>
#include <fstream>
#include <cstring>
#include <dlfcn.h>
#include <sys/system_properties.h>
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

// ---------- Target games ----------
static const std::vector<std::string> TARGET_GAMES = {
    "com.tencent.ig", "com.pubg.krmobile", "com.activision.callofduty.shooter",
    "com.miHoYo.GenshinImpact", "com.miHoYo.hkrpg", "com.epicgames.fortnite",
    "com.mobilelegends.mi", "com.mobile.legends", "com.garena.game.codm",
    "com.roblox.client", "com.supercell.brawlstars", "com.riotgames.league.wildrift"
};

// ---------- Spoof table ----------
struct SpoofProfile { const char *model, *brand, *manufacturer, *device, *product; };
static SpoofProfile g_profile = {"NX729J","ZTE","nubia","NX729J","NX729J"}; // default RedMagic 10

static void applyProfile(const std::string &key){
    if      (key.find("ROG9")       != std::string::npos) g_profile = {"ASUS_AI2401_A","asus","ASUS","AI2401","AI2401"};
    else if (key.find("REDMAGIC11") != std::string::npos) g_profile = {"NX769J","ZTE","nubia","NX769J","NX769J"};
    else if (key.find("S26ULTRA")   != std::string::npos) g_profile = {"SM-S948B","samsung","samsung","pa2qxxx","pa2qxxx"};
    else if (key.find("FOLD8")      != std::string::npos) g_profile = {"SM-F966B","samsung","samsung","pa3qxxx","pa3qxxx"};
    else if (key.find("DEFAULT")    != std::string::npos) g_profile = {"NX729J","ZTE","nubia","NX729J","NX729J"};
}

// ---------- Original function pointers ----------
using fn_prop_get  = int (*)(const char *, char *);
using fn_prop_find = const prop_info *(*)(const char *);
static fn_prop_get  orig_get  = nullptr;
static fn_prop_find orig_find = nullptr;
static bool g_armed = false;

// ---------- Hooked __system_property_get ----------
static int hooked_prop_get(const char *name, char *value){
    int n = orig_get ? orig_get(name, value) : -1;
    if (!name || !value) return n;
    std::string k(name);
    auto setv = [&](const char *v){ std::strncpy(value, v, PROP_VALUE_MAX-1); value[PROP_VALUE_MAX-1]=0; return (int)std::strlen(v); };
    if      (k=="ro.product.model"        || k=="ro.product.odm.model"        || k=="ro.product.vendor.model")        return setv(g_profile.model);
    else if (k=="ro.product.brand"        || k=="ro.product.odm.brand"        || k=="ro.product.vendor.brand")        return setv(g_profile.brand);
    else if (k=="ro.product.manufacturer" || k=="ro.product.odm.manufacturer" || k=="ro.product.vendor.manufacturer")return setv(g_profile.manufacturer);
    else if (k=="ro.product.device"       || k=="ro.product.odm.device"       || k=="ro.product.vendor.device")       return setv(g_profile.device);
    else if (k=="ro.product.name"         || k=="ro.product.odm.name"         || k=="ro.product.vendor.name")         return setv(g_profile.product);
    return n;
}

// ---------- Minimal PLT/GOT hook via dlsym override is complex;
//          instead we use the reliable approach: intercept via LD_PRELOAD-style
//          symbol interposition is not possible in Zygisk, so we patch the GOT
//          of the loaded libc. For portability we use a simpler, proven method:
//          override through a wrapper that games call via dlsym(RTLD_DEFAULT).
//          Most games read props through Java SystemProperties or native
//          __system_property_get resolved at runtime — we hook the latter by
//          replacing the symbol in the dynamic linker's resolution using
//          android_dlext is unavailable, so we fall back to GOT patching of
//          the main executable + libandroid_runtime. ----------

#include <link.h>
#include <elf.h>
#include <sys/mman.h>
#include <unistd.h>

static bool patch_got(void *base, const char *sym, void *hook){
    // Walk ELF program headers to find PT_DYNAMIC, then patch GOT entries for `sym`
    auto *ehdr = (ElfW(Ehdr)*)base;
    auto *phdr = (ElfW(Phdr)*)((char*)base + ehdr->e_phoff);
    ElfW(Dyn) *dyn = nullptr; size_t dynsz = 0;
    for (int i=0;i<ehdr->e_phnum;i++){
        if (phdr[i].p_type == PT_DYNAMIC){ dyn = (ElfW(Dyn)*)((char*)base + phdr[i].p_vaddr); dynsz = phdr[i].p_memsz; break; }
    }
    if (!dyn) return false;
    ElfW(Sym) *symtab=nullptr; char *strtab=nullptr; ElfW(Rela)*rela=nullptr; size_t relasz=0;
    ElfW(Rel)*rel=nullptr; size_t relsz=0;
    for (auto *d=dyn; d->d_tag!=DT_NULL; d++){
        switch(d->d_tag){
            case DT_SYMTAB: symtab=(ElfW(Sym)*)(base+d->d_un.d_ptr); break;
            case DT_STRTAB: strtab=(char*)(base+d->d_un.d_ptr); break;
            case DT_JMPREL: rela=(ElfW(Rela)*)(base+d->d_un.d_ptr); break;
            case DT_PLTRELSZ: relasz=d->d_un.d_val; break;
            case DT_REL: rel=(ElfW(Rel)*)(base+d->d_un.d_ptr); break;
            case DT_RELSZ: relsz=d->d_un.d_val; break;
        }
    }
    if (!symtab||!strtab) return false;
    bool patched=false;
    auto try_patch = [&](void **slot){
        // resolve symbol index from rela/rel is complex; instead compare resolved addr
        return false;
    };
    // Simpler robust approach: iterate .rela.plt and .rel.plt, resolve each sym,
    // if it matches our target name, mprotect the slot and overwrite.
    auto handle_rela = [&](ElfW(Rela)*r, size_t sz){
        for (size_t i=0;i<sz/sizeof(ElfW(Rela));i++){
            auto idx = ELF_R_SYM(r[i].r_info);
            const char *name = strtab + symtab[idx].st_name;
            if (std::strcmp(name, sym)==0){
                void **slot = (void**)((char*)base + r[i].r_offset);
                long page = sysconf(_SC_PAGESIZE);
                void *pagebase = (void*)((uintptr_t)slot & ~(page-1));
                mprotect(pagebase, page, PROT_READ|PROT_WRITE);
                *slot = hook;
                mprotect(pagebase, page, PROT_READ);
                patched = true;
            }
        }
    };
    auto handle_rel = [&](ElfW(Rel)*r, size_t sz){
        for (size_t i=0;i<sz/sizeof(ElfW(Rel));i++){
            auto idx = ELF_R_SYM(r[i].r_info);
            const char *name = strtab + symtab[idx].st_name;
            if (std::strcmp(name, sym)==0){
                void **slot = (void**)((char*)base + r[i].r_offset);
                long page = sysconf(_SC_PAGESIZE);
                void *pagebase = (void*)((uintptr_t)slot & ~(page-1));
                mprotect(pagebase, page, PROT_READ|PROT_WRITE);
                *slot = hook;
                mprotect(pagebase, page, PROT_READ);
                patched = true;
            }
        }
    };
    if (rela && relasz) handle_rela(rela, relasz);
    if (rel  && relsz)  handle_rel(rel, relsz);
    return patched;
}

static int cb_dl_phdr(struct dl_phdr_info *info, size_t, void *){
    if (!info->dlpi_name) return 0;
    std::string n(info->dlpi_name);
    // Patch the main executable and key libs that resolve __system_property_get
    if (n.empty() || n.find("libandroid_runtime")!=std::string::npos ||
        n.find("libutils")!=std::string::npos || n.find("libc.so")!=std::string::npos ||
        n.find("libgame")!=std::string::npos || n.find("libUE4")!=std::string::npos ||
        n.find("libil2cpp")!=std::string::npos || n.find("libunity")!=std::string::npos){
        patch_got((void*)info->dlpi_addr, "__system_property_get", (void*)hooked_prop_get);
    }
    return 0;
}

static void install_hooks(){
    if (g_armed) return;
    orig_get = (fn_prop_get)dlsym(RTLD_DEFAULT, "__system_property_get");
    dl_iterate_phdr(cb_dl_phdr, nullptr);
    g_armed = true;
}

// ---------- Zygisk module ----------
class StellarTweaksModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override { this->api=api; this->env=env; }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        if (!args) return;
        const char *raw = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!raw) return;
        std::string pkg(raw);
        env->ReleaseStringUTFChars(args->nice_name, raw);

        bool hit=false;
        for (auto &t: TARGET_GAMES) if (pkg==t){ hit=true; break; }
        if (!hit){ api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY); return; }

        // Read enabled list — skip if this pkg is disabled
        std::ifstream en("/data/adb/stellartweaks/enabled_apps");
        if (en.is_open()){
            std::string line; std::getline(en, line); en.close();
            if (!line.empty() && line.find(pkg)==std::string::npos){
                api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY); return;
            }
        }

        // Read chosen device profile
        std::ifstream cf("/data/adb/stellartweaks/config");
        std::string key="DEFAULT";
        if (cf.is_open()){ std::getline(cf, key); cf.close(); }
        if (key.find("IPHONE18")!=std::string::npos){ api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY); return; }

        applyProfile(key);
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (g_armed) return;
        install_hooks();   // GOT-patch __system_property_get across loaded libs
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override { api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY); }
    void postServerSpecialize(const ServerSpecializeArgs *args) override {}

private:
    Api *api=nullptr; JNIEnv *env=nullptr;
};

REGISTER_ZYGISK_MODULE(StellarTweaksModule)
