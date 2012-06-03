## $Id:  $
##
## Top makefile designed to build Python for MorphOS platform.
##
## This file is Copyright by Guillaume ROGUEZ.
##
## GCC-4.x is required as default compiler
##

######### Environment

OS       := $(shell uname -s)
BUILDDIR = .
SRCDIR   = ..

PLATFORM = morphos

ifeq ("$(OS)", "MorphOS")
TMPDIR = ram:t
SDK_INSTALL_PREFIX ?= /usr/local
SYS_INSTALL_DIR ?= /sys
else
TMPDIR = /tmp
SDK_INSTALL_PREFIX ?= /opt/gg/usr/local
SYS_INSTALL_DIR = $(SDK_INSTALL_PREFIX)
endif

ifneq ($(findstring debug, $(MAKECMDGOALS)), debug)
DEFINES += NDEBUG
MODE    = final
else
DEFINES += Py_DEBUG
MODE    = debug
endif

BUILD_DATE		:= $(shell /bin/date "+%Y.%m.%d")
COMPILE_DATE	:= $(shell /bin/date "+%d.%m.%Y")
SCMVERSION		:= $(shell python c:hg parent --template "{latesttag}-{latesttagdistance}")
SVNVERSION		:= $(shell svnversion $(SRCDIR))
HGVERSION		:= $(shell python c:hg id -i)
HGTAG			:= $(shell python c:hg parent --template "{latesttag}")
HGBRANCH		:= $(shell python c:hg id -b)

DEPDIR := $(BUILDDIR)/deps/$(MODE)
OBJDIR := $(BUILDDIR)/objs/$(MODE)
LIBSDIR := $(BUILDDIR)/libs/$(MODE)
INCDIR := $(BUILDDIR)/include

INCLUDES = $(SRCDIR) $(SRCDIR)/MorphOS $(SRCDIR)/Include $(SRCDIR)/MorphOS/include $(OBJDIR) .

DEFINES  += \
	__MORPHOS_SHAREDLIBS=1 \
	AROS_ALMOST_COMPATIBLE \
	USE_INLINE_STDARG \
	PLATFORM=\"$(PLATFORM)\" \
	Py_DEFAULT_RECURSION_LIMIT=200 \
	SVNVERSION=\"$(SVNVERSION)\" \
	HGVERSION=\"$(HGVERSION)\" \
	HGTAG=\"$(HGTAG)\" \
	HGBRANCH=\"$(HGBRANCH)\"

VPATH := $(SRCDIR):$(OBJDIR)

VERSION_MAJOR	 = 2
VERSION_MINOR	 = 7
VERSION_REVISION = 3
VERSION			 = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_REVISION)
MOSVERSION		 = $(VERSION_MAJOR)$(VERSION_MINOR)

LIBRARY_SHORTNAME = python$(VERSION_MAJOR).library

SUFFIX = $(VERSION_MAJOR).$(VERSION_MINOR)
BINARY_NAME = python$(SUFFIX)
PYLIB_NAME = libpython$(SUFFIX).a
PYMLIB_NAME = libpym$(SUFFIX).a
PYINC_INSTALL_DIR = python$(SUFFIX)
PYLIB_INSTALL_DIR = python$(SUFFIX)

DYNLIB_INSTALL_DIR = $(SYS_INSTALL_DIR)/Libs/$(PYLIB_INSTALL_DIR)/lib-dynload
PURELIB_INSTALL_DIR = $(SYS_INSTALL_DIR)/Libs/$(PYLIB_INSTALL_DIR)

######### Tools Binaries

#GCCVER = -4.4.3

CP        = cp -av
ECHO      = echo -e
#BIN_PREFIX = ppc-morphos-
CC        = $(BIN_PREFIX)gcc$(GCCVER)
LD        = $(BIN_PREFIX)ld
AR        = $(BIN_PREFIX)ar
NM        = $(BIN_PREFIX)nm
STRIP     = $(BIN_PREFIX)strip
CVINCLUDE = cvinclude.pl

######### Tools Flags

ARFLAGS = rcsv

OPT = -O2 -mmultiple -mstring -mregnames -fno-strict-aliasing
OPT_BASEREL = $(OPT) -mbaserel32 -mresident32
CC_WARNS = \
	-Wall \
	-Wno-format \
	-Wunused \
	-Wuninitialized \
	-Wstrict-prototypes \

