#include <jni.h>
#include <string>
#include <vector>
#include <fstream>
#include <sys/system_properties.h>
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

static const std::vector<std::string> TARGET_GAMES = {
    "com.tencent.ig",
    "com.pubg.krmobile",
    "com.activision.callofduty.shooter",
    "com.miHoYo.GenshinImpact",
    "com.miHoYo.hkrpg",
    "com.epicgames.fortnite",
    "com.mobilelegends.mi",
    "com.mobile.legends",
    "com.garena.game.codm",
    "com.roblox.client",
    "com.supercell.brawlstars",
    "com.riotgames.league.wildrift"
};

class StellarTweaksModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        if (!args) return;

        const char *raw_pkg = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!raw_pkg) return;
        std::string pkg(raw_pkg);
        env->ReleaseStringUTFChars(args->nice_name, raw_pkg);

        bool is_target = false;
        for (const auto &t : TARGET_GAMES) {
            if (pkg == t) { is_target = true; break; }
        }

        if (!is_target) {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        std::ifstream file("/data/adb/stellartweaks/config");
        std::string profile = "DEFAULT";
        if (file.is_open()) { std::getline(file, profile); file.close(); }

        if (profile.find("ROG9") != std::string::npos) {
            __system_property_set("ro.product.model", "ASUS_AI2401_A");
            __system_property_set("ro.product.brand", "asus");
            __system_property_set("ro.product.manufacturer", "ASUS");
        } else if (profile.find("REDMAGIC11") != std::string::npos) {
            __system_property_set("ro.product.model", "NX769J");
            __system_property_set("ro.product.brand", "ZTE");
            __system_property_set("ro.product.manufacturer", "nubia");
        } else if (profile.find("S26ULTRA") != std::string::npos) {
            __system_property_set("ro.product.model", "SM-S948B");
            __system_property_set("ro.product.brand", "samsung");
            __system_property_set("ro.product.manufacturer", "samsung");
        } else if (profile.find("FOLD8") != std::string::npos) {
            __system_property_set("ro.product.model", "SM-F966B");
            __system_property_set("ro.product.brand", "samsung");
            __system_property_set("ro.product.manufacturer", "samsung");
        } else {
            __system_property_set("ro.product.model", "NX729J");
            __system_property_set("ro.product.brand", "ZTE");
            __system_property_set("ro.product.manufacturer", "nubia");
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {}

    void preServerSpecialize(ServerSpecializeArgs *args) override {
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }
    void postServerSpecialize(const ServerSpecializeArgs *args) override {}

private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
};

REGISTER_ZYGISK_MODULE(StellarTweaksModule)
