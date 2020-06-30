# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path

from mantid.simpleapi import Load, logger, EnggEstimateFocussedBackground, ConvertUnits, Plus, Minus
from matplotlib.pyplot import subplots


class FittingDataModel(object):
    def __init__(self):
        self._loaded_workspaces = {}  # Map stores using {WorkspaceName: Workspace}
        self._background_workspaces = {}
        self._last_added = []  # List of workspace names loaded in the last load action.
        self._bg_params = dict()  # {ws_name: [isSub, niter, xwindow, doSG]}

    def load_files(self, filenames_string, xunit):
        self._last_added = []
        filenames = [name.strip() for name in filenames_string.split(",")]
        for filename in filenames:
            ws_name = self._generate_workspace_name(filename, xunit)
            try:
                ws = Load(filename, OutputWorkspace=ws_name)
                if xunit != "TOF":
                    ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws_name, Target=xunit)
                if ws.getNumberHistograms() == 1:
                    self._loaded_workspaces[ws_name] = ws
                    self._background_workspaces[ws_name] = None
                    self._last_added.append(ws_name)
                else:
                    logger.warning(
                        f"Invalid number of spectra in workspace {ws_name}. Skipping loading of file.")
            except RuntimeError as e:
                logger.error(
                    f"Failed to load file: {filename}. Error: {e}. \n Continuing loading of other files.")

    def get_loaded_workspaces(self):
        return self._loaded_workspaces

    def get_background_workspaces(self):
        return self._background_workspaces

    def get_bg_params(self):
        return self._bg_params

    def do_background_subtraction(self, ws_name, bg_params):
        ws = self._loaded_workspaces[ws_name]
        ws_bg = self._background_workspaces[ws_name]
        bg_changed = False
        if ws_bg and bg_params != self._bg_params[ws_name]:
            # add bg back on to data
            self.undo_background_subtraction(ws_name, update_bg_params=False)
            bg_changed = True
        if bg_changed or not ws_bg:
            # revaluate background (or evaluate for first time)
            self._bg_params[ws_name] = bg_params
            ws_bg = self.estimate_background(ws_name, *bg_params[1:])
        Minus(LHSWorkspace=ws, RHSWorkspace=ws_bg, OutputWorkspace=ws_name)

    def undo_background_subtraction(self, ws_name, update_bg_params=True):
        Plus(LHSWorkspace=ws_name, RHSWorkspace=self.get_background_workspaces()[ws_name],
             OutputWorkspace=ws_name)
        self._bg_params[ws_name][0] = not update_bg_params

    def estimate_background(self, ws_name, niter, xwindow, doSGfilter):
        ws_bg = EnggEstimateFocussedBackground(InputWorkspace=ws_name, OutputWorkspace=ws_name + "_bg",
                                               NIterations=niter, XWindow=xwindow, ApplyFilterSG=doSGfilter)
        self._background_workspaces[ws_name] = ws_bg
        return ws_bg

    def plot_background_figure(self, ws_name):
        ws = self._loaded_workspaces()[ws_name]
        ws_bg = self._background_workspaces()[ws_name]
        if ws_bg:
            fig, ax = subplots(2, 1, sharex=True, gridspec_kw={'height_ratios': [2, 1]},
                               subplot_kw={'projection': 'mantid'})
            tmp = Plus(LHSWorkspace=ws_name, RHSWorkspace=ws_bg, StoreInADS=False)
            ax[0].plot(tmp, 'x')
            ax[0].plot(ws_bg, '-r')
            ax[1].plot(ws, 'x')
            fig.show()

    def get_last_added(self):
        return self._last_added

    def get_sample_log_from_ws(self, ws_name, log_name):
        return self._loaded_workspaces[ws_name].getSampleDetails().getLogData(log_name).value

    @staticmethod
    def _generate_workspace_name(filepath, xunit):
        return path.splitext(path.split(filepath)[1])[0] + '_' + xunit
