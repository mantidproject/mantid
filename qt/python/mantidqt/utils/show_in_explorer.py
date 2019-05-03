# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

import os
import subprocess
import sys

from mantid.kernel import logger


class ShowInExplorer(object):
    @staticmethod
    def open_directory(path):
        try:
            if sys.platform == "win32":
                os.startfile(path)
            elif sys.platform == 'darwin':
                subprocess.check_call(['open', '--', path])
            elif sys.platform == 'linux2':
                call_params = ['xdg-open', path]
                subprocess.check_call(call_params)
        except Exception as ex:
            # it is hard to narrow down the type of exception, as too many things can go wrong:
            # - subprocess.CalledProcessError if the process has an error
            # - OSError if the command cannot be found (on Linux)
            # - WindowsError is the directory does not exist
            # However, catching this general exception makes sure that there will not be a crash
            # regardless of what the error is
            logger.notice("Could not open the folder in explorer.")
            logger.debug("Error encountered: {}".format(ex))
