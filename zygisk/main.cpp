#include <jni.h>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <sys/system_properties.h>
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

// Games the module will inject into
static const char* TARGETS[] = {
    "com.tencent.ig","com.pubg.krmobile","com.activision.callofduty.shooter",
    "com.garena.game.codm","com.miHoYo.GenshinImpact","com.miHoYo.hkrpg",
    "com.epicgames.fortnite","com.mobilelegends.mi","com.mobile.legends",
    "com.roblox.client","com.supercell.brawlstars","com.riotgames.league.wildrift",
    "com.dts.freefireth","com.dts.freefiremax","com.axlebolt.standoff2",
    "com.innersloth.spacemafia","com.mojang.minecraftpe", NULL
};

static char g_model[96]        = "NX729J";
static char g_brand[96]        = "ZTE";
static char g_manufacturer[96] = "nubia";

// Read a single key=value line from the props file (plain C, no <fstream>)
static void readProp(const char* path, const char* key, char* out, size_t outsz){
    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[256];
    size_t klen = strlen(key);
    while (fgets(line, sizeof(line), f)){
        if (strncmp(line, key, klen) == 0 && line[klen] == '='){
            char* v = line + klen + 1;
            size_t n = strlen(v);
            if (n > 0 && (v[n-1] == '\n' || v[n-1] == '\r')) v[--n] = 0;
            if (n >= outsz) n = outsz - 1;
            memcpy(out, v, n);
            out[n] = 0;
            break;
        }
    }
    fclose(f);
}

static bool isTarget(const char* pkg){
    for (int i = 0; TARGETS[i]; i++){
        if (strcmp(pkg, TARGETS[i]) == 0) return true;
    }
    return false;
}

static bool isEnabled(const char* pkg){
    FILE* f = fopen("/data/adb/curtyyytweaks/enabled_apps", "r");
    if (!f) return true; // if no list yet, allow all targets
    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = 0;
    fclose(f);
    return strstr(buf, pkg) != NULL;
}

class CurtyyyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api* api, JNIEnv* env) override { this->api = api; this->env = env; }

    void preAppSpecialize(AppSpecializeArgs* args) override {
        if (!args || !args->nice_name) return;
        const char* pkg = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!pkg) return;

        bool hit = isTarget(pkg);
        bool en  = isEnabled(pkg);
        env->ReleaseStringUTFChars(args->nice_name, pkg);

        if (!hit || !en){
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        // Load spoof identity written by the WebUI
        const char* cfg = "/data/adb/curtyyytweaks/spoof.props";
        readProp(cfg, "model",        g_model,        sizeof(g_model));
        readProp(cfg, "brand",        g_brand,        sizeof(g_brand));
        readProp(cfg, "manufacturer", g_manufacturer, sizeof(g_manufacturer));

        __system_property_set("ro.product.model",        g_model);
        __system_property_set("ro.product.brand",        g_brand);
        __system_property_set("ro.product.manufacturer", g_manufacturer);
        __system_property_set("ro.product.vendor.model",        g_model);
        __system_property_set("ro.product.vendor.brand",        g_brand);
        __system_property_set("ro.product.vendor.manufacturer", g_manufacturer);
        __system_property_set("ro.product.odm.model",           g_model);
        __system_property_set("ro.product.odm.brand",           g_brand);
        __system_property_set("ro.product.odm.manufacturer",    g_manufacturer);
    }

    void postAppSpecialize(const AppSpecializeArgs*) override {}
    void preServerSpecialize(ServerSpecializeArgs*) override { api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY); }
    void postServerSpecialize(const ServerSpecializeArgs*) override {}

private:
    Api* api = nullptr;
    JNIEnv* env = nullptr;
};

REGISTER_ZYGISK_MODULE(CurtyyyModule)