CC_WARNS += \
	-Wno-pointer-sign \
	-Wframe-larger-than=16384

CFLAGS = -noixemul -pipe -g

OPT += -fno-unit-at-a-time

CFLAGS_BASEREL = $(CFLAGS)

CPPFLAGS = $(CFLAGS) $(CC_WARNS) $(OPT) $(DEFINES:%=-D%) $(INCLUDES:%=-I%)
CPPFLAGS_BASEREL = $(CFLAGS_BASEREL) $(CC_WARNS) $(OPT_BASEREL) $(DEFINES:%=-D%) $(INCLUDES:%=-I%)

LDFLAGS_SHARED = --traditional-format \
	--cref -Map=$(BUILDDIR)/mapfile.txt -m morphos \
	-fl libnix --warn-common --warn-once

#LDFLAGS_SHARED_BASEREL = --traditional-format \
#	 --cref -Map=$(BUILDDIR)/mapfile.txt -m morphos_baserel \
#	 -datadata-reloc -fl libb32 -fl libnix --warn-common --warn-once

LDFLAGS_SHARED_BASEREL = -noixemul -pipe -g -mbaserel32 -mresident32 -nostartfiles \
	-Wl,--traditional-format -Wl,--cref -Wl,-Map=$(BUILDDIR)/mapfile.txt \
	-Wl,--warn-common -Wl,--warn-once
 
#LIBS = -lc -ldl -laboxstubs -labox -lc -lm -lmath -lsyscall -lauto
LIBS = -L/usr/local/lib -lm -lsyscall

ifneq ("$(OS)", "MorphOS")
LIBS += -lnix
endif

STRIPFLAGS = -R.comment -o $@ $<

######### Shell output beautification

COLOR_EMPHASIZE  = "\033[37m"
COLOR_HIGHLIGHT1 = "\033[33m"
COLOR_HIGHLIGHT2 = "\033[32m"
COLOR_BOLD       = "\033[0m\033[1m"
COLOR_NORMAL     = "\033[0m"

CREATING  = $(ECHO) $(COLOR_BOLD)">> Creating "$(COLOR_HIGHLIGHT2)"$@"$(COLOR_NORMAL)
COMPILING = $(ECHO) $(COLOR_BOLD)">> CC "$(COLOR_HIGHLIGHT2)"$@"$(COLOR_NORMAL)
LINKING	  = $(ECHO) $(COLOR_HIGHLIGHT1)">> Linking "$(COLOR_BOLD)"$@"$(COLOR_NORMAL)
ARCHIVING = $(ECHO) $(COLOR_HIGHLIGHT1)">> Archiving "$(COLOR_BOLD)"$@"$(COLOR_NORMAL)
MAKINGDIR = $(ECHO) $(COLOR_HIGHLIGHT1)">> Making directory "$(COLOR_BOLD)"$(@D)"$(COLOR_NORMAL)

######### Automatic rules

.SUFFIXES:
.SUFFIXES: .c .h .s .o .bro .a .X .fd .d .db .sym .library

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	-@test ! -d $(@D) && $(MAKINGDIR) && mkdir -p $(@D)
	@$(COMPILING)
	$(CC) -c $(CPPFLAGS) $< -o $@

$(OBJDIR)/%.o : $(SRCDIR)/%.s
	-@test ! -d $(@D) && $(MAKINGDIR) && mkdir -p $(@D)
	@$(COMPILING)
	$(CC) -c $(CPPFLAGS) $< -o $@

$(OBJDIR)/%.bro : $(SRCDIR)/%.c
	-@test ! -d $(@D) && $(MAKINGDIR) && mkdir -p $(@D)
	@$(COMPILING)
	$(CC) -c $(CPPFLAGS_BASEREL) $< -o $@

$(OBJDIR)/%.bro : $(SRCDIR)/%.s
	-@test ! -d $(@D) && $(MAKINGDIR) && mkdir -p $(@D)
	@$(COMPILING)
	$(CC) -c $(CPPFLAGS_BASEREL) $< -o $@

%.sym: %.db
	$(NM) -n $^ > $@

%.library.db: $(OBJS)
	-@test ! -d $(@D) && $(MAKINGDIR) && mkdir -p $(@D)
	@$(LINKING)
