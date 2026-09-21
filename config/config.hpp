#ifndef MYTS_CONFIG_CONFIG_HPP
#define MYTS_CONFIG_CONFIG_HPP

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cctype>
#include <cstdlib>

namespace myts {
namespace config {

/**
 * @brief Case-insensitive ASCII equality comparison.
 */
inline bool iequals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Represents a single key-value configuration entry.
 */
struct Entry {
    std::string key;
    std::string value;
};

/**
 * @brief Represents an INI section containing multiple key-value entries.
 */
struct Section {
    std::string name;
    std::vector<Entry> entries;

    [[nodiscard]] std::optional<std::string_view> get(std::string_view key) const noexcept {
        for (const auto& entry : entries) {
            if (iequals(entry.key, key)) {
                return std::string_view(entry.value);
            }
        }
        return std::nullopt;
    }
};

/**
 * @brief INI configuration parser and storage container.
 *
 * Implements case-insensitive lookup, comment handling, and whitespace trimming.
 */
class Config {
public:
    Config() = default;

    /**
     * @brief Parses INI-formatted text.
     * @param text String view of INI file contents.
     * @return true on successful parsing.
     */
    bool load_text(std::string_view text) {
        sections_.clear();
        Section* cur_sec = nullptr;

        size_t start = 0;
        while (start < text.size()) {
            size_t end = text.find('\n', start);
            if (end == std::string_view::npos) {
                end = text.size();
            }

            std::string_view line = text.substr(start, end - start);
            start = end + 1;

            // Trim leading/trailing whitespace
            while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front()))) {
                line.remove_prefix(1);
            }
            while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) {
                line.remove_suffix(1);
            }

            // Ignore empty lines and full line comments
            if (line.empty() || line.front() == '#' || line.front() == ';') {
                continue;
            }

            // Section header
            if (line.front() == '[' && line.back() == ']') {
                std::string_view sec_name = line.substr(1, line.size() - 2);
                sections_.push_back(Section{std::string(sec_name), {}});
                cur_sec = &sections_.back();
                continue;
            }

            // Key = Value entry
            size_t eq_pos = line.find('=');
            if (eq_pos != std::string_view::npos && cur_sec != nullptr) {
                std::string_view k = line.substr(0, eq_pos);
                std::string_view v = line.substr(eq_pos + 1);

                // Strip inline comments in value (if any)
                size_t comment_pos = v.find_first_of(";#");
                if (comment_pos != std::string_view::npos) {
                    v = v.substr(0, comment_pos);
                }

                // Trim k
                while (!k.empty() && std::isspace(static_cast<unsigned char>(k.front()))) {
                    k.remove_prefix(1);
                }
                while (!k.empty() && std::isspace(static_cast<unsigned char>(k.back()))) {
                    k.remove_suffix(1);
                }

                // Trim v
                while (!v.empty() && std::isspace(static_cast<unsigned char>(v.front()))) {
                    v.remove_prefix(1);
                }
                while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back()))) {
                    v.remove_suffix(1);
                }

                cur_sec->entries.push_back(Entry{std::string(k), std::string(v)});
            }
        }
        return true;
    }

    /**
     * @brief Looks up a string value by section and key name.
     */
    [[nodiscard]] std::optional<std::string_view> get(std::string_view section, std::string_view key) const noexcept {
        for (const auto& sec : sections_) {
            if (iequals(sec.name, section)) {
                return sec.get(key);
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Looks up an integer value with fallback.
     */
    [[nodiscard]] int get_int(std::string_view section, std::string_view key, int default_val) const noexcept {
        auto val = get(section, key);
        if (!val.has_value()) {
            return default_val;
        }
        return std::atoi(std::string(*val).c_str());
    }

private:
    std::vector<Section> sections_;
};

} // namespace config
} // namespace myts

#endif // MYTS_CONFIG_CONFIG_HPP
