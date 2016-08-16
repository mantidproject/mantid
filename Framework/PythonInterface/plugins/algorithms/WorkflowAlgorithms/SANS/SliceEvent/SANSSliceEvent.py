# pylint: disable=too-few-public-methods

""" SANSSliceEvent takes out a slice from a event workspace."""

from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator,
                           FloatArrayProperty)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress)

from SANS.SliceEvent.Slicer import (SliceEventFactory, get_scaled_workspace)
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSFunctions import append_to_sans_file_tag
from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager


class SANSSliceEvent(DataProcessorAlgorithm):
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
        self.declareProperty(MatrixWorkspaceProperty(SANSConstants.input_workspace, '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The input workspace. If it is an event workspace, then the slice is taken. '
                                 'In case of a Workspace2D the original workspace is returned')

        self.declareProperty(MatrixWorkspaceProperty("InputWorkspaceMonitor", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The monitor workspace associated with the main input workspace.')

        # ---------------
        # OUTPUT
        # ---------------
        self.declareProperty('SliceEventFactor', defaultValue=1.0,
                             direction=Direction.Output,
                             doc='The factor of the event slicing. This corresponds to the proportion of the the total '
                                 'proton charge, which the slice corresponds to.')

        self.declareProperty(MatrixWorkspaceProperty(SANSConstants.output_workspace, '', direction=Direction.Output),
                             doc='The sliced workspace')

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspaceMonitor", '', direction=Direction.Output),
                             doc='The output monitor workspace which has the correct slice factor applied to it.')

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)

        progress = Progress(self, start=0.0, end=1.0, nreports=3)
        # Get the correct SANS move strategy from the SANSMoveFactory
        input_workspace = self.getProperty(SANSConstants.input_workspace).value
        slicer = SliceEventFactory.create_slicer(state, input_workspace)
        slice_info = state.slice

        # Perform the slicing
        progress.report("Starting to slice the workspace.")
        sliced_workspace, slice_factor = slicer.create_slice(input_workspace, slice_info)

        # Scale the monitor accordingly
        progress.report("Scaling the monitors.")
        self.scale_monitors(slice_factor)

        # Set the outputs
        append_to_sans_file_tag(sliced_workspace, "_sliced")
        self.setProperty(SANSConstants.output_workspace, sliced_workspace)
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
            errors.update({"SANSSMove": str(err)})
        return errors

    def scale_monitors(self, slice_factor):
        monitor_workspace = self.getProperty("InputWorkspaceMonitor").value
        if slice_factor < 1.0:
            monitor_workspace = get_scaled_workspace(monitor_workspace, slice_factor)
        append_to_sans_file_tag(monitor_workspace, "_sliced")
        self.setProperty("OutputWorkspaceMonitor", monitor_workspace)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSSliceEvent)