#$(LD) -o $@ $(LDFLAGS_SHARED_BASEREL) $(OBJS) $(LIBS)
	$(CC) -o $@ $(LDFLAGS_SHARED_BASEREL) $(OBJS) $(LIBS)

%.library: %.library.db
	@$(CREATING)
	$(STRIP) $(STRIPFLAGS)

%.a: $(OBJS)
	-@test ! -d $(@D) && $(MAKINGDIR) && mkdir -p $(@D)
	@$(ARCHIVING)
	$(AR) $(ARFLAGS) $@ $(OBJS)

%.pym: $(OBJS) $(OBJDIR)/libpython_glue.a
	@$(CREATING)
	$(LD) -o $@ $(LDFLAGS_SHARED) $^ $(LIBS)

# Automatic dependencies generation

$(DEPDIR)/%.d : %.c pyconfig.h
	-@test ! -z "$(@D)" -a ! -d "$(@D)" && $(MAKINGDIR) && mkdir -p "$(@D)"
	@$(CREATING)
	$(SHELL) -ec '$(CC) -MM $(CPPFLAGS) -DPy_BUILD_CORE -DBUILD_DEPS $< 2>>ram:error.log \
		| sed '\''s%\($(notdir $*)\)\.o[ :]*%$(OBJDIR)/$(dir $*)\1.o $(OBJDIR)/$(dir $*)\1.bro : %g'\'' > $@; \
		[ -s $@ ] || rm -fv $@'

##########################################################################
# Machine dependent

MACHDEP	= morphos
MACHDEP_DIR = MorphOS

EMUL_SRCS = \
	$(MACHDEP_DIR)/emul/utime.c \
	$(MACHDEP_DIR)/emul/popen.c \
	$(MACHDEP_DIR)/emul/popenSubTask.c

MACHDEP_SRCS = \
	$(MACHDEP_DIR)/morphos.c \
	$(MACHDEP_DIR)/config.c \
	$(MACHDEP_DIR)/getpath.c \
	$(EMUL_SRCS)

MACHDEP_MODULES_SRCS = \
	Modules/errnomodule.c \
	Modules/pwdmodule.c \
	Modules/_sre.c \
	Modules/_codecsmodule.c \
	Modules/_weakref.c \
	Modules/zipimport.c \
	Modules/symtablemodule.c \
	Modules/xxsubtype.c \
	Modules/datetimemodule.c \
	Modules/_struct.c \
	Modules/binascii.c \
	Modules/cStringIO.c \
	Modules/mathmodule.c \
	Modules/_randommodule.c \
	$(MACHDEP_DIR)/morphosmodule.c \
	$(MACHDEP_DIR)/timemodule.c

EMUL_OBJS = $(EMUL_SRCS:.c=.o)
MACHDEP_OBJS = $(MACHDEP_SRCS:.c=.o)

THREADSRC = Python/thread.c Modules/threadmodule.c
UNICODE_SRCS = Objects/unicodeobject.c Objects/unicodectype.c
SIGNAL_SRCS = $(MACHDEP_DIR)/signalmodule.c
SIGNAL_OBJS = $(SIGNAL_SRCS:.c=.o)

ALL_SOURCES += $(MACHDEP_SRCS) $(SIGNAL_SRCS)

%/getpath.o %/getpath.bro : \
	DEFINES+=VERSION='"$(VERSION_MAJOR).$(VERSION_MINOR)"' \
	VERSION_SHORT='"$(PYTHON_SHAREDLIB_VERSION)"'

##########################################################################
# Modules

MODULE_SRCS = \
	Modules/getpath.c \
	Modules/main.c \
	Modules/gcmodule.c \
	Modules/getbuildinfo.c \
	$(MACHDEP_MODULES_SRCS)
MODULE_OBJS = $(MODULE_SRCS:.c=.o)

ALL_SOURCES += $(MODULE_SRCS)

##########################################################################
# Parser

PGEN = Parser/pgen$(EXE)

PSRCS = \
	Parser/acceler.c \
	Parser/grammar1.c \
	Parser/listnode.c \
	Parser/node.c \
	Parser/parser.c \
	Parser/parsetok.c \
	Parser/bitset.c \
	Parser/metagrammar.c \
	Parser/firstsets.c \
	Parser/grammar.c \
	Parser/pgen.c

