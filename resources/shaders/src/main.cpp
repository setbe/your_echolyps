#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

bool has_shader_extension(const fs::path& path) {
    const std::vector<std::string> exts = { ".vert", ".frag", ".geom", ".comp" };
    std::string ext = path.extension().string();
    return std::find(exts.begin(), exts.end(), ext) != exts.end();
}

std::string make_variable_name(const fs::path& path) {
    std::string name = path.stem().string() + "_" + path.extension().string().substr(1); // e.g., "default_frag"
    std::replace(name.begin(), name.end(), '-', '_'); // sanitize
    std::replace(name.begin(), name.end(), '.', '_');
    return name;
}

int main() {
    fs::path current_dir = fs::current_path();
    fs::path shader_dir = current_dir;
    fs::path output_file = current_dir / ".." / ".." / "src" / "shaders.hpp";

    std::ofstream out(output_file);
    if (!out) {
        std::cerr << "Cannot open output file: " << output_file << "\n";
        return 1;
    }

    out << "#pragma once\n\n";

    for (const auto& entry : fs::directory_iterator(shader_dir)) {
        if (!entry.is_regular_file() || !has_shader_extension(entry.path()))
            continue;

        std::ifstream file(entry.path());
        if (!file) {
            std::cerr << "Failed to open: " << entry.path() << "\n";
            continue;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        std::string var_name = make_variable_name(entry.path());

        out << "constexpr const char* " << var_name << " = R\"SHDR(" << content << ")SHDR\";\n\n";
    }

    std::cout << "Generated " << output_file << "\n";
    return 0;
}
