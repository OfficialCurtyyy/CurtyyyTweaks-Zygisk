#include <jni.h>
#include <string>
#include <vector>
#include <sys/system_properties.h>
#include <zygisk.hpp>
#include <fstream>
#include <sstream>

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

// Target Games List (Updated with your requests)
const std::vector<std::string> TARGET_GAMES = {
    "com.tencent.ig", 
    "com.pubg.krmobile", 
    "com.activision.callofduty.shooter",
    "com.miHoYo.GenshinImpact", 
    "com.miHoYo.hkrpg", 
    "com.epicgames.fortnite",
    "com.mobilelegends.mi",      // Mobile Legends
    "com.garena.game.codm",      // Call of Duty Mobile (Garena)
    "com.roblox.client",         // Roblox
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

        // Check if the launched app is in our target games list
        bool is_target = false;
        for (const auto& target : TARGET_GAMES) {
            if (pkg == target) {
                is_target = true;
                break;
            }
        }

        // If not a target game, unload module to save memory
        if (!is_target) {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        // Read configuration file safely using standard C++ file I/O
        std::ifstream file("/data/adb/stellartweaks/config");
        std::string profile = "DEFAULT";
        if (file.is_open()) {
            std::getline(file, profile);
            file.close();
        }

        // Apply Spoofs based on WebUI selection
        if (profile.find("ROG9") != std::string::npos) {
            __system_property_set("ro.product.model", "ASUS_AI2401_A");
            __system_property_set("ro.product.brand", "asus");
            __system_property_set("ro.product.manufacturer", "ASUS");
        } 
        else if (profile.find("REDMAGIC11") != std::string::npos) {
            __system_property_set("ro.product.model", "NX769J");
            __system_property_set("ro.product.brand", "ZTE");
            __system_property_set("ro.product.manufacturer", "nubia");
        }
        else if (profile.find("S26ULTRA") != std::string::npos) {
            __system_property_set("ro.product.model", "SM-S948B");
            __system_property_set("ro.product.brand", "samsung");
            __system_property_set("ro.product.manufacturer", "samsung");
        }
        else if (profile.find("FOLD8") != std::string::npos) {
            __system_property_set("ro.product.model", "SM-F966B");
            __system_property_set("ro.product.brand", "samsung");
            __system_property_set("ro.product.manufacturer", "samsung");
        }
        else {
            // Default fallback (RedMagic 10 Pro)
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
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(StellarTweaksModule)
