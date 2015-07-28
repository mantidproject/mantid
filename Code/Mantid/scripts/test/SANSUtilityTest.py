
import unittest
# Need to import mantid before we import SANSUtility
import mantid
from mantid.simpleapi import *
from mantid.api import mtd, WorkspaceGroup
from mantid.kernel import DateAndTime, time_duration, FloatTimeSeriesProperty,BoolTimeSeriesProperty,StringTimeSeriesProperty,StringPropertyWithValue
import SANSUtility as su
import re
import random

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
    
def addSampleLogEntry(log_name, ws, start_time, extra_time_shift):
    number_of_times = 10
    for i in range(0, number_of_times):

        val = random.randrange(0, 10, 1)
        date = DateAndTime(start_time)
        date +=  int(i*1e9)
        date += int(extra_time_shift*1e9)
        AddTimeSeriesLog(ws, Name=log_name, Time=date.__str__().strip(), Value=val)

def provide_event_ws_with_entries(name, start_time,number_events =0, extra_time_shift = 0.0, proton_charge = True, proton_charge_empty = False):
     # Create the event workspace
    ws = CreateSampleWorkspace(WorkspaceType= 'Event', NumEvents = number_events, OutputWorkspace = name)

    # Add the proton_charge log entries
    if proton_charge:
        if proton_charge_empty:
            addSampleLogEntry('proton_charge', ws, start_time, extra_time_shift)
        else:
            addSampleLogEntry('proton_charge', ws, start_time, extra_time_shift)

    # Add some other time series log entry
    addSampleLogEntry('time_series_2', ws, start_time, extra_time_shift)
    return ws

def provide_event_ws_custom(name, start_time, extra_time_shift = 0.0, proton_charge = True, proton_charge_empty = False):
    return provide_event_ws_with_entries(name=name, start_time=start_time,number_events = 100, extra_time_shift = extra_time_shift, proton_charge=proton_charge, proton_charge_empty=proton_charge_empty)

def provide_event_ws(name, start_time, extra_time_shift):
    return provide_event_ws_custom(name = name, start_time = start_time, extra_time_shift = extra_time_shift,  proton_charge = True)

def provide_event_ws_wo_proton_charge(name, start_time, extra_time_shift):
    return provide_event_ws_custom(name = name, start_time = start_time, extra_time_shift = extra_time_shift,  proton_charge = False)

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





