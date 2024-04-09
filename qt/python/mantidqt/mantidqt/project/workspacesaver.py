# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import os.path

from mantid.api import AnalysisDataService as ADS, IMDEventWorkspace
from mantid.dataobjects import MDHistoWorkspace, GroupingWorkspace
from mantid import logger


class WorkspaceSaver(object):
    def __init__(self, directory):
        """

        :param directory:
        """
        self.directory = directory
        self.output_list = []

    def save_workspaces(self, workspaces_to_save=None):
        """
        Use the private method _get_workspaces_to_save to get a list of workspaces that are present in the ADS to save
        to the directory that was passed at object creation time, it will also add each of them to the output_list
        private instance variable on the WorkspaceSaver class.
        :param workspaces_to_save: List of Strings; The workspaces that are to be saved to the project.
        """

        # Handle getting here and nothing has been given passed
        if workspaces_to_save is None:
            return

        workspaces = ADS.retrieveWorkspaces(workspaces_to_save)

        for workspace, workspace_name in zip(workspaces, workspaces_to_save):
            # Get the workspace from the ADS
            place_to_save_workspace = os.path.join(self.directory, workspace_name)

            from mantid.simpleapi import SaveMD, SaveNexusProcessed

            try:
                if isinstance(workspace, MDHistoWorkspace) or isinstance(workspace, IMDEventWorkspace):
                    # Save normally using SaveMD
                    SaveMD(InputWorkspace=workspace, Filename=place_to_save_workspace + ".nxs")
                elif isinstance(workspace, GroupingWorkspace):
                    # catch this rather than leave SaveNexusProcessed to raise error to avoid message of type error
                    # being logged
                    raise RuntimeError("Grouping Workspaces not supported by SaveNexusProcessed")
                else:
                    # Save normally using SaveNexusProcessed
                    SaveNexusProcessed(InputWorkspace=workspace, Filename=place_to_save_workspace + ".nxs")
                self.output_list.append(workspace_name)
            except Exception as exc:
                logger.warning("Couldn't save workspace in project: \"" + workspace_name + '" because ' + str(exc))

    def get_output_list(self):
        """
        Get the output_list
        :return: List; String list of the workspaces that were saved
        """
        return self.output_list
