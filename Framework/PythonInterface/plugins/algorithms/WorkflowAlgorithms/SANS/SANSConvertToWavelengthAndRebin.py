# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""SANSConvertToWavelengthAndRebin algorithm converts to wavelength units and performs a rebin."""

import json
from json import JSONDecodeError
from typing import List, Tuple

from mantid.api import (
    DataProcessorAlgorithm,
    WorkspaceGroupProperty,
    MatrixWorkspaceProperty,
    AlgorithmFactory,
    PropertyMode,
    Progress,
    WorkspaceGroup,
)
from mantid.dataobjects import EventWorkspace
from mantid.kernel import Direction, StringListValidator, Property
from sans_core.common.constants import EMPTY_NAME
from sans_core.common.enums import RebinType, RangeStepType
from sans_core.common.general_functions import (
    create_unmanaged_algorithm,
    append_to_sans_file_tag,
    get_input_workspace_as_copy_if_not_same_as_output_workspace,
)


class SANSConvertToWavelengthAndRebin(DataProcessorAlgorithm):
    WAV_PAIRS = "WavelengthPairs"

    def category(self):
        return "SANS\\Wavelength"

    def summary(self):
        return "Convert the units of time-of-flight workspace to units of wavelength and performs a rebin."

    def PyInit(self):
        # Workspace which is to be masked
        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="The workspace which is to be converted to wavelength",
        )

        self.declareProperty(
            self.WAV_PAIRS,
            defaultValue="",
            direction=Direction.Input,
            doc="A JSON encoded list of wavelength ranges. E.g. [[1., 2.], [2., 3.]]",
        )
        self.declareProperty(
            "WavelengthStep", defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc="The step size of the wavelength binning."
        )

        # Step type
        allowed_step_types = StringListValidator([RangeStepType.LOG.value, RangeStepType.LIN.value])
        self.declareProperty(
            "WavelengthStepType",
            RangeStepType.LIN.value,
            validator=allowed_step_types,
            direction=Direction.Input,
            doc="The step type for rebinning.",
        )

        # Rebin type
        allowed_rebin_methods = StringListValidator([RebinType.REBIN.value, RebinType.INTERPOLATING_REBIN.value])
        self.declareProperty(
            "RebinMode",
            RebinType.REBIN.value,
            validator=allowed_rebin_methods,
            direction=Direction.Input,
            doc="The method which is to be applied to the rebinning.",
        )

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            doc="A grouped workspace containing the output workspaces" " in the same order as the input pairs.",
        )

    def PyExec(self):
        workspace = get_input_workspace_as_copy_if_not_same_as_output_workspace(self)
        wavelength_pairs: List[Tuple[float, float]] = json.loads(self.getProperty(self.WAV_PAIRS).value)
        progress = Progress(self, start=0.0, end=1.0, nreports=1 + len(wavelength_pairs))  # 1 - convert units

        # Convert the units into wavelength
        progress.report("Converting workspace to wavelength units.")
        workspace = self._convert_units_to_wavelength(workspace)

        # Get the rebin option
        output_group = WorkspaceGroup()
        for pair in wavelength_pairs:
            rebin_string = self._get_rebin_string(workspace, *pair)
            progress.report(f"Converting wavelength range: {rebin_string}")

            # Perform the rebin
            rebin_options = self._get_rebin_params(rebin_string, workspace)
            out_ws = self._perform_rebin(rebin_options)

            append_to_sans_file_tag(out_ws, "_toWavelength")
            output_group.addWorkspace(out_ws)
        self.setProperty("OutputWorkspace", output_group)

    def _get_rebin_params(self, rebin_string, workspace):
        rebin_type = RebinType(self.getProperty("RebinMode").value)
        if rebin_type is RebinType.REBIN:
            rebin_options = {"InputWorkspace": workspace, "PreserveEvents": True, "Params": rebin_string}
        else:
            rebin_options = {"InputWorkspace": workspace, "Params": rebin_string}
        return rebin_options

    def validateInputs(self):
        errors = dict()
        # Check the wavelength
        wavelength_json = self.getProperty(self.WAV_PAIRS).value
        try:
            wavelengths: List[Tuple[float, float]] = json.loads(wavelength_json)
        except JSONDecodeError as e:
            errors.update({self.WAV_PAIRS: f"Failed to decode JSON. Exception was: {e}"})
            return errors

        if not all(isinstance(internal_item, list) for internal_item in wavelengths):
            errors.update(
                {
                    self.WAV_PAIRS: "A list of pairs (i.e. list of lists) is required."
                    " A single pair must be container within an outer list too."
                }
            )
            return errors  # The below checks aren't really possible as we don't have a clue what we have now

        def _check_individual_pair(wavelength_low, wavelength_high):
            if wavelength_low is not None and wavelength_high is not None and wavelength_low > wavelength_high:
                errors.update({self.WAV_PAIRS: "The lower wavelength setting needs to be smaller " "than the higher wavelength setting."})
            if wavelength_low is not None and wavelength_low < 0:
                errors.update({self.WAV_PAIRS: "The wavelength cannot be smaller than 0."})
            if wavelength_high is not None and wavelength_high < 0:
                errors.update({self.WAV_PAIRS: "The wavelength cannot be smaller than 0."})

        for pair in wavelengths:
            _check_individual_pair(*pair)

        wavelength_step = self.getProperty("WavelengthStep").value
        if wavelength_step is not None and wavelength_step < 0:
            errors.update({"WavelengthStep": "The wavelength step cannot be smaller than 0."})

        # Check the workspace
        workspace = self.getProperty("InputWorkspace").value
        rebin_type = RebinType(self.getProperty("RebinMode").value)
        if rebin_type is RebinType.INTERPOLATING_REBIN and isinstance(workspace, EventWorkspace):
            errors.update({"RebinMode": "An interpolating rebin cannot be applied to an EventWorkspace."})
        return errors

    def _convert_units_to_wavelength(self, workspace):
        convert_name = "ConvertUnits"
        convert_options = {"InputWorkspace": workspace, "Target": "Wavelength"}
        convert_alg = create_unmanaged_algorithm(convert_name, **convert_options)
        convert_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        convert_alg.setProperty("OutputWorkspace", workspace)
        convert_alg.execute()
        return convert_alg.getProperty("OutputWorkspace").value

    def _get_rebin_string(self, workspace, wavelength_low, wavelength_high):
        # If the wavelength has not been specified, then get it from the workspace. Only the first spectrum is checked
        # The lowest wavelength value is to be found in the spectrum located at workspaces index 0 is a very
        # strong assumption, but it existed in the previous implementation.
        if wavelength_low is None or wavelength_low == Property.EMPTY_DBL:
            wavelength_low = min(workspace.readX(0))

        if wavelength_high is None or wavelength_high == Property.EMPTY_DBL:
            wavelength_high = max(workspace.readX(0))

        wavelength_step = self.getProperty("WavelengthStep").value
        step_type = RangeStepType(self.getProperty("WavelengthStepType").value)
        pre_factor = -1 if step_type in [RangeStepType.LOG, RangeStepType.RANGE_LOG] else 1
        wavelength_step *= pre_factor
        return str(wavelength_low) + "," + str(wavelength_step) + "," + str(wavelength_high)

    def _perform_rebin(self, rebin_options):
        rebin_type = RebinType(self.getProperty("RebinMode").value)
        rebin_name = "Rebin" if rebin_type is RebinType.REBIN else "InterpolatingRebin"
        rebin_alg = create_unmanaged_algorithm(rebin_name, **rebin_options)
        rebin_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        rebin_alg.execute()
        return rebin_alg.getProperty("OutputWorkspace").value


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSConvertToWavelengthAndRebin)
