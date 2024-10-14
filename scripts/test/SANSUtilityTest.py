# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import random
import re
import unittest

import numpy as np

import SANSUtility as su
from mantid.api import mtd, WorkspaceGroup, AlgorithmManager, AnalysisDataService, FileFinder
from mantid.kernel import (
    DateAndTime,
    time_duration,
    FloatTimeSeriesProperty,
    BoolTimeSeriesProperty,
    StringTimeSeriesProperty,
    StringPropertyWithValue,
    V3D,
    Quat,
)

# Need to import mantid before we import SANSUtility
from mantid.simpleapi import (
    config,
    AddTimeSeriesLog,
    CloneWorkspace,
    CreateSampleWorkspace,
    CreateWorkspace,
    DeleteWorkspace,
    GroupWorkspaces,
    Load,
    MaskDetectors,
    SaveNexusProcessed,
)

TEST_STRING_DATA = "SANS2D0003434-add" + su.ADDED_EVENT_DATA_TAG
TEST_STRING_MON = "SANS2D0003434-add_monitors" + su.ADDED_EVENT_DATA_TAG

TEST_STRING_DATA1 = TEST_STRING_DATA + "_1"
TEST_STRING_MON1 = TEST_STRING_MON + "_1"

TEST_STRING_DATA2 = TEST_STRING_DATA + "_2"
TEST_STRING_MON2 = TEST_STRING_MON + "_2"

TEST_STRING_DATA3 = TEST_STRING_DATA + "_3"
TEST_STRING_MON3 = TEST_STRING_MON + "_3"


def provide_group_workspace_for_added_event_data(event_ws_name, monitor_ws_name, out_ws_name):
    CreateWorkspace(DataX=[1, 2, 3], DataY=[2, 3, 4], OutputWorkspace=monitor_ws_name)
    CreateSampleWorkspace(WorkspaceType="Event", OutputWorkspace=event_ws_name)
    GroupWorkspaces(InputWorkspaces=[event_ws_name, monitor_ws_name], OutputWorkspace=out_ws_name)


def addSampleLogEntry(log_name, ws, start_time, extra_time_shift, make_linear=False):
    number_of_times = 10
    for i in range(0, number_of_times):
        if make_linear:
            val = float(i)
        else:
            val = random.randrange(0, 10, 1)
        date = DateAndTime(start_time)
        date += int(i * 1e9)
        date += int(extra_time_shift * 1e9)
        AddTimeSeriesLog(ws, Name=log_name, Time=date.__str__().strip(), Value=val)


def provide_event_ws_with_entries(
    name,
    start_time,
    number_events=0,
    extra_time_shift=0.0,
    proton_charge=True,
    proton_charge_empty=False,
    log_names=None,
    make_linear=False,
):
    # Create the event workspace
    ws = CreateSampleWorkspace(WorkspaceType="Event", NumEvents=number_events, OutputWorkspace=name)

    # Add the proton_charge log entries
    if proton_charge:
        if proton_charge_empty:
            addSampleLogEntry("proton_charge", ws, start_time, extra_time_shift)
        else:
            addSampleLogEntry("proton_charge", ws, start_time, extra_time_shift)

    # Add some other time series log entry
    if log_names is None:
        addSampleLogEntry("time_series_2", ws, start_time, extra_time_shift, make_linear)
    else:
        for name in log_names:
            addSampleLogEntry(name, ws, start_time, extra_time_shift, make_linear)
    return ws


def provide_event_ws_custom(name, start_time, extra_time_shift=0.0, proton_charge=True, proton_charge_empty=False):
    return provide_event_ws_with_entries(
        name=name,
        start_time=start_time,
        number_events=100,
        extra_time_shift=extra_time_shift,
        proton_charge=proton_charge,
        proton_charge_empty=proton_charge_empty,
    )


def provide_event_ws(name, start_time, extra_time_shift):
    return provide_event_ws_custom(name=name, start_time=start_time, extra_time_shift=extra_time_shift, proton_charge=True)


def provide_event_ws_wo_proton_charge(name, start_time, extra_time_shift):
    return provide_event_ws_custom(name=name, start_time=start_time, extra_time_shift=extra_time_shift, proton_charge=False)


def provide_histo_workspace_with_one_spectrum(ws_name, x_start, x_end, bin_width):
    CreateSampleWorkspace(OutputWorkspace=ws_name, NumBanks=1, BankPixelWidth=1, XMin=x_start, XMax=x_end, BinWidth=bin_width)


def provide_workspace_with_x_errors(
    workspace_name,
    use_xerror=True,
    nspec=1,
    x_in=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    y_in=[2, 2, 2, 2, 2, 2, 2, 2, 2],
    e_in=[1, 1, 1, 1, 1, 1, 1, 1, 1],
    x_error=[1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9],
):
    x = []
    y = []
    e = []
    for item in range(0, nspec):
        x = x + x_in
        y = y + y_in
        e = e + e_in

    CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=nspec, UnitX="MomentumTransfer", OutputWorkspace=workspace_name)
    if use_xerror:
        ws = mtd[workspace_name]
        for hists in range(0, nspec):
            x_error_array = np.asarray(x_error)
            ws.setDx(hists, x_error_array)


# This test does not pass and was not used before 1/4/2015. SansUtilitytests was disabled.
class SANSUtilityTest(unittest.TestCase):
    # def checkValues(self, list1, list2):

    #    def _check_single_values( v1, v2):
    #        self.assertAlmostEqual(v1, v2)

    #    self.assertEqual(len(list1), len(list2))
    #    for v1,v2 in zip(list1, list2):
    #        start_1,stop_1 = v1
    #        start_2, stop_2 = v2
    #        _check_single_values(start_1, start_2)
    #        _check_single_values(stop_1, stop_2)

    # def test_checkValues(self):
    #    """sanity check to ensure that the others will work correctly"""
    #    values = [
    #        [[1,2],],
    #        [[None, 3],[4, None]],
    #    ]
    #    for singlevalues in values:
    #        self.checkValues(singlevalues, singlevalues)

    # def test_parse_strings(self):
    #    inputs = { '1-2':[[1,2]],         # single period syntax  min < x < max
    #               '1.3-5.6':[[1.3,5.6]], # float
    #               '1-2,3-4':[[1,2],[3,4]],# more than one slice
    #               '>1':[[1, -1]],       # just lower bound
    #               '<5':[[-1, 5]],      # just upper bound
    #               '<5,8-9': [[-1, 5], [8,9]],
    #               '1:2:5': [[1,3], [3,5]] # syntax: start, step, stop
    #        }

    #    for (k, v) in inputs.items():
    #        self.checkValues(su.sliceParser(k),v)

    # def test_accept_spaces(self):
    #    self.checkValues(su.sliceParser("1 - 2, 3 - 4"), [[1,2],[3,4]])

    # def test_invalid_values_raise(self):
    #    invalid_strs = ["5>6", ":3:", "MAX<min"]
    #    for val in invalid_strs:
    #        self.assertRaises(SyntaxError, su.sliceParser, val)

    # def test_empty_string_is_valid(self):
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
        self.assertEqual(2, len(mtd))

        self.assertEqual(result.getNumberHistograms(), len(det_ids))
        self.assertEqual(result.getDetector(0).getID(), 100)
        self.assertEqual(result.getDetector(1).getID(), 102)
        self.assertEqual(result.getDetector(2).getID(), 104)

        ws = CreateSampleWorkspace("Histogram", "Multiple Peaks")
        det_ids = list(range(100, 299, 2))
        result = su.extract_spectra(ws, det_ids, "result")

    def test_get_masked_det_ids(self):
        ws = CreateSampleWorkspace("Histogram", "Multiple Peaks")

        MaskDetectors(Workspace=ws, DetectorList=[100, 102, 104])

        masked_det_ids = list(su.get_masked_det_ids(ws))

        self.assertTrue(100 in masked_det_ids)
        self.assertTrue(102 in masked_det_ids)
        self.assertTrue(104 in masked_det_ids)
        self.assertEqual(len(masked_det_ids), 3)

    def test_merge_to_ranges(self):
        self.assertEqual([[1, 4]], su._merge_to_ranges([1, 2, 3, 4]))
        self.assertEqual([[1, 3], [5, 7]], su._merge_to_ranges([1, 2, 3, 5, 6, 7]))
        self.assertEqual([[1, 3], [5, 5], [7, 9]], su._merge_to_ranges([1, 2, 3, 5, 7, 8, 9]))
        self.assertEqual([[1, 1]], su._merge_to_ranges([1]))


class TestBundleAddedEventDataFilesToGroupWorkspaceFile(unittest.TestCase):
    def _prepare_workspaces(self, names):
        CreateSampleWorkspace(WorkspaceType="Event", OutputWorkspace=names[0])
        CreateWorkspace(DataX=[1, 1, 1], DataY=[1, 1, 1], OutputWorkspace=names[1])

        temp_save_dir = config["defaultsave.directory"]
        if temp_save_dir == "":
            temp_save_dir = os.getcwd()

        data_file_name = os.path.join(temp_save_dir, names[0] + ".nxs")
        monitor_file_name = os.path.join(temp_save_dir, names[1] + ".nxs")

        SaveNexusProcessed(InputWorkspace=names[0], Filename=data_file_name)
        SaveNexusProcessed(InputWorkspace=names[1], Filename=monitor_file_name)

        file_names = [data_file_name, monitor_file_name]

        return file_names

    def _cleanup_workspaces(self, names):
        for name in names:
            DeleteWorkspace(name)

    def test_load_valid_added_event_data_and_monitor_file_produces_group_ws(self):
        # Arrange
        names = ["event_data", "monitor"]
        file_names = self._prepare_workspaces(names=names)
        self._cleanup_workspaces(names=names)

        # Act
        group_ws_name = "g_ws"
        output_group_file_name = su.bundle_added_event_data_as_group(file_names[0], file_names[1], False)

        Load(Filename=output_group_file_name, OutputWorkspace=group_ws_name)
        group_ws = mtd[group_ws_name]

        # Assert
        self.assertTrue(isinstance(group_ws, WorkspaceGroup))
        self.assertEqual(group_ws.size(), 2)
        self.assertTrue(os.path.exists(file_names[0]))  # File for group workspace exists
        self.assertFalse(os.path.exists(file_names[1]))  # File for monitors is deleted

        # Clean up
        ws_names_to_delete = []
        for ws_name in mtd.getObjectNames():
            if ws_name != group_ws_name:
                ws_names_to_delete.append(str(ws_name))
        self._cleanup_workspaces(names=ws_names_to_delete)

        if os.path.exists(file_names[0]):
            os.remove(file_names[0])


class TestLoadingAddedEventWorkspaceNameParsing(unittest.TestCase):
    def do_test_load_check(self, event_name, monitor_name):
        out_name = "out_ws"
        provide_group_workspace_for_added_event_data(event_ws_name=event_name, monitor_ws_name=monitor_name, out_ws_name=out_name)
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
        self.do_test_load_check(event_name=event_name, monitor_name=monitor_name)

        # Check when there is a _1 ending
        event_name1 = TEST_STRING_DATA1
        monitor_name1 = TEST_STRING_MON1
        self.do_test_load_check(event_name=event_name1, monitor_name=monitor_name1)

        # Check when there is a _2 ending
        event_name2 = TEST_STRING_DATA2
        monitor_name2 = TEST_STRING_MON2
        self.do_test_load_check(event_name=event_name2, monitor_name=monitor_name2)

        # Check when there is a multiple ending
        event_name3 = TEST_STRING_DATA3
        monitor_name3 = TEST_STRING_MON3
        self.do_test_load_check(event_name=event_name3, monitor_name=monitor_name3)


class TestLoadingAddedEventWorkspaceExtraction(unittest.TestCase):
    _appendix = "_monitors"

    def do_test_extraction(self, event_name, monitor_name):
        out_ws_name = "out_group"
        event_name_expect = out_ws_name
        monitor_name_expect = out_ws_name + self._appendix

        provide_group_workspace_for_added_event_data(event_ws_name=event_name, monitor_ws_name=monitor_name, out_ws_name=out_ws_name)
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
    def compare_added_workspaces(self, ws1, ws2, out_ws, start_time1, start_time2, extra_time_shift, isOverlay):
        self._compare_added_logs(ws1, ws2, out_ws, start_time1, start_time2, extra_time_shift, isOverlay)
        # Could compare events here, but trusting add algorithm for now

    def _compare_added_logs(self, ws1, ws2, out_ws, time1, time2, extra_time_shift, isOverlay):
        run1 = ws1.getRun()
        run2 = ws2.getRun()
        out_run = out_ws.getRun()
        props_out = out_run.getProperties()

        # Check that all times of workspace1 and workspace2 can be found in the outputworkspace
        for prop in props_out:
            if (
                isinstance(prop, FloatTimeSeriesProperty)
                or isinstance(prop, BoolTimeSeriesProperty)
                or isinstance(prop, StringTimeSeriesProperty)
            ):
                prop1 = run1.getProperty(prop.name)
                prop2 = run2.getProperty(prop.name)
                self._compare_time_series(prop, prop1, prop2, time1, time2, extra_time_shift, isOverlay)
            elif isinstance(prop, StringPropertyWithValue):
                pass

    def _compare_time_series(self, prop_out, prop_in1, prop_in2, time1, time2, extra_time_shift, isOverlay):
        times_out = prop_out.times
        times1 = prop_in1.times
        times2 = prop_in2.times

        # Total time difference is TIME1 - (TIME2 + extraShift)
        shift = 0.0
        if isOverlay:
            shift = time_duration.totalNanoseconds(DateAndTime(time1) - DateAndTime(time2)) / 1e9 - extra_time_shift

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
            converted_time = time + int(shift * 1e9)
            self.assertTrue(converted_time in times_out)

    def _compare_overlapping_times(self, prop_in1, prop_out, times1, times2, times_out, shift):
        overlap_times = []
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
            self.assertEqual(value_in1, value_out)

    def _add_single_log(self, workspace, name, value):
        alg_log = AlgorithmManager.createUnmanaged("AddSampleLog")
        alg_log.initialize()
        alg_log.setChild(True)
        alg_log.setProperty("Workspace", workspace)
        alg_log.setProperty("LogName", name)
        alg_log.setProperty("LogText", str(value))
        alg_log.setProperty("LogType", "Number")
        alg_log.execute()

    def test_two_files_are_added_correctly_for_overlay_on(self):
        isOverlay = True
        names = ["ws1", "ws2"]
        out_ws_name = "out_ws"
        # Create event ws1
        start_time_1 = "2010-01-01T00:00:00"
        proton_charge_1 = 10.2
        ws1 = provide_event_ws_with_entries(names[0], start_time_1, extra_time_shift=0.0)
        self._add_single_log(ws1, "gd_prtn_chrg", proton_charge_1)
        # Create event ws2
        start_time_2 = "2012-01-01T00:10:00"
        proton_charge_2 = 30.2
        ws2 = provide_event_ws(names[1], start_time_2, extra_time_shift=0.0)
        self._add_single_log(ws2, "gd_prtn_chrg", proton_charge_2)
        # Create adder
        adder = su.AddOperation(isOverlay, "")
        # Act
        adder.add(ws1, ws2, out_ws_name, 0)
        out_ws = mtd[out_ws_name]
        # Assert
        self.compare_added_workspaces(ws1, ws2, out_ws, start_time_1, start_time_2, extra_time_shift=0.0, isOverlay=isOverlay)

    def test_two_files_are_added_correctly_for_overlay_on_and_inverted_times(self):
        isOverlay = True
        names = ["ws1", "ws2"]
        out_ws_name = "out_ws"
        # Create event ws1
        start_time_1 = "2012-01-01T00:10:00"
        proton_charge_1 = 10.2
        ws1 = provide_event_ws_with_entries(names[0], start_time_1, extra_time_shift=0.0)
        self._add_single_log(ws1, "gd_prtn_chrg", proton_charge_1)
        # Create event ws2
        start_time_2 = "2010-01-01T00:00:00"
        proton_charge_2 = 30.2
        ws2 = provide_event_ws(names[1], start_time_2, extra_time_shift=0.0)
        self._add_single_log(ws2, "gd_prtn_chrg", proton_charge_2)
        # Create adder
        adder = su.AddOperation(isOverlay, "")
        # Act
        adder.add(ws1, ws2, out_ws_name, 0)
        out_ws = mtd[out_ws_name]
        # Assert
        self.compare_added_workspaces(ws1, ws2, out_ws, start_time_1, start_time_2, extra_time_shift=0.0, isOverlay=isOverlay)

    def test_two_files_are_added_correctly_for_overlay_off(self):
        isOverlay = False
        names = ["ws1", "ws2"]
        out_ws_name = "out_ws"
        # Create event ws1
        start_time_1 = "2010-01-01T00:00:00"
        proton_charge_1 = 10.2
        ws1 = provide_event_ws_with_entries(names[0], start_time_1, extra_time_shift=0.0)
        self._add_single_log(ws1, "gd_prtn_chrg", proton_charge_1)
        # Create event ws2
        start_time_2 = "2012-01-01T01:00:00"
        proton_charge_2 = 30.2
        ws2 = provide_event_ws(names[1], start_time_2, extra_time_shift=0.0)
        self._add_single_log(ws2, "gd_prtn_chrg", proton_charge_2)
        # Create adder
        adder = su.AddOperation(False, "")
        # Act
        adder.add(ws1, ws2, out_ws_name, 0)
        out_ws = mtd[out_ws_name]
        # Assert
        self.compare_added_workspaces(ws1, ws2, out_ws, start_time_1, start_time_2, extra_time_shift=0.0, isOverlay=isOverlay)

    def test_two_files_are_added_correctly_with_time_shift(self):
        isOverlay = True
        names = ["ws1", "ws2"]
        out_ws_name = "out_ws"
        time_shift = 100
        # Create event ws1
        start_time_1 = "2010-01-01T00:00:00"
        proton_charge_1 = 10.2
        ws1 = provide_event_ws_with_entries(names[0], start_time_1, extra_time_shift=0.0)
        self._add_single_log(ws1, "gd_prtn_chrg", proton_charge_1)
        # Create event ws2
        start_time_2 = "2012-01-01T01:10:00"
        proton_charge_2 = 30.2
        ws2 = provide_event_ws(names[1], start_time_2, extra_time_shift=time_shift)
        self._add_single_log(ws2, "gd_prtn_chrg", proton_charge_2)
        # Create adder
        adder = su.AddOperation(True, str(time_shift))
        # Act
        adder.add(ws1, ws2, out_ws_name, 0)
        out_ws = mtd[out_ws_name]
        # Assert
        self.compare_added_workspaces(ws1, ws2, out_ws, start_time_1, start_time_2, extra_time_shift=time_shift, isOverlay=isOverlay)

    def test_multiple_files_are_overlayed_correctly(self):
        isOverlay = True
        names = ["ws1", "ws2", "ws3"]
        out_ws_name = "out_ws"
        out_ws_name2 = "out_ws2"
        # Create event ws1
        start_time_1 = "2010-01-01T00:00:00"
        proton_charge_1 = 10.2
        ws1 = provide_event_ws_with_entries(names[0], start_time_1, extra_time_shift=0.0)
        self._add_single_log(ws1, "gd_prtn_chrg", proton_charge_1)
        # Create event ws2
        start_time_2 = "2012-01-01T00:00:00"
        proton_charge_2 = 30.2
        ws2 = provide_event_ws(names[1], start_time_2, extra_time_shift=0.0)
        self._add_single_log(ws2, "gd_prtn_chrg", proton_charge_2)
        # Create event ws3
        start_time_3 = "2013-01-01T00:00:00"
        proton_charge_3 = 20.2
        ws3 = provide_event_ws(names[2], start_time_3, extra_time_shift=0.0)
        self._add_single_log(ws3, "gd_prtn_chrg", proton_charge_3)
        # Create adder
        adder = su.AddOperation(True, "")
        # Act
        adder.add(ws1, ws2, out_ws_name, 0)
        adder.add(out_ws_name, ws3, out_ws_name2, 0)
        out_ws2 = mtd[out_ws_name2]
        out_ws = mtd[out_ws_name]
        # Assert
        self.compare_added_workspaces(out_ws, ws2, out_ws2, start_time_1, start_time_2, extra_time_shift=0.0, isOverlay=isOverlay)

    def test_that_bad_proton_charges_are_estimated_correctly(self):
        ws = self._create_workspace_with_bad_logs()

        # Clean the logs and estimate values - has a single bad value at the start
        pc = ws.getRun().getProperty("proton_charge")
        self.assertEqual(len(pc.times), 3)
        ws = su._clean_logs(ws, True)

        # Get proton charge logs after operation
        pc = ws.getRun().getProperty("proton_charge")
        # one value got added to the end
        self.assertEqual(len(pc.times), 4)
        # duration should not be hours rather than years
        stats = ws.getRun().getStatistics("proton_charge")
        self.assertEqual(stats.duration / 3600, 3.0)  # hours
        self.assertEqual(stats.time_mean, (10 + 12 + 12) / 3.0)
        roi = ws.getRun().getTimeROI().toTimeIntervals()[0]
        self.assertEqual((roi[1] - roi[0]).total_seconds() / 3600.0, 3)

    def test_that_bad_proton_charges_can_be_removed_without_estimating_good_values(self):
        # If we pass a False boolean to _clean_logs, we should remove the bad proton charges
        # but not re-estimate good values
        ws = self._create_workspace_with_bad_logs()

        pc = ws.getRun().getProperty("proton_charge")
        self.assertEqual(len(pc.times), 3)

        # Clean the logs, but do not estimate new values
        ws = su._clean_logs(ws, False)

        # Get proton charge logs after operation
        pc = ws.getRun().getProperty("proton_charge")
        self.assertEqual(len(pc.times), 3)  # no extra values

        # duration should not be hours rather than years
        stats = ws.getRun().getStatistics("proton_charge")
        self.assertEqual(stats.duration / 3600, 2.0)  # hours
        self.assertEqual(stats.time_mean, (10 + 12) / 2.0)
        roi = ws.getRun().getTimeROI().toTimeIntervals()[0]
        self.assertEqual((roi[1] - roi[0]).total_seconds() / 3600.0, 2)

    def _create_workspace_with_bad_logs(self):
        ws = CreateSampleWorkspace(WorkspaceType="Event", NumEvents=0, OutputWorkspace="ws")

        # Add sample logs, including 1 bad proton charge
        for time, val in zip(["1900-01-01T00:00:00", "2019-01-01T00:00:00", "2019-01-01T01:00:00"], [0, 10, 12]):
            date = DateAndTime(time)
            AddTimeSeriesLog(ws, Name="proton_charge", Time=date.__str__().strip(), Value=val)
        return ws


class TestCombineWorkspacesFactory(unittest.TestCase):
    def test_that_factory_returns_overlay_class(self):
        factory = su.CombineWorkspacesFactory()
        alg = factory.create_add_algorithm(True)
        self.assertTrue(isinstance(alg, su.OverlayWorkspaces))

    def test_that_factory_returns_overlay_class_add_alg_false(self):
        factory = su.CombineWorkspacesFactory()
        alg = factory.create_add_algorithm(False)
        self.assertTrue(isinstance(alg, su.PlusWorkspaces))


class TestOverlayWorkspaces(unittest.TestCase):
    def test_time_from_proton_charge_log_is_recovered(self):
        # Arrange
        names = ["ws1", "ws2"]
        out_ws_name = "out_ws"

        start_time_1 = "2010-01-01T00:00:00"
        event_ws_1 = provide_event_ws(names[0], start_time_1, extra_time_shift=0.0)

        start_time_2 = "2012-01-01T00:00:00"
        event_ws_2 = provide_event_ws(names[1], start_time_2, extra_time_shift=0.0)

        # Act
        overlayWorkspaces = su.OverlayWorkspaces()
        time_difference = overlayWorkspaces._extract_time_difference_in_seconds(event_ws_1, event_ws_2)

        # Assert
        expected_time_difference = time_duration.totalNanoseconds(DateAndTime(start_time_1) - DateAndTime(start_time_2)) / 1e9
        self.assertEqual(time_difference, expected_time_difference)

        # Clean up
        self._clean_up(names)
        self._clean_up(out_ws_name)

    def test_that_time_difference_adds_correct_optional_shift(self):
        # Arrange
        names = ["ws1", "ws2"]
        out_ws_name = "out_ws"

        start_time_1 = "2010-01-01T00:00:00"
        event_ws_1 = provide_event_ws(names[0], start_time_1, extra_time_shift=0.0)

        # Extra shift in seconds
        optional_time_shift = 1000
        start_time_2 = "2012-01-01T00:00:00"
        event_ws_2 = provide_event_ws(names[1], start_time_2, extra_time_shift=optional_time_shift)

        # Act
        overlayWorkspaces = su.OverlayWorkspaces()
        time_difference = overlayWorkspaces._extract_time_difference_in_seconds(event_ws_1, event_ws_2)

        # Assert
        expected_time_difference = time_duration.totalNanoseconds(DateAndTime(start_time_1) - DateAndTime(start_time_2)) / 1e9
        expected_time_difference -= optional_time_shift  # Need to subtract as we add the time shift to the subtrahend
        self.assertEqual(time_difference, expected_time_difference)

        # Clean up
        self._clean_up(names)
        self._clean_up(out_ws_name)

    def test_error_is_raised_if_proton_charge_is_missing(self):
        # Arrange
        names = ["ws1", "ws2"]
        out_ws_name = "out_ws"

        start_time_1 = "2010-01-01T00:00:00"
        event_ws_1 = provide_event_ws_custom(name=names[0], start_time=start_time_1, extra_time_shift=0.0, proton_charge=False)

        # Extra shift in seconds
        start_time_2 = "2012-01-01T00:00:00"
        event_ws_2 = provide_event_ws_custom(name=names[1], start_time=start_time_2, extra_time_shift=0.0, proton_charge=False)

        # Act and Assert
        overlayWorkspaces = su.OverlayWorkspaces()
        args = [event_ws_1, event_ws_2]
        kwargs = {}
        self.assertRaises(RuntimeError, overlayWorkspaces._extract_time_difference_in_seconds, *args, **kwargs)

        # Clean up
        self._clean_up(names)
        self._clean_up(out_ws_name)

    def test_correct_time_difference_is_extracted(self):
        pass

    def test_workspaces_are_normalized_by_proton_charge(self):
        pass

    def _clean_up(self, names):
        for name in names:
            if name in mtd.getObjectNames():
                DeleteWorkspace(name)


class TestTimeShifter(unittest.TestCase):
    def test_zero_shift_when_out_of_range(self):
        # Arrange
        time_shifts = ["12", "333.6", "-232"]
        time_shifter = su.TimeShifter(time_shifts)

        # Act and Assert
        self.assertEqual(time_shifter.get_Nth_time_shift(0), 12.0)
        self.assertEqual(time_shifter.get_Nth_time_shift(1), 333.6)
        self.assertEqual(time_shifter.get_Nth_time_shift(2), -232.0)
        self.assertEqual(time_shifter.get_Nth_time_shift(3), 0.0)

    def test_zero_shift_when_bad_cast(self):
        # Arrange
        time_shifts = ["12", "33a.6", "-232"]
        time_shifter = su.TimeShifter(time_shifts)

        # Act and Assert
        self.assertEqual(time_shifter.get_Nth_time_shift(0), 12.0)
        self.assertEqual(time_shifter.get_Nth_time_shift(1), 0.0)
        self.assertEqual(time_shifter.get_Nth_time_shift(2), -232.0)
        self.assertEqual(time_shifter.get_Nth_time_shift(3), 0.0)


class TestZeroErrorFreeWorkspace(unittest.TestCase):
    def _setup_workspace(self, name, type):
        ws = CreateSampleWorkspace(
            OutputWorkspace=name,
            WorkspaceType=type,
            Function="One Peak",
            NumBanks=1,
            BankPixelWidth=2,
            NumEvents=0,
            XMin=0.5,
            XMax=1,
            BinWidth=1,
            PixelSpacing=1,
            BankDistanceFromSample=1,
        )
        if type == "Histogram":
            errors = ws.dataE
            # For first and third spectra set to 0.0
            errors(0)[0] = 0.0
            errors(2)[0] = 0.0

    def _removeWorkspace(self, name):
        if name in mtd:
            mtd.remove(name)

    def test_that_non_existent_ws_creates_error_message(self):
        # Arrange
        ws_name = "original"
        ws_clone_name = "clone"
        # Act
        message, complete = su.create_zero_error_free_workspace(input_workspace_name=ws_name, output_workspace_name=ws_clone_name)
        # Assert
        message.strip()
        self.assertTrue(message)
        self.assertTrue(not complete)

    def test_that_bad_zero_error_removal_creates_error_message(self):
        # Arrange
        ws_name = "original"
        ws_clone_name = "clone"
        self._setup_workspace(ws_name, "Event")
        # Act
        message, complete = su.create_zero_error_free_workspace(input_workspace_name=ws_name, output_workspace_name=ws_clone_name)
        # Assert
        message.strip()
        self.assertTrue(message)
        self.assertTrue(ws_clone_name not in mtd)
        self.assertTrue(not complete)

        self._removeWorkspace(ws_name)
        self.assertTrue(ws_name not in mtd)

    def test_that_zeros_are_removed_correctly(self):
        # Arrange
        ws_name = "original"
        ws_clone_name = "clone"
        self._setup_workspace(ws_name, "Histogram")
        # Act
        message, complete = su.create_zero_error_free_workspace(input_workspace_name=ws_name, output_workspace_name=ws_clone_name)
        # Assert
        message.strip()
        print(message)
        # self.assertTrue(not message)
        # self.assertTrue(complete)
        self.assertNotEqual(mtd[ws_name], mtd[ws_clone_name])

        self._removeWorkspace(ws_name)
        self._removeWorkspace(ws_clone_name)
        self.assertTrue(ws_name not in mtd)
        self.assertTrue(ws_clone_name not in mtd)

    def test_throws_for_non_Workspace2D(self):
        # Arrange
        ws_name = "test"
        type = "Event"
        self._setup_workspace(ws_name, type)
        ws = mtd[ws_name]

        # Act and Assert
        self.assertRaises(ValueError, su.remove_zero_errors_from_workspace, ws)

        self._removeWorkspace(ws_name)
        self.assertTrue(ws_name not in mtd)

    def test_removes_zero_errors_correctly(self):
        # Arrange
        ws_name = "test"
        type = "Histogram"
        self._setup_workspace(ws_name, type)
        ws = mtd[ws_name]

        # Act and Assert
        errors = ws.dataE
        self.assertEqual(errors(0)[0], 0.0)
        self.assertNotEqual(errors(1)[0], 0.0)
        self.assertEqual(errors(2)[0], 0.0)
        self.assertNotEqual(errors(3)[0], 0.0)

        su.remove_zero_errors_from_workspace(ws)

        self.assertEqual(errors(0)[0], su.ZERO_ERROR_DEFAULT)
        self.assertNotEqual(errors(1)[0], 0.0)
        self.assertNotEqual(errors(1)[0], su.ZERO_ERROR_DEFAULT)
        self.assertEqual(errors(2)[0], su.ZERO_ERROR_DEFAULT)
        self.assertNotEqual(errors(3)[0], 0.0)
        self.assertNotEqual(errors(3)[0], su.ZERO_ERROR_DEFAULT)

        self._removeWorkspace(ws_name)
        self.assertTrue(ws_name not in mtd)

    def test_that_deletion_of_non_existent_ws_creates_error_message(self):
        # Arrange
        ws_name = "ws"
        # Act
        message, complete = su.delete_zero_error_free_workspace(input_workspace_name=ws_name)
        # Assert
        message.strip()
        self.assertTrue(message)
        self.assertTrue(not complete)

    def test_that_deletion_of_extent_ws_is_successful(self):
        # Arrange
        ws_name = "ws"
        self._setup_workspace(ws_name, "Histogram")
        # Act + Assert
        self.assertTrue(ws_name in mtd)
        message, complete = su.delete_zero_error_free_workspace(input_workspace_name=ws_name)
        message.strip()
        self.assertTrue(not message)
        self.assertTrue(complete)
        self.assertTrue(ws_name not in mtd)

    def test_non_Q1D_and_Qxy_history_is_not_valid_and_produces_error_message(self):
        # Arrange
        ws_name = "ws"
        self._setup_workspace(ws_name, "Histogram")
        # Act
        message, complete = su.is_valid_ws_for_removing_zero_errors(input_workspace_name=ws_name)
        # Assert
        message.strip()
        self.assertTrue(message)
        self.assertTrue(not complete)

        self._removeWorkspace(ws_name)
        self.assertTrue(ws_name not in mtd)


class TestRenameMonitorsForMultiPeriodEventData(unittest.TestCase):
    monitor_appendix = "_monitors"

    def test_monitors_are_renamed_correctly(self):
        # Arrange
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
        self.assertEqual(ws_mon_1.name(), ws_1.name() + self.monitor_appendix, "Monitors should be renamed to xxxx_monitors")
        self.assertEqual(ws_mon_2.name(), ws_2.name() + self.monitor_appendix, "Monitors should be renamed to xxxx_monitors")
        self.assertEqual(ws_mon_3.name(), ws_3.name() + self.monitor_appendix, "Monitors should be renamed to xxxx_monitors")

        # Clean up
        for element in mtd.getObjectNames():
            if element in mtd:
                DeleteWorkspace(element)

    def test_expection_is_raised_when_workspaec_and_monitor_mismatch(self):
        # Arrange
        ws_1 = CreateSampleWorkspace()
        ws_2 = CreateSampleWorkspace()
        ws_3 = CreateSampleWorkspace()

        ws_mon_1 = CreateSampleWorkspace()
        ws_mon_2 = CreateSampleWorkspace()

        ws_group = GroupWorkspaces(InputWorkspaces=[ws_1, ws_2, ws_3])
        ws_mon_group = GroupWorkspaces(InputWorkspaces=[ws_mon_1, ws_mon_2])

        # Act + Assert
        args = {"monitor_worksapce": ws_mon_group, "workspace": ws_group, "appendix": self.monitor_appendix}
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
        input = "34"
        # Act
        result = su.is_convertible_to_int(input)
        # Assert
        self.assertTrue(result)

    def test__converts_false_to_integer_when_non_convertible_string(self):
        # Arrange
        input = "34_gt"
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
        result = su.is_valid_xml_file_list(input)
        # Assert
        self.assertTrue(result)

    def test_finds_invalid_xml_file_list(self):
        # Arrange
        input = ["test1.xml", "test2.ccl", "test3.xml"]
        # Act
        result = su.is_valid_xml_file_list(input)
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


class HelperRescaleShift(object):
    def __init__(self, hasValues=True, min=1, max=2):
        super(HelperRescaleShift, self).__init__()
        self.qRangeUserSelected = hasValues
        self.qMin = min
        self.qMax = max