PARSER_SRCS = $(PSRCS) Parser/myreadline.c Parser/tokenizer.c
PARSER_OBJS = $(PARSER_SRCS:.c=.o)

PGSRCS = \
	Objects/obmalloc.c \
	Python/mysnprintf.c \
	Python/pyctype.c \
	Parser/tokenizer_pgen.c \
	Parser/printgrammar.c \
	Parser/pgenmain.c

PGENSRCS = $(PSRCS) $(PGSRCS)
PGENOBJS = $(PGENOBJS:.c=.o)

ALL_SOURCES += $(PARSER_SRCS) $(PGENSRCS)

##########################################################################
# Python

DYNLOADFILE = dynload_morphos.c

PYTHON_SRCS = \
		Python/_warnings.c \
		Python/Python-ast.c \
		Python/asdl.c \
		Python/ast.c \
		Python/bltinmodule.c \
		Python/ceval.c \
		Python/compile.c \
		Python/codecs.c \
		Python/errors.c \
		Python/frozen.c \
		Python/frozenmain.c \
		Python/future.c \
		Python/getargs.c \
		Python/getcompiler.c \
		Python/getcopyright.c \
		Python/getplatform.c \
		Python/getversion.c \
		Python/graminit.c \
		Python/import.c \
		Python/importdl.c \
		Python/marshal.c \
		Python/modsupport.c \
		Python/mystrtoul.c \
		Python/mysnprintf.c \
		Python/peephole.c \
		Python/pyarena.c \
		Python/pyctype.c \
		Python/pyfpe.c \
		Python/pymath.c \
		Python/pystate.c \
		Python/pythonrun.c \
		Python/random.c \
		Python/structmember.c \
		Python/symtable.c \
		Python/sysmodule.c \
		Python/traceback.c \
		Python/getopt.c \
		Python/pystrcmp.c \
		Python/pystrtod.c \
		Python/dtoa.c \
		Python/formatter_unicode.c \
		Python/formatter_string.c \
		Python/$(DYNLOADFILE) \
		$(THREADSRC)

PYTHON_OBJS = $(PYTHON_SRCS:.c=.o)

ALL_SOURCES += $(PYTHON_SRCS)

##########################################################################
# Objects

OBJECT_SRCS = \
	Objects/abstract.c \
	Objects/boolobject.c \
	Objects/bufferobject.c \
	Objects/bytes_methods.c \
	Objects/bytearrayobject.c \
	Objects/capsule.c \
	Objects/cellobject.c \
	Objects/classobject.c \
	Objects/cobject.c \
	Objects/codeobject.c \
	Objects/complexobject.c \
	Objects/descrobject.c \
	Objects/enumobject.c \
	Objects/exceptions.c \
	Objects/genobject.c \
	Objects/fileobject.c \
	Objects/floatobject.c \
	Objects/frameobject.c \
	Objects/funcobject.c \
	Objects/intobject.c \
	Objects/iterobject.c \
	Objects/listobject.c \
	Objects/longobject.c \
	Objects/dictobject.c \
	Objects/memoryobject.c \
	Objects/methodobject.c \
	Objects/moduleobject.c \
	Objects/object.c \
	Objects/obmalloc.c \
	Objects/rangeobject.c \
	Objects/setobject.c \
	Objects/sliceobject.c \
	Objects/stringobject.c \
	Objects/structseq.c \
	Objects/tupleobject.c \
	Objects/typeobject.c \
	Objects/weakrefobject.c \
	$(UNICODE_SRCS)

OBJECT_OBJS = $(OBJECT_SRCS:.c=.o)

ALL_SOURCES += $(OBJECT_SRCS)

##########################################################################
# Python Core Archive (for library version)

PYTHON_CORELIB = libcore.a

_PYTHON_COREOBJS = \
	$(MACHDEP_OBJS) \
	$(SPECIAL_OBJS) \
	$(PARSER_OBJS) \
	$(OBJECT_OBJS) \
	$(PYTHON_OBJS) \
	$(MODULE_OBJS) \
	$(SIGNAL_OBJS) \
	$(MODOBJS)
PYTHON_COREOBJS = $(_PYTHON_COREOBJS:.o=.bro)

$(PYTHON_COREOBJS:%=$(OBJDIR)/%) : DEFINES+=Py_BUILD_CORE DONT_WRAP_VARS

