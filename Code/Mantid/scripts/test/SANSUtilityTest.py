
import unittest
# Need to import mantid before we import SANSUtility
import mantid
from mantid.simpleapi import *
from mantid.api import mtd, WorkspaceGroup
import SANSUtility as su
import re

TEST_STRING_DATA = 'SANS2D0003434-add' + su.ADDED_EVENT_DATA_TAG
TEST_STRING_MON = 'SANS2D0003434-add_monitors' + su.ADDED_EVENT_DATA_TAG

TEST_STRING_DATA1 = TEST_STRING_DATA + '_1'
TEST_STRING_MON1 = TEST_STRING_MON + '_1'

TEST_STRING_DATA2 = TEST_STRING_DATA + '_2'
TEST_STRING_MON2 = TEST_STRING_MON + '_2'

TEST_STRING_DATA3 = TEST_STRING_DATA + '_3'
TEST_STRING_MON3 = TEST_STRING_MON + '_3'

def provide_group_workspace_for_added_event_data(event_ws_name, monitor_ws_name, out_ws_name):
    CreateWorkspace(DataX = [1,2,3], DataY = [2,3,4], OutputWorkspace = monitor_ws_name)
    CreateSampleWorkspace(WorkspaceType= 'Event', OutputWorkspace = event_ws_name)
    GroupWorkspaces(InputWorkspaces = [event_ws_name, monitor_ws_name ], OutputWorkspace = out_ws_name)
    
# This test does not pass and was not used before 1/4/2015. SansUtilitytests was disabled.

class SANSUtilityTest(unittest.TestCase):

    #def checkValues(self, list1, list2):

    #    def _check_single_values( v1, v2):
    #        self.assertAlmostEqual(v1, v2)

    #    self.assertEqual(len(list1), len(list2))
    #    for v1,v2 in zip(list1, list2):
    #        start_1,stop_1 = v1
    #        start_2, stop_2 = v2
    #        _check_single_values(start_1, start_2)
    #        _check_single_values(stop_1, stop_2)

    #def test_checkValues(self):
    #    """sanity check to ensure that the others will work correctly"""
    #    values = [
    #        [[1,2],],
    #        [[None, 3],[4, None]],
    #    ]
    #    for singlevalues in values:
    #        self.checkValues(singlevalues, singlevalues)
    
    #def test_parse_strings(self):
    #    inputs = { '1-2':[[1,2]],         # single period syntax  min < x < max
    #               '1.3-5.6':[[1.3,5.6]], # float
    #               '1-2,3-4':[[1,2],[3,4]],# more than one slice
    #               '>1':[[1, -1]],       # just lower bound
    #               '<5':[[-1, 5]],      # just upper bound
    #               '<5,8-9': [[-1, 5], [8,9]],
    #               '1:2:5': [[1,3], [3,5]] # sintax: start, step, stop                   
    #        }

    #    for (k, v) in inputs.items(): 
    #        self.checkValues(su.sliceParser(k),v)

    #def test_accept_spaces(self):
    #    self.checkValues(su.sliceParser("1 - 2, 3 - 4"), [[1,2],[3,4]])
        
    #def test_invalid_values_raise(self):
    #    invalid_strs = ["5>6", ":3:", "MAX<min"]
    #    for val in invalid_strs:
    #        self.assertRaises(SyntaxError, su.sliceParser, val)

    #def test_empty_string_is_valid(self):
    #    self.checkValues(su.sliceParser(""), [[-1,-1]])

    def test_extract_spectra(self):
        mtd.clear()

        ws = CreateSampleWorkspace("Histogram", "Multiple Peaks")
        det_ids = [100, 102, 104]

        result = su.extract_spectra(ws, det_ids, "result")

        # Essentially, do we end up with our original workspace and the resulting
        # workspace in the ADS, and NOTHING else?
        self.assertTrue("result" in mtd)
        self.assertTrue("ws" in mtd)
        self.assertEquals(2, len(mtd))

        self.assertEquals(result.getNumberHistograms(), len(det_ids))
        self.assertEquals(result.getDetector(0).getID(), 100)
        self.assertEquals(result.getDetector(1).getID(), 102)
        self.assertEquals(result.getDetector(2).getID(), 104)

        ws = CreateSampleWorkspace("Histogram", "Multiple Peaks")
        det_ids = range(100, 299, 2)
        result = su.extract_spectra(ws, det_ids, "result")

    def test_get_masked_det_ids(self):
        ws = CreateSampleWorkspace("Histogram", "Multiple Peaks")

        MaskDetectors(Workspace=ws, DetectorList=[100, 102, 104])

        masked_det_ids = list(su.get_masked_det_ids(ws))

        self.assertTrue(100 in masked_det_ids)
        self.assertTrue(102 in masked_det_ids)
        self.assertTrue(104 in masked_det_ids)
        self.assertEquals(len(masked_det_ids), 3)

    def test_merge_to_ranges(self):
        self.assertEquals([[1, 4]],                 su._merge_to_ranges([1, 2, 3, 4]))
        self.assertEquals([[1, 3], [5, 7]],         su._merge_to_ranges([1, 2, 3, 5, 6, 7]))
        self.assertEquals([[1, 3], [5, 5], [7, 9]], su._merge_to_ranges([1, 2, 3, 5, 7, 8, 9]))
        self.assertEquals([[1, 1]],                 su._merge_to_ranges([1]))

