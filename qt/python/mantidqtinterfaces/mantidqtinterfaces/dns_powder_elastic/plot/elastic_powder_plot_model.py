# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS powder elastic plotting tab model.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_powder_elastic.helpers.converters import elastic_two_theta_to_d, elastic_two_theta_to_q

from mantid.simpleapi import mtd


class DNSElasticPowderPlotModel(DNSObsModel):
    def __init__(self, parent):
        super().__init__(parent=parent)
        self._plotted_script_number = 0

    @staticmethod
    def get_max_int_of_workspaces(workspaces):
        workspaces = [ws for ws in workspaces if ws != "simulation"]
        if workspaces:
            return max([max(mtd[f"mat_{ws}"].extractY()[0]) for ws in workspaces])
        return 1

    @staticmethod
    def get_x_y_yerr(ws, x_axis, max_int, wavelength):
        x = _get_x(ws)
        y = _get_y(ws)
        y_err = _get_yerr(ws)
        x, y = _scale_simulation(ws, x, y, max_int)
        x = _convert_x_axis(x, x_axis, wavelength)
        return [x, y, y_err]

    def _data_list_updated(self, workspaces, data_list, script_number):
        compare = [f"mat_{x}" for x in data_list]
        return script_number != self._plotted_script_number or workspaces != compare  # check is necessary for simulation

    def get_updated_ws_list(self, workspaces, data_list, script_number):
        workspaces = sorted(workspaces)
        workspaces = _add_simulation_to_ws_list(workspaces)
        updated = self._data_list_updated(workspaces, data_list, script_number)
        if updated:
            self._set_script_number(script_number)
        return [workspaces, updated]

    @staticmethod
    def get_y_norm_label(norm):
        if norm:
            return "Counts/Monitor Counts"
        return "Counts/s"

    @staticmethod
    def get_x_axis_label(x_axis):
        if x_axis == "q":
            return r"q $(\AA^{-1})$"
        if x_axis == "d":
            return r"d $(\AA)$"
        return r"2$\theta$ (deg)"

    def _set_script_number(self, script_number):
        self._plotted_script_number = script_number


def _get_x(ws):
    x = mtd[f"mat_{ws}"].extractX()[0] * 2.0
    x = (abs(x[1] - x[0]) / 2 + x)[0:-1]
    return x


def _get_y(ws):
    return mtd[f"mat_{ws}"].extractY()[0]


def _get_yerr(ws):
    return mtd[f"mat_{ws}"].extractE()[0]


def _convert_x_axis(x, x_axis, wavelength):
    if x_axis == "d":
        return elastic_two_theta_to_d(x, wavelength)
    if x_axis == "q":
        return elastic_two_theta_to_q(x, wavelength)
    return x


def _scale_simulation(ws, x, y, max_int):
    if ws == "simulation":
        x = x / 2.0
        y = y / max(y) * max_int
    return [x, y]


def _add_simulation_to_ws_list(workspaces):
    if _check_workspace_exists("mat_simulation") and "mat_simulation" not in workspaces:
        workspaces.append("mat_simulation")
    return workspaces


def _check_workspace_exists(workspace):
    try:
        dummy = mtd[workspace]  # noqa: F841
    except KeyError:
        return False
    return True
