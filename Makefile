VERSION = "1.0.0"

PROG = jsd-helper

# probably want to append something like '/64', '/x86_64[-linux-gnu]', '/amd64'
LIBDIR ?= lib

OS := $(shell uname -s)
MACH ?= 64

# If CC is set to 'cc', *_cc flags (Sun studio compiler) will be used.
# If set to 'gcc', the corresponding GNU C flags (*_gcc) will be used.
# For all others one needs to add corresponding rules.
CC ?= gcc
OPTIMZE_cc ?= -xO3
OPTIMZE_gcc ?= -O3
#OPTIMZE ?= $(OPTIMZE_$(CC)) -DNDEBUG

CFLAGS_cc = -xcode=pic32
CFLAGS_cc += -errtags -erroff=%none,E_UNRECOGNIZED_PRAGMA_IGNORED,E_ATTRIBUTE_UNKNOWN,E_NONPORTABLE_BIT_FIELD_TYPE -errwarn=%all -D_XOPEN_SOURCE=600 -D__EXTENSIONS__=1
CFLAGS_cc += -pedantic -v

CFLAGS_gcc = -fPIC -fsigned-char -pipe -Wno-unknown-pragmas -Wno-unused-result
CFLAGS_gcc += -fdiagnostics-show-option -Wall -Werror
CFLAGS_gcc += -pedantic -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wnested-externs -Winline -Wextra -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wundef -Wunused -Wno-variadic-macros -Wno-parentheses -Wcast-align -Wcast-qual
CFLAGS_gcc += -Wno-unused-function -Wno-multistatement-macros

CFLAGS ?= -m$(MACH) $(CFLAGS_$(CC)) $(OPTIMZE) -g
CFLAGS += -std=c11 -DVERSION=\"$(VERSION)\"
CFLAGS += -D_XOPEN_SOURCE=600
CFLAGS += $(CFLAGS_$(OS))

LIBS_SunOS =
LIBS_Linux = -lbsd

LIBS ?= $(LIBS_$(OS))

SHARED_gcc := -shared
LDFLAGS_cc := -zdefs -Bdirect -zdiscard-unused=dependencies $(LIBS)
LDFLAGS_gcc := -zdefs -Wl,--as-needed $(LIBS)
RPATH_OPT_cc := -R
RPATH_OPT_gcc := -Wl,-rpath=

LDFLAGS ?= -m$(MACH) $(LDFLAGS_$(CC))
RPATH_OPT := $(RPATH_OPT_$(CC))

LIBCFLAGS = $(CFLAGS) $(SHARED) $(LDFLAGS) -lc

PROGS = jsd-helper

PROGSRCS = jsd-helper.c c_origin.c
PROGOBJS = $(PROGSRCS:%.c=%.o) 

all:	$(PROGS)

$(PROGS):	LDFLAGS += $(RPATH_OPT)\$$ORIGIN:\$$ORIGIN/../$(LIBDIR)

$(PROGS):	Makefile $(PROGOBJS)
	$(CC) -o $@ $(PROGOBJS) $(LDFLAGS)

.PHONY: clean distclean install depend

# for maintainers to get _all_ deps wrt. source headers properly honored
DEPENDFILE := makefile.dep

depend: $(DEPENDFILE)

# on Ubuntu, makedepend is included in the 'xutils-dev' package
$(DEPENDFILE): *.c *.h
	makedepend -f - -Y/usr/include *.c 2>/dev/null | \
		sed -e 's@/usr/include/[^ ]*@@g' -e '/: *$$/ d' >makefile.dep

clean:
	rm -f *.o *~ $(PROGS) core gmon.out a.out

distclean: clean
	rm -f $(DEPENDFILE) *.rej *.orig

-include $(DEPENDFILE)