class TestBundleAddedEventDataFilesToGroupWorkspaceFile(unittest.TestCase):
    def _prepare_workspaces(self, names):
        CreateSampleWorkspace(WorkspaceType = 'Event', OutputWorkspace = names[0])
        CreateWorkspace(DataX = [1,1,1], DataY = [1,1,1], OutputWorkspace = names[1])

        temp_save_dir = config['defaultsave.directory']
        if (temp_save_dir == ''):
            temp_save_dir = os.getcwd()

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
        names = ['event_data', 'monitor']
        file_names = self._prepare_workspaces(names = names)
        self._cleanup_workspaces(names = names)

        # Act
        group_ws_name = 'g_ws'
        output_group_file_name = su.bundle_added_event_data_as_group(file_names[0], file_names[1])

        Load(Filename = output_group_file_name, OutputWorkspace = group_ws_name)
        group_ws = mtd[group_ws_name]

        # Assert
        self.assertTrue(isinstance(group_ws, WorkspaceGroup))
        self.assertEqual(group_ws.size(), 2)
        self.assertTrue(os.path.exists(file_names[0])) # File for group workspace exists
        self.assertFalse(os.path.exists(file_names[1]))  # File for monitors is deleted

        # Clean up
        ws_names_to_delete = []
        for ws_name in mtd.getObjectNames():
            if ws_name != group_ws_name:
                ws_names_to_delete.append(str(ws_name))
        self._cleanup_workspaces(names = ws_names_to_delete)

        if os.path.exists(file_names[0]):
            os.remove(file_names[0])


