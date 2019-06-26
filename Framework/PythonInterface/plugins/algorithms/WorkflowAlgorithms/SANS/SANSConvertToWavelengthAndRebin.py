# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

""" SANSConvertToWavelengthAndRebin algorithm converts to wavelength units and performs a rebin."""

from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, StringListValidator, Property)
from mantid.dataobjects import EventWorkspace
from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress)
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import (create_unmanaged_algorithm, append_to_sans_file_tag,
                                           get_input_workspace_as_copy_if_not_same_as_output_workspace)
from sans.common.enums import (RebinType, RangeStepType)


class SANSConvertToWavelengthAndRebin(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Wavelength'

    def summary(self):
        return 'Convert the units of time-of-flight workspace to units of wavelength and performs a rebin.'

    def PyInit(self):
        # Workspace which is to be masked
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The workspace which is to be converted to wavelength')

        self.declareProperty('WavelengthLow', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='The low value of the wavelength binning.')
        self.declareProperty('WavelengthHigh', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='The high value of the wavelength binning.')
        self.declareProperty('WavelengthStep', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='The step size of the wavelength binning.')

        # Step type
        allowed_step_types = StringListValidator([RangeStepType.Log.name,
                                                  RangeStepType.Lin.name])
        self.declareProperty('WavelengthStepType', RangeStepType.Lin.name,
                             validator=allowed_step_types, direction=Direction.Input,
                             doc='The step type for rebinning.')

        # Rebin type
        allowed_rebin_methods = StringListValidator([RebinType.Rebin.name,
                                                     RebinType.InterpolatingRebin.name])
        self.declareProperty("RebinMode", RebinType.Rebin.name,
                             validator=allowed_rebin_methods, direction=Direction.Input,
                             doc="The method which is to be applied to the rebinning.")

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc='The output workspace.')

    def PyExec(self):
        workspace = get_input_workspace_as_copy_if_not_same_as_output_workspace(self)

        progress = Progress(self, start=0.0, end=1.0, nreports=3)

        # Convert the units into wavelength
        progress.report("Converting workspace to wavelength units.")
        workspace = self._convert_units_to_wavelength(workspace)

        # Get the rebin option
        rebin_type = RebinType[self.getProperty("RebinMode").value]
        rebin_string = self._get_rebin_string(workspace)
        if rebin_type is RebinType.Rebin:
            rebin_options = {"InputWorkspace": workspace,
                             "PreserveEvents": True,
                             "Params": rebin_string}
        else:
            rebin_options = {"InputWorkspace": workspace,
                             "Params": rebin_string}

        # Perform the rebin
        progress.report("Performing rebin.")
        workspace = self._perform_rebin(rebin_type, rebin_options, workspace)

        append_to_sans_file_tag(workspace, "_toWavelength")
        self.setProperty("OutputWorkspace", workspace)
        progress.report("Finished converting to wavelength.")

    def validateInputs(self):
        errors = dict()
        # Check the wavelength
        wavelength_low = self.getProperty("WavelengthLow").value
        wavelength_high = self.getProperty("WavelengthHigh").value
        if wavelength_low is not None and wavelength_high is not None and wavelength_low > wavelength_high:
            errors.update({"WavelengthLow": "The lower wavelength setting needs to be smaller "
                                            "than the higher wavelength setting."})

        if wavelength_low is not None and wavelength_low < 0:
            errors.update({"WavelengthLow": "The wavelength cannot be smaller than 0."})

        if wavelength_high is not None and wavelength_high < 0:
            errors.update({"WavelengthHigh": "The wavelength cannot be smaller than 0."})

        wavelength_step = self.getProperty("WavelengthStep").value
        if wavelength_step is not None and wavelength_step < 0:
            errors.update({"WavelengthStep": "The wavelength step cannot be smaller than 0."})

        # Check the workspace
        workspace = self.getProperty("InputWorkspace").value
        rebin_type = RebinType[self.getProperty("RebinMode").value]
        if rebin_type is RebinType.InterpolatingRebin and isinstance(workspace, EventWorkspace):
            errors.update({"RebinMode": "An interpolating rebin cannot be applied to an EventWorkspace."})
        return errors

    def _convert_units_to_wavelength(self, workspace):
        convert_name = "ConvertUnits"
        convert_options = {"InputWorkspace": workspace,
                           "Target": "Wavelength"}
        convert_alg = create_unmanaged_algorithm(convert_name, **convert_options)
        convert_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        convert_alg.setProperty("OutputWorkspace", workspace)
        convert_alg.execute()
        return convert_alg.getProperty("OutputWorkspace").value

    def _get_rebin_string(self, workspace):
        wavelength_low = self.getProperty("WavelengthLow").value
        wavelength_high = self.getProperty("WavelengthHigh").value

        # If the wavelength has not been specified, then get it from the workspace. Only the first spectrum is checked
        # The lowest wavelength value is to be found in the spectrum located at workspaces index 0 is a very
        # strong assumption, but it existed in the previous implementation.
        if wavelength_low is None or wavelength_low == Property.EMPTY_DBL:
            wavelength_low = min(workspace.readX(0))

        if wavelength_high is None or wavelength_high == Property.EMPTY_DBL:
            wavelength_high = max(workspace.readX(0))

        wavelength_step = self.getProperty("WavelengthStep").value
        step_type = RangeStepType[self.getProperty("WavelengthStepType").value]
        pre_factor = -1 if step_type == RangeStepType.Log else 1
        wavelength_step *= pre_factor
        return str(wavelength_low) + "," + str(wavelength_step) + "," + str(wavelength_high)

    def _perform_rebin(self, rebin_type, rebin_options, workspace):
        rebin_name = "Rebin" if rebin_type is RebinType.Rebin else "InterpolatingRebin"
        rebin_alg = create_unmanaged_algorithm(rebin_name, **rebin_options)
        rebin_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        rebin_alg.setProperty("OutputWorkspace", workspace)
        rebin_alg.execute()
        return rebin_alg.getProperty("OutputWorkspace").value


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSConvertToWavelengthAndRebin)
