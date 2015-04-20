import mantid
import unittest
from mantid.simpleapi import *
from mantid.api import WorkspaceGroup
import SANSUtility as su
import re

class TestSliceStringParser(unittest.TestCase):

    def checkValues(self, list1, list2):

        def _check_single_values( v1, v2):
            self.assertAlmostEqual(v1, v2)

        self.assertEqual(len(list1), len(list2))
        for v1,v2 in zip(list1, list2):
            start_1,stop_1 = v1
            start_2, stop_2 = v2
            _check_single_values(start_1, start_2)
            _check_single_values(stop_1, stop_2)

    def test_checkValues(self):
        """sanity check to ensure that the others will work correctly"""
        values = [  [[1,2],],
                  [[None, 3],[4, None]],
                 ]
        for singlevalues in values:
            self.checkValues(singlevalues, singlevalues)


    def test_parse_strings(self):
        inputs = { '1-2':[[1,2]],         # single period syntax  min < x < max
                   '1.3-5.6':[[1.3,5.6]], # float
                   '1-2,3-4':[[1,2],[3,4]],# more than one slice
                   '>1':[[1, -1]],       # just lower bound
                   '<5':[[-1, 5]],      # just upper bound
                   '<5,8-9': [[-1, 5], [8,9]],
                   '1:2:5': [[1,3], [3,5]] # sintax: start, step, stop
            }

        for (k, v) in inputs.items():
            self.checkValues(su.sliceParser(k),v)

    def test_accept_spaces(self):
        self.checkValues(su.sliceParser("1 - 2, 3 - 4"), [[1,2],[3,4]])

    def test_invalid_values_raise(self):
        invalid_strs = ["5>6", ":3:", "MAX<min"]
        for val in invalid_strs:
            self.assertRaises(SyntaxError, su.sliceParser, val)

    def test_empty_string_is_valid(self):
        self.checkValues(su.sliceParser(""), [[-1,-1]])

class TestBundleAddedEventDataFilesToGroupWorkspaceFile(unittest.TestCase):
    def _prepare_workspaces(self, names):
        CreateSampleWorkspace(WorkspaceType = 'Event', OutputWorkspace = names[0])
        CreateWorkspace(DataX = [1,1,1], DataY = [1,1,1], OutputWorkspace = names[1])
        
        temp_save_dir = config['defaultsave.directory']
        data_file_name = os.path.join(temp_save_dir, names[0] + '.nxs')
        monitor_file_name = os.path.join(temp_save_dir, names[1] + '.nxs')

        SaveNexusProcessed(InputWorkspace = names[0], Filename = data_file_name)
        SaveNexusProcessed(InputWorkspace = names[1], Filename = monitor_file_name)

        file_names = [data_file_name, monitor_file_name]

        return file_names


    def _cleanup_workspaces(self, names):
        for name in names:
            DeleteWorkspace(name)


    def test_load_valid_added_event_data_and_monitor_file_produces_group_ws(self):
        # Arrange
        names = ['even_data', 'monitor']
        file_names = self._prepare_workspaces(names = names)
        
        # Act
        group_ws_name = 'g_ws'
        output_group_file_name = su.bundle_added_event_data_as_group(file_names[0], file_names[1])
        Load(Filename = output_group_file_name, OutputWorkspace = group_ws_name)
        group_ws = mtd[group_ws_name]
        print ''
        # Assert
        self.assertTrue(isinstance(group_ws, WorkspaceGroup))
        self.assertEqual(group_ws.size(), 2)
        self.assertFalse(os.path.exists(file_names[0]))
        self.assertFalse(os.path.exists(file_names[1]))

        # Clean up
        names.append(group_ws_name)
        self._cleanup_workspaces(names = names)


if __name__ == "__main__":
    unittest.main()