class TestLoadingAddedEventWorkspaceNameParsing(unittest.TestCase):

    def do_test_load_check(self, event_name, monitor_name):
        out_name = 'out_ws'
        provide_group_workspace_for_added_event_data(event_ws_name = event_name, monitor_ws_name = monitor_name, out_ws_name = out_name)
        out_ws = mtd[out_name]
        self.assertTrue(su.check_child_ws_for_name_and_type_for_added_eventdata(out_ws))
        DeleteWorkspace(out_ws)

    def test_check_regex_for_data(self):
        # Check when there is no special ending
        self.assertNotEqual(None, re.search(su.REG_DATA_NAME, TEST_STRING_DATA))
        # Check when there is a _1 ending
        self.assertNotEqual(None, re.search(su.REG_DATA_NAME, TEST_STRING_DATA1))
        # Check when there is a _2 ending
        self.assertNotEqual(None, re.search(su.REG_DATA_NAME, TEST_STRING_DATA2))
        # Check when there is a multiple ending
        self.assertNotEqual(None, re.search(su.REG_DATA_NAME, TEST_STRING_DATA3))


    def test_check_regex_for_data_monitors(self):
        # Check when there is no special ending
        self.assertNotEqual(None, re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_MON))
        # Check when there is a _1 ending
        self.assertNotEqual(None, re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_MON1))
        # Check when there is a _2 ending
        self.assertNotEqual(None, re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_MON2))
        # Check when there is a multiple ending
        self.assertNotEqual(None, re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_MON3))

    def test_regexes_do_not_clash(self):
        # Check when there is no special ending
        self.assertEqual(None, re.search(su.REG_DATA_NAME, TEST_STRING_MON)) 
        self.assertEqual(None, re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_DATA))
        # Check when there is a _1 ending
        self.assertEqual(None, re.search(su.REG_DATA_NAME, TEST_STRING_MON1)) 
        self.assertEqual(None, re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_DATA1))
        # Check when there is a _2 ending
        self.assertEqual(None, re.search(su.REG_DATA_NAME, TEST_STRING_MON2)) 
        self.assertEqual(None, re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_DATA2))
        # Check when there is a multiple ending
        self.assertEqual(None, re.search(su.REG_DATA_NAME, TEST_STRING_MON3)) 
        self.assertEqual(None, re.search(su.REG_DATA_MONITORS_NAME, TEST_STRING_DATA3))
    
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
    _appendix = '_monitors'

    def do_test_extraction(self, event_name, monitor_name):
        out_ws_name = 'out_group'
        event_name_expect = out_ws_name
        monitor_name_expect = out_ws_name + self._appendix

        provide_group_workspace_for_added_event_data(event_ws_name = event_name, monitor_ws_name = monitor_name, out_ws_name = out_ws_name)
        out_ws_group = mtd[out_ws_name]

        # Act
        su.extract_child_ws_for_added_eventdata(out_ws_group, self._appendix)

        # Assert
        self.assertTrue(event_name_expect in mtd)
        self.assertTrue(monitor_name_expect in mtd)

        DeleteWorkspace(event_name_expect)
        DeleteWorkspace(monitor_name_expect)


    def test_extract_data_and_monitor_child_ws(self):
        # Check when there is no special ending
        self.do_test_extraction(TEST_STRING_DATA, TEST_STRING_MON)




