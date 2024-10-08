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
#include "c_origin.h"

#ifdef __sun
#define _STRUCTURED_PROC 1
#include <sys/fcntl.h>
#include <procfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#else /* __linux */
#include <bsd/string.h>
#include <unistd.h>
#endif /* __sun__  aka Solaris */
#include <assert.h>
#include <stdlib.h>
#include <sys/param.h>

/**
 * get_origin_rel:
 *
 * Get the ORIGIN aka path to this running program, cut off the given number
 * of directories starting from the end and finally appends the given tail.
 * No checks are made, whether the thus constructed path actually exists!
 *
 * @param up   .. number of directories to cut off. Values &lt; 1 are treated
 *                as 0, i.e. just the basename of this application gets removed.
 *				  If path consists of a "/" only, cutting off stops no matter,
 *                whether <var>up</var> sub pathes have been cut off or not. 
 * @param tail .. path to append, after the required parts have been cut off.
 *                If %NULL or <var>up</var> == 0 or exec path cannot be
 *				  determined, nothing gets append. Otherwise, a '/' (if the
 *				  resulting path is != '/') and the tail gets append.
 * @param fallback	if the origin cannot be determined, return a duplicate
 *					of this parameter (see strdup);
 *
 * E.g. if started via /opt/apps/bin/myapp it would return
 * /opt/apps/bin for get_origin_rel(0, NULL) or /opt/apps/lib for
 * get_origin_rel(1, "lib").
 *
 * Return value: a newly-allocated string that must be freed with free(),
 *   which is a copy of %fallback if the path cannot be determined.
 */
char *
get_origin_rel(int up, const char *tail, const char *fallback)
{
#if defined(__linux__) || defined(__sun)
  char my_origin[MAXPATHLEN] = { '\0' };
  int olen = 0;
#ifdef __sun
  /* get the name from initial argv[0] */
  psinfo_t ps;
  int fd;
  char **argv;

  if ( (fd = open("/proc/self/psinfo", O_RDONLY)) > -1 ) {
    const char *execname;
    int res = read( fd, &ps, sizeof(psinfo_t) );
    if (res > 0) {
      argv = (char **) ps.pr_argv;
      /* always try argv[0] first */
      if (realpath(argv[0], my_origin) && (my_origin[0] == '/')) {
        olen = strlen(my_origin);
      } else if ((execname = getexecname())){
        /* no absolute name - so probably invoked like
           "eval 'exec perl -S $0 ${1+"$@"}'"    or
           "#!/bin/env perl"
         */
        realpath(execname, my_origin);
        olen = strlen(my_origin);
      }
    }
    close(fd);
  }
#else /* assume this is __linux__ */
  olen = readlink("/proc/self/exe", my_origin, sizeof(my_origin));
  if ( olen < 1 || my_origin[0] == '[') {
    olen = 0;
  }
#endif /* __linux__ */
  if (olen > 0) {
    /* remove everything after the last / incl. the / itself */
    assert(my_origin[0] == '/');
	/* +1 for the basename */
	if (up <= 0) {
		up = 1;
	} else {
		up++;
	}
	for (;up != 0 && olen > 1; up--) {
		while (olen > 1 && my_origin[olen - 1] != '/')
			--olen;
		my_origin[--olen] = '\0';
	}
	if (tail != NULL) {
		if (olen > 1) {
			strlcat(my_origin, "/", MAXPATHLEN);
		}
		strlcat(my_origin, tail, MAXPATHLEN);
	}
    return strdup(my_origin);
  }
#endif
/* end  __linux__ || __sun__ */
  return fallback != NULL ? strdup(fallback) : NULL;
}

/**
 * get_origin:
 *
 * A convinience function for get_origin_rel(0, NULL, NULL).
 *
 * Return value: a newly-allocated string that must be freed with free() or
 *   %NULL if unable to determine.
 */
char *
get_origin()
{
	return get_origin_rel(0, NULL, NULL);
}

