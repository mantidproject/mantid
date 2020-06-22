# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path

from mantid.simpleapi import Load, logger, EnggEstimateFocussedBackground, ConvertUnits


class FittingDataModel(object):
    def __init__(self):
        self._loaded_workspaces = {}  # Map stores using {WorkspaceName: Workspace}
        self._background_workspaces = {}
        self._last_added = []  # List of workspace names loaded in the last load action.

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
                        "Invalid number of spectra in workspace {}. Skipping loading of file.".
                            format(ws_name))
            except RuntimeError as e:
                logger.error(
                    "Failed to load file: {}. Error: {}. \n Continuing loading of other files.".
                        format(filename, e))

    def get_loaded_workspaces(self):
        return self._loaded_workspaces

    def get_background_workspaces(self):
        return self._background_workspaces

    def estimate_background(self, ws_name, niter, xwindow, doSGfilter):
        ws_bg = EnggEstimateFocussedBackground(InputWorkspace=ws_name, OutputWorkspace=ws_name + "_bg",
                                               NIterations=niter, XWindow=xwindow, ApplyFilterSG=doSGfilter)
        self._background_workspaces[ws_name] = ws_bg
        return ws_bg

    def get_last_added(self):
        return self._last_added

    def get_sample_log_from_ws(self, ws_name, log_name):
        return self._loaded_workspaces[ws_name].getSampleDetails().getLogData(log_name).value

    @staticmethod
    def _generate_workspace_name(filepath, xunit):
        return path.splitext(path.split(filepath)[1])[0] + '_' + xunit