class AddOperationTest(unittest.TestCase):
    def compare_added_workspaces(self,ws1, ws2, out_ws, start_time1, start_time2, extra_time_shift, isOverlay):
        self._compare_added_logs(ws1, ws2, out_ws, start_time1, start_time2, extra_time_shift, isOverlay)
        # Could compare events here, but trusting add algorithm for now

    def _compare_added_logs(self, ws1, ws2, out_ws,time1, time2, extra_time_shift, isOverlay):
        run1 = ws1.getRun()
        run2 = ws2.getRun()
        out_run = out_ws.getRun()
        props_out = out_run.getProperties()

        # Check that all times of workspace1 and workspace2 can be found in the outputworkspace
        for prop in props_out:
            if isinstance(prop, FloatTimeSeriesProperty) or isinstance(prop, BoolTimeSeriesProperty) or isinstance(prop, StringTimeSeriesProperty):
                prop1 = run1.getProperty(prop.name)
                prop2 = run2.getProperty(prop.name)
                self._compare_time_series(prop, prop1, prop2, time1, time2, extra_time_shift, isOverlay)
            elif isinstance(prop, StringPropertyWithValue):
                pass

    def _compare_time_series(self, prop_out, prop_in1, prop_in2, time1, time2, extra_time_shift, isOverlay):
        times_out = prop_out.times
        times1 = prop_in1.times
        times2 = prop_in2.times

        # Total time differnce is TIME1 - (TIME2 + extraShift)
        shift = 0.0
        if isOverlay:
            shift = time_duration.total_nanoseconds(DateAndTime(time1)- DateAndTime(time2))/1e9 - extra_time_shift

        # Check ws1 against output
        # We shift the second workspace onto the first workspace
        shift_ws1 = 0
        self._assert_times(times1, times_out, shift_ws1)
        # Check ws2 against output
        self._assert_times(times2, times_out, shift)
        # Check overlapping times
        self._compare_overlapping_times(prop_in1, prop_out, times1, times2, times_out, shift)

    def _assert_times(self, times_in, times_out, shift):
        for time in times_in:
            # Add the shift in nanaoseconds to the DateAndTime object ( the plus operator is defined
            # for nanoseconds)
            converted_time = time + int(shift*1e9)
            self.assertTrue(converted_time in times_out)

    def _compare_overlapping_times(self, prop_in1, prop_out, times1, times2, times_out, shift):
        overlap_times= []
        for time in times1:
            if time in times2:
                overlap_times.append(time)
        # Now go through all those overlap times and check that the value of the 
        # first workspace is recorded in the output
        for overlap_time in overlap_times:
            times1_list = list(times1)
            timesout_list = list(times_out)
            index_in1 = times1_list.index(overlap_time)
            index_out = timesout_list.index(overlap_time)
            value_in1 = prop_in1.nthValue(index_in1)
            value_out = prop_out.nthValue(index_out)
            self.assertTrue(value_in1 == value_out)

    def test_two_files_are_added_correctly_for_overlay_on(self):
        isOverlay = True
        names =['ws1', 'ws2']
        out_ws_name = 'out_ws'
        # Create event ws1
        start_time_1 = "2010-01-01T00:00:00"
        ws1 = provide_event_ws_with_entries(names[0],start_time_1, extra_time_shift = 0.0)
        # Create event ws2
        start_time_2 = "2012-01-01T00:10:00"
        ws2 = provide_event_ws(names[1],start_time_2, extra_time_shift = 0.0)
        # Create adder
        adder = su.AddOperation(isOverlay, '')
        # Act
        adder.add(ws1, ws2,out_ws_name, 0)
        out_ws = mtd[out_ws_name]
        # Assert
        self.compare_added_workspaces(ws1, ws2, out_ws, start_time_1, start_time_2, extra_time_shift = 0.0, isOverlay = isOverlay)

    def test_two_files_are_added_correctly_for_overlay_on_and_inverted_times(self):
        isOverlay = True
        names =['ws1', 'ws2']
        out_ws_name = 'out_ws'
        # Create event ws1
        start_time_1 = "2012-01-01T00:10:00"
        ws1 = provide_event_ws_with_entries(names[0],start_time_1, extra_time_shift = 0.0)
        # Create event ws2
        start_time_2 = "2010-01-01T00:00:00"
        ws2 = provide_event_ws(names[1],start_time_2, extra_time_shift = 0.0)
        # Create adder
        adder = su.AddOperation(isOverlay, '')
        # Act
        adder.add(ws1, ws2,out_ws_name, 0)
        out_ws = mtd[out_ws_name]
        # Assert
        self.compare_added_workspaces(ws1, ws2, out_ws, start_time_1, start_time_2, extra_time_shift = 0.0, isOverlay = isOverlay)

    def test_two_files_are_added_correctly_for_overlay_off(self):
        isOverlay = False
        names =['ws1', 'ws2']
        out_ws_name = 'out_ws'
        # Create event ws1
        start_time_1 = "2010-01-01T00:00:00"
        ws1 = provide_event_ws_with_entries(names[0],start_time_1, extra_time_shift = 0.0)
        # Create event ws2
        start_time_2 = "2012-01-01T01:00:00"
        ws2 = provide_event_ws(names[1],start_time_2, extra_time_shift = 0.0)
        # Create adder
        adder = su.AddOperation(False, '')
        # Act
        adder.add(ws1, ws2,out_ws_name, 0)
        out_ws = mtd[out_ws_name]
        # Assert
        self.compare_added_workspaces(ws1, ws2, out_ws, start_time_1, start_time_2, extra_time_shift = 0.0, isOverlay = isOverlay)

    def test_two_files_are_added_correctly_with_time_shift(self):
        isOverlay = True
        names =['ws1', 'ws2']
        out_ws_name = 'out_ws'
        time_shift = 100
        # Create event ws1
        start_time_1 = "2010-01-01T00:00:00"
        ws1 = provide_event_ws_with_entries(names[0],start_time_1, extra_time_shift = 0.0)
        # Create event ws2
        start_time_2 = "2012-01-01T01:10:00"
        ws2 = provide_event_ws(names[1],start_time_2, extra_time_shift = time_shift )
        # Create adder
        adder = su.AddOperation(True, str(time_shift))
        # Act
        adder.add(ws1, ws2,out_ws_name, 0)
        out_ws = mtd[out_ws_name]
        # Assert
        self.compare_added_workspaces(ws1, ws2, out_ws, start_time_1, start_time_2, extra_time_shift = time_shift, isOverlay = isOverlay)


    def test_multiple_files_are_overlayed_correctly(self):
        isOverlay = True
        names =['ws1', 'ws2', 'ws3']
        out_ws_name = 'out_ws'
        out_ws_name2 = 'out_ws2'
        # Create event ws1
        start_time_1 = "2010-01-01T00:00:00"
        ws1 = provide_event_ws_with_entries(names[0],start_time_1, extra_time_shift = 0.0)
        # Create event ws2
        start_time_2 = "2012-01-01T00:00:00"
        ws2 = provide_event_ws(names[1],start_time_2, extra_time_shift = 0.0)
        # Create event ws3
        start_time_3 = "2013-01-01T00:00:00"
        ws3 = provide_event_ws(names[2],start_time_3, extra_time_shift = 0.0)
        # Create adder
        adder = su.AddOperation(True, '')
        # Act
        adder.add(ws1, ws2,out_ws_name, 0)
        adder.add(out_ws_name, ws3, out_ws_name2, 0)
        out_ws2 = mtd[out_ws_name2]
        out_ws = mtd[out_ws_name]
        # Assert
        self.compare_added_workspaces(out_ws, ws2, out_ws2, start_time_1, start_time_2, extra_time_shift = 0.0, isOverlay = isOverlay)

