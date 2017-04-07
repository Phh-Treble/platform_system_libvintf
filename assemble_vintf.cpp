/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <string>

#include <vintf/parse_string.h>
#include <vintf/parse_xml.h>

namespace android {
namespace vintf {

/**
 * Slurps the device manifest file and add build time flag to it.
 */
class AssembleVintf {
public:
    template<typename T>
    static bool getFlag(const std::string& key, T* value) {
        const char *envValue = getenv(key.c_str());
        if (envValue == NULL) {
            std::cerr << "Required " << key << " flag." << std::endl;
            return false;
        }

        if (!parse(envValue, value)) {
            std::cerr << "Cannot parse " << envValue << "." << std::endl;
            return false;
        }
        return true;
    }

    static bool assemble(std::basic_istream<char>& inFile, std::basic_ostream<char>& outFile) {
        std::stringstream ss;
        ss << inFile.rdbuf();
        std::string fileContent = ss.str();

        HalManifest halManifest;
        if (!gHalManifestConverter(&halManifest, fileContent)) {
            std::cerr << "Illformed HAL manifest: " << gHalManifestConverter.lastError()
                    << std::endl;
            return false;
        }

        if (halManifest.mType == SchemaType::DEVICE) {
            if (!getFlag("BOARD_SEPOLICY_VERS", &halManifest.device.mSepolicyVersion)) {
                return false;
            }
        }

        std::string outFileContent = gHalManifestConverter(halManifest);
        outFile << outFileContent;
        outFile.flush();

        return true;
    }
};

}  // namespace vintf
}  // namespace android

void help() {
    std::cerr <<
        "assemble_vintf -h\n"
        "               Display this help text.\n"
        "assemble_vintf -i <input file> [-o <output file>]\n"
        "               Fill in build-time flags into the given manifest.\n"
        "               If no designated output file, write to stdout.\n";
}

int main(int argc, char **argv) {
    std::ifstream inFile;
    std::ofstream outFile;
    std::ostream* outFileRef = &std::cout;
    int res;
    while((res = getopt(argc, argv, "hi:o:v:")) >= 0) {
        switch (res) {
            case 'i': {
                inFile.open(optarg);
                if (!inFile.is_open()) {
                    std::cerr << "Failed to open " << optarg << std::endl;
                    return 1;
                }
            } break;

            case 'o': {
                outFile.open(optarg);
                if (!outFile.is_open()) {
                    std::cerr << "Failed to open " << optarg << std::endl;
                    return 1;
                }
                outFileRef = &outFile;
            } break;

            case 'h':
            default: {
                help();
                return 1;
            } break;
        }
    }

    if (!inFile.is_open()) {
        std::cerr << "Missing input file" << std::endl;
        help();
        return 1;
    }

    return ::android::vintf::AssembleVintf::assemble(inFile, *outFileRef)
            ? 0 : 1;
}

