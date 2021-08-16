import importlib
import os
import pathlib
import site
import subprocess
import sys
import tempfile

import scipy

from abins.test_helpers import find_file
from mantid.simpleapi import SimulatedDensityOfStates
from systemtesting import MantidSystemTest


class SimulatedDensityOfStatesTest(MantidSystemTest):
    """Make sure normal case will run without Euphonic installed"""
    def runTest(self):
        SimulatedDensityOfStates(CASTEPFile=find_file('Na2SiF6_CASTEP.phonon'),
                                 Function='Gaussian',
                                 SpectrumType='DOS',
                                 OutputWorkspace='Na2SiF6_DOS')

    def validate(self):
        return ('Na2SiF6_DOS', 'Na2SiF6_DOS.nxs')


class SimulatedDensityOfStatesEuphonicTest(MantidSystemTest):
    """"Install Euphonic library to temporary prefix and check results"""
    @staticmethod
    def _add_libs_from_prefix(prefix_path):
        package_dirs = []
        for lib_dir in ('lib', 'lib64'):
            if (prefix_path / lib_dir).is_dir():
                site_packages = next(
                    (prefix_path / lib_dir).iterdir()) / 'site-packages'
                if site_packages.is_dir():
                    site.addsitedir(site_packages)
                    package_dirs.append(site_packages)

        if package_dirs:
            return package_dirs
        else:
            if not prefix_path.is_dir():
                raise FileNotFoundError(
                    f"Install prefix {prefix_path} does not exist.")
            else:
                directory_contents = list(prefix_path.iterdir())
                raise FileNotFoundError(
                    ("Could not find site-packages for temporary dir. "
                     "Here are the directory contents: ")
                    + "; ".join(directory_contents))

    def runTest(self):
        # First check if we are using debian-style Pip, where --system
        # is needed to prevent default --user option from breaking --prefix.

        if '--system' in subprocess.check_output(
                [sys.executable, "-m", "pip", "install", "-h"]).decode():
            compatibility_args = ['--system']
        else:
            compatibility_args = []

        with tempfile.TemporaryDirectory() as scipy_prefix, \
                tempfile.TemporaryDirectory() as euphonic_prefix:

            process = subprocess.run([sys.executable, "-m", "pip", "install",
                                      "--prefix", scipy_prefix]
                                     + compatibility_args
                                     + ["pip", "packaging"],
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT)

            prefix_path = pathlib.Path(scipy_prefix)

            # Add to path if anything was installed
            env = os.environ.copy()
            if list(prefix_path.iterdir()):
                scipy_site_packages = self._add_libs_from_prefix(prefix_path)

                process_pythonpath =  ':'.join([str(dir) for dir in scipy_site_packages] + [os.environ['PYTHONPATH']])
                env['PYTHONPATH'] = process_pythonpath

            # Install minimum Scipy version for Euphonic if necessary
            from packaging import version
            if version.parse(scipy.version.version) < version.parse('1.0.1'):
                process = subprocess.run([sys.executable, "-m", "pip", "install",
                                          "--ignore-installed",
                                          "--no-deps",
                                          "--prefix", scipy_prefix,
                                         "scipy==1.0", "pytest"],
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT,
                                         env=env)
                print(process.stdout.decode('utf-8'))

                # Add prefix again, in case nothing was installed before
                scipy_site_packages = self._add_libs_from_prefix(prefix_path)

                importlib.reload(scipy)

            process = subprocess.run([sys.executable, "-m", "pip", "install",
                                      "--prefix", euphonic_prefix,
                                      "euphonic[phonopy_reader]"],
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT,
                                     env=env)
            print(process.stdout.decode('utf-8'))

            self._add_libs_from_prefix(pathlib.Path(euphonic_prefix))

            import pint  # noqa: F401
            import euphonic  # noqa: F401

            SimulatedDensityOfStates(ForceConstantsFile=find_file('phonopy-Al.yaml'),
                                     Function='Gaussian',
                                     SpectrumType='DOS',
                                     OutputWorkspace='phonopy-Al_DOS')

    def validate(self):
        return ('phonopy-Al_DOS', 'phonopy-Al_DOS.nxs')
