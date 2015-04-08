import sys

print sys.executable
for item in sys.path:
    print item

import unittest
import re
# Need to import mantid before we import SANSUtility
import mantid
from mantid.simpleapi import CreateWorkspace, CreateSampleWorkspace, GroupWorkspaces, DeleteWorkspace
from mantid.api import mtd
import SANSUtility as su

TEST_STRING = 'SANS2D0003434-add_monitors_1'
TEST_STRING2 = 'SANS2D0003434-add_1'

def provide_group_workspace_for_added_event_data(event_ws_name, monitor_ws_name, out_ws_name):
    CreateWorkspace(DataX = [1,2,3], DataY = [2,3,4], OutputWorkspace = monitor_ws_name)
    CreateSampleWorkspace(WorkspaceType= 'Event', OutputWorkspace = event_ws_name)
    GroupWorkspaces(InputWorkspaces = [event_ws_name, monitor_ws_name ], OutputWorkspace = out_ws_name)


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


class TestLoadingAddedEventWorkspaceNameParsing(unittest.TestCase):

    def test_check_regex_for_data(self):
        self.assertIsNotNone(re.search(su.REG_DATA_NAME, TEST_STRING2))

    def test_check_regex_for_data_monitors(self):
        self.assertIsNotNone(re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING))

    def test_regexes_do_not_clash(self):
        self.assertIsNone(re.search(su.REG_DATA_NAME, TEST_STRING)) 
        self.assertIsNone(re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING2))
    
    def test_check_child_file_names_for_valid_names(self):
        event_name = TEST_STRING2
        monitor_name = TEST_STRING
        out_name = 'out_ws'
        provide_group_workspace_for_added_event_data(event_ws_name = event_name, monitor_ws_name = monitor_name, out_ws_name = out_name)
        out_ws = mtd[out_name]
        self.assertTrue(su.check_child_ws_for_name_and_type_for_added_eventdata(out_ws))
        DeleteWorkspace(out_ws)


class TestLoadingAddedEventWorkspaceExtraction(unittest.TestCase):

    def test_extract_data_and_monitor_child_ws(self):
        # Arrange
        event_base_name = 'eventWsSANS'
        monitor_base_name = 'monitorWsSANS'
        event_ws_name = event_base_name  + '_1' 
        monitor_ws_name = monitor_base_name  + '_1'
        out_ws_name = 'out_group'
        provide_group_workspace_for_added_event_data(event_ws_name = event_ws_name, monitor_ws_name = monitor_ws_name, out_ws_name = out_ws_name)
        out_ws_group = mtd[out_ws_name]

        # Act
        su.extract_child_ws_for_added_eventdata(out_ws_group)

        # Assert
        # Make sure that two files exist with a truncated name
        self.assertTrue(event_base_name in mtd)
        self.assertTrue(monitor_base_name in mtd)

        self.assertTrue(event_ws_name in mtd)
        self.assertTrue(monitor_ws_name in mtd)

        # Make sure they are true copies which are present after deleting the group
        DeleteWorkspace(out_ws_group)
        self.assertTrue(event_base_name in mtd)
        self.assertTrue(monitor_base_name in mtd)

        self.assertFalse(event_ws_name in mtd)
        self.assertFalse(monitor_ws_name in mtd)

        DeleteWorkspace(event_base_name)
        DeleteWorkspace(monitor_base_name)

if __name__ == "__main__":
    unittest.main()
