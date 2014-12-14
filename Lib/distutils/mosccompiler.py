"""distutils.mosccompiler

Contains the MOSCCompiler class, a subclass of CCompiler that handles
the "typical" MorphOS-style command-line C compiler:
  * macros defined with -Dname[=value]
  * macros undefined with -Uname
  * include search directories specified with -Idir
  * libraries specified with -lllib
  * library search directories specified with -Ldir
  * compile handled by 'gcc' (or similar) executable with -c option:
    compiles .c to .o
  * link static library handled by 'ar' command (possibly with 'ranlib')
  * link shared library handled by 'gcc'
"""

__revision__ = "$Id: $"

import os, sys

from distutils import sysconfig
from distutils.dep_util import newer
from distutils.ccompiler import \
     CCompiler, gen_preprocess_options, gen_lib_options
from distutils.errors import \
     DistutilsExecError, CompileError, LibError, LinkError
from distutils import log

# XXX Things not currently handled:
#   * optimization/debug/warning flags; we just use whatever's in Python's
#     Makefile and live with it.  Is this adequate?  If not, we might
#     have to have a bunch of subclasses GNUCCompiler, SGICCompiler,
#     SunCCompiler, and I suspect down that road lies madness.
#   * even if we don't know a warning flag from an optimization flag,
#     we need some way for outsiders to feed preprocessor/compiler/linker
#     flags in to us -- eg. a sysadmin might want to mandate certain flags
#     via a site config file, or a user might want to set something for
#     compiling this module distribution only via the setup.py command
#     line, whatever.  As long as these options come from something on the
#     current system, they can be as system-dependent as they like, and we
#     should just happily stuff them into the preprocessor/compiler/linker
#     options and carry on.

