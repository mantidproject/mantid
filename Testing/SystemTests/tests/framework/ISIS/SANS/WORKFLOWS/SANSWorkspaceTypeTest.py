# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
import systemtesting
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import CreateSampleWorkspace, DeleteWorkspace, SaveNexusProcessed
from SANSUtility import can_load_as_event_workspace
import os


def create_file_name(base_name):
    temp_save_dir = config["defaultsave.directory"]
    if temp_save_dir == "":
        temp_save_dir = os.getcwd()
    filename = os.path.join(temp_save_dir, base_name + ".nxs")
    return filename


def remove_temporary_file(filename):
    if os.path.exists(filename):
        os.remove(filename)


def clean_up_workspaces():
    for element in mtd.getObjectNames():
        if element in mtd:
            DeleteWorkspace(element)


class SANSProcessedEventWorkspaceInFile(systemtesting.MantidSystemTest):
    """
    Check if a processed nexus file is correctly detected to contain
    an event workspace.
    """

    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        # Arrange
        base_name = "processed_event"
        filename = create_file_name(base_name)
        ws = CreateSampleWorkspace("Event")
        SaveNexusProcessed(InputWorkspace=ws, Filename=filename)
        # Act
        can_load = can_load_as_event_workspace(filename)
        # Assert
        if can_load:
            self._success = True
        else:
            self._success = False
        # Clean up
        remove_temporary_file(filename)
        clean_up_workspaces()

    def validate(self):
        return self._success


class SANSProcessedHistoWorkspaceInFile(systemtesting.MantidSystemTest):
    """
    Check if a processed nexus file is correctly detected to contain
    a histo workspace.
    """

    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        # Arrange
        base_name = "processed_histo"
        filename = create_file_name(base_name)
        ws = CreateSampleWorkspace()
        SaveNexusProcessed(InputWorkspace=ws, Filename=filename)
        # Act
        can_load = can_load_as_event_workspace(filename)
        # Assert
        if not can_load:
            self._success = True
        else:
            self._success = False
        # Clean up
        remove_temporary_file(filename)
        clean_up_workspaces()

    def validate(self):
        return self._success
