# pylint: disable=too-few-public-methods

""" SANSMaskWorkspace algorithm applies the masks of SANSMask state to a workspace."""

from mantid.kernel import (Direction, StringListValidator, Property)
from mantid.dataobjects import EventWorkspace
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress)
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSFunctions import (create_unmanaged_algorithm, append_to_sans_file_tag)


class SANSConvertToWavelength(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Mask'

    def summary(self):
        return 'Convert the units of a SANS workspace to wavelength'

    def PyInit(self):
        # Workspace which is to be masked
        self.declareProperty(MatrixWorkspaceProperty(SANSConstants.input_workspace, '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The workspace which is to be converted to wavelength')

        self.declareProperty('WavelengthLow', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='The low value of the wavelength binning.')
        self.declareProperty('WavelengthHigh', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='The high value of the wavelength binning.')
        self.declareProperty('WavelengthStep', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='The step size of the wavelength binning.')

        # Step type
        allowed_step_types = StringListValidator(["LOG", "LIN"])
        self.declareProperty('WavelengthStepType', "LIN", validator=allowed_step_types, direction=Direction.Input,
                             doc='The step type for rebinning.')

        # Rebin type
        allowed_rebin_methods = StringListValidator(["Rebin", "InterpolatingRebin"])
        self.declareProperty("RebinMode", "Rebin", validator=allowed_rebin_methods, direction=Direction.Input,
                             doc="The method which is to be applied to the rebinning.")

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc='The output workspace.')

    def PyExec(self):
        workspace = self.getProperty(SANSConstants.input_workspace).value
        progress = Progress(self, start=0.0, end=1.0, nreports=3)

        # Convert the units into wavelength
        progress.report("Converting workspace to wavelength units.")
        workspace = self._convert_units_to_wavelength(workspace)

        # Get the rebin option
        rebin_name = self.getProperty("RebinMode").value
        rebin_string = self._get_rebin_string(workspace)
        if rebin_name == "Rebin":
            rebin_options = {SANSConstants.input_workspace: workspace,
                             SANSConstants.output_workspace: SANSConstants.dummy,
                             "PreserveEvents": True,
                             "Params": rebin_string}
        else:
            rebin_options = {SANSConstants.input_workspace: workspace,
                             SANSConstants.output_workspace: SANSConstants.dummy,
                             "Params": rebin_string}
        # # Perform the rebin
        progress.report("Performing rebin.")
        workspace = self._perform_rebin(rebin_name, rebin_options)
        append_to_sans_file_tag(workspace, "_toWavelength")
        self.setProperty(SANSConstants.output_workspace, workspace)
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
        workspace = self.getProperty(SANSConstants.input_workspace).value
        rebin_mode = self.getProperty("RebinMode").value
        if rebin_mode == "InterpolatingRebin" and isinstance(workspace, EventWorkspace):
            errors.update({"RebinMode": "An interpolating rebin cannot be applied to an EventWorkspace."})
        return errors

    def _convert_units_to_wavelength(self, workspace):

        convert_name = "ConvertUnits"
        convert_options = {SANSConstants.input_workspace: workspace,
                           SANSConstants.output_workspace: SANSConstants.dummy,
                           "Target": "Wavelength"}
        convert_alg = create_unmanaged_algorithm(convert_name, **convert_options)
        convert_alg.execute()
        return convert_alg.getProperty(SANSConstants.output_workspace).value

    def _get_rebin_string(self, workspace):
        wavelength_low = self.getProperty("WavelengthLow").value
        wavelength_high = self.getProperty("WavelengthHigh").value

        # If the wavelength has not been specified, the get it from the workspace. Only the first spectrum is checked
        # The the lowest wavelength value is to be found in the spectrum located at workspaces index 0 is a very
        # strong assumption, but it existed in the previous implementation.
        if wavelength_low is None or wavelength_low == Property.EMPTY_DBL:
            wavelength_low = min(workspace.readX(0))

        if wavelength_high is None or wavelength_high == Property.EMPTY_DBL:
            wavelength_high = max(workspace.readX(0))

        wavelength_step = self.getProperty("WavelengthStep").value
        step_type = self.getProperty("WavelengthStepType").value
        pre_factor = -1 if step_type == "LOG" else 1
        wavelength_step *= pre_factor
        return str(wavelength_low) + "," + str(wavelength_step) + "," + str(wavelength_high)

    def _perform_rebin(self, rebin_name, rebin_options):
        rebin_alg = create_unmanaged_algorithm(rebin_name, **rebin_options)
        rebin_alg.execute()
        return rebin_alg.getProperty(SANSConstants.output_workspace).value


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSConvertToWavelength)
