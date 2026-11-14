#include <jni.h>
#include <string>
#include <vector>
#include <fstream>
#include <sys/system_properties.h>

// Use relative include that works with ndk-build LOCAL_C_INCLUDES
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
        if (!args || !args->nice_name) return;

        const char* raw_pkg = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!raw_pkg) return;
        std::string pkg(raw_pkg);
        env->ReleaseStringUTFChars(args->nice_name, raw_pkg);

        bool is_target = false;
        for (const auto& target : TARGET_GAMES) {
            if (pkg == target) {
                is_target = true;
                break;
            }
        }

        if (!is_target) {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        // Read config file
        std::ifstream file("/data/adb/stellartweaks/config");
        std::string profile = "DEFAULT";
        if (file.is_open()) {
            std::getline(file, profile);
            file.close();
        }

        // Apply device spoof
        const char* model = "NX729J";
        const char* brand = "ZTE";
        const char* mfr   = "nubia";

        if (profile.find("ROG9") != std::string::npos) {
            model = "ASUS_AI2401_A"; brand = "asus"; mfr = "ASUS";
        } else if (profile.find("REDMAGIC11") != std::string::npos) {
            model = "NX769J"; brand = "ZTE"; mfr = "nubia";
        } else if (profile.find("S26ULTRA") != std::string::npos) {
            model = "SM-S948B"; brand = "samsung"; mfr = "samsung";
        } else if (profile.find("FOLD8") != std::string::npos) {
            model = "SM-F966B"; brand = "samsung"; mfr = "samsung";
        }

        __system_property_set("ro.product.model", model);
        __system_property_set("ro.product.brand", brand);
        __system_property_set("ro.product.manufacturer", mfr);
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {}
    void preServerSpecialize(ServerSpecializeArgs *) override {
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }
    void postServerSpecialize(const ServerSpecializeArgs *) override {}

private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
};

REGISTER_ZYGISK_MODULE(StellarTweaksModule)
