# pylint: disable=too-few-public-methods

""" SANSMaskWorkspace algorithm applies the masks of SANSMask state to a workspace."""

from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress)

from SANS.Mask.MaskWorkspace import MaskFactory
from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSEnumerations import DetectorType
from SANS2.Common.SANSFunctions import append_to_sans_file_tag


class SANSMaskWorkspace(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Mask'

    def summary(self):
        return 'Masks a SANS workspaces.'

    def PyInit(self):
        # State
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        # Workspace which is to be masked
        self.declareProperty(MatrixWorkspaceProperty(SANSConstants.workspace, '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.InOut),
                             doc='The sample scatter workspace. This workspace does not contain monitors.')

        # The component, i.e. HAB or LAB
        allowed_detectors = StringListValidator(["LAB", "HAB"])
        self.declareProperty("Component", "LAB", validator=allowed_detectors, direction=Direction.Input,
                             doc="The component of the instrument which is to be masked.")

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)

        component = self._get_component()

        # Get the correct SANS masking strategy from the SANSMaskFactory
        workspace = self.getProperty(SANSConstants.workspace).value
        mask_factory = MaskFactory()
        masker = mask_factory.create_masker(state, component)

        # Perform the masking
        number_of_masking_options = 7
        progress = Progress(self, start=0.0, end=1.0, nreports=number_of_masking_options)
        mask_info = state.mask
        workspace = masker.mask_workspace(mask_info, workspace, component, progress)

        append_to_sans_file_tag(workspace, "_masked")
        self.setProperty(SANSConstants.workspace, workspace)
        progress.report("Completed masking the workspace")

    def _get_component(self):
        component_as_string = self.getProperty("Component").value
        if component_as_string == "HAB":
            component = DetectorType.Hab
        else:
            component = DetectorType.Lab
        return component

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        state_property_manager = self.getProperty("SANSState").value
        try:
            state = create_deserialized_sans_state_from_property_manager(state_property_manager)
            state.property_manager = state_property_manager
            state.validate()
        except ValueError as err:
            errors.update({"SANSSMask": str(err)})
        return errors


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSMaskWorkspace)
