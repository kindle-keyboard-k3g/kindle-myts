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
 *
 * $Id: terminal.h 7959 2010-12-04 07:37:03Z luigi $
 */

#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
struct sess;

/**
 * @brief Creates a new terminal session with PTY subshell, grid buffers, and scrollback.
 * @param cmd Initial command to execute (e.g. "/bin/sh").
 * @param name Unique identifier name for session.
 * @param rows Screen rows.
 * @param cols Screen columns.
 * @param sb_lines Scrollback buffer line capacity.
 * @param cb Callback invoked on session exit/destruction.
 * @return Allocated struct sess pointer, or NULL on failure.
 */
struct sess *term_new(char *cmd, const char *name,
	int rows, int cols, int sb_lines, void (*cb)(struct sess *));

/**
 * @brief Finds an active terminal session by name.
 * @param name Name identifier.
 * @return Matching struct sess pointer, or NULL if not found.
 */
struct sess *term_find(const char *name);

/**
 * @brief Retrieves the assigned name of a session.
 * @param s Terminal session pointer.
 * @return C string session name.
 */
const char *term_name(struct sess *s);

/**
 * @brief Injects keyboard input sequence directly into session's PTY master descriptor.
 * @param s Terminal session.
 * @param k Nul-terminated character sequence.
 * @return Number of bytes written, or negative on error.
 */
int term_keyin(struct sess *s, char *k);

/**
 * @brief Sends a POSIX signal to the child process running inside the terminal session.
 * @param sh Terminal session.
 * @param sig Signal number (e.g. SIGINT, SIGHUP, SIGTERM).
 * @return Result of kill() syscall.
 */
int term_kill(struct sess *sh, int sig);

/**
 * @enum term_state_flags
 * @brief Bit flags indicating which fields to update when calling term_state.
 */
enum { TS_MOD = 1, TS_CB = 2, TS_NAME = 4 };

/**
 * @struct term_state
 * @brief Snapshot of terminal emulator dimensions, screen buffer, and cursor state.
 */
struct term_state {
	int flags;                   /**< Update mask flags (TS_MOD, TS_CB, TS_NAME) */
	int modified;                /**< Dirty flag indicating visual updates */
	int rows;                    /**< Grid rows */
	int cols;                    /**< Grid cols */
	int cur;                     /**< Linear cursor offset (row * cols + col) */
	int pid;                     /**< Child process PID */
	int top;                     /**< Top visible line offset */
	void (*cb)(struct sess *);   /**< Event callback */
	char *name;                  /**< Session name */
	char *data;                  /**< Character grid memory */
	char *attr;                  /**< Cell attributes memory */
	char *sb_data;               /**< Scrollback character memory */
	char *sb_attr;               /**< Scrollback attribute memory */
};

/**
 * @brief Queries or updates the snapshot state of a terminal session.
 * @param sh Terminal session.
 * @param ptr Pointer to term_state struct to fill or apply.
 * @return Dirty/modified status (non-zero if modified).
 */
int term_state(struct sess *sh, struct term_state *ptr);

#ifdef __cplusplus
}
#endif

#endif /* _TERMINAL_H_ */
