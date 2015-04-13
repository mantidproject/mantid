
import unittest
import re
# Need to import mantid before we import SANSUtility
import mantid
from mantid.simpleapi import CreateWorkspace, CreateSampleWorkspace, GroupWorkspaces, DeleteWorkspace
from mantid.api import mtd
import SANSUtility as su

TEST_STRING_DATA = 'SANS2D0003434-add'
TEST_STRING_MON = 'SANS2D0003434-add_monitors'

TEST_STRING_DATA1 = 'SANS2D0003434-add_1'
TEST_STRING_MON1 = 'SANS2D0003434-add_monitors_1'

TEST_STRING_DATA2 = 'SANS2D0003434-add_2'
TEST_STRING_MON2 = 'SANS2D0003434-add_monitors_2'

TEST_STRING_DATA3 = 'SANS2D0003434-add_3'
TEST_STRING_MON3 = 'SANS2D0003434-add_monitors_3'


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

    def do_test_load_check(self, event_name, monitor_name):
        out_name = 'out_ws'
        provide_group_workspace_for_added_event_data(event_ws_name = event_name, monitor_ws_name = monitor_name, out_ws_name = out_name)
        out_ws = mtd[out_name]
        self.assertTrue(su.check_child_ws_for_name_and_type_for_added_eventdata(out_ws))
        DeleteWorkspace(out_ws)

    def test_check_regex_for_data(self):
        # Check when there is no special ending
        self.assertIsNotNone(re.search(su.REG_DATA_NAME, TEST_STRING_DATA))
        # Check when there is a _1 ending
        self.assertIsNotNone(re.search(su.REG_DATA_NAME, TEST_STRING_DATA1))
        # Check when there is a _2 ending
        self.assertIsNotNone(re.search(su.REG_DATA_NAME, TEST_STRING_DATA2))
        # Check when there is a multiple ending
        self.assertIsNotNone(re.search(su.REG_DATA_NAME, TEST_STRING_DATA3))


    def test_check_regex_for_data_monitors(self):
        # Check when there is no special ending
        self.assertIsNotNone(re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_MON))
        # Check when there is a _1 ending
        self.assertIsNotNone(re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_MON1))
        # Check when there is a _2 ending
        self.assertIsNotNone(re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_MON2))
        # Check when there is a multiple ending
        self.assertIsNotNone(re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_MON3))

    def test_regexes_do_not_clash(self):
        # Check when there is no special ending
        self.assertIsNone(re.search(su.REG_DATA_NAME, TEST_STRING_MON)) 
        self.assertIsNone(re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_DATA))
        # Check when there is a _1 ending
        self.assertIsNone(re.search(su.REG_DATA_NAME, TEST_STRING_MON1)) 
        self.assertIsNone(re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_DATA1))
        # Check when there is a _2 ending
        self.assertIsNone(re.search(su.REG_DATA_NAME, TEST_STRING_MON2)) 
        self.assertIsNone(re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_DATA2))
        # Check when there is a multiple ending
        self.assertIsNone(re.search(su.REG_DATA_NAME, TEST_STRING_MON3)) 
        self.assertIsNone(re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_DATA3))
    
    def test_check_child_file_names_for_valid_names(self):
        # Check when there is no special ending
        event_name = TEST_STRING_DATA
        monitor_name = TEST_STRING_MON
        self.do_test_load_check(event_name = event_name, monitor_name = monitor_name)

        # Check when there is a _1 ending
        event_name1 = TEST_STRING_DATA1
        monitor_name1 = TEST_STRING_MON1
        self.do_test_load_check(event_name = event_name1, monitor_name = monitor_name1)

        # Check when there is a _2 ending
        event_name2 = TEST_STRING_DATA2
        monitor_name2 = TEST_STRING_MON2
        self.do_test_load_check(event_name = event_name2, monitor_name = monitor_name2)

        # Check when there is a multiple ending
        event_name3 = TEST_STRING_DATA3
        monitor_name3 = TEST_STRING_MON3
        self.do_test_load_check(event_name = event_name3, monitor_name = monitor_name3)


class TestLoadingAddedEventWorkspaceExtraction(unittest.TestCase):
    def do_test_extraction(self, event_name, monitor_name, event_name_expect, monitor_name_expect):
        out_ws_name = 'out_group'
        provide_group_workspace_for_added_event_data(event_ws_name = event_name, monitor_ws_name = monitor_name, out_ws_name = out_ws_name)
        out_ws_group = mtd[out_ws_name]

        # Act
        su.extract_child_ws_for_added_eventdata(out_ws_group)

        # Assert
        # Make sure that two files exist with a truncated name

        self.assertTrue(event_name_expect in mtd)
        self.assertTrue(monitor_name_expect in mtd)

        DeleteWorkspace(event_name_expect)
        DeleteWorkspace(monitor_name_expect)

    def test_pruning_of_data_and_monitor_child_names(self):
        # Check when there is no special ending
        out = su.get_pruned_added_event_data_names(TEST_STRING_DATA)
        self.assertTrue(TEST_STRING_DATA == out)
        out_mon = su.get_pruned_added_event_data_names(TEST_STRING_MON)
        self.assertTrue(TEST_STRING_MON == out_mon)

        # Check when there is a _1 ending
        out1 = su.get_pruned_added_event_data_names(TEST_STRING_DATA1)
        self.assertTrue(TEST_STRING_DATA == out1)
        out1_mon = su.get_pruned_added_event_data_names(TEST_STRING_MON1)
        self.assertTrue(TEST_STRING_MON == out1_mon)

        # Check when there is a _2 ending
        out2 = su.get_pruned_added_event_data_names(TEST_STRING_DATA2)
        self.assertTrue(TEST_STRING_DATA == out2)
        out2_mon = su.get_pruned_added_event_data_names(TEST_STRING_MON2)
        self.assertTrue(TEST_STRING_MON == out2_mon)

        # Check when there is a multiple ending
        out3 = su.get_pruned_added_event_data_names(TEST_STRING_DATA3)
        self.assertTrue(TEST_STRING_DATA == out3)
        out3_mon = su.get_pruned_added_event_data_names(TEST_STRING_MON3)
        self.assertTrue(TEST_STRING_MON == out3_mon)

    def test_extract_data_and_monitor_child_ws(self):
        # Check when there is no special ending
        self.do_test_extraction(TEST_STRING_DATA, TEST_STRING_MON, TEST_STRING_DATA, TEST_STRING_MON)
        # Check when there is a _1 ending
        self.do_test_extraction(TEST_STRING_DATA1, TEST_STRING_MON1, TEST_STRING_DATA, TEST_STRING_MON)
        # Check when there is a _2 ending
        self.do_test_extraction(TEST_STRING_DATA2, TEST_STRING_MON2, TEST_STRING_DATA, TEST_STRING_MON)
        # Check when there is a multiple ending
        self.do_test_extraction(TEST_STRING_DATA3, TEST_STRING_MON3, TEST_STRING_DATA, TEST_STRING_MON)


if __name__ == "__main__":
    unittest.main()
