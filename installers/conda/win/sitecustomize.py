# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.


def _add_executable_path_to_path():
    """
    Adds the sys.executable and plugin directory to the DLL load list of allowed directories
    """
    import os
    import sys

    executable_dir = os.path.dirname(sys.executable)
    plugin_dir = os.path.join(executable_dir, os.pardir, "plugins", "qt5")

    os.environ["PATH"] = f'{executable_dir};{plugin_dir};{os.environ["PATH"]}'
    os.add_dll_directory(executable_dir)
    os.add_dll_directory(plugin_dir)


_add_executable_path_to_path()
# Remove this from the namespace to avoid name leakage
del _add_executable_path_to_path
