# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import importlib
from mantid import logger


def package_installed(package_name: str, show_warning: bool = False) -> bool:
    """
    A utility function which checks to see if a pip package is installed.
    @param package_name: the name of a pip package.
    @param show_warning: a warning message will be shown if the package is not installed
    @returns True if the package is installed, else False.
    """
    is_installed = importlib.util.find_spec(package_name) is not None
    if not is_installed and show_warning:
        logger.warning(
            f"The '{package_name}' pip package is not installed. Follow these instructions to pip install the package:\n"
            f"https://docs.mantidproject.org/nightly/concepts/PipInstall.html"
        )
    return is_installed
