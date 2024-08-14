/**
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License") 1.1!
 * You may not use this file except in compliance with the License.
 *
 * See  https://spdx.org/licenses/CDDL-1.1.html  for the specific
 * language governing permissions and limitations under the License.
 *
 * Copyright 2002 Jens Elkner (jel+origin@linofee.org)
 */
#ifndef _ORIGIN_H
#define _ORIGIN_H
#ifdef  __cplusplus
extern "C" {
#endif

/* Get the origin of a program. Returns %NULL if unable to determine the
 * origin.
 */
char* get_origin(void);
/* Get the origin of a program, cut off up trailing sub dirs and append tail.
 * Returns a copy of %fallback if unable to determine the origin.
 */
char* get_origin_rel(int up, const char *tail, const char *fallback);
#endif
#ifdef __cplusplus
}
#endif