$(OBJDIR)/$(PYTHON_CORELIB) : OBJS=$(PYTHON_COREOBJS:%=$(OBJDIR)/%)
$(OBJDIR)/$(PYTHON_CORELIB) : $(PYTHON_COREOBJS:%=$(OBJDIR)/%)

##########################################################################
# Python Shared Library - Generated files and SDK

syms : $(OBJDIR)/$(PYTHON_CORELIB)
	$(NM) -fposix -ACg --defined-only $(OBJDIR)/$(PYTHON_CORELIB) | sed -n "s/.*: \(_*Py\w*\) \(\w\).*/\2:\1/p" > $@

syms.func : $(INCDIR)/fd/python$(VERSION_MAJOR)_lib.fd
	sed -ne "/)$$/ s/(.*)(.*)//p" $< > $@

$(BUILDDIR)/libprotos.h : syms.func
	@$(CREATING)
	echo -e "/* GENERATED FILE. DO NOT EDIT IT MANUALLY */\n\n#ifndef PYTHON_LIBPROTOS_H\n#define PYTHON_LIBPROTOS_H\n " > $@
	sed "s/\(.*\)/LIBPROTO(\1);/g" $< >> $@
	echo -e "\n#define PYTHON_FUNCARRAY \\" >> $@
	sed 's/\(.*\)/  LIBDECL(\1) \\/g' $< >> $@
	echo -e "  (APTR) -1\n\n#endif /* PYTHON_LIBPROTOS_H */" >> $@

$(OBJDIR)/libpython_glue.a : $(INCDIR)/fd/python$(VERSION_MAJOR)_lib.fd $(INCDIR)/clib/python$(VERSION_MAJOR)_protos.h
	-@test ! -d "$(@D)" && $(MAKINGDIR) && mkdir -p "$(@D)"
	@$(CREATING)
	$(CVINCLUDE) --fd $(INCDIR)/fd/python$(VERSION_MAJOR)_lib.fd --clib $(INCDIR)/clib/python$(VERSION_MAJOR)_protos.h --gluelib $@

%/trampolines.s: $(INCDIR)/fd/python$(VERSION_MAJOR)_lib.fd $(INCDIR)/clib/python$(VERSION_MAJOR)_protos.h
	@$(CREATING)
	$(CVINCLUDE) --fd $(INCDIR)/fd/python$(VERSION_MAJOR)_lib.fd --clib $(INCDIR)/clib/python$(VERSION_MAJOR)_protos.h --breltrampolines $@

ALL_SOURCES += $(MACHDEP_DIR)/libpython-shared.c

$(OBJDIR)/$(MACHDEP_DIR)/libpython-shared.o: \
	DEFINES+=LIBRARY_SHORTNAME='"$(LIBRARY_SHORTNAME)"' VERSION=$(MOSVERSION) Py_BUILD_CORE

$(OBJDIR)/$(PYLIB_NAME): $(OBJDIR)/libpython_glue.a $(OBJDIR)/$(MACHDEP_DIR)/libpython-shared.o
	$(CP) $< $@
	$(AR) $(ARFLAGS) $@ $(OBJDIR)/$(MACHDEP_DIR)/libpython-shared.o

_PYM_OBJS = pym.o pym_exported.o
PYM_OBJS = $(_PYM_OBJS:%=$(OBJDIR)/$(MACHDEP_DIR)/%)

$(PYM_OBJS): DEFINES+=Py_BUILD_CORE

$(OBJDIR)/$(PYMLIB_NAME): $(OBJDIR)/libpython_glue.a $(EMUL_OBJS:%=$(OBJDIR)/%) $(PYM_OBJS)
	$(CP) $< $@
	$(AR) $(ARFLAGS) $@ $(EMUL_OBJS:%=$(OBJDIR)/%) $(PYM_OBJS)

core : $(OBJDIR)/$(PYTHON_CORELIB)

SDK_DEPS = \
	$(SRCDIR)/pyconfig.h \
	$(INCDIR)/fd/python$(VERSION_MAJOR)_lib.fd \
	$(INCDIR)/clib/python$(VERSION_MAJOR)_protos.h \
	$(INCDIR)/libraries/python$(VERSION_MAJOR)_gvars.h \
	$(OBJDIR)/$(PYLIB_NAME) \
	$(OBJDIR)/$(PYMLIB_NAME)