class TestCombineWorkspacesFactory(unittest.TestCase):
    def test_that_factory_returns_overlay_class(self):
        factory = su.CombineWorkspacesFactory()
        alg = factory.create_add_algorithm(True)
        self.assertTrue(isinstance(alg, su.OverlayWorkspaces))

    def test_that_factory_returns_overlay_class(self):
        factory = su.CombineWorkspacesFactory()
        alg = factory.create_add_algorithm(False)
        self.assertTrue(isinstance(alg, su.PlusWorkspaces))

class TestOverlayWorkspaces(unittest.TestCase):
    def test_time_from_proton_charge_log_is_recovered(self):
        # Arrange
        names =['ws1', 'ws2']
        out_ws_name = 'out_ws'

        start_time_1 = "2010-01-01T00:00:00"
        event_ws_1 = provide_event_ws(names[0],start_time_1, extra_time_shift = 0.0)

        start_time_2 = "2012-01-01T00:00:00"
        event_ws_2 = provide_event_ws(names[1],start_time_2, extra_time_shift = 0.0)

        # Act
        overlayWorkspaces = su.OverlayWorkspaces()
        time_difference = overlayWorkspaces._extract_time_difference_in_seconds(event_ws_1, event_ws_2)

        # Assert
        expected_time_difference = time_duration.total_nanoseconds(DateAndTime(start_time_1)- DateAndTime(start_time_2))/1e9
        self.assertEqual(time_difference, expected_time_difference)

        # Clean up 
        self._clean_up(names)
        self._clean_up(out_ws_name)

    def test_that_time_difference_adds_correct_optional_shift(self):
         # Arrange
        names =['ws1', 'ws2']
        out_ws_name = 'out_ws'

        start_time_1 = "2010-01-01T00:00:00"
        event_ws_1 = provide_event_ws(names[0],start_time_1, extra_time_shift = 0.0)

        # Extra shift in seconds
        optional_time_shift = 1000
        start_time_2 = "2012-01-01T00:00:00"
        event_ws_2 = provide_event_ws(names[1],start_time_2, extra_time_shift = optional_time_shift)

        # Act
        overlayWorkspaces = su.OverlayWorkspaces()
        time_difference = overlayWorkspaces._extract_time_difference_in_seconds(event_ws_1, event_ws_2)

        # Assert
        expected_time_difference = time_duration.total_nanoseconds(DateAndTime(start_time_1)- DateAndTime(start_time_2))/1e9
        expected_time_difference -= optional_time_shift # Need to subtract as we add the time shift to the subtrahend
        self.assertEqual(time_difference, expected_time_difference)

        # Clean up 
        self._clean_up(names)
        self._clean_up(out_ws_name)

    def test_error_is_raised_if_proton_charge_is_missing(self):
        # Arrange
        names =['ws1', 'ws2']
        out_ws_name = 'out_ws'

        start_time_1 = "2010-01-01T00:00:00"
        event_ws_1 = provide_event_ws_custom(name = names[0], start_time = start_time_1, extra_time_shift = 0.0, proton_charge = False)

        # Extra shift in seconds
        start_time_2 = "2012-01-01T00:00:00"
        event_ws_2 = provide_event_ws_custom(name = names[1], start_time = start_time_2, extra_time_shift = 0.0,proton_charge = False)

        # Act and Assert
        overlayWorkspaces = su.OverlayWorkspaces()
        args=[event_ws_1, event_ws_2]
        kwargs = {}
        self.assertRaises(RuntimeError, overlayWorkspaces._extract_time_difference_in_seconds, *args, **kwargs)

        # Clean up 
        self._clean_up(names)
        self._clean_up(out_ws_name)

    def test_correct_time_difference_is_extracted(self):
        pass
    def test_workspaces_are_normalized_by_proton_charge(self):
        pass

    def _clean_up(self,names):
        for name in names:
            if name in mtd.getObjectNames():
                DeleteWorkspace(name)