class MorphOSCCompiler(CCompiler):

    compiler_type = 'morphos'
    compiler_prefix = ''

    # These are used by CCompiler in two places: the constructor sets
    # instance attributes 'preprocessor', 'compiler', etc. from them, and
    # 'set_executable()' allows any of these to be set.  The defaults here
    # are pretty generic; they will probably have to be set by an outsider
    # (eg. using information discovered by the sysconfig about building
    # Python extensions).
    executables = {'preprocessor' : compiler_prefix+"gcc -E -noixemul",
                   'compiler'     : compiler_prefix+"gcc -noixemul",
                   'compiler_so'  : compiler_prefix+"gcc -noixemul",
                   'compiler_cxx' : compiler_prefix+"g++ -noixemul",
                   'linker_so'    : compiler_prefix+"gcc -noixemul",
                   'linker_exe'   : compiler_prefix+"gcc -noixemul",
                   'archiver'     : compiler_prefix+"ar -cr",
                   'ranlib'       : compiler_prefix+"ranlib",
                   'stripper'     : compiler_prefix+"strip",
                  }

    # Needed for the filename generation methods provided by the base
    # class, CCompiler.  NB. whoever instantiates/uses a particular
    # UnixCCompiler instance should set 'shared_lib_ext' -- we set a
    # reasonable common default here, but it's not necessarily used on all
    # Unices!

    src_extensions = [".c",".C",".cc",".cxx",".cpp",".m"]
    obj_extension = ".o"
    static_lib_extension = shared_lib_extension = ".a"
    static_lib_format = shared_lib_format = "lib%s%s"

    def __init__(self, verbose=0, dry_run=0, force=0):
        CCompiler.__init__(self, verbose, dry_run, force)

        cflags, ldflags, ldflags_shared = sysconfig.get_config_vars('CFLAGS', 'LDFLAGS', 'LDFLAGS_SHARED')

        cflags = ' ' + cflags + ' -I'+os.path.dirname(sysconfig.get_config_h_filename())

        cpp = self.executables['preprocessor'] + cflags
        cc_cmd = self.executables['compiler'] + cflags
        cxx_cmd = self.executables['compiler_cxx'] + cflags
        ld_cmd = self.executables['compiler'] + ' ' + ldflags
        ldshared_cmd = self.executables['compiler'] + ' ' + ldflags_shared

        self.set_executables(preprocessor=cpp,
                             compiler=cc_cmd,
                             compiler_so=cc_cmd,
                             compiler_cxx=cxx_cmd,
                             linker_so=ldshared_cmd,
                             linker_exe=ld_cmd)

    def preprocess(self, source,
                   output_file=None, macros=None, include_dirs=None,
                   extra_preargs=None, extra_postargs=None):
        ignore, macros, include_dirs = \
            self._fix_compile_args(None, macros, include_dirs)
        pp_opts = gen_preprocess_options(macros, include_dirs)
        pp_args = [self.preprocessor] + pp_opts
        if output_file:
            pp_args.extend(['-o', output_file])
        if extra_preargs:
            pp_args[:0] = extra_preargs
        if extra_postargs:
            pp_args.extend(extra_postargs)
        pp_args.append(source)

        # We need to preprocess: either we're being forced to, or we're
        # generating output to stdout, or there's a target output file and
        # the source file is newer than the target (or the target doesn't
        # exist).
        if self.force or output_file is None or newer(source, output_file):
            if output_file:
                self.mkpath(os.path.dirname(output_file))
            try:
                self.spawn(pp_args)
            except DistutilsExecError as msg:
                raise CompileError(msg)

    def _compile(self, obj, src, ext, cc_args, extra_postargs, pp_opts):
        try:
            self.spawn(self.compiler_so + cc_args + [src, '-o', obj] +
                       extra_postargs)
        except DistutilsExecError as msg:
            raise CompileError(msg)

    def create_static_lib(self, objects, output_libname,
                          output_dir=None, debug=0, target_lang=None):
        objects, output_dir = self._fix_object_args(objects, output_dir)

        output_filename = \
            self.library_filename(output_libname, output_dir=output_dir)

        if self._need_link(objects, output_filename):
            self.mkpath(os.path.dirname(output_filename))
            self.spawn(self.archiver +
                       [output_filename] +
                       objects + self.objects)

            if not debug:
                try:
                    self.spawn(self.stripper + ['-R.comment', '--strip-debug', output_filename])
                except DistutilsExecError as msg:
                    raise LibError(msg)
        else:
            log.debug("skipping %s (up-to-date)", output_filename)

    def link(self, target_desc, objects,
             output_filename, output_dir=None, libraries=None,
             library_dirs=None, runtime_library_dirs=None,
             export_symbols=None, debug=0, extra_preargs=None,
             extra_postargs=None, build_temp=None, target_lang=None):
        objects, output_dir = self._fix_object_args(objects, output_dir)
        libraries, library_dirs, runtime_library_dirs = \
            self._fix_lib_args(libraries, library_dirs, runtime_library_dirs)

        pymbase = sysconfig.get_config_var('PYMBASE')
        if pymbase.startswith(os.getcwd()):
            if debug:
                pymbase = os.path.join(pymbase, 'debug')
            else:
                pymbase = os.path.join(pymbase, 'final')
        library_dirs.insert(0, pymbase)

        lib_opts = gen_lib_options(self, library_dirs, runtime_library_dirs,
                                   libraries)
        if not isinstance(output_dir, str) and output_dir is not None:
            raise TypeError("'output_dir' must be a string or None")
        if output_dir is not None:
            output_filename = os.path.join(output_dir, output_filename)

        if self._need_link(objects, output_filename):
            ld_args = (objects + self.objects +
                       lib_opts + ['-o', output_filename])
            if debug:
                ld_args[:0] = ['-g']
            if extra_preargs:
                ld_args[:0] = extra_preargs
            if extra_postargs:
                ld_args.extend(extra_postargs)
            self.mkpath(os.path.dirname(output_filename))
            try:
                if target_desc == CCompiler.EXECUTABLE:
                    linker = self.linker_exe
                else:
                    linker = self.linker_so
                if target_lang == "c++" and self.compiler_cxx:
                    linker = self.compiler_cxx
                self.spawn(linker + ld_args)
            except DistutilsExecError as msg:
                raise LinkError(msg)

            if not debug:
                stripflags = [ '-R.comment' ]
                if target_desc == CCompiler.EXECUTABLE: 
                    stripflags.append('--strip-all')
                else:
                    stripflags.append('--strip-debug')
                try:
                    self.spawn(self.stripper + stripflags + [output_filename])
                except DistutilsExecError as msg:
                    raise LibError(msg)
        else:
            log.debug("skipping %s (up-to-date)", output_filename)

    # -- Miscellaneous methods -----------------------------------------
    # These are all used by the 'gen_lib_options() function, in
    # ccompiler.py.

    def library_dir_option(self, dir):
        if not dir: dir = '.'
        return "-L" + dir

    def runtime_library_dir_option(self, dir):
        return "-Wl,-R" + dir

    def library_option(self, lib):
        return "-l" + lib

    def find_library_file(self, dirs, lib, debug=0):
        static_f = self.library_filename(lib, lib_type='static')

        for dir in dirs:
            static = os.path.join(dir, static_f)
            if os.path.exists(static):
                return static

        # Oops, didn't find it in *any* of 'dirs'
        return None
