
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
                      'build mantid within a conda environment'),
                     ('install-prefix', None,
                      'cmake install prefix'),
                     ('install-mantid-cpp', None, 'Install the mantid cpp libs')] + build_ext.user_options)

    def get_ext_built(self, name):
        suffix = sysconfig.get_config_var('SO')
        return os.path.join('mantid', *os.path.split(name)) + suffix

    def get_ext_filename(self, ext_name):
        filename = super().get_ext_filename(ext_name)
        suffix = sysconfig.get_config_var('EXT_SUFFIX')
        ext = os.path.splitext(filename)[1]
        return filename.replace(suffix, "") + ext

    def initialize_options(self):
        build_ext.initialize_options(self)
        self.extra_cmake_args = ''
        self.build_type = os.environ.get('MANTID_BUILD_TYPE', 'Release')
        self.cmake_generator = os.environ.get('MANTID_CMAKE_GENERATOR', 'Ninja')
        self.with_conda = os.environ.get('MANTID_WITH_CONDA', 'True')
        self.install_mantid_cpp = os.environ.get('MANTID_INSTALL_CPP_LIBS', 'True')
        self.install_prefix = os.environ.get('CONDA_PREFIX', self.build_lib)

    def finalize_options(self):
        if self.inplace:
            self.install_mantid_cpp = False
        return super().finalize_options()

    def build_extensions(self):
        try:
            subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError('Cannot find CMake executable')

        # build directory is in build temp
        build_directory = os.path.abspath(self.build_temp)

        cmake_args = [
                '-DENABLE_WORKBENCH=OFF',
                '-DENABLE_DOCS=OFF',
                '-DUSE_SETUPPY=True',
                '-DINPLACE_BUILD=True',
                '-DCMAKE_BUILD_TYPE=' + self.build_type,
                '-DPYTHON_EXECUTABLE={}'.format(sys.executable),
                '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + build_directory,
                '-DCMAKE_INSTALL_PREFIX=' + self.install_prefix,
                '-DCONDA_BUILD='+ self.with_conda,
                '-G' + self.cmake_generator,
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
        subprocess.check_call(['cmake', '--build', '.', '--config', self.build_type],
                              cwd=self.build_temp)
        if self.install_mantid_cpp:
            self.announce("Installating mantid cpp files", level=3)

            subprocess.check_call(['cmake', '--build', '.', '--config', self.build_type, '--target', 'install'],
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
      cmdclass={'build_ext': CMakeBuild},
      classifiers=[
          "Programming Language :: Python :: 3",
          "License :: OSI Approved :: MIT License",
          "Operating System :: MacOS",
          "Operating System :: Microsoft :: Windows",
          "Operating System :: POSIX :: Linux",
      ],
      )
