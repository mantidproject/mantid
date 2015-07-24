import unittest
import re
# Need to import mantid before we import SANSUtility
import mantid
from mantid.simpleapi import *
import ISISCommandInterface as ci


class TestEventWorkspaceCheck(unittest.TestCase):
    def _create_file_name(self, name):
        temp_save_dir = config['defaultsave.directory']
        if (temp_save_dir == ''):
            temp_save_dir = os.getcwd()
        return os.path.join(temp_save_dir, names + '.nxs')

    def _clean_up(self, file_name):
        if os.path.exists(file_name):
            os.remove(file_name)

    def test_that_event_workspace_is_detected(self):
        # Arrange
        ws = CreateSampleWorkspace()
        file_name = self._create_file_name('dummy')
        SaveNexus(Filename= file_name, InputWorkspace=ws)
        # Act
        result = ci.check_if_event_workspace(file_name)
        self.assertFalse(result)
        # Clean Up
        self._clean_up(file_name)

    def test_that_histogram_workspace_is_detected(self):
        # Arrange
        ws = CreateSampleWorkspace('Event')
        file_name = self._create_file_name('dummy')
        SaveNexus(Filename= file_name, InputWorkspace=ws)
        # Act
        result = ci.check_if_event_workspace(file_name)
        self.assertTrue(result)
        # Clean Up
        self._clean_up(file_name)