class TestTimeShifter(unittest.TestCase):
    def test_zero_shift_when_out_of_range(self):
        # Arrange
        time_shifts = ['12', '333.6', '-232']
        time_shifter = su.TimeShifter(time_shifts)

        # Act and Assert
        self.assertEqual(time_shifter.get_Nth_time_shift(0), 12.0)
        self.assertEqual(time_shifter.get_Nth_time_shift(1), 333.6)
        self.assertEqual(time_shifter.get_Nth_time_shift(2), -232.0)
        self.assertEqual(time_shifter.get_Nth_time_shift(3), 0.0)

    def test_zero_shift_when_bad_cast(self):
        # Arrange
        time_shifts = ['12', '33a.6', '-232']
        time_shifter = su.TimeShifter(time_shifts)

        # Act and Assert
        self.assertEqual(time_shifter.get_Nth_time_shift(0), 12.0)
        self.assertEqual(time_shifter.get_Nth_time_shift(1), 0.0)
        self.assertEqual(time_shifter.get_Nth_time_shift(2), -232.0)
        self.assertEqual(time_shifter.get_Nth_time_shift(3), 0.0)

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


class TestRenameMonitorsForMultiPeriodEventData(unittest.TestCase):
    monitor_appendix="_monitors"
    def test_monitors_are_renamed_correctly(self):
        #Arrange
        ws_1 = CreateSampleWorkspace()
        ws_2 = CreateSampleWorkspace()
        ws_3 = CreateSampleWorkspace()

        ws_mon_1 = CreateSampleWorkspace()
        ws_mon_2 = CreateSampleWorkspace()
        ws_mon_3 = CreateSampleWorkspace()

        ws_group = GroupWorkspaces(InputWorkspaces=[ws_1, ws_2, ws_3])
        ws_mon_group = GroupWorkspaces(InputWorkspaces=[ws_mon_1, ws_mon_2, ws_mon_3])

        # Act
        su.rename_monitors_for_multiperiod_event_data(ws_mon_group, ws_group, self.monitor_appendix)

        # Assert
        self.assertTrue(ws_mon_1.name() == ws_1.name() + self.monitor_appendix, "Monitors should be renamed to xxxx_monitors")
        self.assertTrue(ws_mon_2.name() == ws_2.name() + self.monitor_appendix, "Monitors should be renamed to xxxx_monitors")
        self.assertTrue(ws_mon_3.name() == ws_3.name() + self.monitor_appendix, "Monitors should be renamed to xxxx_monitors")

        # Clean up
        for element in mtd.getObjectNames():
            if element in mtd:
                DeleteWorkspace(element)

    def test_expection_is_raised_when_workspaec_and_monitor_mismatch(self):
        #Arrange
        ws_1 = CreateSampleWorkspace()
        ws_2 = CreateSampleWorkspace()
        ws_3 = CreateSampleWorkspace()

        ws_mon_1 = CreateSampleWorkspace()
        ws_mon_2 = CreateSampleWorkspace()

        ws_group = GroupWorkspaces(InputWorkspaces=[ws_1, ws_2, ws_3])
        ws_mon_group = GroupWorkspaces(InputWorkspaces=[ws_mon_1, ws_mon_2])

        # Act + Assert
        args = {'monitor_worksapce': ws_mon_group, 'workspace':ws_group, 'appendix':self.monitor_appendix}
        self.assertRaises(RuntimeError, su.rename_monitors_for_multiperiod_event_data, *args)

        # Clean up
        for element in mtd.getObjectNames():
            if element in mtd:
                DeleteWorkspace(element)

