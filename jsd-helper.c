/* License: CDDL 1.0   (see http://opensource.org/licenses/cddl-1.0)
   Copyright 2024 Jens Elkner (jel+jsd-helper@cs.ovgu.de). All rights reserved. */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "c_origin.h"

static const char *version = "Version: 1.0\n\t(C) 2024 by Jens Elkner, Otto-von-Guericke-Universität Magdeburg.\n\tAll rights reserved.\n";

/*
 * This is a simple wrapper around systemd-run, which allows an un-privileged
 * user (the one who is allowed to exec this suid program) to start transient
 * units named jsu-[:alnum:]* 
 *
 * To get this wrapper working, it needs to be installed as
 * suid binary owned by 'root' and executable by the users or groups, which
 * should be able to run the required operations. E.g.:
 *
 * 1) cp jsd-helper /usr/local/bin/jsd-helper
 * 2) chown root:staff /usr/local/bin/jsd-helper
 * 3) chmod 4750 /usr/local/bin/jsd-helper
 * 4) optional: set exec permission using withdrawn POSIX ACLs:
 *    chacl u::r-x,g::r-x,o::---,u:juppy:r-x,m::rwx /usr/local/bin/jsd-helper
 *	  OR
 *	  setfacl -m u:juppy:r-x /usr/local/bin/jsd-helper
 *
 * In case you really trust 'sudo' and know its config syntax properly, you may
 * write an alternative script, which makes proper use of sudo to accomplish
 * the same thing, and copy it to /usr/sbin/systemd-run-helper.
 */
static void
usage(char *prog) {
	fprintf(stderr, "Usage:\n%s \\\n"
	"    systemd-run --unit uname --working-directory dirname \\\n"
	"    --uid=num --gid=num [--slice=name] [--property={key}={value}]... \\\n"
	"    command [arg]...\n\tOR\n%s \\\nsystemctl {stop|reset-failed} uname|jhub.slice\n",
	prog, prog);
	fprintf(stderr, "\n%s", version);
	exit(1);
}

static char *
dup_env(const char *name) {
	char *value = getenv(name);
	if (value == NULL)
		return (NULL);

	size_t sz = strlen(name) + strlen(value) + 2; /* name, =, value, NUL */
	char *str;

	if ((str = malloc(sz)) == NULL)
		return (NULL);

	(void) snprintf(str, sz, "%s=%s", name, value);
	return (str);
}

static int
is_number(char *s) {
	int n = strlen(s);
	if (n < 6)
		return n;
	n--;
	for (; n >= 6; n--) {
		if (!isdigit(s[n]))
			return n;
	}
	return -1;
}

static int
is_printable(char *s) {
	int i = strlen(s);
	if (i == 0)
		return 0;
	i--;
	for (; i >= 0; i--)
		if (isprint(s[i]) == 0)
			return i;
	return -1;
}

enum PARAM {
	UNIT = 1,
	WD = 1 << 1,
	UID = 1 << 2,
	GID = 1 << 3,
	SLICE = 1 << 4,
	PROP = 1 << 5,
	CMD = 1 << 6,
	CTL = 1 << 7,
	RUN = 1 << 8
};

static int
check_unit_name(char *s, int arg_num, const char *optional) {
	if ((strncmp(s, "jsu-", 4) == 0) && (is_printable(s + 4) == -1))
		return -1;

	if ((optional != NULL) && (strcmp(optional, s) == 0))
		return -1;

	fprintf(stderr,"Arg %d must be a unit name starting with 'jsu-'\n",arg_num);
	return 0;
}

