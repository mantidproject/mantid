# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS elastic powder plot model
"""
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import \
    DNSObsModel
from mantidqtinterfaces.dns_powder_elastic.helpers.converters import \
    el_twotheta_to_d, el_twotheta_to_q

from mantid.simpleapi import mtd


class DNSElasticPowderPlotModel(DNSObsModel):
    def __init__(self, parent):
        super().__init__(parent=parent)
        self._plotted_script_number = 0

    @staticmethod
    def get_max_int_of_workspaces(workspaces):
        workspaces = [ws for ws in workspaces if ws != 'simulation']
        if workspaces:
            return max([
                max(mtd[f'mat_{ws}'].extractY()[0])
                for ws in workspaces
            ])
        return 1

    @staticmethod
    def get_x_y_yerr(ws, xaxis, max_int, wavelength):
        x = _get_x(ws)
        y = _get_y(ws)
        yerr = _get_yerr(ws)
        x, y = _scale_simulation(ws, x, y, max_int)
        x = _convert_x_axis(x, xaxis, wavelength)
        return [x, y, yerr]

    def _datalist_updated(self, workspaces, datalist, scriptnumer):
        compare = [f'mat_{x}' for x in datalist]
        return (scriptnumer != self._plotted_script_number
                or workspaces != compare)  # check is nesesary for simulation

    def get_updated_ws_list(self, workspaces, datalist, scriptnumer):
        workspaces = sorted(workspaces)
        workspaces = _add_simulation_to_ws_list(workspaces)
        updated = self._datalist_updated(workspaces, datalist, scriptnumer)
        if updated:
            self._set_script_number(scriptnumer)
        return [workspaces, updated]

    @staticmethod
    def get_y_norm_label(norm):
        if norm:
            return 'normed to monitor'
        return 'Counts/s'

    @staticmethod
    def get_x_axis_label(xaxis):
        if xaxis == 'q':
            return r'$q (\AA^{-1})$'
        if xaxis == 'd':
            return r'$d (\AA)$'
        return '2 theta (degree)'

    def _set_script_number(self, scriptnumber):
        self._plotted_script_number = scriptnumber


def _get_x(ws):
    x = mtd[f'mat_{ws}'].extractX()[0] * 2
    x = (abs(x[1] - x[0]) / 2 + x)[0:-1]
    return x


def _get_y(ws):
    return mtd[f'mat_{ws}'].extractY()[0]


def _get_yerr(ws):
    return mtd[f'mat_{ws}'].extractE()[0]


def _convert_x_axis(x, xaxis, wavelength):
    if xaxis == 'd':
        return el_twotheta_to_d(x, wavelength)
    if xaxis == 'q':
        return el_twotheta_to_q(x, wavelength)
    return x


def _scale_simulation(ws, x, y, max_int):
    if ws == 'simulation':
        x = x / 2.0
        y = y / max(y) * max_int
    return [x, y]


def _add_simulation_to_ws_list(workspaces):
    if (_check_workspace_exists('mat_simulation')
            and 'mat_simulation' not in workspaces):
        workspaces.append('mat_simulation')
    return workspaces


def _check_workspace_exists(workspace):
    try:
        dummy = mtd[workspace]  # noqa: F841
    except KeyError:
        return False
    return True