class TestConvertibleToInteger(unittest.TestCase):
    def test_converts_true_to_integer_when_integer(self):
        # Arrange
        input = 3
        # Act
        result = su.is_convertible_to_int(input)
        # Assert
        self.assertTrue(result)

    def test_converts_true_to_integer_when_convertible_string(self):
        # Arrange
        input = '34'
        # Act
        result = su.is_convertible_to_int(input)
        # Assert
        self.assertTrue(result)

    def test__converts_false_to_integer_when_non_convertible_string(self):
        # Arrange
        input = '34_gt'
        # Act
        result = su.is_convertible_to_int(input)
        # Assert
        self.assertFalse(result)

class TestConvertibleToFloat(unittest.TestCase):
    def test_converts_true_to_float_when_float(self):
        # Arrange
        input = 3.8
        # Act
        result = su.is_convertible_to_float(input)
        # Assert
        self.assertTrue(result)

    def test_convertible_true_to_float_when_convertible_string(self):
        # Arrange
        input = "4.78"
        # Act
        result = su.is_convertible_to_float(input)
        # Assert
        self.assertTrue(result)

    def test_converts_false_to_float_when_convertible_string(self):
        # Arrange
        input = "4.78_tg"
        # Act
        result = su.is_convertible_to_float(input)
        # Assert
        self.assertFalse(result)

class TestValidXmlFileList(unittest.TestCase):
    def test_finds_valid_xml_file_list(self):
        # Arrange
        input = ["test1.xml", "test2.xml", "test3.xml"]
        # Act
        result =su.is_valid_xml_file_list(input)
        # Assert
        self.assertTrue(result)

    def test_finds_invalid_xml_file_list(self):
        # Arrange
        input = ["test1.xml", "test2.ccl", "test3.xml"]
        # Act
        result =su.is_valid_xml_file_list(input)
        # Assert
        self.assertFalse(result)

    def test_finds_empty_list(self):
        # Arrange
        input = []
        # Act
        result = su.is_valid_xml_file_list(input)
        # Assert
        self.assertFalse(result)

class TestConvertToAndFromPythonStringList(unittest.TestCase):
    def test_converts_from_string_to_list(self):
        # Arrange
        input = "test1.xml, test2.xml, test3.xml"
        # Act
        result = su.convert_to_string_list(input)
        # Assert
        expected = "['test1.xml','test2.xml','test3.xml']"
        self.assertEqual(expected, result)
    def test_converts_from_list_to_string(self):
        # Arrange
        input = ["test1.xml", "test2.xml", "test3.xml"]
        # Act
        result = su.convert_from_string_list(input)
        # Assert
        expected = "test1.xml,test2.xml,test3.xml"
        self.assertEqual(expected, result)

if __name__ == "__main__":
    unittest.main()