ALL_SOURCES += \
	$(MACHDEP_DIR)/pym.c \
	$(MACHDEP_DIR)/pym_exported.c

##########################################################################
# Python Shared Library

PYTHON_SHAREDLIB_VERSION  = $(MOSVERSION)
PYTHON_SHAREDLIB_REVISION = $(VERSION_REVISION)

PYTHON_SHAREDLIB_HEADSRCS = \
	$(MACHDEP_DIR)/libheader.c \
	$(MACHDEP_DIR)/libinit.c \
	$(MACHDEP_DIR)/libuser.c

ALL_SOURCES += $(PYTHON_SHAREDLIB_HEADSRCS)

_PYTHON_SHAREDLIB_HEADOBJS = \
	$(MACHDEP_DIR)/libheader.o \
	$(MACHDEP_DIR)/libinit.o \
	$(MACHDEP_DIR)/libuser.bro \
	$(MACHDEP_DIR)/trampolines.bro

PYTHON_SHAREDLIB_HEADOBJS = $(_PYTHON_SHAREDLIB_HEADOBJS:%=$(OBJDIR)/%)

$(OBJDIR)/$(MACHDEP_DIR)/libheader.o : \
	$(SRCDIR)/$(MACHDEP_DIR)/libheader.h \
	$(BUILDDIR)/libprotos.h

$(OBJDIR)/$(MACHDEP_DIR)/libheader.o : \
	DEFINES+=LIBRARY_VERSION=$(PYTHON_SHAREDLIB_VERSION) \
		LIBRARY_REVISION=$(PYTHON_SHAREDLIB_REVISION) \
		COMPILE_DATE='"$(COMPILE_DATE)"' \
	LIBRARY_SHORTNAME='"$(LIBRARY_SHORTNAME)"' \
		LIBRARY_VERSION_STR='"$(PYTHON_SHAREDLIB_VERSION).$(PYTHON_SHAREDLIB_REVISION)"' \
		MORPHOS_VERSION_STR='"$(MOSVERSION)"'

$(LIBSDIR)/$(LIBRARY_SHORTNAME).db: DEFINES+=DONT_WRAP_VARS Py_BUILD_CORE
$(LIBSDIR)/$(LIBRARY_SHORTNAME).db: OBJS=$(PYTHON_SHAREDLIB_HEADOBJS) $(OBJDIR)/$(MACHDEP_DIR)/python$(VERSION_MAJOR)_gvars.bro $(OBJDIR)/$(PYTHON_CORELIB)
$(LIBSDIR)/$(LIBRARY_SHORTNAME).db: $(OBJDIR)/$(PYTHON_CORELIB) $(PYTHON_SHAREDLIB_HEADOBJS) $(OBJDIR)/$(MACHDEP_DIR)/python$(VERSION_MAJOR)_gvars.bro

ifneq ("$(OS)", "MorphOS")
$(LIBSDIR)/$(LIBRARY_SHORTNAME).db: $(OBJDIR)/$(MACHDEP_DIR)/stubs.bro
$(LIBSDIR)/$(LIBRARY_SHORTNAME).db: OBJS+=$(OBJDIR)/$(MACHDEP_DIR)/stubs.bro
endif

library:  $(LIBSDIR)/$(LIBRARY_SHORTNAME).sym $(LIBSDIR)/$(LIBRARY_SHORTNAME)

##########################################################################
# Python shell interpreter

ALL_SOURCES += $(MACHDEP_DIR)/python_main.c

$(OBJDIR)/$(MACHDEP_DIR)/python_main.o: \
	DEFINES+=LIBVERSION='"$(PYTHON_SHAREDLIB_VERSION)"' \
		LIBREVISION='"$(PYTHON_SHAREDLIB_REVISION)"' \
		BUILDDATE='"$(BUILD_DATE)"' \
		VERSION='"$(VERSION)"'
$(OBJDIR)/$(MACHDEP_DIR)/python_main.o: OPT+=-fcall-saved-r13
$(OBJDIR)/$(MACHDEP_DIR)/python_main.o: $(SDK_DEPS)

$(BUILDDIR)/$(BINARY_NAME).db : $(OBJDIR)/$(MACHDEP_DIR)/python_main.o $(OBJDIR)/$(PYLIB_NAME)
	@$(CREATING)
	$(CC) $(CFLAGS) -Wl,--traditional-format $< -o $@ -L$(OBJDIR) -l$(PYLIB_NAME:lib%.a=%) $(LIBS)

