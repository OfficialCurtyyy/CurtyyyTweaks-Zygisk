#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <zygisk.hpp>

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

// Target Games List
const std::vector<std::string> TARGET_GAMES = {
    "com.tencent.ig", "com.pubg.krmobile", "com.activision.callofduty.shooter",
    "com.miHoYo.GenshinImpact", "com.miHoYo.hkrpg", "com.epicgames.fortnite",
    "com.mobilelegends.mi", "com.supercell.brawlstars", "com.riotgames.league.wildrift"
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

        // If not a target game, unload module to save memory
        if (!is_target) {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        // Connect to companion process to read config safely
        int fd = api->connectCompanion();
        if (fd >= 0) {
            char buffer[256] = {0};
            read(fd, buffer, sizeof(buffer));
            close(fd);

            std::string profile(buffer);
            
            // Spoof based on selected device profile
            if (profile.find("ROG9") != std::string::npos) {
                spoofDevice("ASUS_AI2401_A", "asus", "ASUS");
            } 
            else if (profile.find("REDMAGIC11") != std::string::npos) {
                spoofDevice("NX769J", "ZTE", "nubia");
            }
            else if (profile.find("S26ULTRA") != std::string::npos) {
                spoofDevice("SM-S948B", "samsung", "samsung");
            }
            else if (profile.find("FOLD8") != std::string::npos) {
                spoofDevice("SM-F966B", "samsung", "samsung");
            }
            else {
                // Default to RedMagic 10 Pro
                spoofDevice("NX729J", "ZTE", "nubia");
            }
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {}
    
    void preServerSpecialize(ServerSpecializeArgs *args) override {
        // Never inject into system_server
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }
    void postServerSpecialize(const ServerSpecializeArgs *args) override {}

private:
    Api *api;
    JNIEnv *env;

    // Helper function to send spoofing commands to the companion daemon
    void spoofDevice(const char* model, const char* brand, const char* manufacturer) {
        // In Zygisk, we write the desired properties to a temporary file 
        // or pass them via the companion socket. For this module, we use 
        // the companion to set props dynamically.
        
        int fd = api->connectCompanion();
        if (fd >= 0) {
            // Send command format: "SET|model|brand|manufacturer"
            std::string cmd = std::string("SET|") + model + "|" + brand + "|" + manufacturer;
            write(fd, cmd.c_str(), cmd.length());
            close(fd);
        }
    }
};

// Companion handler runs in the root daemon context
static void companion_handler(int fd) {
    char buffer[256] = {0};
    read(fd, buffer, sizeof(buffer));
    
    std::string cmd(buffer);
    if (cmd.find("SET|") == 0) {
        // Parse the string
        // Format: SET|model|brand|manufacturer
        size_t p1 = cmd.find('|', 4);
        size_t p2 = cmd.find('|', p1 + 1);
        
        if (p1 != std::string::npos && p2 != std::string::npos) {
            std::string model = cmd.substr(4, p1 - 4);
            std::string brand = cmd.substr(p1 + 1, p2 - p1 - 1);
            std::string mfr = cmd.substr(p2 + 1);
            
            // Because companion runs as root, we can safely use system() 
            // to set properties via resetprop
            std::string sys_cmd = "resetprop ro.product.model \"" + model + "\" && "
                                  "resetprop ro.product.brand \"" + brand + "\" && "
                                  "resetprop ro.product.manufacturer \"" + mfr + "\"";
            system(sys_cmd.c_str());
        }
    }
}

REGISTER_ZYGISK_MODULE(StellarTweaksModule)
REGISTER_ZYGISK_COMPANION(companion_handler)
