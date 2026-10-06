#include "mod/Config.h"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    const char* outJson   = (argc > 1) ? argv[1] : "config.json";
    const char* outSchema = (argc > 2) ? argv[2] : "config.schema.json";

    headbob::Config cfg;
    cfg.loadDefaults();

    {
        std::ofstream f(outJson);
        if (!f) {
            std::cerr << "Failed to write " << outJson << "\n";
            return 1;
        }
        f << headbob::toJson(cfg, true);
    }
    {
        std::ofstream f(outSchema);
        if (!f) {
            std::cerr << "Failed to write " << outSchema << "\n";
            return 1;
        }
        f << headbob::schemaJson();
    }
    std::cout << "Wrote " << outJson << " and " << outSchema << "\n";
    return 0;
}