$(SRCDIR)/$(BINARY_NAME) : $(BUILDDIR)/$(BINARY_NAME).db
	@$(CREATING)
	$(STRIP) $(STRIPFLAGS)
	chmod +x $@

binary: $(SRCDIR)/$(BINARY_NAME)

##########################################################################
# General Rules

.DEFAULT: all
.PHONY: all clean distclean debug final force sdk deps core library binary
.PHONY: install install-bin install-sdk install-dynlib install-purelib release mkdyn

all: core library sdk binary

force:;

debug final:

printvar:
	-@echo $(VAR)

clean:
	rm -f $(BUILDDIR)/mapfile.txt $(SRCDIR)/pyconfig.h $(SRCDIR)/$(MACHDEP_DIR)/libprotos.h $(BUILDDIR)/trampolines.s
	rm -f $(SRCDIR)/$(BINARY_NAME) $(BUILDDIR)/$(BINARY_NAME).db
	rm -rf $(SRCDIR)/build
	-[ -d $(OBJDIR) ] && find $(OBJDIR) -name "*.[aoX]" -o -name "*.bro" -exec rm -v {} ";"
	-[ -d $(LIBSDIR) ] && find $(LIBSDIR) -name "*.library*" -exec rm -v {} ";"
	find $(SRCDIR) -name "*.py[oc]" -exec rm -v {} ";"

distclean: clean
	rm -f syms* dump* *~
	rm -rf $(BUILDDIR)/deps $(BUILDDIR)/objs $(BUILDDIR)/libs

$(SRCDIR)/pyconfig.h: pyconfig.h.mos
	$(CP) $< $@

ALL_SOURCES_SAFE = $(filter-out %/libheader.c,$(ALL_SOURCES))

sdk: $(SDK_DEPS)

mkdyn: $(SRCDIR)/$(BINARY_NAME) install-bin install-sdk
	(cd $(SRCDIR); $(BINARY_NAME) -E setup.py build_ext -DNDEBUG -D__MORPHOS_SHAREDLIBS build_scripts)

install-bin: library $(SRCDIR)/$(BINARY_NAME)
	-[ "$(OS)" = "MorphOS" ] && ( \
		$(ECHO) $(COLOR_BOLD)">> Installing binaries in "$(COLOR_HIGHLIGHT1)$(SYS_INSTALL_DIR)$(COLOR_NORMAL); \
		[ -d $(SYS_INSTALL_DIR)/Libs ] || mkdir -p $(SYS_INSTALL_DIR)/Libs; \
		[ -d $(SYS_INSTALL_DIR)/C ] || mkdir -p $(SYS_INSTALL_DIR)/C; \
		cp -a $(LIBSDIR)/$(LIBRARY_SHORTNAME) $(SYS_INSTALL_DIR)/Libs; \
		cp -a $(SRCDIR)/$(BINARY_NAME) $(SYS_INSTALL_DIR)/C; \
		rm -f $(SYS_INSTALL_DIR)/C/python; \
		ln -fs $(SYS_INSTALL_DIR)/C/$(BINARY_NAME) $(SYS_INSTALL_DIR)/C/python; )