class TestExtractionOfQRange(unittest.TestCase):
    def _delete_workspace(self, workspace_name):
        if workspace_name in mtd:
            DeleteWorkspace(workspace_name)

    def test_that_correct_q_range_is_extracted_from_rescaleAndShift_object_which_lies_inside_the_data_range(self):
        # Arrange
        front_q_min = 10
        front_q_max = 20
        front_name = "front_ws"
        rear_q_min = 1
        rear_q_max = 30
        rear_name = "rear_ws"
        bin_width = 1
        provide_histo_workspace_with_one_spectrum(rear_name, rear_q_min, rear_q_max, bin_width)
        provide_histo_workspace_with_one_spectrum(front_name, front_q_min, front_q_max, bin_width)
        rescale_shift = HelperRescaleShift(True, 15, 17)
        # Act
        result_q_min, result_q_max = su.get_start_q_and_end_q_values(
            rear_data_name=rear_name, front_data_name=front_name, rescale_shift=rescale_shift
        )
        # Assert
        self.assertEqual(15, result_q_min)
        self.assertEqual(17, result_q_max)
        # Clean up
        self._delete_workspace(front_name)
        self._delete_workspace(rear_name)

    def test_that_correct_q_range_is_extracted_when_no_rescaleAndShift_is_applied_and_front_detector_data_lies_within_rear_detector_data(
        self,
    ):
        # Arrange
        front_q_min = 10
        front_q_max = 20
        front_name = "front_ws"
        rear_q_min = 1
        rear_q_max = 30
        rear_name = "rear_ws"
        bin_width = 1
        provide_histo_workspace_with_one_spectrum(rear_name, rear_q_min, rear_q_max, bin_width)
        provide_histo_workspace_with_one_spectrum(front_name, front_q_min, front_q_max, bin_width)
        rescale_shift = HelperRescaleShift(False, 1, 2)
        # Act
        result_q_min, result_q_max = su.get_start_q_and_end_q_values(
            rear_data_name=rear_name, front_data_name=front_name, rescale_shift=rescale_shift
        )
        # Assert
        self.assertEqual(10, result_q_min)
        self.assertEqual(20, result_q_max)
        # Clean up
        self._delete_workspace(front_name)
        self._delete_workspace(rear_name)

    def test_that_execption_is_raised_when_data_does_not_overlap(self):
        # Arrange
        front_q_min = 10
        front_q_max = 20
        front_name = "front_ws"
        rear_q_min = 1
        rear_q_max = 9
        rear_name = "rear_ws"
        bin_width = 1
        provide_histo_workspace_with_one_spectrum(rear_name, rear_q_min, rear_q_max, bin_width)
        provide_histo_workspace_with_one_spectrum(front_name, front_q_min, front_q_max, bin_width)
        rescale_shift = HelperRescaleShift(False, 1, 2)
        # Act + Assert
        args = []
        kwargs = {"rear_data_name": rear_name, "front_data_name": front_name, "rescale_shift": rescale_shift}
        self.assertRaises(RuntimeError, su.get_start_q_and_end_q_values, *args, **kwargs)

        # Clean up
        self._delete_workspace(front_name)
        self._delete_workspace(rear_name)


class TestErrorPropagationFitAndRescale(unittest.TestCase):
    def _createWorkspace(self, x1, y1, err1, x2, y2, err2):
        front = CreateWorkspace(DataX=x1, DataY=y1, DataE=err1, NSpec=1, UnitX="MomentumTransfer")
        rear = CreateWorkspace(DataX=x2, DataY=y2, DataE=err2, NSpec=1, UnitX="MomentumTransfer")
        return front, rear

    def test_that_error_is_transferred(self):
        # Arrange
        x1 = [1, 2, 3, 4, 5, 6, 7, 8, 9]
        x2 = [1, 2, 3, 4, 5, 6, 7, 8, 9]
        y1 = [2, 2, 2, 2, 2, 2, 2, 2]
        y2 = [2, 2, 2, 2, 2, 2, 2, 2]
        e1 = [1, 1, 1, 1, 1, 1, 1, 1]
        e2 = [2, 2, 2, 2, 2, 2, 2, 2]
        front, rear = self._createWorkspace(x1, y1, e1, x2, y2, e2)

        x_min = 3
        x_max = 7
        # Act
        f_return, r_return = su.get_error_corrected_front_and_rear_data_sets(front, rear, x_min, x_max)

        # Assert
        self.assertEqual(5, len(f_return.dataX(0)))
        self.assertEqual(5, len(r_return.dataX(0)))

        expected_errors_in_rear = [np.sqrt(5), np.sqrt(5), np.sqrt(5), np.sqrt(5)]
        self.assertEqual(expected_errors_in_rear[0], r_return.dataE(0)[0])
        self.assertEqual(expected_errors_in_rear[1], r_return.dataE(0)[1])
        self.assertEqual(expected_errors_in_rear[2], r_return.dataE(0)[2])
        self.assertEqual(expected_errors_in_rear[3], r_return.dataE(0)[3])

        # Clean up
        DeleteWorkspace(front)
        DeleteWorkspace(rear)


class TestGetCorrectQResolution(unittest.TestCase):
    def test_error_is_passed_from_original_to_subtracted_workspace(self):
        # Arrange
        orig_name = "orig"
        can_name = "can"
        result_name = "result"
        provide_workspace_with_x_errors(orig_name, True)
        provide_workspace_with_x_errors(can_name, True)
        provide_workspace_with_x_errors(result_name, False)
        orig = mtd[orig_name]
        can = mtd[can_name]
        result = mtd[result_name]
        # Act
        su.correct_q_resolution_for_can(orig, can, result)
        # Assert
        dx_orig = orig.dataDx(0)
        dx_result = result.dataDx(0)
        self.assertTrue(result.hasDx(0))
        for index in range(0, len(dx_orig)):
            self.assertEqual(dx_orig[index], dx_result[index])
        # Clean up
        DeleteWorkspace(orig)
        DeleteWorkspace(can)
        DeleteWorkspace(result)

    def test_error_is_ignored_for_more_than_one_spectrum(self):
        # Arrange
        orig_name = "orig"
        can_name = "can"
        result_name = "result"
        provide_workspace_with_x_errors(orig_name, True, 2)
        provide_workspace_with_x_errors(can_name, True, 2)
        provide_workspace_with_x_errors(result_name, False, 2)
        orig = mtd[orig_name]
        can = mtd[can_name]
        result = mtd[result_name]
        # Act
        su.correct_q_resolution_for_can(orig, can, result)
        # Assert
        self.assertFalse(result.hasDx(0))
        # Clean up
        DeleteWorkspace(orig)
        DeleteWorkspace(can)
        DeleteWorkspace(result)

    def test_error_is_not_passed_on_when_did_not_exist_beforehand(self):
        # Arrange
        orig_name = "orig"
        can_name = "can"
        result_name = "result"
        provide_workspace_with_x_errors(orig_name, False, 1)
        provide_workspace_with_x_errors(can_name, False, 1)
        provide_workspace_with_x_errors(result_name, False, 1)
        orig = mtd[orig_name]
        can = mtd[can_name]
        result = mtd[result_name]
        # Act
        su.correct_q_resolution_for_can(orig, can, result)
        # Assert
        self.assertFalse(result.hasDx(0))
        # Clean up
        DeleteWorkspace(orig)
        DeleteWorkspace(can)
        DeleteWorkspace(result)


class TestGetQResolutionForMergedWorkspaces(unittest.TestCase):
    def test_error_is_ignored_for_more_than_one_spectrum(self):
        # Arrange
        front_name = "front"
        rear_name = "rear"
        result_name = "result"
        provide_workspace_with_x_errors(front_name, True, 2)
        provide_workspace_with_x_errors(rear_name, True, 2)
        provide_workspace_with_x_errors(result_name, False, 2)
        front = mtd[front_name]
        rear = mtd[rear_name]
        result = mtd[result_name]
        scale = 2.0
        # Act
        su.correct_q_resolution_for_merged(front, rear, result, scale)
        # Assert
        self.assertFalse(result.hasDx(0))
        # Clean up
        DeleteWorkspace(front)
        DeleteWorkspace(rear)
        DeleteWorkspace(result)

    def test_error_is_ignored_when_only_one_input_has_dx(self):
        # Arrange
        front_name = "front"
        rear_name = "rear"
        result_name = "result"
        provide_workspace_with_x_errors(front_name, True, 1)
        provide_workspace_with_x_errors(rear_name, False, 1)
        provide_workspace_with_x_errors(result_name, False, 1)
        front = mtd[front_name]
        rear = mtd[rear_name]
        result = mtd[result_name]
        scale = 2.0
        # Act
        su.correct_q_resolution_for_merged(front, rear, result, scale)
        # Assert
        self.assertFalse(result.hasDx(0))
        # Clean up
        DeleteWorkspace(front)
        DeleteWorkspace(rear)
        DeleteWorkspace(result)

    def test_that_non_matching_workspaces_are_detected(self):
        # Arrange
        front_name = "front"
        rear_name = "rear"
        result_name = "result"
        x1 = [1, 2, 3]
        e1 = [1, 1]
        y1 = [2, 2]
        dx1 = [1.0, 2.0]
        x2 = [1, 2, 3, 4]
        e2 = [1, 1, 1]
        y2 = [2, 2, 2]
        dx2 = [1.0, 2.0, 3.0]
        provide_workspace_with_x_errors(front_name, True, 1, x1, y1, e1, dx1)
        provide_workspace_with_x_errors(rear_name, True, 1, x2, y2, e2, dx2)
        provide_workspace_with_x_errors(result_name, False, 1)
        front = mtd[front_name]
        rear = mtd[rear_name]
        result = mtd[result_name]
        scale = 2.0
        # Act
        su.correct_q_resolution_for_merged(front, rear, result, scale)
        # Assert
        self.assertFalse(result.hasDx(0))
        # Clean up
        DeleteWorkspace(front)
        DeleteWorkspace(rear)
        DeleteWorkspace(result)

    def test_correct_x_error_is_produced(self):
        # Arrange
        x = [1, 2, 3]
        e = [1, 1]
        y_front = [2, 2]
        dx_front = [1.0, 2.0]
        y_rear = [1.5, 1.5]
        dx_rear = [3.0, 2.0]
        front_name = "front"
        rear_name = "rear"
        result_name = "result"
        provide_workspace_with_x_errors(front_name, True, 1, x, y_front, e, dx_front)
        provide_workspace_with_x_errors(rear_name, True, 1, x, y_rear, e, dx_rear)
        provide_workspace_with_x_errors(result_name, False, 1, x, y_front, e)
        front = mtd[front_name]
        rear = mtd[rear_name]
        result = mtd[result_name]
        scale = 2.0
        # Act
        su.correct_q_resolution_for_merged(front, rear, result, scale)
        # Assert
        self.assertTrue(result.hasDx(0))

        dx_expected_0 = (dx_front[0] * y_front[0] * scale + dx_rear[0] * y_rear[0]) / (y_front[0] * scale + y_rear[0])
        dx_expected_1 = (dx_front[1] * y_front[1] * scale + dx_rear[1] * y_rear[1]) / (y_front[1] * scale + y_rear[1])
        dx_result = result.readDx(0)
        self.assertEqual(len(dx_result), 2)
        self.assertEqual(dx_result[0], dx_expected_0)
        self.assertEqual(dx_result[1], dx_expected_1)

        # Clean up
        DeleteWorkspace(front)
        DeleteWorkspace(rear)
        DeleteWorkspace(result)


