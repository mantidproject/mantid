# pylint: disable=invalid-name

""" SANSReductionCore algorithm runs the sequence of reduction steps which are necessary to reduce a data set."""

from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode)

from SANS2.Common.SANSConstants import SANSConstants
from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS2.Common.SANSFunctions import create_unmanaged_algorithm


class SANSReductionCore(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Reduction'

    def summary(self):
        return ' Runs the the core reduction elements.'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        # WORKSPACES
        # Scatter Workspaces
        self.declareProperty(MatrixWorkspaceProperty('ScatterWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The scatter workspace. This workspace does not contain monitors.')
        self.declareProperty(MatrixWorkspaceProperty('ScatterMonitorWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The scatter monitor workspace. This workspace only contains monitors.')

        # Transmission Workspace
        self.declareProperty(MatrixWorkspaceProperty('TransmissionWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The transmission workspace.')

        # Direct Workspace
        self.declareProperty(MatrixWorkspaceProperty('DirectWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The direct workspace.')

        self.setPropertyGroup("ScatterWorkspace", 'Data')
        self.setPropertyGroup("ScatterMonitorWorkspace", 'Data')
        self.setPropertyGroup("TransmissionWorkspace", 'Data')
        self.setPropertyGroup("DirectWorkspace", 'Data')

        self.declareProperty("UseOptimizations", True, direction=Direction.Input,
                             doc="When enabled the ADS is being searched for already loaded and reduced workspaces. "
                                 "Depending on your concrete reduction, this could provide a significant"
                                 " performance boost")

        allowed_detectors = StringListValidator(["LAB", "HAB"])
        self.declareProperty("Component", "LAB", validator=allowed_detectors, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

        # ----------
        # OUTPUT
        # ----------
        self.declareProperty(MatrixWorkspaceProperty(SANSConstants.output_workspace, '', direction=Direction.Output),
                             doc='The output workspace.')

        self.declareProperty(MatrixWorkspaceProperty('SumOfCounts', '', optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc='The sum of the counts of the output workspace.')

        self.declareProperty(MatrixWorkspaceProperty('SumOfNormFactors', '', optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc='The sum of the counts of the output workspace.')

    def PyExec(self):
        # Get the input
        state = self._get_state()
        component_as_string = self.getProperty("Component").value

        # --------------------------------------------------------------------------------------------------------------
        # 1. Crop workspace by detector name
        #    This will create a reduced copy of the original workspace with only those spectra which are relevant
        #    for this particular reduction.
        # --------------------------------------------------------------------------------------------------------------
        workspace = self._get_cropped_workspace(component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 2. Perform dark run subtraction
        #    This will subtract a dark background from the scatter workspace. Note that dark background subtraction
        #    will also affect the transmission calculation later on.
        # --------------------------------------------------------------------------------------------------------------

        # --------------------------------------------------------------------------------------------------------------
        # 3. Create event slice
        #    If we are dealing with an event workspace as input, this will cut out a time-based (use-defined) slice.
        #    In case of a histogram workspace, nothing happens.
        # --------------------------------------------------------------------------------------------------------------
        monitor_workspace = self._get_monitor_workspace()
        workspace, monitor_workspace, slice_event_factor = self._slice(state, workspace, monitor_workspace)

        # ------------------------------------------------------------
        # 4. Move the workspace into the correct position
        #    The detectors in the workspaces are set such that the beam centre is at (0,0). The position is
        #    a user-specified value which can be obtained with the help of the beam centre finder.
        # ------------------------------------------------------------
        workspace = self._move(state, workspace, component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 5. Apply masking (pixel masking and time masking)
        # --------------------------------------------------------------------------------------------------------------
        workspace = self._mask(state, workspace, component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 6. Convert to Wavelength
        # --------------------------------------------------------------------------------------------------------------

        # --------------------------------------------------------------------------------------------------------------
        # 7. Multiply by volume and absolute scale
        # --------------------------------------------------------------------------------------------------------------

        # ------------------------------------------------------------
        # 8. Create adjustment workspaces, those are
        #     1. pixel-based adjustments
        #     2. wavelength-based adjustments
        #     3. pixel-and-wavelength-based adjustments
        # ------------------------------------------------------------

        # ------------------------------------------------------------
        # 9. Convert event workspaces to histogram workspaces
        # ------------------------------------------------------------

        # ------------------------------------------------------------
        # 10. Convert to Q
        # ------------------------------------------------------------

        # ------------------------------------------------------------
        # ------------------------------------------------------------
        # Populate the output
        self.setProperty(SANSConstants.output_workspace, workspace)

        # Publish temporary workspaces if required

    def _get_cropped_workspace(self, component):
        scatter_workspace = self.getProperty("ScatterWorkspace").value
        crop_name = "SANSCrop"
        crop_options = {SANSConstants.input_workspace: scatter_workspace,
                        SANSConstants.output_workspace: SANSConstants.dummy,
                        "Component": component}
        crop_alg = create_unmanaged_algorithm(crop_name, **crop_options)
        crop_alg.execute()
        return crop_alg.getProperty(SANSConstants.output_workspace).value

    def _slice(self, state, workspace, monitor_workspace):
        state_serialized = state.property_manager
        slice_name = "SANSSliceEvent"
        slice_options = {"SANSState": state_serialized,
                         SANSConstants.input_workspace: workspace,
                         "InputWorkspaceMonitor": monitor_workspace,
                         SANSConstants.output_workspace: SANSConstants.dummy,
                         "OutputWorkspaceMonitor": "dummy2"}
        slice_alg = create_unmanaged_algorithm(slice_name, **slice_options)
        slice_alg.execute()

        workspace = slice_alg.getProperty(SANSConstants.output_workspace).value
        monitor_workspace = slice_alg.getProperty("OutputWorkspaceMonitor").value
        slice_event_factor = slice_alg.getProperty("SliceEventFactor").value
        return workspace, monitor_workspace, slice_event_factor

    def _move(self, state, workspace, component):
        # First we set the workspace to zero, since it might have been moved around by the user in the ADS
        # Second we use the initial move to bring the workspace into the correct position
        state_serialized = state.property_manager
        move_name = "SANSMove"
        move_options = {"SANSState": state_serialized,
                        SANSConstants.workspace: workspace,
                        "MoveType": "SetToZero",
                        "Component": ""}
        move_alg = create_unmanaged_algorithm(move_name, **move_options)
        move_alg.execute()
        workspace = move_alg.getProperty(SANSConstants.workspace).value

        # Do the initial move
        move_alg.setProperty("MoveType", "InitialMove")
        move_alg.setProperty("Component", component)
        move_alg.setProperty(SANSConstants.workspace, workspace)
        move_alg.execute()
        return move_alg.getProperty(SANSConstants.workspace).value

    def _mask(self, state, workspace, component):
        state_serialized = state.property_manager
        mask_name = "SANSMaskWorkspace"
        mask_options = {"SANSState": state_serialized,
                        SANSConstants.workspace: workspace,
                        "Component": component}
        mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
        mask_alg.execute()
        return mask_alg.getProperty(SANSConstants.workspace).value

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        try:
            state = self._get_state()
            state.validate()
        except ValueError as err:
            errors.update({"SANSSingleReduction": str(err)})
        return errors

    def _get_state(self):
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)
        state.property_manager = state_property_manager
        return state

    def _get_monitor_workspace(self):
        monitor_workspace = self.getProperty("ScatterMonitorWorkspace").value

        clone_name = "CloneWorkspace"
        clone_options = {SANSConstants.input_workspace: monitor_workspace,
                         SANSConstants.output_workspace: SANSConstants.dummy}
        clone_alg = create_unmanaged_algorithm(clone_name, **clone_options)
        clone_alg.execute()
        return clone_alg.getProperty(SANSConstants.output_workspace).value

# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSReductionCore)
