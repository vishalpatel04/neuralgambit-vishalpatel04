#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace neuralgambit {

// Minimal parser for the flat {"fen": "solution"} objects used by the
// mate_in_N.json puzzle files. Not a general-purpose JSON parser.
inline std::vector<std::pair<std::string, std::string>> parseFlatJsonObject(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open puzzle file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string text = buffer.str();

    std::vector<std::pair<std::string, std::string>> entries;

    auto readString = [&text](size_t& i) {
        // Assumes text[i] == '"'.
        std::string value;
        ++i;
        while (i < text.size() && text[i] != '"') {
            if (text[i] == '\\' && i + 1 < text.size()) {
                ++i;
            }
            value += text[i];
            ++i;
        }
        ++i;  // Skip closing quote.
        return value;
    };

    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '"') {
            std::string key = readString(i);
            // Skip whitespace/colon to the value string.
            while (i < text.size() && text[i] != '"') ++i;
            std::string value = readString(i);
            entries.emplace_back(std::move(key), std::move(value));
        } else {
            ++i;
        }
    }
    return entries;
}

}  // namespace neuralgambit
