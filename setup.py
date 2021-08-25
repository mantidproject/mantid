
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import sys
import subprocess
from pathlib import Path
from pprint import pprint

from distutils import sysconfig
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from setuptools.command.install_lib import install_lib


# Command line flags forwarded to CMake
cmake_cmd_args = []
for f in sys.argv:
    if f.startswith('-D'):
        cmake_cmd_args.append(f)

for f in cmake_cmd_args:
    sys.argv.remove(f)


class CMakeExtension(Extension):
    def __init__(self, name, cmake_lists_dir='.', sources=[], **kwa):
        Extension.__init__(self, name, sources=sources, **kwa)
        self.cmake_lists_dir = os.path.abspath(cmake_lists_dir)


class CMakeBuild(build_ext):

    user_options = ([('cmake-generator=', None, 'CMake generator, default ninja'),
                     ('extra-cmake-args=', None, 'extra arguments for CMake'),
                     ('build-type=', None,
                      'build type (debug or release), default release'),
                     ('with-conda', None,
                      'build pyarrow with TensorFlow support'),
                     ('install-prefix', None,
                      'bundle the (shared) Boost libraries')])

    def get_ext_built(self, name):
        suffix = sysconfig.get_config_var('SO')
        return os.path.join('mantid', *os.path.split(name)) + suffix

    def get_ext_filename(self, ext_name):
        filename = super().get_ext_filename(ext_name)
        suffix = sysconfig.get_config_var('EXT_SUFFIX')
        ext = os.path.splitext(filename)[1]
        return filename.replace(suffix, "") + ext

    def build_extensions(self):
        try:
            subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError('Cannot find CMake executable')

        cfg = "Release"

        # build directory is in build temp
        build_directory = os.path.abspath(self.build_temp)

        cmake_args = [
                '-DENABLE_WORKBENCH=OFF',
                '-DENABLE_OPENGL=OFF',
                '-DENABLE_DOCS=OFF',
                '-DCMAKE_BUILD_TYPE=%s' % cfg,
                '-DPYTHON_EXECUTABLE={}'.format(sys.executable),
                '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + build_directory,
                '-GNinja',
                '-DINPLACE_BUILD=True',
                '-DCMAKE_INSTALL_PREFIX=' + os.environ['CONDA_PREFIX'],
                '-DCONDA_BUILD=True',
                '-DUSE_SETUPPY=True',
            ]

        cmake_args += cmake_cmd_args

        pprint(cmake_args)

        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        # Config and build the extension
        cmake_list_dir = os.path.abspath(os.path.dirname(__file__))
        print("-- Running cmake configure for mantid")
        subprocess.check_call(['cmake', cmake_list_dir] + cmake_args,
                              cwd=self.build_temp)
        print("-- Running cmake --build for mantid")
        subprocess.check_call(['cmake', '--build', '.', '--config', cfg],
                              cwd=self.build_temp)

        # Move from build temp to final position
        for ext in self.extensions:
            # The extension is supposed to be here..
            dest_path = Path(self.get_ext_fullpath(ext.name)).resolve()
            # but is currently here..
            dest_file = self.get_ext_filename(ext.name).split('/')[-1]
            source_path = os.path.abspath(os.path.join(self.build_temp, "bin", dest_file))
            # Now move the lib
            dest_directory = dest_path.parents[0]
            dest_directory.mkdir(parents=True, exist_ok=True)
            self.copy_file(source_path, dest_path)


class InstallCMakeLibs(install_lib):
    """
    We need to install the mantid-cpp files
    The python extensions installation will be handled by setup tools
    """

    def run(self):

        self.announce("Installating mantid cpp files", level=3)

        # We have already built the libraries in the previous build_ext step
        self.skip_build = True

        subprocess.check_call(['cmake', '--build', '.', '--config', "Release", '--target', 'install'],
                              cwd=self.build_dir)

        # we have some options now
        # we've just installed a bunch of cpp libs which setup tools has no idea about
        # we can manually add these files to the outfiles, so they are tracked
        # but in doing so we need to hard code the file locations

        super().run()


# define mantid framework extensions
ext_modules = [
  CMakeExtension('mantid.kernel._kernel'),
  CMakeExtension('mantid.api._api'),
  CMakeExtension('mantid.geometry._geometry'),
  CMakeExtension('mantid.dataobjects._dataobjects'),
  CMakeExtension('mantid._plugins._curvefitting')]

setup(name='mantid',
      packages=['mantid', 'mantid.kernel', 'mantid.api', 'mantid.utils',
                'mantid.geometry', 'mantid.plots', 'mantid.dataobjects', 'mantid.py36compat'],
      version="6.2.0",
      description='Generate displacement fields with known volume changes',
      author='Mantid Project Developers',
      package_dir={'': 'Framework/PythonInterface'},
      ext_modules=ext_modules,
      cmdclass={'build_ext': CMakeBuild,
                'install_lib': InstallCMakeLibs,
                },
      classifiers=[
          "Programming Language :: Python :: 3",
          "License :: OSI Approved :: MIT License",
          "Operating System :: MacOS",
          "Operating System :: Microsoft :: Windows",
          "Operating System :: POSIX :: Linux",
      ],
      )
