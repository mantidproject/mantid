# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

""" Converts a workspace from wavelengths to momentum transfer."""
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, PropertyManagerProperty, CompositeValidator)
from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode,
                        WorkspaceUnitValidator)

from sans.common.constants import EMPTY_NAME
from sans.common.enums import (ReductionDimensionality, RangeStepType)
from sans.common.general_functions import (create_unmanaged_algorithm, append_to_sans_file_tag)
from sans.state.state_base import create_deserialized_sans_state_from_property_manager
from sans.algorithm_detail.q_resolution_calculator import QResolutionCalculatorFactory


class SANSConvertToQ(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\ConvertToQ'

    def summary(self):
        return 'Converts a SANS workspace to momentum transfer.'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        # State
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        # Main workspace
        workspace_validator = CompositeValidator()
        workspace_validator.add(WorkspaceUnitValidator("Wavelength"))
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input,
                                                     validator=workspace_validator),
                             doc='The main input workspace.')

        # Adjustment workspaces
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspaceWavelengthAdjustment", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input,
                                                     validator=workspace_validator),
                             doc='The workspace which contains only wavelength-specific adjustments, ie which affects '
                                 'all spectra equally.')
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspacePixelAdjustment", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The workspace which contains only pixel-specific adjustments, ie which affects '
                                 'all bins within a spectrum equally.')
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspaceWavelengthAndPixelAdjustment", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input,
                                                     validator=workspace_validator),
                             doc='The workspace which contains wavelength- and pixel-specific adjustments.')

        self.declareProperty('OutputParts', defaultValue=False,
                             direction=Direction.Input,
                             doc='Set to true to output two additional workspaces which will have the names '
                                 'OutputWorkspace_sumOfCounts OutputWorkspace_sumOfNormFactors. The division '
                                 'of _sumOfCounts and _sumOfNormFactors equals the workspace returned by the '
                                 'property OutputWorkspace (default is false).')

        # ----------
        # Output
        # ----------
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc="The reduced workspace")

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)

        # Perform either a 1D reduction or a 2D reduction
        convert_to_q = state.convert_to_q
        reduction_dimensionality = convert_to_q.reduction_dimensionality
        if reduction_dimensionality is ReductionDimensionality.OneDim:
            output_workspace, sum_of_counts_workspace, sum_of_norms_workspace = self._run_q_1d(state)
        else:
            output_workspace, sum_of_counts_workspace, sum_of_norms_workspace = self._run_q_2d(state)

        # Set the output
        append_to_sans_file_tag(output_workspace, "_convertq")
        self.setProperty("OutputWorkspace", output_workspace)
        output_parts = self.getProperty("OutputParts").value
        if output_parts:
            self._set_partial_workspaces(sum_of_counts_workspace, sum_of_norms_workspace)

    def _run_q_1d(self, state):
        data_workspace = self.getProperty("InputWorkspace").value
        wavelength_adjustment_workspace = self.getProperty("InputWorkspaceWavelengthAdjustment").value
        pixel_adjustment_workspace = self.getProperty("InputWorkspacePixelAdjustment").value
        wavelength_and_pixel_adjustment_workspace = self.getProperty("InputWorkspaceWavelengthAndPixelAdjustment").value

        # Get QResolution
        convert_to_q = state.convert_to_q
        q_resolution_factory = QResolutionCalculatorFactory()
        q_resolution_calculator = q_resolution_factory.create_q_resolution_calculator(state)
        q_resolution_workspace = q_resolution_calculator.get_q_resolution_workspace(convert_to_q, data_workspace)

        output_parts = self.getProperty("OutputParts").value

        # Extract relevant settings
        q_binning = convert_to_q.q_1d_rebin_string
        use_gravity = convert_to_q.use_gravity
        gravity_extra_length = convert_to_q.gravity_extra_length
        radius_cutoff = convert_to_q.radius_cutoff * 1000.  # Q1D2 expects the radius cutoff to be in mm
        wavelength_cutoff = convert_to_q.wavelength_cutoff

        q1d_name = "Q1D"
        q1d_options = {"DetBankWorkspace": data_workspace,
                       "OutputWorkspace": EMPTY_NAME,
                       "OutputBinning": q_binning,
                       "AccountForGravity": use_gravity,
                       "RadiusCut": radius_cutoff,
                       "WaveCut": wavelength_cutoff,
                       "OutputParts": output_parts,
                       "ExtraLength": gravity_extra_length}
        if wavelength_adjustment_workspace:
            q1d_options.update({"WavelengthAdj": wavelength_adjustment_workspace})
        if pixel_adjustment_workspace:
            q1d_options.update({"PixelAdj": pixel_adjustment_workspace})
        if wavelength_and_pixel_adjustment_workspace:
            q1d_options.update({"WavePixelAdj": wavelength_and_pixel_adjustment_workspace})
        if q_resolution_workspace:
            q1d_options.update({"QResolution": q_resolution_workspace})

        q1d_alg = create_unmanaged_algorithm(q1d_name, **q1d_options)
        q1d_alg.execute()
        reduced_workspace = q1d_alg.getProperty("OutputWorkspace").value

        # Get the partial workspaces
        sum_of_counts_workspace, sum_of_norms_workspace = self._get_partial_output(output_parts, q1d_alg,
                                                                                   do_clean=False)

        return reduced_workspace, sum_of_counts_workspace, sum_of_norms_workspace

    def _run_q_2d(self, state):
        """
        This method performs a 2D data reduction on our workspace.

        Note that it does not perform any q resolution calculation, nor any wavelength-and-pixel adjustment. The
        output workspace contains two numerical axes.
        :param state: a SANSState object
        :return: the reduced workspace, the sum of counts workspace, the sum of norms workspace or
                 the reduced workspace, None, None
        """
        data_workspace = self.getProperty("InputWorkspace").value
        wavelength_adjustment_workspace = self.getProperty("InputWorkspaceWavelengthAdjustment").value
        pixel_adjustment_workspace = self.getProperty("InputWorkspacePixelAdjustment").value

        output_parts = self.getProperty("OutputParts").value

        # Extract relevant settings
        convert_to_q = state.convert_to_q
        max_q_xy = convert_to_q.q_xy_max
        log_binning = True if convert_to_q.q_xy_step_type is RangeStepType.Log else False
        delta_q = convert_to_q.q_xy_step
        radius_cutoff = convert_to_q.radius_cutoff / 1000.  # Qxy expects the radius cutoff to be in mm
        wavelength_cutoff = convert_to_q.wavelength_cutoff
        use_gravity = convert_to_q.use_gravity
        gravity_extra_length = convert_to_q.gravity_extra_length

        qxy_name = "Qxy"
        qxy_options = {"InputWorkspace": data_workspace,
                       "OutputWorkspace": EMPTY_NAME,
                       "MaxQxy": max_q_xy,
                       "DeltaQ": delta_q,
                       "IQxQyLogBinning": log_binning,
                       "AccountForGravity": use_gravity,
                       "RadiusCut": radius_cutoff,
                       "WaveCut": wavelength_cutoff,
                       "OutputParts": output_parts,
                       "ExtraLength": gravity_extra_length}
        if wavelength_adjustment_workspace:
            qxy_options.update({"WavelengthAdj": wavelength_adjustment_workspace})
        if pixel_adjustment_workspace:
            qxy_options.update({"PixelAdj": pixel_adjustment_workspace})

        qxy_alg = create_unmanaged_algorithm(qxy_name, **qxy_options)
        qxy_alg.execute()

        reduced_workspace = qxy_alg.getProperty("OutputWorkspace").value
        reduced_workspace = self._replace_special_values(reduced_workspace)

        # Get the partial workspaces
        sum_of_counts_workspace, sum_of_norms_workspace = self._get_partial_output(output_parts, qxy_alg, do_clean=True)

        return reduced_workspace, sum_of_counts_workspace, sum_of_norms_workspace

    def _get_partial_output(self, output_parts, alg, do_clean=False):
        if output_parts:
            sum_of_counts_workspace = alg.getProperty("SumOfCounts").value
            sum_of_norms_workspace = alg.getProperty("sumOfNormFactors").value
            if do_clean:
                sum_of_counts_workspace = self._replace_special_values(sum_of_counts_workspace)
                sum_of_norms_workspace = self._replace_special_values(sum_of_norms_workspace)
        else:
            sum_of_counts_workspace = None
            sum_of_norms_workspace = None
        return sum_of_counts_workspace, sum_of_norms_workspace

    def _set_partial_workspaces(self, sum_of_counts_workspace, sum_of_norms_workspace):
        """
        Sets the partial output, ie the sum of the counts workspace and the sum of the normalization workspace
        :param sum_of_counts_workspace: the sum of the counts workspace
        :param sum_of_norms_workspace: the sum of the normalization workspace
        """
        self.declareProperty(MatrixWorkspaceProperty("SumOfCounts", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc="The sum of the counts workspace.")
        self.declareProperty(MatrixWorkspaceProperty("SumOfNormFactors", '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc="The sum of the normalizations workspace.")

        output_name = self.getProperty("OutputWorkspace").name
        sum_of_counts_workspace_name = output_name + "_sumOfCounts"
        sum_of_norms_workspace_name = output_name + "_sumOfNormFactors"

        self.setPropertyValue("SumOfCounts", sum_of_counts_workspace_name)
        self.setPropertyValue("SumOfNormFactors", sum_of_norms_workspace_name)

        self.setProperty("SumOfCounts", sum_of_counts_workspace)
        self.setProperty("SumOfNormFactors", sum_of_norms_workspace)

    def _replace_special_values(self, workspace):
        replace_name = "ReplaceSpecialValues"
        replace_options = {"InputWorkspace": workspace,
                           "OutputWorkspace": EMPTY_NAME,
                           "NaNValue": 0.,
                           "InfinityValue": 0.}
        replace_alg = create_unmanaged_algorithm(replace_name, **replace_options)
        replace_alg.execute()
        return replace_alg.getProperty("OutputWorkspace").value

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        state_property_manager = self.getProperty("SANSState").value
        try:
            state = create_deserialized_sans_state_from_property_manager(state_property_manager)
            state.property_manager = state_property_manager
            state.validate()
        except ValueError as err:
            errors.update({"SANSSConvertToQ": str(err)})
        return errors


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSConvertToQ)
