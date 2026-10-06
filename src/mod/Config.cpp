#include "mod/Config.h"
#include <fstream>
#include <sstream>

namespace headbob {

Config& Config::get() {
    static Config instance;
    return instance;
}

void Config::loadDefaults() {
    *this = Config{};
    mode = Mode::Bodycam;
    // MC 1.26.32x (v126322) — "Multiplayer level - tick camera systems" xref
    // unique=true matches=1 @ 0x9f13e0c
    cameraBlendSig =
        "F4 4F 06 A9 FD 03 01 91 54 D0 3B D5 F3 03 00 AA 88 ?? ?? F9 A8 83 1F F8 ?? ?? ?? D0 08 A1 11 91 08 FD DF 08 ?? ?? ?? ?? ?? ?? ?? D0 21 E0 0C 91 E0 63 00 91 ?? ?? ?? 95";
    moduleName = "libminecraftpe.so";
}

std::string modeToString(Mode m) {
    switch (m) {
        case Mode::Bodycam: return "bodycam";
        case Mode::Comfort: return "comfort";
        case Mode::Custom:  return "custom";
        default:            return "default";
    }
}

Mode stringToMode(const std::string& s) {
    if (s == "bodycam") return Mode::Bodycam;
    if (s == "comfort") return Mode::Comfort;
    if (s == "custom")  return Mode::Custom;
    return Mode::Default;
}

namespace {

bool extractBool(const std::string& json, const std::string& key, bool& out) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    auto t = json.find("true", pos);
    auto f = json.find("false", pos);
    if (t != std::string::npos && (f == std::string::npos || t < f)) { out = true; return true; }
    if (f != std::string::npos) { out = false; return true; }
    return false;
}

bool extractFloat(const std::string& json, const std::string& key, float& out) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    try {
        size_t idx = 0;
        out = std::stof(json.substr(pos), &idx);
        return true;
    } catch (...) { return false; }
}

bool extractString(const std::string& json, const std::string& key, std::string& out) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return false;
    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return false;
    out = json.substr(pos + 1, end - pos - 1);
    return true;
}

} // namespace

std::string toJson(const Config& c, bool pretty) {
    const char* nl = pretty ? "\n" : "";
    const char* ind = pretty ? "  " : "";
    std::ostringstream o;
    o << "{" << nl
      << ind << "\"enabled\": " << (c.enabled ? "true" : "false") << "," << nl
      << ind << "\"idleEnabled\": " << (c.idleEnabled ? "true" : "false") << "," << nl
      << ind << "\"enableWhenMounted\": " << (c.enableWhenMounted ? "true" : "false") << "," << nl
      << ind << "\"enableThirdPerson\": " << (c.enableThirdPerson ? "true" : "false") << "," << nl
      << ind << "\"mode\": \"" << modeToString(c.mode) << "\"," << nl
      << ind << "\"globalStrength\": " << c.globalStrength << "," << nl
      << ind << "\"walkMult\": " << c.walkMult << "," << nl
      << ind << "\"runMult\": " << c.runMult << "," << nl
      << ind << "\"stepStrength\": " << c.stepStrength << "," << nl
      << ind << "\"stepRate\": " << c.stepRate << "," << nl
      << ind << "\"swayStrength\": " << c.swayStrength << "," << nl
      << ind << "\"cornerStrength\": " << c.cornerStrength << "," << nl
      << ind << "\"breathStrength\": " << c.breathStrength << "," << nl
      << ind << "\"breathRate\": " << c.breathRate << "," << nl
      << ind << "\"driftStrength\": " << c.driftStrength << "," << nl
      << ind << "\"driftRate\": " << c.driftRate << "," << nl
      << ind << "\"idleShakeMult\": " << c.idleShakeMult << "," << nl
      << ind << "\"smoothHz\": " << c.smoothHz << "," << nl
      << ind << "\"damping\": " << c.damping << "," << nl
      << ind << "\"fadeLerp\": " << c.fadeLerp << "," << nl
      << ind << "\"moduleName\": \"" << c.moduleName << "\"," << nl
      << ind << "\"cameraBlendSig\": \"" << c.cameraBlendSig << "\"," << nl
      << ind << "\"localPlayerTickSig\": \"" << c.localPlayerTickSig << "\"" << nl
      << "}" << nl;
    return o.str();
}

std::string schemaJson() {
    return R"({
  "title": "Realistic Head Bob",
  "type": "object",
  "properties": {
    "enabled": { "type": "boolean", "default": true },
    "mode": {
      "type": "string",
      "enum": ["default", "bodycam", "comfort", "custom"],
      "default": "bodycam"
    },
    "globalStrength": { "type": "number", "minimum": 0, "maximum": 3, "default": 0.32 },
    "stepStrength": { "type": "number", "minimum": 0, "maximum": 3, "default": 1.0 },
    "swayStrength": { "type": "number", "minimum": 0, "maximum": 3, "default": 1.0 },
    "smoothHz": { "type": "number", "minimum": 0.25, "maximum": 6, "default": 1.75 },
    "damping": { "type": "number", "minimum": 0.1, "maximum": 3, "default": 1.25 },
    "moduleName": { "type": "string", "default": "libminecraftpe.so" },
    "cameraBlendSig": { "type": "string", "default": "" },
    "localPlayerTickSig": { "type": "string", "default": "" }
  }
}
)";
}

bool Config::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::stringstream buf;
    buf << f.rdbuf();
    const std::string json = buf.str();

    extractBool(json, "enabled", enabled);
    extractBool(json, "idleEnabled", idleEnabled);
    extractBool(json, "enableWhenMounted", enableWhenMounted);
    extractBool(json, "enableThirdPerson", enableThirdPerson);

    std::string modeStr;
    if (extractString(json, "mode", modeStr))
        mode = stringToMode(modeStr);

    extractFloat(json, "globalStrength", globalStrength);
    extractFloat(json, "walkMult", walkMult);
    extractFloat(json, "runMult", runMult);
    extractFloat(json, "stepStrength", stepStrength);
    extractFloat(json, "stepRate", stepRate);
    extractFloat(json, "swayStrength", swayStrength);
    extractFloat(json, "cornerStrength", cornerStrength);
    extractFloat(json, "breathStrength", breathStrength);
    extractFloat(json, "breathRate", breathRate);
    extractFloat(json, "driftStrength", driftStrength);
    extractFloat(json, "driftRate", driftRate);
    extractFloat(json, "idleShakeMult", idleShakeMult);
    extractFloat(json, "smoothHz", smoothHz);
    extractFloat(json, "damping", damping);
    extractFloat(json, "fadeLerp", fadeLerp);
    extractString(json, "moduleName", moduleName);
    extractString(json, "cameraBlendSig", cameraBlendSig);
    extractString(json, "localPlayerTickSig", localPlayerTickSig);
    return true;
}

bool Config::saveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << toJson(*this, true);
    return true;
}

} // namespace headbob
