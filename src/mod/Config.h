#pragma once
#include <string>

namespace headbob {

enum class Mode {
    Default,
    Bodycam,
    Comfort,
    Custom
};

struct Config {
    bool  enabled           = true;
    bool  idleEnabled       = true;
    bool  enableWhenMounted = false;
    bool  enableThirdPerson = false;

    Mode  mode              = Mode::Bodycam;

    float globalStrength    = 0.32f;
    float walkMult          = 1.0f;
    float runMult           = 1.25f;
    float stepStrength      = 1.0f;
    float stepRate          = 1.0f;
    float swayStrength      = 1.0f;
    float cornerStrength    = 1.0f;
    float breathStrength    = 1.0f;
    float breathRate        = 0.25f;
    float driftStrength     = 1.0f;
    float driftRate         = 0.20f;
    float idleShakeMult     = 1.0f;
    float smoothHz          = 1.75f;
    float damping           = 1.25f;
    float fadeLerp          = 0.018f;

    std::string cameraBlendSig;
    std::string localPlayerTickSig;
    std::string moduleName = "libminecraftpe.so";

    static Config& get();
    void loadDefaults();
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;
};

std::string toJson(const Config& c, bool pretty = true);
std::string schemaJson();
std::string modeToString(Mode m);
Mode stringToMode(const std::string& s);

} // namespace headbob