class TestZeroErrorFreeWorkspace(unittest.TestCase):
    def _setup_workspace(self, name, type):
        ws = CreateSampleWorkspace(OutputWorkspace = name, WorkspaceType=type, Function='One Peak',NumBanks=1,BankPixelWidth=2,NumEvents=0,XMin=0.5,XMax=1,BinWidth=1,PixelSpacing=1,BankDistanceFromSample=1)
        if type == 'Histogram':
            errors = ws.dataE
            # For first and third spectra set to 0.0
            errors(0)[0] = 0.0
            errors(2)[0] = 0.0

    def _removeWorkspace(self, name):
        if name in mtd:
            mtd.remove(name)

    def test_that_non_existent_ws_creates_error_message(self):
        # Arrange
        ws_name = 'original'
        ws_clone_name = 'clone'
        # Act
        message, complete = su.create_zero_error_free_workspace(input_workspace_name = ws_name, output_workspace_name = ws_clone_name)
        # Assert
        message.strip()
        self.assertTrue(message)
        self.assertTrue(not complete)

    def test_that_bad_zero_error_removal_creates_error_message(self):
        # Arrange
        ws_name = 'original'
        ws_clone_name = 'clone'
        self._setup_workspace(ws_name, 'Event')
        # Act
        message, complete= su.create_zero_error_free_workspace(input_workspace_name = ws_name, output_workspace_name = ws_clone_name)
        # Assert
        message.strip()
        self.assertTrue(message)
        self.assertTrue(not ws_clone_name in mtd)
        self.assertTrue(not complete)

        self._removeWorkspace(ws_name)
        self.assertTrue(not ws_name in mtd)

    def test_that_zeros_are_removed_correctly(self):
        # Arrange
        ws_name = 'original'
        ws_clone_name = 'clone'
        self._setup_workspace(ws_name, 'Histogram')
        # Act
        message, complete = su.create_zero_error_free_workspace(input_workspace_name = ws_name, output_workspace_name = ws_clone_name)
        # Assert
        message.strip()
        print message
       # self.assertTrue(not message)
        #self.assertTrue(complete)
        self.assertTrue(mtd[ws_name] != mtd[ws_clone_name])

        self._removeWorkspace(ws_name)
        self._removeWorkspace(ws_clone_name)
        self.assertTrue(not ws_name in mtd)
        self.assertTrue(not ws_clone_name in mtd)

    def test_throws_for_non_Workspace2D(self):
        # Arrange
        ws_name = 'test'
        type ='Event'
        self._setup_workspace(ws_name, type)
        ws = mtd[ws_name]

        # Act and Assert
        self.assertRaises(ValueError, su.remove_zero_errors_from_workspace, ws)

        self._removeWorkspace(ws_name)
        self.assertTrue(not ws_name in mtd)

    def test_removes_zero_errors_correctly(self):
        # Arrange
        ws_name = 'test'
        type ='Histogram'
        self._setup_workspace(ws_name, type)
        ws = mtd[ws_name]

        # Act and Assert
        errors = ws.dataE
        self.assertTrue(errors(0)[0] == 0.0)
        self.assertTrue(errors(1)[0] != 0.0)
        self.assertTrue(errors(2)[0] == 0.0)
        self.assertTrue(errors(3)[0] != 0.0)

        su.remove_zero_errors_from_workspace(ws)

        self.assertTrue(errors(0)[0] == su.ZERO_ERROR_DEFAULT)
        self.assertTrue(errors(1)[0] != 0.0)
        self.assertTrue(errors(1)[0] != su.ZERO_ERROR_DEFAULT)
        self.assertTrue(errors(2)[0] == su.ZERO_ERROR_DEFAULT)
        self.assertTrue(errors(3)[0] != 0.0)
        self.assertTrue(errors(3)[0] != su.ZERO_ERROR_DEFAULT)

        self._removeWorkspace(ws_name)
        self.assertTrue(not ws_name in mtd)

    def test_that_deletion_of_non_existent_ws_creates_error_message(self):
        # Arrange
        ws_name = 'ws'
        # Act
        message, complete = su.delete_zero_error_free_workspace(input_workspace_name = ws_name)
        # Assert
        message.strip()
        self.assertTrue(message)
        self.assertTrue(not complete)

    def test_that_deletion_of_extent_ws_is_successful(self):
        # Arrange
        ws_name = 'ws'
        self._setup_workspace(ws_name, 'Histogram')
        # Act + Assert
        self.assertTrue(ws_name in mtd)
        message, complete = su.delete_zero_error_free_workspace(input_workspace_name = ws_name)
        message.strip()
        self.assertTrue(not message)
        self.assertTrue(complete)
        self.assertTrue(not ws_name in mtd)

    def test_non_Q1D_and_Qxy_history_is_not_valid_and_produces_error_message(self):
        # Arrange
        ws_name = 'ws'
        self._setup_workspace(ws_name, 'Histogram')
        # Act
        message, complete = su.is_valid_ws_for_removing_zero_errors(input_workspace_name = ws_name)
        # Assert
        message.strip()
        self.assertTrue(message)
        self.assertTrue(not complete)

        self._removeWorkspace(ws_name)
        self.assertTrue(not ws_name in mtd)

if __name__ == "__main__":
    unittest.main()
