/* buffer.c - automatic buffer structure */

/*
 * Copyright (c) 2008, Natacha Porté
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * COMPILE TIME OPTIONS
 *
 * BUFFER_STATS • if defined, stats are kept about memory usage
 * TRACK_BUFFERS • if defined, buffers are tracked in a struct parray
 */
#define BUFFER_STATS



#define BUFFER_STDARG

#include "buffer.h"

#ifdef TRACK_BUFFER_DEBUG
#undef TRACK_BUFFERS
#endif

#if (defined TRACK_BUFFERS) || (defined TRACK_BUFFER_DEBUG)
#include "array.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/********************
 * GLOBAL VARIABLES *
 ********************/

#ifdef BUFFER_STATS
long buffer_stat_nb = 0;
size_t buffer_stat_alloc_bytes = 0;
#endif
#ifdef TRACK_BUFFERS
struct parray all_buffers = { 0, 0, 0 };
#endif
#ifdef TRACK_BUFFER_DEBUG
struct array all_buffers = { 0, 0, 0, sizeof (struct buf_debug_data) };
#endif


/***************************
 * STATIC HELPER FUNCTIONS *
 ***************************/

/* lower • retruns the lower-case variant of the input char */
static char
lower(char c) {
	return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c; }



/********************
 * BUFFER FUNCTIONS *
 ********************/

/* bufcasecmp • case-insensitive buffer comparison */
int
bufcasecmp(const struct buf *a, const struct buf *b) {
	size_t i = 0;
	size_t cmplen;
	if (a == b) return 0;
	if (!a) return -1; else if (!b) return 1;
	cmplen = (a->size < b->size) ? a->size : b->size;
	while (i < cmplen && lower(a->data[i]) == lower(b->data[i])) ++i;
	if (i < a->size) {
		if (i < b->size)  return lower(a->data[i]) - lower(b->data[i]);
		else		  return 1; }
	else {	if (i < b->size)  return -1;
		else		  return 0; } }


/* bufcmp • case-sensitive buffer comparison */
int
bufcmp(const struct buf *a, const struct buf *b) {
	size_t i = 0;
	size_t cmplen;
	if (a == b) return 0;
	if (!a) return -1; else if (!b) return 1;
	cmplen = (a->size < b->size) ? a->size : b->size;
	while (i < cmplen && a->data[i] == b->data[i]) ++i;
	if (i < a->size) {
		if (i < b->size)	return a->data[i] - b->data[i];
		else			return 1; }
	else {	if (i < b->size)	return -1;
		else			return 0; } }


/* bufcmps • case-sensitive comparison of a string to a buffer */
int
bufcmps(const struct buf *a, const char *b) {
	const size_t len = strlen(b);
	size_t cmplen = len;
	int r;
	if (!a || !a->size) return b ? 0 : -1;
	if (len < a->size) cmplen = a->size;
	r = strncmp(a->data, b, cmplen);
	if (r) return r;
	else if (a->size == len) return 0;
	else if (a->size < len) return -1;
	else return 1; }


/* bufdup • buffer duplication */
struct buf *
#ifndef TRACK_BUFFER_DEBUG
bufdup(const struct buf *src, size_t dupunit) {
#else
bufdup_(const struct buf *src, size_t dupunit, const char *file, int line) {
	struct buf_debug_data *bdd;
#endif
	size_t blocks;
	struct buf *ret;
	if (src == 0) return 0;
	ret = malloc(sizeof (struct buf));
	if (ret == 0) return 0;
	ret->unit = dupunit;
	ret->size = src->size;
	ret->ref = 1;
	if (!src->size) {
#ifdef BUFFER_STATS
		buffer_stat_nb += 1;
#endif
		ret->asize = 0;
		ret->data = 0;
		return ret; }
	blocks = (src->size + dupunit - 1) / dupunit;
	ret->asize = blocks * dupunit;
	ret->data = malloc(ret->asize);
	if (ret->data == 0) {
		free(ret);
		return 0; }
	memcpy(ret->data, src->data, src->size);
#ifdef BUFFER_STATS
	buffer_stat_nb += 1;
	buffer_stat_alloc_bytes += ret->asize;
#endif
#ifdef TRACK_BUFFERS
	parr_push(&all_buffers, ret);
#endif
#ifdef TRACK_BUFFER_DEBUG
	bdd = arr_item(&all_buffers, arr_newitem(&all_buffers));
	bdd->buf = ret;
	bdd->dupped = 1;
	bdd->ctime = time(0);
	bdd->file = file;
	bdd->line = line;
	return ret; }
#else
	return ret; }
#endif


/* bufgrow • increasing the allocated size to the given value */
int
bufgrow(struct buf *buf, size_t neosz) {
	size_t neoasz;
	void *neodata;
	if (!buf || !buf->unit) return 0;
	if (buf->asize >= neosz) return 1;
	neoasz = buf->asize + buf->unit;
	while (neoasz < neosz) neoasz += buf->unit;
	neodata = realloc(buf->data, neoasz);
	if (!neodata) return 0;
#ifdef BUFFER_STATS
	buffer_stat_alloc_bytes += (neoasz - buf->asize);
#endif
	buf->data = neodata;
	buf->asize = neoasz;
	return 1; }


