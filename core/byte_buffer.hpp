#ifndef MYTS_CORE_BYTE_BUFFER_HPP
#define MYTS_CORE_BYTE_BUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace myts {
namespace core {

/**
 * @brief Non-owning view of a contiguous byte sequence.
 */
struct ByteView {
    const uint8_t* data{nullptr};
    size_t size{0};

    constexpr ByteView() noexcept = default;
    constexpr ByteView(const uint8_t* d, size_t s) noexcept : data(d), size(s) {}
};

/**
 * @brief Contiguous, growable byte array designed for binary frames and raw I/O.
 *
 * Avoids exceptions and heavy std::vector headers, using malloc/realloc internally
 * with explicit error handling to strictly protect embedded Kindle memory.
 */
class ByteBuffer {
public:
    /**
     * @brief Constructs an empty ByteBuffer.
     */
    constexpr ByteBuffer() noexcept : data_(nullptr), size_(0), capacity_(0) {}

    /**
     * @brief Destructor releasing heap allocated memory.
     */
    ~ByteBuffer() noexcept {
        free(data_);
    }

    ByteBuffer(const ByteBuffer&) = delete;
    ByteBuffer& operator=(const ByteBuffer&) = delete;

    /**
     * @brief Move constructor transferring buffer ownership.
     */
    ByteBuffer(ByteBuffer&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    /**
     * @brief Move assignment operator transferring buffer ownership.
     */
    ByteBuffer& operator=(ByteBuffer&& other) noexcept {
        if (this != &other) {
            free(data_);
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    /**
     * @brief Appends raw bytes to the end of the buffer.
     * @param bytes Pointer to input byte data.
     * @param len Number of bytes to append.
     * @return true if appended successfully, false on allocation failure.
     */
    bool append(const void* bytes, size_t len) noexcept {
        if (bytes == nullptr || len == 0) {
            return true;
        }
        if (!reserve(size_ + len)) {
            return false;
        }
        std::memcpy(data_ + size_, bytes, len);
        size_ += len;
        return true;
    }

    /**
     * @brief Reserves minimum capacity without changing buffer size.
     * @param new_cap Desired capacity in bytes.
     * @return true if capacity ensured, false on allocation failure.
     */
    bool reserve(size_t new_cap) noexcept {
        if (new_cap <= capacity_) {
            return true;
        }
        size_t alloc_cap = capacity_ == 0 ? 64 : capacity_ * 2;
        if (alloc_cap < new_cap) {
            alloc_cap = new_cap;
        }
        uint8_t* new_data = static_cast<uint8_t*>(std::realloc(data_, alloc_cap));
        if (new_data == nullptr) {
            return false;
        }
        data_ = new_data;
        capacity_ = alloc_cap;
        return true;
    }

    /**
     * @brief Discards n bytes from the front of the buffer, shifting the rest left.
     * @param n Number of prefix bytes to remove.
     * @return true if shifted, false if n exceeds buffer size.
     */
    bool shift_prefix(size_t n) noexcept {
        if (n > size_) {
            return false;
        }
        if (n == 0) {
            return true;
        }
        size_t remaining = size_ - n;
        if (remaining > 0) {
            std::memmove(data_, data_ + n, remaining);
        }
        size_ = remaining;
        return true;
    }

    /**
     * @brief Clears buffer contents without releasing allocated memory.
     */
    void clear() noexcept {
        size_ = 0;
    }

    /**
     * @brief Returns pointer to mutable byte data.
     */
    [[nodiscard]] uint8_t* data() noexcept {
        return data_;
    }

    /**
     * @brief Returns const pointer to byte data.
     */
    [[nodiscard]] const uint8_t* data() const noexcept {
        return data_;
    }

    /**
     * @brief Returns current number of bytes stored.
     */
    [[nodiscard]] size_t size() const noexcept {
        return size_;
    }

    /**
     * @brief Returns total capacity allocated.
     */
    [[nodiscard]] size_t capacity() const noexcept {
        return capacity_;
    }

    /**
     * @brief Returns non-owning ByteView of the buffer contents.
     */
    [[nodiscard]] ByteView view() const noexcept {
        return ByteView(data_, size_);
    }

private:
    uint8_t* data_;
    size_t size_;
    size_t capacity_;
};

} // namespace core
} // namespace myts

#endif // MYTS_CORE_BYTE_BUFFER_HPP
