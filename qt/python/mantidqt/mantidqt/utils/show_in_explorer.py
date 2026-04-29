# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import os
import subprocess

from mantid.kernel import logger
import mantid.kernel.environment as mtd_env


class ShowInExplorer(object):
    @staticmethod
    def open_directory(path):
        try:
            if mtd_env.is_windows():
                os.startfile(path)  # noqa: S606
            elif mtd_env.is_mac():
                subprocess.check_call(["open", "--", path])
            elif mtd_env.is_linux():
                call_params = ["xdg-open", path]
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
