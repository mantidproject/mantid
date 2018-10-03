# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,no-init
from __future__ import (absolute_import, division, print_function)
import stresstesting
from mantid.simpleapi import *
from SANSUtility import can_load_as_event_workspace
import os

# WORKAROUND FOR IMPORT ISSUE IN UBUNTU --- START
CAN_IMPORT_NXS_TEST = True
try:
    import nxs # pylint: disable=unused-import # noqa
except ImportError:
    CAN_IMPORT_NXS_TEST = False
# WORKAROUND FOR IMPORT ISSUE IN UBUNTU --- STOP


def create_file_name(base_name):
    temp_save_dir = config['defaultsave.directory']
    if temp_save_dir == '':
        temp_save_dir = os.getcwd()
    filename = os.path.join(temp_save_dir, base_name + '.nxs')
    return filename


def remove_temporary_file(filename):
    if os.path.exists(filename):
        os.remove(filename)


def clean_up_workspaces():
    for element in mtd.getObjectNames():
        if element in mtd:
            DeleteWorkspace(element)


class SANSProcessedEventWorkspaceInFile(stresstesting.MantidStressTest):
    '''
    Check if a processed nexus file is correctly detected to contain
    an event workspace.
    '''

    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self._success = False

    def runTest(self):
        if CAN_IMPORT_NXS_TEST:
            # Arrange
            base_name = "processed_event"
            filename = create_file_name(base_name)
            ws = CreateSampleWorkspace("Event")
            SaveNexusProcessed(InputWorkspace=ws, Filename = filename)
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
        else:
            self._success = True

    def validate(self):
        return self._success


class SANSProcessedHistoWorkspaceInFile(stresstesting.MantidStressTest):
    '''
    Check if a processed nexus file is correctly detected to contain
    a histo workspace.
    '''

    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self._success = False

    def runTest(self):
        if CAN_IMPORT_NXS_TEST:
            # Arrange
            base_name = "processed_histo"
            filename = create_file_name(base_name)
            ws = CreateSampleWorkspace()
            SaveNexusProcessed(InputWorkspace=ws, Filename = filename)
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
        else:
            self._success = True

    def validate(self):
        return self._success