class TestDetectingValidUserFileExtensions(unittest.TestCase):
    def _do_test(self, file_name, expected):
        result = su.is_valid_user_file_extension(file_name)
        self.assertEqual(result, expected)

    def test_that_detects_txt_file(self):
        # Arrange
        file_name = "/path1/path2/file.Txt"
        expected = True
        # Act and Assert
        self._do_test(file_name, expected)

    def test_that_fails_for_random_file_extension(self):
        # Arrange
        file_name = "/path1/path2/file.abc"
        expected = False
        # Act and Assert
        self._do_test(file_name, expected)

    def test_that_detects_when_ending_starts_with_number(self):
        # Arrange
        file_name = "/path1/path2/file.091A"
        expected = True
        # Act and Assert
        self._do_test(file_name, expected)


class TestCorrectingCummulativeSampleLogs(unittest.TestCase):
    def _clean_up_workspaces(self):
        for name in mtd.getObjectNames():
            DeleteWorkspace(name)

    def _check_that_increasing(self, series):
        for index in range(1, len(series)):
            larger_or_equal = series[index] > series[index - 1] or series[index] == series[index - 1]
            self.assertTrue(larger_or_equal)

    def _check_that_values_of_series_are_the_same(self, series1, series2):
        zipped = list(zip(series1, series2))
        areEqual = True
        for e1, e2 in zipped:
            isEqual = e1 == e2
            areEqual &= isEqual
        self.assertTrue(areEqual)

    def _has_the_same_length_as_input(self, out, in1, in2, log_entry):
        len_out = len(out.getRun().getProperty(log_entry).value)
        len_in1 = len(in1.getRun().getProperty(log_entry).value)
        len_in2 = len(in2.getRun().getProperty(log_entry).value)
        return len_out == (len_in1 + len_in2)

    def _add_single_log(self, workspace, name, value):
        alg_log = AlgorithmManager.createUnmanaged("AddSampleLog")
        alg_log.initialize()
        alg_log.setChild(True)
        alg_log.setProperty("Workspace", workspace)
        alg_log.setProperty("LogName", name)
        alg_log.setProperty("LogText", str(value))
        alg_log.setProperty("LogType", "Number")
        alg_log.execute()

    def _assert_that_good_proton_charge_is_correct(self, in1, in2, out):
        in1_charge = in1.getRun().getProperty("gd_prtn_chrg").value
        in2_charge = in2.getRun().getProperty("gd_prtn_chrg").value
        out_charge = out.getRun().getProperty("gd_prtn_chrg").value
        self.assertEqual(out_charge, (in1_charge + in2_charge))

    def test_that_non_overlapping_workspaces_have_their_cummulative_sample_log_added_correctly(self):
        # Arrange
        names = ["ws1", "ws2", "out"]
        log_names = ["good_uah_log", "good_frames", "new_series"]
        start_time_1 = "2010-01-01T00:00:00"
        proton_charge_1 = 10.2
        lhs = provide_event_ws_with_entries(names[0], start_time_1, extra_time_shift=0.0, log_names=log_names, make_linear=True)
        self._add_single_log(lhs, "gd_prtn_chrg", proton_charge_1)

        start_time_2 = "2010-01-01T00:00:12"
        proton_charge_2 = 30.2
        rhs = provide_event_ws_with_entries(names[1], start_time_2, extra_time_shift=0.0, log_names=log_names, make_linear=True)
        self._add_single_log(rhs, "gd_prtn_chrg", proton_charge_2)

        start_time_3 = "2010-02-01T00:00:00"
        proton_charge_3 = 20.2
        out = provide_event_ws_with_entries(names[2], start_time_3, extra_time_shift=0.0, log_names=log_names, make_linear=True)
        self._add_single_log(out, "gd_prtn_chrg", proton_charge_3)

        out_ref = CloneWorkspace(InputWorkspace=out)

        # Act
        converter = su.CummulativeTimeSeriesPropertyAdder(total_time_shift_seconds=0.0)
        converter.extract_sample_logs_from_workspace(lhs, rhs)
        converter.apply_cummulative_logs_to_workspace(out)

        # Assert
        # good_uah_log should be sorted
        self._check_that_increasing(out.getRun().getProperty(log_names[0]).times)
        self._check_that_increasing(out.getRun().getProperty(log_names[0]).value)
        self.assertTrue(self._has_the_same_length_as_input(out, lhs, rhs, log_names[0]))

        # Check gd_prtn_chrg
        self._assert_that_good_proton_charge_is_correct(lhs, rhs, out)

        # good_frames should be sorted
        self._check_that_increasing(out.getRun().getProperty(log_names[1]).times)
        self._check_that_increasing(out.getRun().getProperty(log_names[1]).value)
        self.assertTrue(self._has_the_same_length_as_input(out, lhs, rhs, log_names[1]))

        # new_series should not have been touched
        self._check_that_values_of_series_are_the_same(
            out_ref.getRun().getProperty(log_names[2]).times, out.getRun().getProperty(log_names[2]).times
        )
        self._check_that_values_of_series_are_the_same(
            out_ref.getRun().getProperty(log_names[2]).value, out.getRun().getProperty(log_names[2]).value
        )
        self.assertFalse(self._has_the_same_length_as_input(out, lhs, rhs, log_names[2]))

        # Clean up
        self._clean_up_workspaces()

    def test_that_overlapping_workspaces_have_their_cummulative_sample_log_added_correctly(self):
        # Arrange
        names = ["ws1", "ws2", "out"]
        log_names = ["good_uah_log", "good_frames", "new_series"]
        start_time_1 = "2010-01-01T00:00:00"
        proton_charge_1 = 10.2
        lhs = provide_event_ws_with_entries(names[0], start_time_1, extra_time_shift=0.0, log_names=log_names, make_linear=True)
        self._add_single_log(lhs, "gd_prtn_chrg", proton_charge_1)

        # The rhs workspace should have an overlap time of about 5s
        start_time_2 = "2010-01-01T00:00:05"
        proton_charge_2 = 19.2
        rhs = provide_event_ws_with_entries(names[1], start_time_2, extra_time_shift=0.0, log_names=log_names, make_linear=True)
        self._add_single_log(rhs, "gd_prtn_chrg", proton_charge_2)

        start_time_3 = "2010-02-01T00:00:00"
        proton_charge_3 = 30.2
        out = provide_event_ws_with_entries(names[2], start_time_3, extra_time_shift=0.0, log_names=log_names, make_linear=True)
        self._add_single_log(out, "gd_prtn_chrg", proton_charge_3)

        out_ref = CloneWorkspace(InputWorkspace=out)

        # Act
        converter = su.CummulativeTimeSeriesPropertyAdder(total_time_shift_seconds=0.0)
        converter.extract_sample_logs_from_workspace(lhs, rhs)
        converter.apply_cummulative_logs_to_workspace(out)

        # Assert
        # good_uah_log should be sorted
        self._check_that_increasing(out.getRun().getProperty(log_names[0]).times)
        self._check_that_increasing(out.getRun().getProperty(log_names[0]).value)
        self.assertTrue(self._has_the_same_length_as_input(out, lhs, rhs, log_names[0]))

        # Check gd_prtn_chrg
        self._assert_that_good_proton_charge_is_correct(lhs, rhs, out)

        # good_frames should be sorted
        self._check_that_increasing(out.getRun().getProperty(log_names[1]).times)
        self._check_that_increasing(out.getRun().getProperty(log_names[1]).value)
        self.assertTrue(self._has_the_same_length_as_input(out, lhs, rhs, log_names[1]))

        # new_series should not have been touched
        self._check_that_values_of_series_are_the_same(
            out_ref.getRun().getProperty(log_names[2]).times, out.getRun().getProperty(log_names[2]).times
        )
        self._check_that_values_of_series_are_the_same(
            out_ref.getRun().getProperty(log_names[2]).value, out.getRun().getProperty(log_names[2]).value
        )
        self.assertFalse(self._has_the_same_length_as_input(out, lhs, rhs, log_names[2]))

        # Clean up
        self._clean_up_workspaces()

    def test_that_non_overlapping_workspaces_with_time_shift_have_their_cummulative_sample_log_added_correctly(self):
        # Arrange
        names = ["ws1", "ws2", "out"]
        log_names = ["good_uah_log", "good_frames", "new_series"]

        start_time_1 = "2010-01-01T00:00:00"
        proton_charge_1 = 10.2
        lhs = provide_event_ws_with_entries(names[0], start_time_1, extra_time_shift=0.0, log_names=log_names, make_linear=True)
        self._add_single_log(lhs, "gd_prtn_chrg", proton_charge_1)

        # The rhs workspace should have an overlap time of about 5s
        start_time_2 = "2010-01-01T00:00:20"
        proton_charge_2 = 18.2
        rhs = provide_event_ws_with_entries(names[1], start_time_2, extra_time_shift=0.0, log_names=log_names, make_linear=True)
        self._add_single_log(rhs, "gd_prtn_chrg", proton_charge_2)

        start_time_3 = "2010-02-01T00:00:00"
        proton_charge_3 = 80.2
        out = provide_event_ws_with_entries(names[2], start_time_3, extra_time_shift=0.0, log_names=log_names, make_linear=True)
        self._add_single_log(out, "gd_prtn_chrg", proton_charge_3)

        out_ref = CloneWorkspace(InputWorkspace=out)

        # Act
        # Shift the time -15.5 s into the past
        converter = su.CummulativeTimeSeriesPropertyAdder(total_time_shift_seconds=-15.5)
        converter.extract_sample_logs_from_workspace(lhs, rhs)
        converter.apply_cummulative_logs_to_workspace(out)

        # Assert
        # good_uah_log should be sorted
        self._check_that_increasing(out.getRun().getProperty(log_names[0]).times)
        self._check_that_increasing(out.getRun().getProperty(log_names[0]).value)
        self.assertTrue(self._has_the_same_length_as_input(out, lhs, rhs, log_names[0]))

        # Check gd_prtn_chrg
        self._assert_that_good_proton_charge_is_correct(lhs, rhs, out)

        # good_frames should be sorted
        self._check_that_increasing(out.getRun().getProperty(log_names[1]).times)
        self._check_that_increasing(out.getRun().getProperty(log_names[1]).value)
        self.assertTrue(self._has_the_same_length_as_input(out, lhs, rhs, log_names[1]))

        # new_series should not have been touched
        self._check_that_values_of_series_are_the_same(
            out_ref.getRun().getProperty(log_names[2]).times, out.getRun().getProperty(log_names[2]).times
        )
        self._check_that_values_of_series_are_the_same(
            out_ref.getRun().getProperty(log_names[2]).value, out.getRun().getProperty(log_names[2]).value
        )
        self.assertFalse(self._has_the_same_length_as_input(out, lhs, rhs, log_names[2]))

        # Clean up
        self._clean_up_workspaces()

    def test_that_special_sample_logs_are_transferred(self):
        # Arrange, note that the values don't make sense since we are not using "make_linear" but
        # this is not releavnt for the bare functionality
        names = ["ws1", "ws2", "out"]
        log_names = ["good_uah_log", "good_frames", "new_series"]

        start_time = "2010-01-01T00:00:00"
        in_ws = provide_event_ws_with_entries(names[0], start_time, extra_time_shift=0.0, log_names=log_names, make_linear=False)
        self._add_single_log(in_ws, "gd_prtn_chrg", in_ws.getRun().getProperty("good_uah_log").value[-1])

        start_time = "2010-02-01T00:10:00"
        out_ws = provide_event_ws_with_entries(names[2], start_time, extra_time_shift=0.0, log_names=log_names, make_linear=True)
        self._add_single_log(out_ws, "gd_prtn_chrg", out_ws.getRun().getProperty("good_uah_log").value[-1])

        out_ref = CloneWorkspace(InputWorkspace=out_ws)

        # Act
        su.transfer_special_sample_logs(from_ws=in_ws, to_ws=out_ws)

        # Assert
        # The good_uah_log and good_frames should have been transferred
        for index in range(1):
            self._check_that_values_of_series_are_the_same(
                out_ws.getRun().getProperty(log_names[index]).times, in_ws.getRun().getProperty(log_names[index]).times
            )
            self._check_that_values_of_series_are_the_same(
                out_ws.getRun().getProperty(log_names[index]).value, in_ws.getRun().getProperty(log_names[index]).value
            )

        # The gd_prtn_chrg should have been transferred
        self.assertEqual(out_ws.getRun().getProperty("gd_prtn_chrg").value, in_ws.getRun().getProperty("gd_prtn_chrg").value)

        # The new series log should not have been transferred
        self._check_that_values_of_series_are_the_same(
            out_ref.getRun().getProperty(log_names[2]).value, out_ws.getRun().getProperty(log_names[2]).value
        )

        # Clean up
        self._clean_up_workspaces()


