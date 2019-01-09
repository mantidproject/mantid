# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import Engineering.EngineeringCalibration as Cal
import Engineering.EngineeringFocus as Focus

import os


class EnginX:
    def __init__(self, **kwargs):
        if kwargs:
            self.user = kwargs.get("user")
            self.van_run = kwargs.get("vanadium_run")
            if "calibration_directory" in kwargs:
                self.calibration_directory = "{0}/{1}/EnginX_Mantid/Calibration".format(
                    kwargs.get("calibration_directoy"), self.user)
            else:
                self.calibration_directory = "/home/sjenkins/user/{0}/EnginX_Mantid/Calibration".format(self.user)
            if "focus_directory" in kwargs:
                self.focus_directory = "{0}/{1}/EnginX_Mantid/Focus".format(kwargs.get("focus_directoy"), self.user)
            else:
                self.focus_directory = "/home/sjenkins/user/{0}/EnginX_Mantid/Focus".format(self.user)

    def create_vanadium(self):
        van_file = _gen_filename(self.van_run)
        van_curves_file, van_int_file = self._get_van_names()
        Cal.create_vanadium_workspaces(van_file, van_curves_file, van_int_file)

    def create_calibration(self, **kwargs):
        van_curves_file, van_int_file = self._get_van_names()
        if "ceria_run" in kwargs:
            ceria_run = kwargs.get("ceria_run")
        else:
            ceria_run = "241391"
        if "cropped" in kwargs:
            if kwargs.get("cropped") == "banks":
                Cal.create_calibration_cropped_file(False, kwargs.get("bank"), kwargs.get("crop_name"),
                                                    van_curves_file, van_int_file, ceria_run)
            elif kwargs.get("cropped") == "spectra":
                Cal.create_calibration_cropped_file(True, kwargs.get("spectra"), kwargs.get("crop_name"),
                                                    van_curves_file, van_int_file, ceria_run)
        else:
            Cal.create_calibration_files(van_curves_file, van_int_file, ceria_run)

    def focus(self, **kwargs):
        if "run_number" in kwargs:
            run_no = kwargs.get("run_number")
        else:
            return
        van_curves_file, van_int_file = self._get_van_names()
        if "cropped" in kwargs:
            if kwargs.get("cropped") == "banks":
                Focus.focus_cropped(False, kwargs.get("bank"), van_curves_file, van_int_file, run_no)
            elif kwargs.get("cropped") == "spectra":
                Focus.focus_cropped(True, kwargs.get("spectra"), van_curves_file, van_int_file, run_no)
        else:
            Focus.focus_whole(van_curves_file, van_int_file, run_no)

    def _get_van_names(self):
        van_file = _gen_filename(self.van_run)
        van_out = os.path.join(self.calibration_directory, van_file)
        van_int_file = van_out + "_precalculated_vanadium_run_integration.nxs"
        van_curves_file = van_out + "_precalculated_vanadium_run_bank_curves.nxs"
        return van_curves_file, van_int_file


def _gen_filename(run_number):
    return "ENGINX" + ("0" * (8 - len(run_number))) + run_number

