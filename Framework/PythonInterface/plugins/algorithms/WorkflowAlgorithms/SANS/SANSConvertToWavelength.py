# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

""" SANSConvertToWavelength converts to wavelength units """
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, PropertyManagerProperty)
from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode)
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import (create_unmanaged_algorithm, append_to_sans_file_tag,
                                           get_input_workspace_as_copy_if_not_same_as_output_workspace)
from sans.common.enums import (RangeStepType, RebinType)
from sans.state.state_base import create_deserialized_sans_state_from_property_manager


class SANSConvertToWavelength(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Wavelength'

    def summary(self):
        return 'Convert the units of a SANS workspace to wavelength'

    def PyInit(self):
        # State
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The workspace which is to be converted to wavelength')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc='The output workspace.')

    def PyExec(self):
        # State
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)
        wavelength_state = state.wavelength

        # Input workspace
        workspace = get_input_workspace_as_copy_if_not_same_as_output_workspace(self)

        wavelength_name = "SANSConvertToWavelengthAndRebin"
        wavelength_options = {"InputWorkspace": workspace,
                              "WavelengthLow": wavelength_state.wavelength_low[0],
                              "WavelengthHigh": wavelength_state.wavelength_high[0],
                              "WavelengthStep": wavelength_state.wavelength_step,
                              "WavelengthStepType": wavelength_state.wavelength_step_type.name,
                              "RebinMode": wavelength_state.rebin_type.name}
        wavelength_alg = create_unmanaged_algorithm(wavelength_name, **wavelength_options)
        wavelength_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        wavelength_alg.setProperty("OutputWorkspace", workspace)
        wavelength_alg.execute()
        converted_workspace = wavelength_alg.getProperty("OutputWorkspace").value
        append_to_sans_file_tag(converted_workspace, "_wavelength")
        self.setProperty("OutputWorkspace", converted_workspace)

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


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSConvertToWavelength)
