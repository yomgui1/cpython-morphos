# created 1999/03/13, Greg Ward

__revision__ = "$Id$"

import sys, os, string
from distutils.core import Command
from distutils.util import copy_tree

class install_lib (Command):

    description = "install pure Python modules"

    user_options = [
        ('install-dir=', 'd', "directory to install to"),
        ('build-dir=','b', "build directory (where to install from)"),
        ('compile', 'c', "compile .py to .pyc"),
        ('optimize', 'o', "compile .py to .pyo (optimized)"),
        ]
               

    def initialize_options (self):
        # let the 'install' command dictate our installation directory
        self.install_dir = None
        self.build_dir = None
        self.compile = 1
        self.optimize = 1

    def finalize_options (self):

        # Get all the information we need to install pure Python modules
        # from the umbrella 'install' command -- build (source) directory,
        # install (target) directory, and whether to compile .py files.
        self.set_undefined_options ('install',
                                    ('build_lib', 'build_dir'),
                                    ('install_lib', 'install_dir'),
                                    ('compile_py', 'compile'),
                                    ('optimize_py', 'optimize'))


    def run (self):

        # Make sure we have built everything we need first
        if self.distribution.has_pure_modules():
            self.run_peer ('build_py')
        if self.distribution.has_ext_modules():
            self.run_peer ('build_ext')

        # Install everything: simply dump the entire contents of the build
        # directory to the installation directory (that's the beauty of
        # having a build directory!)
        outfiles = self.copy_tree (self.build_dir, self.install_dir)

        # (Optionally) compile .py to .pyc
        # XXX hey! we can't control whether we optimize or not; that's up
        # to the invocation of the current Python interpreter (at least
        # according to the py_compile docs).  That sucks.

        if self.compile:
            from py_compile import compile

            for f in outfiles:
                # only compile the file if it is actually a .py file
                if f[-3:] == '.py':
                    out_fn = f + (__debug__ and "c" or "o")
                    compile_msg = "byte-compiling %s to %s" % \
                                  (f, os.path.basename (out_fn))
                    skip_msg = "byte-compilation of %s skipped" % f
                    self.make_file (f, out_fn, compile, (f,),
                                    compile_msg, skip_msg)
                                    
                                    
    # run ()


    def _mutate_outputs (self, has_any, build_cmd, cmd_option, output_dir):

        if not has_any:
            return []

        build_cmd = self.find_peer (build_cmd)
        build_files = build_cmd.get_outputs()
        build_dir = build_cmd.get_option (cmd_option)

        prefix_len = len (build_dir) + len (os.sep)
        outputs = []
        for file in build_files:
            outputs.append (os.path.join (output_dir, file[prefix_len:]))

        return outputs

    # _mutate_outputs ()
        
    def get_outputs (self):
        """Return the list of files that would be installed if this command
        were actually run.  Not affected by the "dry-run" flag or whether
        modules have actually been built yet."""

        pure_outputs = \
            self._mutate_outputs (self.distribution.has_pure_modules(),
                                  'build_py', 'build_lib',
                                  self.install_dir)


        ext_outputs = \
            self._mutate_outputs (self.distribution.has_ext_modules(),
                                  'build_ext', 'build_lib',
                                  self.install_dir)

        return pure_outputs + ext_outputs

    # get_outputs ()

# class install_lib
