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
 * $Id: myts.h 7958 2010-12-04 01:15:04Z luigi $

This is a framework for event-based programming.
The entire state of the application is reachable through
a global "struct my_args _me".

Each sub-application is described by a "struct app" which
contains callbacks for boot, argument parsing, callbacks,
destructor, and private data.

 */

#ifndef _MYTS_H_
#define	_MYTS_H_
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>	/* gettimeofday */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>	/* inet_aton */

#ifdef __cplusplus
extern "C" {
#endif

extern int verbose;
#ifdef NODEBUG
#define DBG(...) do { } while (0)
#else
#define DBG(level, format, ...)  do {                   \
        if (verbose >= level) {   \
		struct timeval now; gettimeofday(&now, NULL); \
                fprintf(stderr, "%5d.%03d [%-14.14s %4d] " format,       \
			(int)(now.tv_sec %86400), (int)(now.tv_usec / 1000), \
                        __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        } } while(0)
#endif

/**
 * @struct app
 * @brief Sub-application plugin descriptor with lifecycle callbacks.
 */
struct app {
	int (*init)(void);                  /**< Initializer callback */
	int (*parse)(int *argc, char *argv[]); /**< Command-line option parsing */
	int (*start)(void);                 /**< Application startup */
	int (*end)(void);                   /**< Application teardown */
	void *data;                         /**< Pointer to app private state */
};

/**
 * @struct cb_args
 * @brief Callback context passed to select() dispatch handlers.
 */
struct cb_args {
	struct timeval now;   /**< Current wall-clock timestamp */
	struct timeval due;   /**< Earliest due descriptor / timer */
	fd_set *r;            /**< Read file descriptor set */
	fd_set *w;            /**< Write file descriptor set */
	int maxfd;            /**< Highest active file descriptor number */
	int run;              /**< Dispatch mode (0 = prepare select, 1 = handle ready descriptors) */
};

/** @brief Session event handler function pointer type */
typedef int (*cb_fn)(void *sess, struct cb_args *a);

/**
 * @struct sess
 * @brief Base session tracking active descriptor and event callback in the event loop.
 */
struct sess {
	struct sess *next;  /**< Linked list successor */
	struct app *app;    /**< Parent application plugin */
	cb_fn	cb;         /**< Dispatch callback */
	void *arg;          /**< Arbitrary user argument/tag */
	int fd;             /**< Watched file descriptor */
};

/**
 * @struct my_args
 * @brief Global application state and session registry.
 */
struct my_args {
	struct app **all_apps;	// array of all applications
	struct app *app;	// app under service
	struct sess *sess;
	struct sess *tmp_sess;
	struct sess *cur;	// session under service
	int verbose;	/* allow read all file systems */
};
extern struct my_args __me;

/**
 * @brief Allocates and registers a new event session in the global loop.
 * @param size Memory allocation size for session struct.
 * @param fd File descriptor to monitor.
 * @param cb Event callback function.
 * @param arg User context pointer.
 * @return Allocated session pointer.
 */
void *new_sess(int size, int fd, cb_fn cb, void *arg);

/**
 * @brief Adds milliseconds to a timeval struct.
 * @param src Starting timeval.
 * @param ms Milliseconds to add.
 * @param dst Output destination timeval.
 */
void timeradd_ms(const struct timeval *src, int ms, struct timeval *dst);

/**
 * @brief Sets dst timeval to the minimum of dst and cur.
 * @param dst Target timeval to update.
 * @param cur Candidate timeval.
 */
void timersetmin(struct timeval *dst, const struct timeval *cur);

/**
 * @brief Checks if timer is armed and expired relative to now.
 * @param dst Timer timestamp.
 * @param now Current timestamp.
 * @return Non-zero if expired, 0 otherwise.
 */
int timerdue(const struct timeval *dst, const struct timeval *now);

extern int bytesperchar;

#ifdef __cplusplus
}
#endif

#endif /* _MYTS_H_ */
