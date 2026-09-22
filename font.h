/**
 * @file font.h
 * @brief Legacy bitmap font interface for kindle-myts.
 *
 * Provides functions and structures to load and reference rasterized bitmap
 * fonts for terminal character rendering.
 */

#ifndef _FONT_H_
#define _FONT_H_

#include "pixop.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes font lookup table from hex or font files.
 * @param path Path to primary font bitmap / hex file.
 * @param codepage_path Optional codepage translation file.
 * @param w Expected character pixel width.
 * @param h Expected character pixel height.
 * @return 0 on success, negative integer on error.
 */
int init_font(const char *path, const char *codepage_path, int w, int h);

/**
 * @brief Global default font pixmap instance used by screen rendering.
 */
extern struct font font_pixmap;

#ifdef __cplusplus
}
#endif

#endif /* _FONT_H_ */