int
main(int argc, char *argv[])
{
	char *nenv[] = { NULL, NULL }; /* which implies !POSIXLY_CORRECT */
	int i = 1, n = 0, narg = 0, seen = 0, k;
	char *s;
	const char *cmd = NULL;
	struct stat sb;

	if (argc < 4) {
		fprintf(stderr, "Invalid number of arguments\n");
		usage(argv[0]);
	}

	if (strcmp(argv[i], "systemctl") == 0) {
		seen |= CTL;
	} else if (strcmp(argv[i], "systemd-run") == 0) {
		seen |= RUN;
	} else {
		fprintf(stderr,
			"Arg %d should be either 'systemctl' or 'systemd-run'\n", i);
		exit(99);
	}
	i++;
	if ((seen & CTL) == CTL) {
		if (argc != 4) {
			fprintf(stderr, "Invalid number of 'systemctl' arguments\n");
			exit(99);
		}
		cmd = "/bin/systemctl";
		if ((strcmp(argv[i], "stop") != 0)
			&& (strcmp(argv[i], "reset-failed") != 0))
		{
			fprintf(stderr, "'stop' and 'reset-failed' are the only commands\n"
				"accepted for 'systemctl' by this utility.\n");
			exit(99);
		}
		i++;
		if (check_unit_name(argv[i], i, "jhub.slice") != -1)
			exit(99);
		seen |= UNIT;
	} else {
		cmd = "/usr/bin/systemd-run";
		if (argc < 9) {
			fprintf(stderr, "Invalid number of 'systemd-run' arguments\n");
			usage(argv[0]);
		}
		for (i = 2; i < argc; i++) {
			if (narg == UNIT) {
				if (check_unit_name(argv[i], i, NULL) != -1)
					exit(99);
				narg = 0;
			} else if (narg == WD || narg == CMD) {
				if (is_printable(argv[i]) != -1) {
					fprintf(stderr, "Arg %d should be a printable string", i);
					exit(99);
				}
				if (narg == WD)
					narg = 0;
			} else if (strcmp(argv[i], "--unit") == 0) {
				seen |= UNIT;
				narg = UNIT;
			} else if (strcmp(argv[i], "--working-directory") == 0) {
				narg = WD;
				seen |= WD;
			} else if ((k = (strncmp(argv[i], "--uid=", 6)) == 0)
				|| (strncmp(argv[i], "--gid=", 6) == 0))
			{
				if (is_number(argv[i]) != -1) {	
					fprintf(stderr, "Arg %d shall end with a number\n", i);
					exit(99);
				}
				seen |= (k == 0) ? UID : GID;
			} else if (strncmp(argv[i], "--property=", 11) == 0) {
				seen |= PROP;
				if (is_printable(argv[i] + 11) != -1) {
					fprintf(stderr,"Arg %d shall end with a printable string\n",i);
					exit(99);
				}
			} else if (strncmp(argv[i], "--slice=", 8) == 0) {
				seen |= SLICE;
				if (is_printable(argv[i] + 8) != -1) {
					fprintf(stderr,"Arg %d shall end with a printable string\n",i);
					exit(99);
				}
			} else {
				// no match so it must be the cmd + args to run
				seen |= CMD;
				narg = CMD;
				if ((stat(argv[i], &sb) == -1) || ! S_ISREG(sb.st_mode)
					|| (sb.st_mode & S_IXUSR != S_IXUSR))
			   	{
					fprintf(stderr,"Arg %d shall be an executable command\n",i);
					exit(99);
				}
				// if the binary to run is not jupyterhub-singleuser or not
				// in the same directory as this binary, do not execute it.
				s = get_origin_rel(0, "jupyterhub-singleuser", NULL);
				if (!s) {
					fprintf(stderr,"Unable to determine the the directory of %s\n", argv[0]);
					exit(99);
				}
				if (strcmp(s, argv[i]) != 0) {
					fprintf(stderr,"Arg %d: exec Permission denied\n", i);
					free(s);
					exit(99);
				}
				free(s);
			}
		}
		if ((seen & WD) != WD) {
			fprintf(stderr, "missing '--working-directory' arg\n");
			n++;
		}
		if ((seen & UID) != UID) {
			fprintf(stderr, "missing '--uid=...' arg\n");
			n++;
		}
		if ((seen & GID) != GID) {
			fprintf(stderr, "missing '--gid=...' arg\n");
			n++;
		}
		if ((seen & CMD) != CMD) {
			fprintf(stderr, "Missing command to execute\n");
			n++;
		}
	}
	if ((seen & UNIT) != UNIT) {
		fprintf(stderr, "missing %s\n",
			(seen & CTL) == CTL ? "unit name" :"'--unit' arg");
		n++;
	}
	if (n)
		exit(99);

	if (setuid(0) != 0 || setgid(0) != 0) {
		perror("setid");
    	exit(EXIT_FAILURE);
	}
	s = dup_env("TERM");
	if (s != NULL) nenv[0] = s;
	execve(cmd, argv + 1, nenv);	// arg0 gets ignored
	perror("execve");
    exit(EXIT_FAILURE);
}