class TestBenchRotDetection(unittest.TestCase):
    def _get_sample_workspace(self, has_bench_rot=True):
        sample_alg = AlgorithmManager.createUnmanaged("CreateSampleWorkspace")
        sample_alg.setChild(True)
        sample_alg.initialize()
        sample_alg.setProperty("OutputWorkspace", "dummy")
        sample_alg.execute()
        ws = sample_alg.getProperty("OutputWorkspace").value

        if has_bench_rot:
            log_alg = AlgorithmManager.createUnmanaged("AddSampleLog")
            log_alg.setChild(True)
            log_alg.initialize()
            log_alg.setProperty("Workspace", ws)
            log_alg.setProperty("LogName", "Bench_Rot")
            log_alg.setProperty("LogType", "Number")
            log_alg.setProperty("LogText", str(123.5))
            log_alg.execute()
        return ws

    def _do_test(self, expected_raise, workspace, log_dict):
        has_raised = False
        try:
            su.check_has_bench_rot(workspace, log_dict)
        except RuntimeError:
            has_raised = True
        self.assertEqual(has_raised, expected_raise)

    def test_workspace_with_bench_rot_does_not_raise(self):
        # Arrange
        ws = self._get_sample_workspace(has_bench_rot=True)
        # Act + Assert
        expected_raise = False
        log_dict = {"sdfsdf": "sdfsdf"}
        self._do_test(expected_raise, ws, log_dict)

    def test_workspace_without_bench_raises(self):
        # Arrange
        ws = self._get_sample_workspace(has_bench_rot=False)
        # Act + Assert
        expected_raise = True
        log_dict = {"sdfsdf": "sdfsdf"}
        self._do_test(expected_raise, ws, log_dict)

    def test_workspace_without_bench_but_no_log_dict_does_not_raise(self):
        # Arrange
        ws = self._get_sample_workspace(has_bench_rot=False)
        # Act + Assert
        expected_raise = False
        log_dict = None
        self._do_test(expected_raise, ws, log_dict)


class TestQuaternionToAngleAndAxis(unittest.TestCase):
    def _do_test_quaternion(self, angle, axis, expected_axis=None):
        # Act
        quaternion = Quat(angle, axis)
        converted_angle, converted_axis = su.quaternion_to_angle_and_axis(quaternion)

        # Assert
        if expected_axis is not None:
            axis = expected_axis
        self.assertAlmostEqual(angle, converted_angle)
        self.assertAlmostEqual(axis[0], converted_axis[0])
        self.assertAlmostEqual(axis[1], converted_axis[1])
        self.assertAlmostEqual(axis[2], converted_axis[2])

    def test_that_quaternion_can_be_converted_to_axis_and_angle_for_regular(self):
        # Arrange
        angle = 23.0
        axis = V3D(0.0, 1.0, 0.0)
        self._do_test_quaternion(angle, axis)

    def test_that_quaternion_can_be_converted_to_axis_and_angle_for_0_degree(self):
        # Arrange
        angle = 0.0
        axis = V3D(1.0, 0.0, 0.0)
        # There shouldn't be an axis for angle 0
        expected_axis = V3D(0.0, 0.0, 0.0)
        self._do_test_quaternion(angle, axis, expected_axis)

    def test_that_quaternion_can_be_converted_to_axis_and_angle_for_180_degree(self):
        # Arrange
        angle = 180.0
        axis = V3D(0.0, 1.0, 0.0)
        # There shouldn't be an axis for angle 0
        self._do_test_quaternion(angle, axis)


class TestTransmissionName(unittest.TestCase):
    def test_that_suffix_is_added_if_not_exists(self):
        # Arrange
        workspace_name = "test_workspace_name"
        # Act
        unfitted_workspace_name = su.get_unfitted_transmission_workspace_name(workspace_name)
        # Assert
        expected = workspace_name + "_unfitted"
        self.assertEqual(unfitted_workspace_name, expected)

    def test_that_suffix_is_not_added_if_exists(self):
        # Arrange
        workspace_name = "test_workspace_name_unfitted"
        # Act
        unfitted_workspace_name = su.get_unfitted_transmission_workspace_name(workspace_name)
        # Assert
        expected = workspace_name
        self.assertEqual(unfitted_workspace_name, expected)


class TestAddingUserFileExtension(unittest.TestCase):
    def test_that_does_not_alter_user_file_name_when_contains_txt_ending(self):
        self.assertEqual(su.get_user_file_name_options_with_txt_extension("test.TXt"), ["test.TXt"])
        self.assertEqual(su.get_user_file_name_options_with_txt_extension("test.txt"), ["test.txt"])
        self.assertEqual(su.get_user_file_name_options_with_txt_extension("test.TXT"), ["test.TXT"])
        self.assertEqual(su.get_user_file_name_options_with_txt_extension("test.tXt"), ["test.tXt"])

    def test_that_does_alters_user_file_name_when_does_contain_txt_ending(self):
        self.assertEqual(su.get_user_file_name_options_with_txt_extension("test.tt"), ["test.tt.txt", "test.tt.TXT"])
        self.assertEqual(su.get_user_file_name_options_with_txt_extension("test"), ["test.txt", "test.TXT"])


