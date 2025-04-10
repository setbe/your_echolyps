#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdio>
#include <filesystem>

#include <windows.h>

namespace fs = std::filesystem;

// Hard-coded list of shader files to be compiled.
static const std::vector<std::string> kShaderFiles = {
    "default.vert",
    "default.frag"
};

// Function prototypes (defined after main()).
std::vector<unsigned char> read_file(const std::string& path);
std::string to_cpp_array(const std::string& name, const std::vector<unsigned char>& data);
std::string sanitize_name(const std::string& filename);

// Return true if success (exit code 0)
bool run_process(const std::wstring& command) {
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = {};

    // Copy line, because CreateProcessW changes it
    std::wstring cmdline = command;

    BOOL success = CreateProcessW(
        nullptr,
        cmdline.data(),  // mutable buffer
        nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW,  // no console
        nullptr, nullptr,
        &si, &pi
    );

    if (!success) {
        std::cerr << "[ERROR] CreateProcess failed: " << GetLastError() << "\n";
        return false;
    }

    // Wait
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

int main() {
    try {
        fs::path currentDir = fs::current_path();

        // Suppose we want all the shaders in "../" relative to currentDir:
        // e.g. currentDir is ".../vs", we want ".../resources/shaders".
        // We'll combine these carefully:
        fs::path shaderDir = currentDir;

        // The output .hpp file:
        fs::path outputFile = currentDir / ".." / ".." / "src" / "shaders.hpp";

        // Check Vulkan SDK
        char* vulkanSdk = std::getenv("VULKAN_SDK");
        if (!vulkanSdk) {
            std::cerr << "[ERROR] VULKAN_SDK environment variable not set.\n";
            return 1;
        }
        fs::path glslc = fs::path(vulkanSdk) / "Bin" / "glslc.exe";

        if (!fs::exists(glslc)) {
            std::cerr << "[ERROR] glslc.exe not found: " << glslc << "\n";
            return 1;
        }

        // Prepare the header buffer
        std::ostringstream header;
        header << "// Auto-generated shader header\n\n";

        for (const auto& file : kShaderFiles) {
            // First build the nominal path, then canonicalize it:
            fs::path nominalInput = shaderDir / file;
            // If the file must exist prior to compilation, canonical() should succeed:
            fs::path inputPath = fs::canonical(nominalInput);

            // For the .spv, we can place it in the same directory or in currentDir.
            // If it doesn't exist yet, canonical() might fail. So we can do something like:
            fs::path spvPath = fs::absolute(currentDir / (file + ".spv"));

            std::wstringstream wcmd;
            wcmd << L"\"" << glslc.wstring() << L"\" "
                << L"\"" << inputPath.wstring() << L"\" "
                << L"-o \"" << spvPath.wstring() << L"\"";

            if (!run_process(wcmd.str())) {
                std::cerr << "[ERROR] Failed to compile: " << file << "\n";
                return 1;
            }

            // Read .spv
            auto bytes = read_file(spvPath.string());

            // Convert to C++ array
            std::string varName = sanitize_name(file);
            header << to_cpp_array(varName, bytes);

            // Remove the .spv
            std::remove(spvPath.string().c_str());
        }

        // Write out the final .hpp
        {
            std::ofstream outFile(outputFile, std::ios::binary);
            if (!outFile) {
                std::cerr << "[ERROR] Cannot write to output: " << outputFile << "\n";
                return 1;
            }
            outFile << header.str();
        }
        std::cout << "[OK] Embedded shaders into: " << outputFile.string() << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[EXCEPTION] " << e.what() << "\n";
        return 1;
    }
    return 0;
}

/*
    Reads a binary file and returns its contents as a vector of unsigned char.
*/
std::vector<unsigned char> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    return { std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>() };
}

/*
    Converts a vector of bytes into a valid C++ array declaration (xxd -i style).
*/
std::string to_cpp_array(const std::string& name, const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    oss << "unsigned char " << name << "[] = {";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 12 == 0) {
            oss << "\n    ";
        }
        oss << "0x" << std::hex << std::uppercase;
        oss.width(2);
        oss.fill('0');
        oss << static_cast<int>(data[i]);
        if (i + 1 < data.size()) {
            oss << ", ";
        }
    }
    oss << "\n};\n";
    oss << "unsigned int " << name << "_len = " << std::dec << data.size() << ";\n\n";
    return oss.str();
}

/*
    Replaces certain characters (e.g. '.' or '-') in the filename
    to form a valid identifier, then appends "_spv" to the end.
*/
std::string sanitize_name(const std::string& filename) {
    std::string result = filename;
    for (char& c : result) {
        if (c == '.' || c == '-') {
            c = '_';
        }
    }
    return result + "_spv";
}
