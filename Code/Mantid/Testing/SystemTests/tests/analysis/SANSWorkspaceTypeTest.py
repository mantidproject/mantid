#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *
from SANSUtility import can_load_as_event_workspace
import os

def create_file_name(base_name):
    temp_save_dir = config['defaultsave.directory']
    if (temp_save_dir == ''):
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
    def runTest(self):
        # Arrange
        base_name = "processed_event"
        filename = create_file_name(base_name)
        ws = CreateSampleWorkspace("Event")
        SaveNexusProcessed(InputWorkspace=ws, Filename = filename)
        # Act
        can_load = can_load_as_event_workspace(filename)
        # Assert
        if can_load:
            self.success = True
        else:
            self.success = False
        # Clean up
        remove_temporary_file(filename)
        clean_up_workspaces()

    def validate(self):
        return self.success


class SANSProcessedHistoWorkspaceInFile(stresstesting.MantidStressTest):
    '''
    Check if a processed nexus file is correctly detected to contain
    a histo workspace.
    '''
    def runTest(self):
        # Arrange
        base_name = "processed_histo"
        filename = create_file_name(base_name)
        ws = CreateSampleWorkspace()
        SaveNexusProcessed(InputWorkspace=ws, Filename = filename)
        # Act
        can_load = can_load_as_event_workspace(filename)
        # Assert
        if not can_load:
            self.success = True
        else:
            self.success = False
        # Clean up
        remove_temporary_file(filename)
        clean_up_workspaces()

    def validate(self):
        return self.success
