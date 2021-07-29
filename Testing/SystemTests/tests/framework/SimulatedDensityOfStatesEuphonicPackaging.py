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


class SimulatedDensityOfStatesEuphonicPackagingTest(MantidSystemTest):
    def runTest(self):
        SimulatedDensityOfStates(CASTEPFile=find_file('Na2SiF6_CASTEP.phonon'),
                                 Function='Gaussian',
                                 SpectrumType='DOS',
                                 OutputWorkspace='Na2SiF6_DOS')

    def validate(self):
        return ('Na2SiF6_DOS', 'Na2SiF6_DOS.nxs')


class SimulatedDensityOfStatesEuphonicInstallationTest(MantidSystemTest):
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
            try:
                scipy_site_packages = next((prefix_path / 'lib').iterdir()
                                           ) / 'site-packages'
            except FileNotFoundError:
                raise FileNotFoundError("Could not find site-packages for temporary dir. "
                                        "Here are the directory contents: "
                                        "\n".join(list(prefix_path.iterdir())))

            sys.path = [str(scipy_site_packages)] + sys.path
            os.environ['PYTHONPATH'] = str(scipy_site_packages) + ':' + os.environ['PYTHONPATH']

            #raise Exception(list(enumerate(sys.path)))

            #sys.path.append(str(tmp_site_packages))
            importlib.reload(site)
            globals()['packaging'] = importlib.import_module('packaging')

            # Install minimum Scipy version for Euphonic if necessary
            from packaging import version
            if version.parse(scipy.version.version) < version.parse('1.0.1'):
                process = subprocess.run([sys.executable, "-m", "pip", "install",
                                          "--ignore-installed", "--no-deps",
                                          "--prefix", scipy_prefix]
                                         #+ compatibility_args
                                         + ["scipy==1.0", "pytest"],
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT)
                print(process.stdout.decode('utf-8'))

                importlib.reload(site)
                importlib.reload(scipy)

            process = subprocess.run([sys.executable, "-m", "pip", "install",
                                      "--prefix", euphonic_prefix]
                                     #+ compatibility_args
                                     + ["euphonic"],
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT)
            print(process.stdout.decode('utf-8'))

            prefix_path = pathlib.Path(euphonic_prefix)
            euphonic_site_packages = next((prefix_path / 'lib').iterdir()
                                          ) / 'site-packages'
            sys.path.append(str(euphonic_site_packages))

            importlib.reload(site)
            globals()['euphonic'] = importlib.import_module('euphonic')
            import euphonic  # noqa: F401

            SimulatedDensityOfStates(CASTEPFile=find_file('Na2SiF6_CASTEP.phonon'),
                                     Function='Gaussian',
                                     SpectrumType='DOS',
                                     OutputWorkspace='Na2SiF6_DOS')

    def validate(self):
        return ('Na2SiF6_DOS', 'Na2SiF6_DOS.nxs')
