/*
 * Copyright (C) 2010 Luigi Rizzo, Universita' di Pisa
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * $Id: dynstring.h 7958 2010-12-04 01:15:04Z luigi $
 *
 * An implementation of dynamic strings (and in general, extensible
 * data structures) inherited from the one i wrote myself for asterisk.
 *
 * This is similar to the libsbuf that is available in FreeBSD
 *
 * USE: declare the dynamic string:	dynstr s = NULL;
 * then use as asprintf(), e.g.		dsprintf(&s, fmt, ...);
 * or, to append a chunk of bytes:	ds_append(&s, ptr, len)
 *
 * Use ds_len(s), ds_data(s), ds_reset(s), ds_free(s) to get the
 * length, data pointer, reset the content, and free the memory.
 *
 * This code has been originally designed for strings, however
 * ds_append() supports appending arbitrary chunks of bytes to
 * the structure, and in fact it is very convenient to implement
 * some form of dynamic arrays.
 */

#ifndef __DYNSTRING_H
#define __DYNSTRING_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef dynstr
 * @brief Opaque handle to an extensible heap-allocated dynamic buffer structure.
 */
typedef struct __dynstr * dynstr;

/**
 * @brief Formats and appends data to dynamic string like sprintf.
 * @param s Address of pointer to dynstr.
 * @param fmt Printf-style format string.
 * @return Number of characters appended, or negative integer on error.
 */
int dsprintf(dynstr *s, const char *fmt, ...);

/**
 * @brief Appends raw binary or text data chunk to dynamic string.
 * @param s Address of pointer to dynstr.
 * @param d Pointer to source byte buffer.
 * @param len Number of bytes to append.
 * @return Updated length of buffer, or negative on error.
 */
int ds_append(dynstr *s, const void *d, int len);

/**
 * @brief Truncates or extends buffer to specified length.
 * @param s Address of pointer to dynstr.
 * @param desired_size Desired length in bytes.
 * @return 0 on success.
 */
int ds_truncate(dynstr *s, int desired_size);

/**
 * @brief Adjusts dynamic array buffer capacity to hold element at index i of size recsize.
 * @param s Address of pointer to dynstr.
 * @param i Zero-based index of target element.
 * @param recsize Size of each record/element in bytes.
 * @return 0 on success.
 */
int ds_adjust(dynstr *s, int i, int recsize);

/**
 * @brief Returns non-null pointer to character data inside buffer.
 * @param s Dynamic string handle.
 * @return Pointer to buffer contents (or "" if empty). Never returns NULL.
 */
const char *ds_data(dynstr s);

/**
 * @brief Returns current active content length in bytes.
 * @param s Dynamic string handle.
 * @return String length in bytes.
 */
int ds_len(dynstr s);

/**
 * @brief Returns total allocated capacity of buffer in bytes.
 * @param s Dynamic string handle.
 * @return Allocated size in bytes.
 */
int ds_size(dynstr s);

/**
 * @brief Shifts buffer content left by removing initial n bytes.
 * @param s Dynamic string handle.
 * @param n Number of bytes to drop from the front.
 * @return Resulting buffer length.
 */
int ds_shift(dynstr s, int n);

/**
 * @brief Resets dynamic string to zero length without freeing memory buffer.
 * @param s Dynamic string handle.
 */
void ds_reset(dynstr s);

/**
 * @brief Creates dynamic string with preallocated initial capacity.
 * @param len Initial buffer size in bytes.
 * @return Newly created dynstr handle.
 */
dynstr ds_create(int len);

/**
 * @brief Creates a read-only dynamic string wrapper referencing external buffer.
 * @param base Pointer to constant memory.
 * @param len Number of bytes referenced.
 * @return Read-only dynstr handle.
 */
dynstr ds_ref(const char *base, int len);

/**
 * @brief Deallocates dynamic string buffer and metadata.
 * @param s Dynamic string handle to free.
 * @return Always returns NULL for convenient assignment.
 */
void *ds_free(dynstr s);

#ifdef __cplusplus
}
#endif

#endif	/* __DYNSTRING_H */
