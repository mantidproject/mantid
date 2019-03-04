# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

import traceback

from mantid.kernel import logger


def exception_logger(exc_type, exc_value, exc_traceback):
    """
    Captures ALL EXCEPTIONS IN PYTHON. Prevents the Workbench to crash silently, instead it logs the
    error on ERROR level.
    :param exc_type: The type of the exception
    :param exc_value: Value of the exception, typically contains the error message.
    :param exc_traceback: Stack trace of the exception.
    """
    logger.error("".join(traceback.format_exception(exc_type, exc_value, exc_traceback)))
