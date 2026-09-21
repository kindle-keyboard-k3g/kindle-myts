#ifndef MYTS_DISPLAY_HARDWARE_EINK_DRIVER_HPP
#define MYTS_DISPLAY_HARDWARE_EINK_DRIVER_HPP

#include "graphics/eink_display.hpp"
#include "graphics/geometry.hpp"
#include "core/raii.hpp"

#include <linux/types.h>
typedef uint8_t u8;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "linux/einkfb.h"
#pragma GCC diagnostic pop

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <vector>
#include <cstring>

namespace myts {
namespace display {

/**
 * @brief Concrete E-Ink hardware driver interfacing with /dev/fb0 and Kindle einkfb ioctls.
 *
 * Automatically provides an in-memory fallback raster surface when running on
 * development host machines or CI environments without Kindle hardware.
 */
class HardwareEinkDriver : public graphics::IEinkDriver {
public:
    /**
     * @brief Constructs driver and initializes framebuffer connection.
     * @param fb_path Filesystem path to framebuffer device (default "/dev/fb0").
     */
    explicit HardwareEinkDriver(const char* fb_path = "/dev/fb0")
        : width_(600), height_(800), is_hardware_(false), update_count_(0) {
        init_framebuffer(fb_path);
    }

    ~HardwareEinkDriver() override = default;

    HardwareEinkDriver(const HardwareEinkDriver&) = delete;
    HardwareEinkDriver& operator=(const HardwareEinkDriver&) = delete;
    HardwareEinkDriver(HardwareEinkDriver&&) noexcept = default;
    HardwareEinkDriver& operator=(HardwareEinkDriver&&) noexcept = default;

    /**
     * @brief True if connected to real Linux framebuffer device, false if running in mock mode.
     */
    [[nodiscard]] bool is_hardware() const noexcept {
        return is_hardware_;
    }

    /**
     * @brief Framebuffer pixel width.
     */
    [[nodiscard]] int width() const noexcept {
        return width_;
    }

    /**
     * @brief Framebuffer pixel height.
     */
    [[nodiscard]] int height() const noexcept {
        return height_;
    }

    /**
     * @brief Pointer to mapped or allocated 4bpp pixel memory.
     */
    [[nodiscard]] uint8_t* surface_data() noexcept {
        if (is_hardware_ && mmap_.data() != nullptr) {
            return mmap_.data();
        }
        return mock_buffer_.data();
    }

    /**
     * @brief Const pointer to mapped or allocated 4bpp pixel memory.
     */
    [[nodiscard]] const uint8_t* surface_data() const noexcept {
        if (is_hardware_ && mmap_.data() != nullptr) {
            return mmap_.data();
        }
        return mock_buffer_.data();
    }

    /**
     * @brief Size of surface in bytes.
     */
    [[nodiscard]] size_t surface_size() const noexcept {
        return static_cast<size_t>((width_ * height_) / 2);
    }

    /**
     * @brief Cumulative count of refresh operations executed.
     */
    [[nodiscard]] size_t update_count() const noexcept {
        return update_count_;
    }

    /**
     * @brief Performs full display refresh (GC16 flash).
     */
    bool update_display_full() override {
        ++update_count_;
        if (!is_hardware_ || !fd_.is_valid()) {
            return true;
        }

        int ret = ::ioctl(fd_.get(), FBIO_EINK_UPDATE_DISPLAY, fx_update_full);
        return ret == 0;
    }

    /**
     * @brief Performs partial display refresh for the specified dirty region.
     * @param area Bounding box to refresh.
     */
    bool update_display_area(const graphics::Rect& area) override {
        ++update_count_;
        if (area.is_empty()) {
            return true;
        }

        if (!is_hardware_ || !fd_.is_valid()) {
            return true;
        }

        update_area_t ua{};
        ua.x1 = area.x;
        ua.y1 = area.y;
        ua.x2 = area.x + area.width;
        ua.y2 = area.y + area.height;
        ua.which_fx = fx_update_partial;
        ua.buffer = nullptr;

        int ret = ::ioctl(fd_.get(), FBIO_EINK_UPDATE_DISPLAY_AREA, &ua);
        return ret == 0;
    }

private:
    void init_framebuffer(const char* fb_path) {
        if (fb_path == nullptr) {
            setup_mock_fallback();
            return;
        }

        int raw_fd = ::open(fb_path, O_RDWR);
        if (raw_fd < 0) {
            setup_mock_fallback();
            return;
        }

        fd_.reset(raw_fd);

        struct fb_var_screeninfo vinfo{};
        if (::ioctl(fd_.get(), FBIOGET_VSCREENINFO, &vinfo) == 0) {
            width_ = static_cast<int>(vinfo.xres);
            height_ = static_cast<int>(vinfo.yres);
        } else {
            width_ = 600;
            height_ = 800;
        }

        size_t screensize = static_cast<size_t>((width_ * height_) / 2);
        if (!mmap_.map(fd_.get(), screensize, PROT_READ | PROT_WRITE, MAP_SHARED)) {
            fd_.reset();
            setup_mock_fallback();
            return;
        }

        is_hardware_ = true;
    }

    void setup_mock_fallback() {
        is_hardware_ = false;
        width_ = 600;
        height_ = 800;
        mock_buffer_.resize(static_cast<size_t>((width_ * height_) / 2), 0xFF);
    }

    int width_{600};
    int height_{800};
    bool is_hardware_{false};
    size_t update_count_{0};
    core::UniqueFd fd_;
    core::MemoryMapping mmap_;
    std::vector<uint8_t> mock_buffer_;
};

} // namespace display
} // namespace myts

#endif // MYTS_DISPLAY_HARDWARE_EINK_DRIVER_HPP
