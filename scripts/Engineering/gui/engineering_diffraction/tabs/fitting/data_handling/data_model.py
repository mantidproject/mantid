# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from os import path

from mantid.simpleapi import Load, logger


class FittingDataModel(object):
    def __init__(self):
        self._loaded_workspaces = {}  # Map stores using {WorkspaceName: Workspace}

    def load_files(self, filenames_string: str):
        filenames = [name.strip() for name in filenames_string.split(",")]
        for filename in filenames:
            ws_name = self._generate_workspace_name(filename)
            try:
                self._loaded_workspaces[ws_name] = Load(filename, OutputWorkspace=ws_name)
            except RuntimeError as e:
                logger.error(
                    "Failed to load file: {}. Error: {}. \n Continuing loading of other files.".
                    format(filename, e))

    def get_loaded_workspaces(self):
        # TODO: Remove debug line in next PR, when output is visible in the GUI. 
        logger.debug("EngDiff2 Tracked Workspaces: {}".format(self._loaded_workspaces.keys()))
        return self._loaded_workspaces

    @staticmethod
    def _generate_workspace_name(filepath):
        return path.splitext(path.split(filepath)[1])[0]
