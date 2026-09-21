#ifndef MYTS_CORE_STRING_BUILDER_HPP
#define MYTS_CORE_STRING_BUILDER_HPP

#include <string_view>
#include <string>
#include <cstddef>
#include <cstdio>

namespace myts {
namespace core {

/**
 * @brief Lightweight, zero-overhead string builder optimized for embedded Linux.
 *
 * Wraps std::string to provide bounded memory usage, convenient decimal formatting,
 * and string_view interoperability without invoking <iostream>.
 */
class StringBuilder {
public:
    /**
     * @brief Constructs an empty StringBuilder.
     */
    StringBuilder() = default;

    /**
     * @brief Preallocates buffer memory.
     * @param cap Capacity to reserve.
     */
    void reserve(size_t cap) {
        str_.reserve(cap);
    }

    /**
     * @brief Appends string_view text.
     * @param sv Text to append.
     */
    void append(std::string_view sv) {
        str_.append(sv.data(), sv.size());
    }

    /**
     * @brief Appends a single ASCII character.
     * @param c Character to append.
     */
    void append_char(char c) {
        str_.push_back(c);
    }

    /**
     * @brief Formats and appends an integer value.
     * @param val Integer value to format.
     */
    void append_decimal(int val) {
        char buf[32];
        int n = std::snprintf(buf, sizeof(buf), "%d", val);
        if (n > 0) {
            str_.append(buf, static_cast<size_t>(n));
        }
    }

    /**
     * @brief Returns a non-owning std::string_view of current contents.
     */
    [[nodiscard]] std::string_view view() const noexcept {
        return std::string_view(str_);
    }

    /**
     * @brief Returns a null-terminated C string.
     */
    [[nodiscard]] const char* c_str() const noexcept {
        return str_.c_str();
    }

    /**
     * @brief Returns current string length.
     */
    [[nodiscard]] size_t size() const noexcept {
        return str_.size();
    }

    /**
     * @brief Resets string length to zero without releasing capacity.
     */
    void clear() noexcept {
        str_.clear();
    }

private:
    std::string str_;
};

} // namespace core
} // namespace myts

#endif // MYTS_CORE_STRING_BUILDER_HPP
