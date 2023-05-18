# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.dataobjects import Workspace2D
from mantid.kernel import DateAndTime
from mantid.simpleapi import AddTimeSeriesLog, CreateSampleWorkspace
from sans.algorithm_detail.slice_sans_event import slice_sans_event
from sans.state.StateObjects.StateSliceEvent import get_slice_event_builder
from sans.test_helper.test_director import TestDirector


def provide_workspace_with_proton_charge(output_name, is_event=True):
    ws_type = "Event" if is_event else "Histogram"

    # AddTimeSeriesLog forces us to store in ADS
    dummy_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2, WorkspaceType=ws_type, OutputWorkspace=output_name)
    # Provide a proton charge
    time = DateAndTime("2010-01-01T00:10:00")
    value = 1.0
    for index in range(0, 10):
        time += 1000000000
        AddTimeSeriesLog(Workspace=dummy_ws, Name="proton_charge", Type="double", Time=str(time), Value=value)
    return dummy_ws


class SANSSliceEventTest(unittest.TestCase):
    @staticmethod
    def _provide_workspaces(is_event=True):
        workspace = provide_workspace_with_proton_charge(is_event=is_event, output_name="ws")
        monitor_workspace = provide_workspace_with_proton_charge(is_event=False, output_name="monitor")
        return workspace, monitor_workspace

    @staticmethod
    def _get_state(start_time=None, end_time=None):
        test_director = TestDirector()
        state = test_director.construct()

        # Create the slice range if required
        if start_time is not None and end_time is not None:
            data_state = state.data
            slice_builder = get_slice_event_builder(data_state)
            slice_builder.set_start_time(start_time)
            slice_builder.set_end_time(end_time)
            slice_state = slice_builder.build()
            state.slice = slice_state
        return state

    def test_that_histogram_workspace_is_not_sliced(self):
        workspace, monitor_workspace = self._provide_workspaces(is_event=False)
        state = self._get_state(start_time=[1.0], end_time=[2.0])

        returned = slice_sans_event(state_slice=state.slice, input_ws=workspace, input_ws_monitor=monitor_workspace)

        output_workspace = returned["OutputWorkspace"]
        output_workspace_monitor = returned["OutputWorkspaceMonitor"]
        slice_factor = returned["SliceEventFactor"]

        # We expect a scale factor of 1 and an output workspace which is Workspace2D
        self.assertEqual(slice_factor, 1.0)
        self.assertTrue(isinstance(output_workspace, Workspace2D))
        self.assertTrue(isinstance(output_workspace_monitor, Workspace2D))

    def test_that_event_workspace_of_isis_instrument_is_sliced(self):
        workspace, monitor_workspace = self._provide_workspaces(is_event=True)
        state = self._get_state(start_time=[1.0], end_time=[3.0])  # slice from 1-3 seconds of the full ten

        returned = slice_sans_event(state_slice=state.slice, input_ws=workspace, input_ws_monitor=monitor_workspace)

        output_workspace = returned["OutputWorkspace"]
        output_workspace_monitor = returned["OutputWorkspaceMonitor"]
        slice_factor = returned["SliceEventFactor"]

        self.assertEqual(slice_factor, 0.2)
        self.assertLess(output_workspace.getNumberEvents(), workspace.getNumberEvents())
        self.assertLess(output_workspace_monitor.dataY(0)[0], monitor_workspace.dataY(0)[0])


if __name__ == "__main__":
    unittest.main()
