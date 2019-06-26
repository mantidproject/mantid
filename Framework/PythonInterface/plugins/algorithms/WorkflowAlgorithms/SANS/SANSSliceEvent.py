# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

""" SANSSliceEvent takes out a slice from a event workspace."""

from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)
from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress)

from sans.algorithm_detail.slicer import (SliceEventFactory, get_scaled_workspace)
from sans.common.general_functions import append_to_sans_file_tag
from sans.common.enums import DataType
from sans.state.state_base import create_deserialized_sans_state_from_property_manager


class SANSSliceEvent(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\SliceEvent'

    def summary(self):
        return 'Takes an event slice from an event workspace'

    def PyInit(self):
        # ---------------
        # INPUT
        # ---------------
        # State
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        # Workspace which is to be moved
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The input workspace. If it is an event workspace, then the slice is taken. '
                                 'In case of a Workspace2D the original workspace is returned')

        self.declareProperty(MatrixWorkspaceProperty("InputWorkspaceMonitor", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The monitor workspace associated with the main input workspace.')

        # The data type
        allowed_data = StringListValidator([DataType.Sample.name,
                                            DataType.Can.name])
        self.declareProperty("DataType", DataType.Sample.name,
                             validator=allowed_data, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

        # ---------------
        # OUTPUT
        # ---------------
        self.declareProperty('SliceEventFactor', defaultValue=1.0,
                             direction=Direction.Output,
                             doc='The factor of the event slicing. This corresponds to the proportion of the the total '
                                 'proton charge, which the slice corresponds to.')

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", '', direction=Direction.Output),
                             doc='The sliced workspace')

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspaceMonitor", '', direction=Direction.Output),
                             doc='The output monitor workspace which has the correct slice factor applied to it.')

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)

        progress = Progress(self, start=0.0, end=1.0, nreports=3)
        input_workspace = self.getProperty("InputWorkspace").value

        data_type_as_string = self.getProperty("DataType").value
        data_type = DataType[data_type_as_string]

        slicer = SliceEventFactory.create_slicer(state, input_workspace, data_type)
        slice_info = state.slice

        # Perform the slicing
        progress.report("Starting to slice the workspace.")
        sliced_workspace, slice_factor = slicer.create_slice(input_workspace, slice_info)

        # Scale the monitor accordingly
        progress.report("Scaling the monitors.")
        self.scale_monitors(slice_factor)

        # Set the outputs
        append_to_sans_file_tag(sliced_workspace, "_sliced")
        self.setProperty("OutputWorkspace", sliced_workspace)
        self.setProperty("SliceEventFactor", slice_factor)
        progress.report("Finished slicing.")

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        state_property_manager = self.getProperty("SANSState").value
        try:
            state = create_deserialized_sans_state_from_property_manager(state_property_manager)
            state.property_manager = state_property_manager
            state.validate()
        except ValueError as err:
            errors.update({"SANSSliceEvent": str(err)})
        return errors

    def scale_monitors(self, slice_factor):
        monitor_workspace = self.getProperty("InputWorkspaceMonitor").value
        if slice_factor < 1.0:
            monitor_workspace = get_scaled_workspace(monitor_workspace, slice_factor)
        append_to_sans_file_tag(monitor_workspace, "_sliced")
        self.setProperty("OutputWorkspaceMonitor", monitor_workspace)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSSliceEvent)
