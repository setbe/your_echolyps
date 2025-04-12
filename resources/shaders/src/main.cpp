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

static const std::vector<std::string> kShaderFiles = {
    "default.vert",
    "default.frag"
};

std::vector<uint32_t> read_file_aligned_u32(const std::string& path);
std::string to_cpp_array(const std::string& name, const std::vector<uint32_t>& data);
std::string sanitize_name(const std::string& filename);

bool run_process(const std::wstring& command) {
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = {};
    std::wstring cmdline = command;

    BOOL success = CreateProcessW(
        nullptr, cmdline.data(), nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) {
        std::cerr << "[ERROR] CreateProcess failed: " << GetLastError() << "\n";
        return false;
    }

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
        fs::path shaderDir = currentDir;
        fs::path outputFile = currentDir / ".." / ".." / "src" / "shaders.hpp";

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

        std::ostringstream header;
        header << "// Auto-generated shader header\n\n";

        for (const auto& file : kShaderFiles) {
            fs::path inputPath = fs::canonical(shaderDir / file);
            fs::path spvPath = fs::absolute(currentDir / (file + ".spv"));

            std::wstringstream wcmd;
            wcmd << L"\"" << glslc.wstring() << L"\" \"" << inputPath.wstring() << L"\" -o \"" << spvPath.wstring() << L"\"";

            if (!run_process(wcmd.str())) {
                std::cerr << "[ERROR] Failed to compile: " << file << "\n";
                return 1;
            }

            auto words = read_file_aligned_u32(spvPath.string());
            std::string varName = sanitize_name(file);
            header << to_cpp_array(varName, words);
            std::remove(spvPath.string().c_str());
        }

        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile) {
            std::cerr << "[ERROR] Cannot write to output: " << outputFile << "\n";
            return 1;
        }
        outFile << header.str();
        std::cout << "[OK] Embedded shaders into: " << outputFile.string() << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[EXCEPTION] " << e.what() << "\n";
        return 1;
    }
    return 0;
}

std::vector<uint32_t> read_file_aligned_u32(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::vector<char> bytes((std::istreambuf_iterator<char>(f)), {});

    if (bytes.size() % 4 != 0) {
        throw std::runtime_error("SPIR-V file not aligned to 4 bytes: " + path);
    }
    std::vector<uint32_t> result(bytes.size() / 4);
    std::memcpy(result.data(), bytes.data(), bytes.size());
    return result;
}

std::string to_cpp_array(const std::string& name, const std::vector<uint32_t>& data) {
    std::ostringstream oss;
    oss << "const uint32_t " << name << "[] = {";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 8 == 0) {
            oss << "\n    ";
        }
        oss << "0x" << std::hex << std::uppercase << data[i];
        if (i + 1 < data.size()) {
            oss << ", ";
        }
    }
    oss << "\n};\n";
    oss << "const unsigned int " << name << "_len = " << std::dec << data.size() << ";\n\n";
    return oss.str();
}

std::string sanitize_name(const std::string& filename) {
    std::string result = filename;
    for (char& c : result) {
        if (c == '.' || c == '-') {
            c = '_';
        }
    }
    return result + "_spv";
}