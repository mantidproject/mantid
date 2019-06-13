# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest

from mantid.api import FrameworkManager
from mantid.kernel import DateAndTime
from mantid.dataobjects import Workspace2D
from sans.common.general_functions import (create_unmanaged_algorithm)
from sans.test_helper.test_director import TestDirector
from sans.state.slice_event import get_slice_event_builder


def provide_workspace_with_proton_charge(is_event=True):
    sample_name = "CreateSampleWorkspace"
    sample_options = {"OutputWorkspace": "dummy",
                      "NumBanks": 1,
                      "BankPixelWidth": 2}
    if is_event:
        sample_options.update({"WorkspaceType": "Event"})
    else:
        sample_options.update({"WorkspaceType": "Histogram"})

    sample_alg = create_unmanaged_algorithm(sample_name, **sample_options)
    sample_alg.execute()
    workspace = sample_alg.getProperty("OutputWorkspace").value

    # Provide a proton charge
    log_name = "AddTimeSeriesLog"
    log_options = {"Workspace": workspace,
                   "Name": "proton_charge",
                   "Type": "double"}
    log_alg = create_unmanaged_algorithm(log_name, **log_options)
    time = DateAndTime("2010-01-01T00:10:00")
    for index in range(0, 10):
        time += 1000000000
        value = 1.0
        log_alg.setProperty("Time", str(time))
        log_alg.setProperty("Value", value)
        log_alg.execute()
    return workspace


class SANSSliceEventTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    @staticmethod
    def _provide_workspaces(is_event=True):
        workspace = provide_workspace_with_proton_charge(is_event=is_event)
        monitor_workspace = provide_workspace_with_proton_charge(is_event=False)
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
        return state.property_manager

    def test_that_histogram_workspace_is_not_sliced(self):
        # Arrange
        workspace, monitor_workspace = SANSSliceEventTest._provide_workspaces(is_event=False)
        state = SANSSliceEventTest._get_state(start_time=[1.0], end_time=[2.0])

        # Act
        slice_name = "SANSSliceEvent"
        slice_options = {"SANSState": state,
                         "InputWorkspaceMonitor": monitor_workspace,
                         "InputWorkspace": workspace,
                         "OutputWorkspace": "dummy",
                         "OutputWorkspaceMonitor": "dummyMonitor"}
        slice_alg = create_unmanaged_algorithm(slice_name, **slice_options)
        slice_alg.execute()

        output_workspace = slice_alg.getProperty("OutputWorkspace").value
        output_workspace_monitor = slice_alg.getProperty("OutputWorkspaceMonitor").value
        slice_factor = slice_alg.getProperty("SliceEventFactor").value

        # Assert
        # We expect a scale factor of 1 and an output workspace which is Workspace2D
        self.assertEqual(slice_factor,  1.0)
        self.assertTrue(isinstance(output_workspace, Workspace2D))
        self.assertTrue(isinstance(output_workspace_monitor, Workspace2D))

    def test_that_event_workspace_of_isis_instrument_is_sliced(self):
        # Arrange
        workspace, monitor_workspace = SANSSliceEventTest._provide_workspaces(is_event=True)
        state = SANSSliceEventTest._get_state(start_time=[1.0], end_time=[3.0])

        # Act
        slice_name = "SANSSliceEvent"
        slice_options = {"SANSState": state,
                         "InputWorkspaceMonitor": monitor_workspace,
                         "InputWorkspace": workspace,
                         "OutputWorkspace": "dummy",
                         "OutputWorkspaceMonitor": "dummyMonitor"}
        slice_alg = create_unmanaged_algorithm(slice_name, **slice_options)
        slice_alg.execute()

        output_workspace = slice_alg.getProperty("OutputWorkspace").value
        output_workspace_monitor = slice_alg.getProperty("OutputWorkspaceMonitor").value
        slice_factor = slice_alg.getProperty("SliceEventFactor").value

        # Assert
        self.assertEqual(slice_factor,  0.2)
        self.assertLess(output_workspace.getNumberEvents(), workspace.getNumberEvents())
        self.assertLess(output_workspace_monitor.dataY(0)[0], monitor_workspace.dataY(0)[0])


if __name__ == '__main__':
    unittest.main()