install-sdk: sdk
	$(ECHO) $(COLOR_BOLD)">> Installing SDK in "$(COLOR_HIGHLIGHT1)$(SDK_INSTALL_PREFIX)$(COLOR_NORMAL)
	[ -d $(SDK_INSTALL_PREFIX)/include/$(PYINC_INSTALL_DIR) ] || mkdir -p $(SDK_INSTALL_PREFIX)/include/$(PYINC_INSTALL_DIR)
	[ -d $(SDK_INSTALL_PREFIX)/lib ] || mkdir -p $(SDK_INSTALL_PREFIX)/lib
	cp -a $(SRCDIR)/MorphOS/include $(SDK_INSTALL_PREFIX)
	cp -dR $(SRCDIR)/Include/*.h $(SDK_INSTALL_PREFIX)/include/$(PYINC_INSTALL_DIR)
	cp -dR $(SRCDIR)/pyconfig.h $(SDK_INSTALL_PREFIX)/include/$(PYINC_INSTALL_DIR)
	cp -dR $(OBJDIR)/$(PYLIB_NAME) $(OBJDIR)/$(PYMLIB_NAME) $(SDK_INSTALL_PREFIX)/lib

install-dynlib: mkdyn
	-[ "$(OS)" = "MorphOS" ] && ( \
		$(ECHO) $(COLOR_BOLD)">> Installing .pym modules in "$(COLOR_HIGHLIGHT1)$(SYS_INSTALL_DIR)$(COLOR_NORMAL); \
		[ -d $(DYNLIB_INSTALL_DIR) ] || mkdir -p $(DYNLIB_INSTALL_DIR); \
		cp -a $(SRCDIR)/build/lib.$(PLATFORM)-$(SUFFIX)/*.pym $(DYNLIB_INSTALL_DIR); )

install-purelib:
	$(ECHO) $(COLOR_BOLD)">> Installing Python lib files in "$(COLOR_HIGHLIGHT1)$(PURELIB_INSTALL_DIR)$(COLOR_NORMAL);
	cp -a $(SRCDIR)/Lib/* $(PURELIB_INSTALL_DIR)

install: install-bin install-sdk install-dynlib install-purelib

ifneq ("$(RELEASE_DIR)","")
release: mkdyn
	arcname=Python-$(SCMVERSION); \
		dir=$(RELEASE_DIR)/$$arcname; \
		$(ECHO) $(COLOR_BOLD)">> Creating "$(COLOR_HIGHLIGHT2)"$$dir"$(COLOR_NORMAL); \
		rm -rf $$dir; \
		mkdir -p $$dir; \
		$(MAKE) $(MODE) install SYS_INSTALL_DIR=$$dir SDK_INSTALL_PREFIX=$$dir/DevEnv; \
		mkdir -p $$dir/Libs/$(PYLIB_INSTALL_DIR) && (cp -a $(SRCDIR)/Lib/* $$dir/Libs/$(PYLIB_INSTALL_DIR);) && \
			mkdir -p $$dir/Libs/$(PYLIB_INSTALL_DIR)/lib-dynload && \
			cp -a $(SRCDIR)/build/lib.$(PLATFORM)-$(SUFFIX)/*.pym $$dir/Libs/$(PYLIB_INSTALL_DIR)/lib-dynload; \
			find $$dir/Libs/$(PYLIB_INSTALL_DIR)/lib-dynload -name "*.pym" -exec \
				$(STRIP) -R .comment --strip-debug --discard-locals {} ";" ; \
		cp -a $(SRCDIR)/build/scripts-$(SUFFIX)/* $$dir/C/; \
		cp -a $(SRCDIR)/LICENSE $$dir; \
		$(ECHO) $(COLOR_BOLD)">> Archiving "$(COLOR_HIGHLIGHT2)"$$arcname.tar.bz2"$(COLOR_NORMAL); \
		(cd $(RELEASE_DIR); tar cp -O $$arcname | bzip2 > $$arcname.tar.bz2)
else
release:
	$(ECHO) "RELEASE_DIR shall be defined to use 'release' target"
	exit
endif

test: $(BUILDDIR)/$(BINARY_NAME)
	(cd /ram; $(BINARY_NAME) -E -tt -Wd -3 -u -m test.regrtest -udecimal \
        -x test_posixpath \
        -x test_ntpath \
        -x test_ascii_formatd \
        -x test_ctypes \
        -x test_compile \
        -x test_httpservers \
        -x test_ftplib \
        -x test_logging \
        -x test_mailbox \
        -x test_marshal \
        -x test_poplib \
        -x test_smtplib \
        -x test_socket \
        -x test_threading \
        -x test_urllib2_localnet \
        -x test_xmlrpc \
        -x test_imaplib \
        -x test_ssl \
        -x test_socketserver \
        -x test_docxmlrpc \
        )

# CRASH: test_capi, test_minidom
# Cause Insert device popup: test_mailbox
# Block threads: test_logging
# Use non supported subprocess: test_cmd_line

deps: $(ALL_SOURCES_SAFE:%.c=$(DEPDIR)/%.d)

ifeq ("$(filter clean distclean,$(MAKECMDGOALS))","")
$(info Including dependencies...)
include $(ALL_SOURCES_SAFE:%.c=$(DEPDIR)/%.d)
endif
