#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>


// Determines final output filename from a given path
std::string get_output_filename(const std::string& path) {
    size_t pos1 = path.find_last_of("/\\");
    std::string lastComponent = (pos1 == std::string::npos) ? path : path.substr(pos1 + 1);
    if (lastComponent.find('.') != std::string::npos) {
        return path;
    }
    else {
        std::string dir = path;
        if (dir.back() != '/' && dir.back() != '\\') {
            dir.push_back('\\');
        }
        return dir + "shaders.hpp";
    }
}

// Reads a file into a byte vector
std::vector<unsigned char> read_file(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
        throw std::runtime_error("Cannot open file " + filename);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

// Converts binary data to C array in xxd -i style
std::string array_to_cpp(const std::string& array_name, const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    oss << "unsigned char " << array_name << "[] = {";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 12 == 0)
            oss << "\n    ";
        oss << "0x";
        oss << std::hex;
        oss.width(2);
        oss.fill('0');
        oss << static_cast<int>(data[i]);
        if (i != data.size() - 1)
            oss << ", ";
    }
    oss << "\n};\n";
    oss << "unsigned int " << array_name << "_len = " << std::dec << data.size() << ";\n\n";
    return oss.str();
}

int main(int argc, char* argv[]) {
    try {
        // Parse -o argument
        std::string outPath = "shaders.hpp";
        for (int i = 1; i < argc; ++i) {
            std::string arg(argv[i]);
            if (arg == "-o" && i + 1 < argc) {
                outPath = argv[++i];
            }
        }
        outPath = get_output_filename(outPath);

        // Check VULKAN_SDK environment variable
        char* vulkanSdk = std::getenv("VULKAN_SDK");
        if (!vulkanSdk) {
            std::cerr << "[ERROR] VULKAN_SDK environment variable not set.\n";
            return 1;
        }
        std::string vulkanSdkStr(vulkanSdk);
        std::string glslcPath = vulkanSdkStr + "\\Bin\\glslc.exe";

        // Check if glslc.exe exists
        std::ifstream glslcFile(glslcPath);
        if (!glslcFile.good()) {
            std::cerr << "[ERROR] glslc.exe not found at " << glslcPath << "\n";
            return 1;
        }
        glslcFile.close();

        // Compile vertex shader
        std::string compileVertCmd = "\"" + glslcPath + "\" default.vert -o default.vert.spv";
        std::cout << "Compiling vertex shader...\n";
        if (std::system(compileVertCmd.c_str()) != 0) {
            std::cerr << "[ERROR] Failed to compile default.vert\n";
            return 1;
        }

        // Compile fragment shader
        std::string compileFragCmd = "\"" + glslcPath + "\" default.frag -o default.frag.spv";
        std::cout << "Compiling fragment shader...\n";
        if (std::system(compileFragCmd.c_str()) != 0) {
            std::cerr << "[ERROR] Failed to compile default.frag\n";
            return 1;
        }

        std::cout << "[OK] Shaders compiled successfully.\n";

        // Read generated .spv files
        auto vertData = read_file("default.vert.spv");
        auto fragData = read_file("default.frag.spv");

        // Create output C++ header with embedded shader data
        std::ostringstream output;
        output << "// Auto-generated shader header\n\n";
        output << array_to_cpp("default_vert_spv", vertData);
        output << array_to_cpp("default_frag_spv", fragData);

        std::ofstream ofs(outPath, std::ios::binary);
        if (!ofs) {
            std::cerr << "[ERROR] Could not write to output file " << outPath << "\n";
            return 1;
        }
        ofs << output.str();
        ofs.close();
        std::cout << "Shaders embedded into " << outPath << "\n";

        // Remove temporary .spv files
        std::remove("default.vert.spv");
        std::remove("default.frag.spv");

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "[EXCEPTION] " << ex.what() << "\n";
        return 1;
    }
}