/* bufnew • allocation of a new buffer */
struct buf *
#ifndef TRACK_BUFFER_DEBUG
bufnew(size_t unit) {
#else
bufnew_(size_t unit, const char *file, int line) {
	struct buf_debug_data *bdd;
#endif
	struct buf *ret;
	ret = malloc(sizeof (struct buf));
	if (ret) {
#ifdef BUFFER_STATS
		buffer_stat_nb += 1;
#endif
#ifdef TRACK_BUFFERS
		parr_push(&all_buffers, ret);
#endif
		ret->data = 0;
		ret->size = ret->asize = 0;
		ret->ref = 1;
		ret->unit = unit; }
#ifdef TRACK_BUFFER_DEBUG
	if (ret) {
		bdd = arr_item(&all_buffers, arr_newitem(&all_buffers));
		bdd->buf = ret;
		bdd->dupped = 0;
		bdd->ctime = time(0);
		bdd->file = file;
		bdd->line = line; }
	return ret; }
#else
	return ret; }
#endif


/* bufnullterm • NUL-termination of the string array (making a C-string) */
void
bufnullterm(struct buf *buf) {
	if (!buf || !buf->unit) return;
	if (buf->size < buf->asize && buf->data[buf->size] == 0) return;
	if (bufgrow(buf, buf->size + 1))
		buf->data[buf->size] = 0; }


/* bufprintf • formatted printing to a buffer */
void
bufprintf(struct buf *buf, const char *fmt, ...) {
	va_list ap;
	if (!buf || !buf->unit) return;
	va_start(ap, fmt);
	vbufprintf(buf, fmt, ap);
	va_end(ap); }


/* bufput • appends raw data to a buffer */
void
bufput(struct buf *buf, const void *data, size_t len) {
	if (!buf || !bufgrow(buf, buf->size + len)) return;
	memcpy(buf->data + buf->size, data, len);
	buf->size += len; }


/* bufputs • appends a NUL-terminated string to a buffer */
void
bufputs(struct buf *buf, const char *str) {
	bufput(buf, str, strlen (str)); }


/* bufputc • appends a single char to a buffer */
void
bufputc(struct buf *buf, char c) {
	if (!buf || !bufgrow(buf, buf->size + 1)) return;
	buf->data[buf->size] = c;
	buf->size += 1; }


/* bufrelease • decrease the reference count and free the buffer if needed */
void
bufrelease(struct buf *buf) {
	if (!buf || !buf->unit) return;
	buf->ref -= 1;
	if (buf->ref == 0) {
#ifdef TRACK_BUFFERS
		int i = 0;
		while (i < all_buffers.size)
			if (all_buffers.item[i] == buf)
				parr_remove(&all_buffers, i);
			else i += 1;
#endif
#ifdef TRACK_BUFFER_DEBUG
		int i = 0;
		struct buf_debug_data *bdd = all_buffers.base;
		while (i < all_buffers.size)
			if (bdd[i].buf == buf)
				arr_remove(&all_buffers, i);
			else i += 1;
#endif
#ifdef BUFFER_STATS
		buffer_stat_nb -= 1;
		buffer_stat_alloc_bytes -= buf->asize;
#endif
		free(buf->data);
		free(buf); } }


/* bufreset • frees internal data of the buffer */
void
bufreset(struct buf *buf) {
	if (!buf || !buf->unit || !buf->asize) return;
#ifdef BUFFER_STATS
	buffer_stat_alloc_bytes -= buf->asize;
#endif
	free(buf->data);
	buf->data = 0;
	buf->size = buf->asize = 0; }


/* bufset • safely assigns a buffer to another */
void
bufset(struct buf **dest, struct buf *src) {
	if (src) {
		if (src->data && !src->asize) src = bufdup(src, 1);
		else src->ref += 1; }
	bufrelease(*dest);
	*dest = src; }


/* bufslurp • removes a given number of bytes from the head of the array */
void
bufslurp(struct buf *buf, size_t len) {
	if (!buf || !buf->unit || len <= 0) return;
	if (len >= buf->size) {
		buf->size = 0;
		return; }
	buf->size -= len;
	memmove(buf->data, buf->data + len, buf->size); }


/* buftoi • converts the numbers at the beginning of the buf into an int */
int
buftoi(struct buf *buf, size_t offset_i, size_t *offset_o) {
	int r = 0, neg = 0;
	size_t i = offset_i;
	if (!buf || !buf->size) return 0;
	if (buf->data[i] == '+') i += 1;
	else if (buf->data[i] == '-') {
		neg = 1;
		i += 1; }
	while (i < buf->size && buf->data[i] >= '0' && buf->data[i] <= '9') {
		r = (r * 10) + buf->data[i] - '0';
		i += 1; }
	if (offset_o) *offset_o = i;
	return neg ? -r : r; }



/* vbufprintf • stdarg variant of formatted printing into a buffer */
void
vbufprintf(struct buf *buf, const char *fmt, va_list ap) {
	int n;
	va_list ap_save;
	if (buf == 0
	|| (buf->size >= buf->asize && !bufgrow (buf, buf->size + 1)))
		return;
	va_copy(ap_save, ap);
	n = vsnprintf(buf->data + buf->size, buf->asize - buf->size, fmt, ap);
	if (n >= buf->asize - buf->size) {
		if (!bufgrow (buf, buf->size + n + 1)) return;
		n = vsnprintf (buf->data + buf->size,
				buf->asize - buf->size, fmt, ap_save); }
	va_end(ap_save);
	if (n < 0) return;
	buf->size += n; }

/* vim: set filetype=c: */