class TestSelectNewDetector(unittest.TestCase):
    def test_that_for_SANS2D_correct_settings_are_selected(self):
        self.assertEqual(su.get_correct_combinDet_setting("SANS2d", "rear"), "rear")
        self.assertEqual(su.get_correct_combinDet_setting("SANS2D", "FRONT"), "front")
        self.assertEqual(su.get_correct_combinDet_setting("SANS2d", "rear-detector"), "rear")
        self.assertEqual(su.get_correct_combinDet_setting("SANS2D", "FRONT-DETECTOR"), "front")
        self.assertEqual(su.get_correct_combinDet_setting("sAnS2d", "boTH"), "both")
        self.assertEqual(su.get_correct_combinDet_setting("sans2d", "merged"), "merged")

    def test_that_for_LOQ_correct_settings_are_selected(self):
        self.assertEqual(su.get_correct_combinDet_setting("Loq", "main-detector-bank"), "rear")
        self.assertEqual(su.get_correct_combinDet_setting("Loq", "main"), "rear")
        self.assertEqual(su.get_correct_combinDet_setting("LOQ", "Hab"), "front")
        self.assertEqual(su.get_correct_combinDet_setting("lOQ", "boTH"), "both")
        self.assertEqual(su.get_correct_combinDet_setting("loq", "merged"), "merged")

    def test_that_for_LARMOR_correct_settings_are_selected(self):
        self.assertEqual(su.get_correct_combinDet_setting("larmor", "main"), None)
        self.assertEqual(su.get_correct_combinDet_setting("LARMOR", "DetectorBench"), None)

    def test_that_for_unknown_instrument_raises(self):
        args = ["unknown_instrument", "main"]
        self.assertRaises(RuntimeError, su.get_correct_combinDet_setting, *args)

    def test_that_for_unknown_detector_command_raises(self):
        args = ["sans2d", "main"]
        self.assertRaises(RuntimeError, su.get_correct_combinDet_setting, *args)
        args = ["loq", "front"]
        self.assertRaises(RuntimeError, su.get_correct_combinDet_setting, *args)


class TestRenamingOfBatchModeWorkspaces(unittest.TestCase):
    def _create_sample_workspace(self, name="ws"):
        ws = CreateSampleWorkspace(
            Function="Flat background", NumBanks=1, BankPixelWidth=1, NumEvents=1, XMin=1, XMax=14, BinWidth=2, OutputWorkspace=name
        )

        return ws

    def test_that_SANS2D_workspace_is_renamed_correctly(self):
        workspace = self._create_sample_workspace()
        workspace_name = workspace.name()
        out_name = su.rename_workspace_correctly("SANS2D", su.ReducedType.LAB, "test", workspace_name)
        self.assertTrue(AnalysisDataService.doesExist("test_rear"))
        self.assertEqual(out_name, "test_rear")
        out_name = su.rename_workspace_correctly("SANS2D", su.ReducedType.HAB, "test", out_name)
        self.assertTrue(AnalysisDataService.doesExist("test_front"))
        self.assertEqual(out_name, "test_front")
        out_name = su.rename_workspace_correctly("SANS2D", su.ReducedType.Merged, "test", out_name)
        self.assertTrue(AnalysisDataService.doesExist("test_merged"))
        self.assertEqual(out_name, "test_merged")

        if AnalysisDataService.doesExist("test_merged"):
            AnalysisDataService.remove("test_merged")

    def test_for_a_group_workspace_renames_entire_group(self):
        workspace_t0_T1 = CreateSampleWorkspace(
            Function="Flat background", NumBanks=1, BankPixelWidth=1, NumEvents=1, XMin=1, XMax=14, BinWidth=2
        )
        workspace_t1_T2 = CreateSampleWorkspace(
            Function="Flat background", NumBanks=1, BankPixelWidth=1, NumEvents=1, XMin=1, XMax=14, BinWidth=2
        )
        workspace_name = "workspace"
        GroupWorkspaces(InputWorkspaces=[workspace_t0_T1, workspace_t1_T2], OutputWorkspace=workspace_name)

        out_name = su.rename_workspace_correctly("SANS2D", su.ReducedType.LAB, "test", workspace_name)
        self.assertTrue(AnalysisDataService.doesExist("test_rear"))
        self.assertTrue(AnalysisDataService.doesExist("test_t0_T1_rear"))
        self.assertTrue(AnalysisDataService.doesExist("test_t1_T2_rear"))
        self.assertEqual(out_name, "test_rear")

        AnalysisDataService.remove("test_t1_T2_rear")
        AnalysisDataService.remove("test_t0_T1_rear")
        AnalysisDataService.remove("test_rear")

    def test_that_LOQ_workspace_is_renamed_correctly(self):
        workspace = self._create_sample_workspace()
        workspace_name = workspace.name()
        out_name = su.rename_workspace_correctly("LOQ", su.ReducedType.LAB, "test", workspace_name)
        self.assertTrue(AnalysisDataService.doesExist("test_main"))
        self.assertEqual(out_name, "test_main")
        out_name = su.rename_workspace_correctly("LOQ", su.ReducedType.HAB, "test", out_name)
        self.assertTrue(AnalysisDataService.doesExist("test_hab"))
        self.assertEqual(out_name, "test_hab")
        out_name = su.rename_workspace_correctly("LOQ", su.ReducedType.Merged, "test", out_name)
        self.assertTrue(AnalysisDataService.doesExist("test_merged"))
        self.assertEqual(out_name, "test_merged")

        if AnalysisDataService.doesExist("test_merged"):
            AnalysisDataService.remove("test_merged")

    def test_that_LARMOR_workspace_is_not_renamed(self):
        workspace = self._create_sample_workspace()
        workspace_name = workspace.name()

        out_name = su.rename_workspace_correctly("LARMOR", su.ReducedType.LAB, "test", workspace_name)
        self.assertTrue(AnalysisDataService.doesExist("test"))
        self.assertEqual(out_name, "test")

        if AnalysisDataService.doesExist("test"):
            AnalysisDataService.remove("test")

    def test_that_raies_for_unkown_reduction_type(self):
        workspace = self._create_sample_workspace()
        workspace_name = workspace.name()
        args = ["SANS2D", "jsdlkfsldkfj", "test", workspace_name]

        self.assertRaises(RuntimeError, su.rename_workspace_correctly, *args)

        AnalysisDataService.remove("ws")

    def test_run_number_should_be_replaced_if_workspace_starts_with_number(self):
        workspace = self._create_sample_workspace(name="12345rear_1D_w1_W2_t1_T2")
        workspace_name = workspace.name()

        out_name = su.rename_workspace_correctly("SANS2D", su.ReducedType.LAB, "NewName", workspace_name)

        self.assertTrue(AnalysisDataService.doesExist("NewName_rear_1D_w1_W2_t1_T2"))
        self.assertEqual(out_name, "NewName_rear_1D_w1_W2_t1_T2")

        if AnalysisDataService.doesExist("NewName_rear_1D_w1_W2_t1_T2"):
            AnalysisDataService.remove("NewName_rear_1D_w1_W2_t1_T2")

    def test_for_a_group_workspace_renames_entire_group_if_starts_with_number(self):
        self._create_sample_workspace(name="12345rear_1D_w1_W2_t0_T1")
        self._create_sample_workspace(name="12345rear_1D_w1_W2_t1_T2")
        workspace_name = "12345rear_1D_w1_W2"
        GroupWorkspaces(InputWorkspaces=["12345rear_1D_w1_W2_t0_T1", "12345rear_1D_w1_W2_t1_T2"], OutputWorkspace=workspace_name)

        out_name = su.rename_workspace_correctly("SANS2D", su.ReducedType.LAB, "test", workspace_name)
        self.assertTrue(AnalysisDataService.doesExist("test_rear_1D_w1_W2"))
        self.assertTrue(AnalysisDataService.doesExist("test_rear_1D_w1_W2_t0_T1"))
        self.assertTrue(AnalysisDataService.doesExist("test_rear_1D_w1_W2_t1_T2"))
        self.assertEqual(out_name, "test_rear_1D_w1_W2")

        AnalysisDataService.remove("test_rear_1D_w1_W2_t0_T1")
        AnalysisDataService.remove("test_rear_1D_w1_W2_t1_T2")
        AnalysisDataService.remove("test_rear_1D_w1_W2")


class TestEventWorkspaceCheck(unittest.TestCase):
    def test_that_can_identify_event_workspace(self):
        file_name = FileFinder.findRuns("SANS2D00022048")[0]
        self.assertTrue(su.can_load_as_event_workspace(file_name))

    def test_that_can_identify_histo_workspace_as_not_being_event_workspace(self):
        file_name = FileFinder.findRuns("SANS2D00022024")[0]
        self.assertFalse(su.can_load_as_event_workspace(file_name))


if __name__ == "__main__":
    unittest.main()
