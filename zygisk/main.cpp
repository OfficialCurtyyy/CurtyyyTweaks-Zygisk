#include <fstream>
#include <string>
#include <map>

static std::map<std::string,std::string> g_spoof;

static void loadSpoof(){
    g_spoof.clear();
    std::ifstream f("/data/adb/curtyyytweaks/spoof.props");
    std::string line;
    while (std::getline(f, line)){
        auto eq = line.find('=');
        if (eq != std::string::npos) g_spoof[line.substr(0,eq)] = line.substr(eq+1);
    }
}
// In preAppSpecialize, after confirming it's a target game:
loadSpoof();
auto get = [&](const char*k, const char*fb){ auto it=g_spoof.find(k); return it!=g_spoof.end()?it->second.c_str():fb; };
__system_property_set("ro.product.model",        get("model","NX729J"));
__system_property_set("ro.product.brand",        get("brand","ZTE"));
__system_property_set("ro.product.manufacturer", get("manufacturer","nubia"));
