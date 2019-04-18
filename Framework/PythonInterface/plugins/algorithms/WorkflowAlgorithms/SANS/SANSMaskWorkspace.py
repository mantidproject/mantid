# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

""" SANSMaskWorkspace algorithm applies the masks of SANSMask state to a workspace."""

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DistributedDataProcessorAlgorithm,
                        MatrixWorkspaceProperty, Progress, PropertyMode)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)

from sans.algorithm_detail.mask_workspace import create_masker
from sans.common.enums import DetectorType
from sans.common.general_functions import append_to_sans_file_tag
from sans.state.state_base import create_deserialized_sans_state_from_property_manager


class SANSMaskWorkspace(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Mask'

    def summary(self):
        return 'Masks a SANS workspaces.'

    def PyInit(self):
        # State
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        # Workspace which is to be masked
        self.declareProperty(MatrixWorkspaceProperty("Workspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.InOut),
                             doc='The sample scatter workspace. This workspace does not contain monitors.')

        # The component, i.e. HAB or LAB
        allowed_detectors = StringListValidator([DetectorType.to_string(DetectorType.LAB),
                                                 DetectorType.to_string(DetectorType.HAB)])
        self.declareProperty("Component", DetectorType.to_string(DetectorType.LAB), validator=allowed_detectors,
                             direction=Direction.Input,
                             doc="The component of the instrument which is to be masked.")

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)

        component = self._get_component()

        # Get the correct SANS masking strategy from create_masker
        workspace = self.getProperty("Workspace").value
        masker = create_masker(state, component)

        # Perform the masking
        number_of_masking_options = 7
        progress = Progress(self, start=0.0, end=1.0, nreports=number_of_masking_options)
        mask_info = state.mask
        workspace = masker.mask_workspace(mask_info, workspace, component, progress)

        append_to_sans_file_tag(workspace, "_masked")
        self.setProperty("Workspace", workspace)
        progress.report("Completed masking the workspace")

    def _get_component(self):
        component_as_string = self.getProperty("Component").value
        return DetectorType.from_string(component_as_string)

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
