# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import importlib
from mantid import logger
from types import ModuleType


def package_installed(package_name: str) -> bool:
    """
    A utility function which checks to see if a pip package is installed.
    @param package_name: the name of a pip package.
    @returns True if the package is installed, else False.
    """
    is_installed = importlib.util.find_spec(package_name) is not None
    if not is_installed:
        logger.warning(
            f"The '{package_name}' pip package is not installed. Follow these instructions to pip install the package:\n"
            f"    <documentation link here>"
        )
    return is_installed


def import_pip_package(package_name: str) -> ModuleType:
    """
    A utility function which can be used for attempting to import a pip package. If the pip package is not
    installed, it will show a warning message containing a link to install instructions.
    @param package_name: the name of a pip package to import.
    @returns the pip package if it is found, otherwise it returns None.
    """
    if "." in package_name:
        raise ValueError("This function does not support importing submodules of a pip package")

    # If the package is installed, import it
    if package_installed(package_name):
        return __import__(package_name)

    return None
