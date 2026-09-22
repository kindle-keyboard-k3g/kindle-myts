/*
 * (C) 2010 Andy
 *
 * eink screen routines
 */

#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "pixop.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief E-Ink update mode mask */
#define UMODE_MASK 11
/** @brief Buffer acts as mask for pixel update */
#define UMODE_BUFISMASK 14
/** @brief No screen update */
#define UMODE_NONE -1
/** @brief Full flash waveform refresh (GC16) */
#define UMODE_FLASH 20
/** @brief Inverted waveform flash */
#define UMODE_INVERT 21
/** @brief Partial monochrome update (fast) */
#define UMODE_PARTIAL 0	// 0 XXX full and partial are the same ?
/** @brief Full grayscale update */
#define UMODE_FULL 1

/**
 * @struct fbscreen
 * @brief Framebuffer device context and surface wrapper for Kindle screen.
 */
typedef struct fbscreen {
	int fd ;                 /**< Framebuffer /dev/fb0 file descriptor */
	int screensize ;         /**< Total mapped framebuffer memory size in bytes */
	int cur_x, cur_y;        /**< Coordinate cursors for string processing */
	pixmap_t pixmap ;        /**< Pixmap surface mapped onto framebuffer */
	struct font *font;       /**< Default font used for text rendering */
} fbscreen_t;

/**
 * @brief Opens and initializes the Kindle /dev/fb0 framebuffer device.
 * @return Allocated fbscreen_t context, or NULL on error.
 */
fbscreen_t *fb_open(void) ;

/**
 * @brief Closes the framebuffer device and unmaps display memory.
 * @param fb Framebuffer context to destroy.
 */
void	fb_close(fbscreen_t *fb) ;

/**
 * @brief Submits a partial or full rectangular update request to the e-ink controller.
 * @param fb Framebuffer context.
 * @param mode Update mode (e.g. UMODE_PARTIAL, UMODE_FULL, UMODE_FLASH).
 * @param x0 Left pixel bound.
 * @param y0 Top pixel bound.
 * @param x1 Right pixel bound.
 * @param y1 Bottom pixel bound.
 * @param pbuf Optional custom pixel buffer pointer.
 */
void	fb_update_area(fbscreen_t *fb, int mode, int x0, int y0, int x1, int y1, void *pbuf) ;

#ifdef __cplusplus
}
#endif

#endif
