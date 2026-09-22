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
 * $Id: config.h 8060 2010-12-14 01:12:22Z luigi $
 *
 * Parser for config files -- similar to the .ini files
 *
 * Each section starts with [name]
 * and has multiple "key = value" entries.
 * Whitespace is generally allowed both in keys and values,
 * a ';' starts a comment unless it is in quotes, and # on the
 * left hand side also acts as a comment marker.
 * A trivial 'include = filename' statement is also supported.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct entry
 * @brief Key-value pair in a configuration file section.
 */
struct entry {
	struct entry *next; /**< Linked list pointer to next key-value entry */
	char *key;          /**< Pointer to null-terminated key name */
	char *value;        /**< Pointer to null-terminated key value */
	uint16_t len1;      /**< Metadata length */
};

struct section;
struct config;

/**
 * @brief Loads a configuration file (or immediate string if path starts with '\n').
 * @param path File path or immediate string.
 * @param base Base path for relative include statements.
 * @param old Existing configuration object to extend, or NULL to allocate new.
 * @return Loaded struct config pointer, or NULL on failure.
 */
struct config * cfg_read(const char *path, const char *base,
	struct config *old);

/**
 * @brief Frees configuration database and all nested sections and entries.
 * @param pdb Configuration pointer to release.
 */
void cfg_free(struct config *pdb) ;

/**
 * @brief Finds a section by name, or iterates sections.
 * @param cfg Configuration context.
 * @param name Section name (case-insensitive), or NULL to start iteration.
 * @return Pointer to struct section, or NULL if not found.
 */
struct section *cfg_find_section(struct config *cfg, const char *name) ;

/**
 * @brief Finds an entry by key name in the given section.
 * @param s Section pointer.
 * @param key Key name to look up, or NULL to retrieve first entry.
 * @return Const pointer to entry struct, or NULL if not found.
 */
const struct entry *cfg_find_entry(const struct section *s, const char *key) ;

/**
 * @brief Retrieves value string for given section name (or section pointer) and key.
 * @param cfg Configuration context (or NULL if sec is struct section*).
 * @param sec Section name or section pointer.
 * @param key Key name.
 * @return String value, or NULL if not found.
 */
const char *cfg_find_val(struct config *cfg, const char *sec, const char *key);

/**
 * @brief Returns name string of given section.
 * @param s Section pointer.
 * @return Section name string.
 */
const char *cfg_section_name(const struct section *s);

/**
 * @brief Advances pointer past ASCII whitespace characters.
 * @param p Pointer to input string.
 * @return First non-whitespace character pointer.
 */
char *skipws(char *p);

/**
 * @brief Trims trailing whitespace characters before end pointer in place.
 * @param s Pointer to start of string.
 * @param end Pointer to end of string.
 * @return Trimmed null-terminated string pointer.
 */
char *trimws(char *s, char *end);

#ifdef __cplusplus
}
#endif

#endif /* _CONFIG_H_ */
