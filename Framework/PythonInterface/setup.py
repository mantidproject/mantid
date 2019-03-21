# -*- coding: utf-8 -*-
#  This file is part of the mantid workbench.
#
#

from setuptools import find_packages, setup

def patch_setuptools_command(cmd_cls_name):
    import importlib
    cmd_module = importlib.import_module('setuptools.command.' + cmd_cls_name)
    setuptools_command_cls = getattr(cmd_module, cmd_cls_name)

    class CustomCommand(setuptools_command_cls):
        user_options = setuptools_command_cls.user_options[:]
        boolean_options = setuptools_command_cls.boolean_options[:]
        def finalize_options(self):
            build_cmd = self.get_finalized_command('build')
            self.build_lib = 'C:/Users/qbr77747/dev/m/source/build/Framework/PythonInterface/build'
            setuptools_command_cls.finalize_options(self)

    return CustomCommand

CustomBuildPy = patch_setuptools_command('build_py')
CustomInstall = patch_setuptools_command('install')
CustomInstallLib = patch_setuptools_command('install_lib')


# The most basic setup possible to be able to use setup.py develop
setup(
    name='PythonInterface', # probaly the wrong name if someone wants to include it
    version='3.13.20190320.1118',
    packages=find_packages(exclude=['*.test']),
    package_data={'': ['*.ui']},
    cmdclass={'build_py': CustomBuildPy, 'install': CustomInstall, 'install-lib': CustomInstallLib }
)
