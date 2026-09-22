/*
 * (C) 2010 Andy
 *
 * routines to use the eink screen
 */

#ifndef _PIXOP_H_
#define _PIXOP_H_

/**
 * @struct pixmap_str
 * @brief 4bpp packed raster surface representation (1 byte = 2 pixels).
 */
typedef struct pixmap_str {
	int	width;                /**< Surface width in pixels */
	int	height;               /**< Surface height in pixels */
	int	bpp;                  /**< Bits per pixel (typically 4) */
	unsigned char	*surface; /**< Pointer to allocated pixel byte buffer */
} pixmap_t;

/**
 * @struct font
 * @brief Character bitmap font metrics and raster cache.
 */
struct font {
	int	code_first;           /**< Lowest supported character codepoint */
	int	code_last;            /**< Highest supported character codepoint */
	int	width;                /**< Glyph width in pixels */
	int	height;               /**< Glyph height in pixels */
	int	bpp;                  /**< Bits per pixel */
	unsigned char	*pixmap;  /**< Contiguous glyph bitmap surface */
};

/**
 * @brief Truncates offset and length to fit within a bounded coordinate interval.
 * @param ofs Starting coordinate offset (adjusted if negative).
 * @param len Span length (clamped to remain within bound).
 * @param bound Upper bound of valid coordinate space.
 */
static inline void c_truncate(int *ofs, int *len, int bound)
{
        if (*ofs < 0) { /* decrease size by offset */
                *len += *ofs;
                *ofs = 0;
        }
	if (*ofs > bound) {
		*len = 0;
		*ofs = bound;
	}
	if (*len <= 0)
		*len = 0;
        if (*ofs + *len > bound)
                *len = bound - *ofs;
}

/**
 * @brief Retrieves the pixmap slice for a specific character code from font table.
 * @param f Font definition struct.
 * @param code Character codepoint.
 * @param ppx Target pixmap struct populated with glyph slice.
 * @return 0 on success, non-zero if code out of range.
 */
int get_char_pixmap(const struct font *f, int code, pixmap_t *ppx) ;

/**
 * @brief Loads FNG font binary from file path.
 * @param path Filesystem path to font file.
 * @return Pointer to loaded font structure, or NULL on failure.
 */
const struct font * getfngfont(const char *path) ;

/**
 * @brief Releases memory associated with a loaded FNG font.
 * @param f Font struct pointer to free.
 */
void freefngfont(const struct font *f) ;

/**
 * @brief Blits rectangular source pixel region into destination pixmap with clipping.
 * @param dst Destination surface.
 * @param dx Destination top-left X coordinate.
 * @param dy Destination top-left Y coordinate.
 * @param src Source surface.
 * @param sx Source top-left X coordinate.
 * @param sy Source top-left Y coordinate.
 * @param width Width of rectangular block to copy.
 * @param height Height of rectangular block to copy.
 * @param bg Background color keying / fill mode.
 * @return 0 on success.
 */
int pix_blt(pixmap_t* dst, int dx, int dy,
	pixmap_t* src, int sx, int sy, int width, int height, int bg) ;

/**
 * @brief Allocates a new 4bpp pixmap of specified width and height.
 * @param w Width in pixels.
 * @param h Height in pixels.
 * @return Pointer to newly allocated pixmap_t, or NULL on OOM.
 */
pixmap_t * pix_alloc(int w, int h) ;

/**
 * @brief Frees a previously allocated pixmap structure and its pixel surface.
 * @param p Pixmap pointer to release.
 */
void pix_free(pixmap_t *p) ;

#endif
