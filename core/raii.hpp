#ifndef MYTS_CORE_RAII_HPP
#define MYTS_CORE_RAII_HPP

#include <unistd.h>
#include <sys/mman.h>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace myts {
namespace core {

/**
 * @brief Manages ownership of a POSIX file descriptor using RAII semantics.
 *
 * Ensures that the wrapped file descriptor is closed automatically upon object destruction.
 * Move-only semantics guarantee single-ownership semantics without double-close errors.
 */
class UniqueFd {
public:
    /**
     * @brief Constructs an empty UniqueFd representing no descriptor (-1).
     */
    constexpr UniqueFd() noexcept : fd_(-1) {}

    /**
     * @brief Explicitly adopts ownership of an existing open file descriptor.
     * @param fd The raw file descriptor to take ownership of.
     */
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}

    /**
     * @brief Destructor that closes the held descriptor if valid (>= 0).
     */
    ~UniqueFd() noexcept {
        reset();
    }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    /**
     * @brief Move constructor transferring descriptor ownership.
     * @param other The source UniqueFd being moved from.
     */
    UniqueFd(UniqueFd&& other) noexcept : fd_(other.release()) {}

    /**
     * @brief Move assignment operator transferring descriptor ownership.
     * @param other The source UniqueFd being moved from.
     * @return Reference to this object.
     */
    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    /**
     * @brief Closes the currently held descriptor (if any) and takes ownership of a new one.
     * @param new_fd The new descriptor to hold, defaults to -1.
     */
    void reset(int new_fd = -1) noexcept {
        if (fd_ >= 0) {
            close(fd_);
        }
        fd_ = new_fd;
    }

    /**
     * @brief Relinquishes ownership of the held descriptor without closing it.
     * @return The raw descriptor value, or -1 if none was held.
     */
    [[nodiscard]] int release() noexcept {
        int old_fd = fd_;
        fd_ = -1;
        return old_fd;
    }

    /**
     * @brief Accesses the underlying raw descriptor.
     * @return The descriptor value, or -1 if invalid.
     */
    [[nodiscard]] int get() const noexcept {
        return fd_;
    }

    /**
     * @brief Checks if this object currently holds an open descriptor.
     * @return true if valid (>= 0), false otherwise.
     */
    [[nodiscard]] bool is_valid() const noexcept {
        return fd_ >= 0;
    }

    /**
     * @brief Boolean conversion operator checking validity.
     * @return true if valid (>= 0), false otherwise.
     */
    explicit operator bool() const noexcept {
        return is_valid();
    }

private:
    int fd_;
};

/**
 * @brief Manages ownership of a mapped memory region (mmap / munmap) using RAII semantics.
 *
 * Automatically calls munmap upon destruction to guarantee zero memory leaks.
 * Non-copyable and move-only to enforce unique ownership of the virtual address space.
 */
class MemoryMapping {
public:
    /**
     * @brief Constructs an unmapped MemoryMapping instance.
     */
    constexpr MemoryMapping() noexcept : data_(nullptr), size_(0) {}

    /**
     * @brief Destructor unmapping the memory if valid.
     */
    ~MemoryMapping() noexcept {
        unmap();
    }

    MemoryMapping(const MemoryMapping&) = delete;
    MemoryMapping& operator=(const MemoryMapping&) = delete;

    /**
     * @brief Move constructor transferring mapping ownership.
     * @param other The source MemoryMapping instance being moved from.
     */
    MemoryMapping(MemoryMapping&& other) noexcept
        : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    /**
     * @brief Move assignment operator transferring mapping ownership.
     * @param other The source MemoryMapping instance being moved from.
     * @return Reference to this object.
     */
    MemoryMapping& operator=(MemoryMapping&& other) noexcept {
        if (this != &other) {
            unmap();
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    /**
     * @brief Maps a file or anonymous memory region.
     * @param fd File descriptor to map, or -1 for anonymous mappings.
     * @param size Number of bytes to map.
     * @param prot Memory protection flags (e.g. PROT_READ | PROT_WRITE).
     * @param flags Map flags (e.g. MAP_SHARED or MAP_PRIVATE).
     * @param offset Offset in file where mapping begins.
     * @return true if map succeeded, false on error.
     */
    bool map(int fd, size_t size, int prot, int flags, off_t offset = 0) noexcept {
        unmap();
        if (size == 0) {
            return false;
        }
        void* addr = mmap(nullptr, size, prot, flags, fd, offset);
        if (addr == MAP_FAILED) {
            return false;
        }
        data_ = static_cast<uint8_t*>(addr);
        size_ = size;
        return true;
    }

    /**
     * @brief Unmaps the memory region if currently mapped.
     */
    void unmap() noexcept {
        if (data_ != nullptr && data_ != MAP_FAILED && size_ > 0) {
            munmap(data_, size_);
        }
        data_ = nullptr;
        size_ = 0;
    }

    /**
     * @brief Gets pointer to the beginning of the mapped memory region.
     * @return Pointer to byte data, or nullptr if not mapped.
     */
    [[nodiscard]] uint8_t* data() noexcept {
        return data_;
    }

    /**
     * @brief Gets read-only pointer to the mapped memory region.
     * @return Const pointer to byte data, or nullptr if not mapped.
     */
    [[nodiscard]] const uint8_t* data() const noexcept {
        return data_;
    }

    /**
     * @brief Returns size in bytes of the mapped region.
     * @return Size in bytes.
     */
    [[nodiscard]] size_t size() const noexcept {
        return size_;
    }

    /**
     * @brief Checks if this object currently holds an active mapping.
     * @return true if mapped, false otherwise.
     */
    [[nodiscard]] bool is_valid() const noexcept {
        return data_ != nullptr && data_ != MAP_FAILED && size_ > 0;
    }

    /**
     * @brief Boolean conversion checking mapping validity.
     * @return true if mapped, false otherwise.
     */
    explicit operator bool() const noexcept {
        return is_valid();
    }

private:
    uint8_t* data_;
    size_t size_;
};

} // namespace core
} // namespace myts

#endif // MYTS_CORE_RAII_HPP
