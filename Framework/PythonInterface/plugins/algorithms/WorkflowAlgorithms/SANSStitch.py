# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-arguments,too-few-public-methods

from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode
from mantid.kernel import Direction, Property, StringListValidator, UnitFactory, EnabledWhenProperty, PropertyCriterion, Logger
import numpy as np
from sans.common.general_functions import create_unmanaged_algorithm
from sans.common.constants import EMPTY_NAME


class Mode(object):
    class ShiftOnly(object):
        pass

    class ScaleOnly(object):
        pass

    class BothFit(object):
        pass

    class NoneFit(object):
        pass


class SANSStitch(DataProcessorAlgorithm):
    def _make_mode_map(self):
        return {"ShiftOnly": Mode.ShiftOnly, "ScaleOnly": Mode.ScaleOnly, "Both": Mode.BothFit, "None": Mode.NoneFit}

    def category(self):
        return "SANS"

    def summary(self):
        return "Stitch the high angle and low angle banks of a workspace together"

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("HABCountsSample", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="High angle bank sample workspace in Q",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("HABNormSample", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="High angle bank normalization workspace in Q",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("LABCountsSample", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="Low angle bank sample workspace in Q",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("LABNormSample", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="Low angle bank normalization workspace in Q",
        )

        self.declareProperty("ProcessCan", defaultValue=False, direction=Direction.Input, doc="Process the can")

        self.declareProperty(
            MatrixWorkspaceProperty("HABCountsCan", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="High angle bank sample workspace in Q",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("HABNormCan", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="High angle bank normalization workspace in Q",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("LABCountsCan", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="Low angle bank sample workspace in Q",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("LABNormCan", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="Low angle bank normalization workspace in Q",
        )

        allowedModes = StringListValidator(list(self._make_mode_map().keys()))

        self.declareProperty("Mode", "None", validator=allowedModes, direction=Direction.Input, doc="What to fit. Free parameter(s).")

        self.declareProperty("ScaleFactor", defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc="Optional scaling factor")

        self.declareProperty("ShiftFactor", defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc="Optional shift factor")

        self.declareProperty("FitMin", defaultValue=0.0, direction=Direction.Input, doc="Optional minimum q for fit")

        self.declareProperty("FitMax", defaultValue=1000.0, direction=Direction.Input, doc="Optional maximum q for fit")

        self.declareProperty(
            "MergeMask",
            defaultValue=False,
            direction=Direction.Input,
            doc="Controls whether the user has manually specified the merge region",
        )

        self.declareProperty("MergeMin", defaultValue=0.0, direction=Direction.Input, doc="The minimum of the merge region in q")

        self.declareProperty("MergeMax", defaultValue=1000.0, direction=Direction.Input, doc="The maximum of the merge region in q")

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Stitched high and low Q 1-D data"
        )

        self.declareProperty("OutScaleFactor", defaultValue=Property.EMPTY_DBL, direction=Direction.Output, doc="Applied scale factor")
        self.declareProperty("OutShiftFactor", defaultValue=Property.EMPTY_DBL, direction=Direction.Output, doc="Applied shift factor")

        self.setPropertyGroup("Mode", "Fitting")
        self.setPropertyGroup("ScaleFactor", "Fitting")
        self.setPropertyGroup("ShiftFactor", "Fitting")

        self.setPropertyGroup("HABCountsSample", "Sample")
        self.setPropertyGroup("HABNormSample", "Sample")
        self.setPropertyGroup("LABCountsSample", "Sample")
        self.setPropertyGroup("LABNormSample", "Sample")

        self.setPropertyGroup("OutputWorkspace", "Output")
        self.setPropertyGroup("OutScaleFactor", "Output")
        self.setPropertyGroup("OutShiftFactor", "Output")

        can_settings = EnabledWhenProperty("ProcessCan", PropertyCriterion.IsNotDefault)

        self.setPropertyGroup("HABCountsCan", "Can")
        self.setPropertyGroup("HABNormCan", "Can")
        self.setPropertyGroup("LABCountsCan", "Can")
        self.setPropertyGroup("LABNormCan", "Can")
        self.setPropertySettings("HABCountsCan", can_settings)
        self.setPropertySettings("HABNormCan", can_settings)
        self.setPropertySettings("LABCountsCan", can_settings)
        self.setPropertySettings("LABNormCan", can_settings)

    def _divide(self, left, right):
        divide = self.createChildAlgorithm("Divide")
        divide.setProperty("LHSWorkspace", left)
        divide.setProperty("RHSWorkspace", right)
        divide.execute()
        return divide.getProperty("OutputWorkspace").value

    def _multiply(self, left, right):
        multiply = self.createChildAlgorithm("Multiply")
        multiply.setProperty("LHSWorkspace", left)
        multiply.setProperty("RHSWorkspace", right)
        multiply.execute()
        return multiply.getProperty("OutputWorkspace").value

    def _add(self, left, right):
        plus = self.createChildAlgorithm("Plus")
        plus.setProperty("LHSWorkspace", left)
        plus.setProperty("RHSWorkspace", right)
        plus.execute()
        return plus.getProperty("OutputWorkspace").value

    def _subract(self, left, right):
        minus = self.createChildAlgorithm("Minus")
        minus.setProperty("LHSWorkspace", left)
        minus.setProperty("RHSWorkspace", right)
        minus.execute()
        return minus.getProperty("OutputWorkspace").value

    def _crop_to_x_range(self, ws, x_min, x_max):
        crop = self.createChildAlgorithm("CropWorkspace")
        crop.setProperty("InputWorkspace", ws)
        crop.setProperty("XMin", x_min)
        crop.setProperty("XMax", x_max)
        crop.execute()
        return crop.getProperty("OutputWorkspace").value

    def _scale(self, ws, factor, operation="Multiply"):
        scale = self.createChildAlgorithm("Scale")
        scale.setProperty("InputWorkspace", ws)
        scale.setProperty("Operation", operation)
        scale.setProperty("Factor", factor)
        scale.execute()
        scaled = scale.getProperty("OutputWorkspace").value
        return scaled

    def _calculate_merged_q(self, cF, nF, cR, nR, shift_factor, scale_factor):
        # We want: (Cf+shift*Nf+Cr)/(Nf/scale + Nr)
        shifted_norm_front = self._scale(nF, shift_factor)
        scaled_norm_front = self._scale(nF, 1.0 / scale_factor)
        add_counts_and_shift = self._add(cF, shifted_norm_front)
        numerator = self._add(add_counts_and_shift, cR)
        denominator = self._add(scaled_norm_front, nR)
        merged_q = self._divide(numerator, denominator)
        return merged_q

    def _calculate_merged_q_can(self, cF, nF, cR, nR, scale_factor):
        # We want: (Cf_can+Cr_can)/(Nf_can/scale + Nr_can)
        scaled_norm_front = self._scale(nF, 1.0 / scale_factor)
        numerator = self._add(cF, cR)
        denominator = self._add(scaled_norm_front, nR)
        merged_q = self._divide(numerator, denominator)
        return merged_q

    def _crop_out_special_values(self, ws):
        if ws.getNumberHistograms() != 1:
            # Strip zeros is only possible on 1D workspaces
            return ws

        y_vals = ws.readY(0)
        length = len(y_vals)
        # Find the first non-zero value
        start = 0
        for i in range(0, length):
            if not np.isnan(y_vals[i]) and not np.isinf(y_vals[i]):
                start = i
                break
        # Now find the last non-zero value
        stop = 0
        length -= 1
        for j in range(length, 0, -1):
            if not np.isnan(y_vals[j]) and not np.isinf(y_vals[j]):
                stop = j
                break
        # Find the appropriate X values and call CropWorkspace
        x_vals = ws.readX(0)
        start_x = x_vals[start]
        # Make sure we're inside the bin that we want to crop
        if len(y_vals) == len(x_vals):
            end_x = x_vals[stop]
        else:
            end_x = x_vals[stop + 1]
        return self._crop_to_x_range(ws=ws, x_min=start_x, x_max=end_x)

    def _run_fit(self, q_high_angle, q_low_angle, scale_factor, shift_factor, fit_min, fit_max):
        fit_alg = self.createChildAlgorithm("SANSFitShiftScale")
        fit_alg.setProperty("HABWorkspace", q_high_angle)
        fit_alg.setProperty("LABWorkspace", q_low_angle)
        fit_alg.setProperty("Mode", self.getProperty("Mode").value)
        fit_alg.setProperty("ScaleFactor", scale_factor)
        fit_alg.setProperty("ShiftFactor", shift_factor)
        fit_alg.setProperty("FitMin", fit_min)
        fit_alg.setProperty("FitMax", fit_max)
        fit_alg.execute()
        scale_factor_fit = fit_alg.getProperty("OutScaleFactor").value
        shift_factor_fit = fit_alg.getProperty("OutShiftFactor").value
        return scale_factor_fit, shift_factor_fit

    def _check_bins(self, merge_min, merge_max, cF, cR):
        if cF.yIndexOfX(merge_min) == cR.yIndexOfX(merge_max):
            return cF.readX(0)[cR.yIndexOfX(merge_max) + 1]
        else:
            return merge_max

    def _check_merge_range(self, merge_min, merge_max, q_LAB, q_HAB, logger):
        if merge_min < min(q_HAB.dataX(0)):
            merge_min = min(q_HAB.dataX(0))
            logger.warning("Minimum merge region less than data overlap setting to HAB minimum q")
        elif merge_min > max(q_LAB.dataX(0)):
            merge_min = max(q_LAB.dataX(0))
            logger.warning("Minimum merge region greater than data overlap setting to LAB maximum q")
        if merge_max > max(q_LAB.dataX(0)):
            merge_max = max(q_LAB.dataX(0))
            logger.warning("Maximum merge region greater than data overlap setting to LAB maximum q")
        elif merge_max < min(q_HAB.dataX(0)):
            merge_max = min(q_HAB.dataX(0))
            logger.warning("Maximum merge region less than data overlap setting to HAB minimum q")

        return merge_min, merge_max

    def _apply_user_mask_ranges(self, cF, cR, nR, nF, merge_min, merge_max):
        merge_max = self._check_bins(merge_min, merge_max, cF, cR)

        mask_name = "MaskBins"
        mask_options = {"InputWorkspace": cF}
        mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)

        mask_alg.setProperty("InputWorkspace", cF)
        mask_alg.setProperty("OutputWorkspace", EMPTY_NAME)
        mask_alg.setProperty("XMin", min(cF.dataX(0)))
        mask_alg.setProperty("XMax", merge_min)
        mask_alg.execute()
        cF = mask_alg.getProperty("OutputWorkspace").value

        mask_alg.setProperty("InputWorkspace", nF)
        mask_alg.setProperty("OutputWorkspace", EMPTY_NAME)
        mask_alg.setProperty("XMin", min(nF.dataX(0)))
        mask_alg.setProperty("XMax", merge_min)
        mask_alg.execute()
        nF = mask_alg.getProperty("OutputWorkspace").value

        mask_alg.setProperty("InputWorkspace", cR)
        mask_alg.setProperty("OutputWorkspace", EMPTY_NAME)
        mask_alg.setProperty("XMin", merge_max)
        mask_alg.setProperty("XMax", max(cR.dataX(0)))
        mask_alg.execute()
        cR = mask_alg.getProperty("OutputWorkspace").value

        mask_alg.setProperty("InputWorkspace", nR)
        mask_alg.setProperty("OutputWorkspace", EMPTY_NAME)
        mask_alg.setProperty("XMin", merge_max)
        mask_alg.setProperty("XMax", max(nR.dataX(0)))
        mask_alg.execute()
        nR = mask_alg.getProperty("OutputWorkspace").value
        return cR, cF, nR, nF

    def PyExec(self):
        logger = Logger("SANSStitch")

        enum_map = self._make_mode_map()
        mode = enum_map[self.getProperty("Mode").value]
        merge_mask = self.getProperty("MergeMask").value
        merge_min = self.getProperty("MergeMin").value
        merge_max = self.getProperty("MergeMax").value
        cF = self.getProperty("HABCountsSample").value
        cR = self.getProperty("LABCountsSample").value
        nF = self.getProperty("HABNormSample").value
        nR = self.getProperty("LABNormSample").value

        q_high_angle = self._divide(cF, nF)
        q_low_angle = self._divide(cR, nR)

        if self.getProperty("ProcessCan").value:
            cF_can = self.getProperty("HABCountsCan").value
            cR_can = self.getProperty("LABCountsCan").value
            nF_can = self.getProperty("HABNormCan").value
            nR_can = self.getProperty("LABNormCan").value

            q_high_angle_can = self._divide(cF_can, nF_can)
            q_low_angle_can = self._divide(cR_can, nR_can)

            # Now we can do the can subraction.
            q_high_angle = self._subract(q_high_angle, q_high_angle_can)
            q_low_angle = self._subract(q_low_angle, q_low_angle_can)

        q_high_angle = self._crop_out_special_values(q_high_angle)
        q_low_angle = self._crop_out_special_values(q_low_angle)

        if merge_mask:
            merge_min, merge_max = self._check_merge_range(merge_min, merge_max, q_low_angle, q_high_angle, logger)

        shift_factor = self.getProperty("ShiftFactor").value
        scale_factor = self.getProperty("ScaleFactor").value
        fit_min = self.getProperty("FitMin").value
        fit_max = self.getProperty("FitMax").value

        if not mode == Mode.NoneFit:
            scale_factor, shift_factor = self._run_fit(q_high_angle, q_low_angle, scale_factor, shift_factor, fit_min, fit_max)

        min_q = min(min(q_high_angle.dataX(0)), min(q_low_angle.dataX(0)))
        max_q = max(max(q_high_angle.dataX(0)), max(q_low_angle.dataX(0)))

        # Crop our input workspaces
        cF = self._crop_to_x_range(cF, min_q, max_q)
        cR = self._crop_to_x_range(cR, min_q, max_q)
        nF = self._crop_to_x_range(nF, min_q, max_q)
        nR = self._crop_to_x_range(nR, min_q, max_q)

        if merge_mask:
            cR, cF, nR, nF = self._apply_user_mask_ranges(cF, cR, nR, nF, merge_min, merge_max)

        # We want: (Cf+shift*Nf+Cr)/(Nf/scale + Nr)
        merged_q = self._calculate_merged_q(cF=cF, nF=nF, cR=cR, nR=nR, scale_factor=scale_factor, shift_factor=shift_factor)

        if self.getProperty("ProcessCan").value:
            cF_can = self._crop_to_x_range(cF_can, min_q, max_q)
            cR_can = self._crop_to_x_range(cR_can, min_q, max_q)
            nF_can = self._crop_to_x_range(nF_can, min_q, max_q)
            nR_can = self._crop_to_x_range(nR_can, min_q, max_q)

            if merge_mask:
                cR_can, cF_can, nR_can, nF_can = self._apply_user_mask_ranges(cF_can, cR_can, nR_can, nF_can, merge_min, merge_max)

            # Calculate merged q for the can
            merged_q_can = self._calculate_merged_q_can(cF=cF_can, nF=nF_can, cR=cR_can, nR=nR_can, scale_factor=scale_factor)

            # Subtract it from the sample
            merged_q = self._subract(merged_q, merged_q_can)

        q_error_correction = QErrorCorrectionForMergedWorkspaces()
        q_error_correction.correct_q_resolution_for_merged(count_ws_front=cF, count_ws_rear=cR, output_ws=merged_q, scale=scale_factor)

        self.setProperty("OutputWorkspace", merged_q)
        self.setProperty("OutScaleFactor", scale_factor)
        self.setProperty("OutShiftFactor", shift_factor)

    def _validateIs1DFromPropertyName(self, property_name):
        ws = self.getProperty(property_name).value
        if not ws:
            return True  # Mandatory validators to take care of this. Early exit.
        return ws.getNumberHistograms() == 1

    def _validateIsInQ(self, property_name):
        ws = self.getProperty(property_name).value
        if not ws:
            return True  # Mandatory validators to take care of this. Early exit.

        targetUnit = UnitFactory.create("MomentumTransfer")
        return targetUnit.caption() == ws.getAxis(0).getUnit().caption()

    def validateInputs(self):
        errors = dict()

        # Mode compatibility checks
        scale_factor_property = self.getProperty("ScaleFactor")
        shift_factor_property = self.getProperty("ShiftFactor")
        mode_property = self.getProperty("Mode")
        enum_map = self._make_mode_map()
        mode = enum_map[mode_property.value]
        if mode == Mode.NoneFit:
            if scale_factor_property.isDefault:
                errors[scale_factor_property.name] = "ScaleFactor required"
            if shift_factor_property.isDefault:
                errors[shift_factor_property.name] = "ShiftFactor required"
        elif mode == Mode.ScaleOnly:
            if shift_factor_property.isDefault:
                errors[shift_factor_property.name] = "ShiftFactor required"
        elif mode == Mode.ShiftOnly:
            if scale_factor_property.isDefault:
                errors[scale_factor_property.name] = "ScaleFactor required"

        workspace_property_names = ["HABCountsSample", "LABCountsSample", "HABNormSample", "LABNormSample"]
        # 1d data check
        self._validate_1D(workspace_property_names, errors, mode)

        # Units check
        self._validate_units(workspace_property_names, errors)

        process_can = self.getProperty("ProcessCan")
        if bool(process_can.value):
            workspace_property_names = ["HABCountsCan", "HABNormCan", "LABCountsCan", "LABNormCan"]
            # Check existance
            self._validate_provided(workspace_property_names, errors)
            # Check Q units
            self._validate_units(workspace_property_names, errors)
            # Check 1D
            self._validate_1D(workspace_property_names, errors, mode)

        return errors

    def _validate_units(self, workspace_property_names, errors):
        for property_name in workspace_property_names:
            if not self._validateIsInQ(property_name):
                errors[property_name] = "Workspace must have units of momentum transfer"

    def _validate_1D(self, workspace_property_names, errors, mode):
        if mode != Mode.NoneFit:
            for property_name in workspace_property_names:
                if not self._validateIs1DFromPropertyName(property_name):
                    errors[property_name] = "Wrong number of spectra. Must be 1D input"

    def _validate_provided(self, workspace_property_names, errors):
        for property_name in workspace_property_names:
            if not self.getProperty(property_name).value:
                errors[property_name] = "This workspace is required in order to process the can"


class QErrorCorrectionForMergedWorkspaces(object):
    def __init__(self):
        super(QErrorCorrectionForMergedWorkspaces, self).__init__()

    def _divide_q_resolution_by_counts(self, q_res, counts):
        # We are dividing DX by Y.
        q_res_buffer = np.divide(q_res, counts)
        return q_res_buffer

    def _multiply_q_resolution_by_counts(self, q_res, counts):
        # We are dividing DX by Y.
        q_res_buffer = np.multiply(q_res, counts)
        return q_res_buffer

    def correct_q_resolution_for_merged(self, count_ws_front, count_ws_rear, output_ws, scale):
        """
        We need to transfer the DX error values from the original workspaces to the merged worksapce.
        We have:
        C(Q) = Sum_all_lambda_for_particular_Q(Counts(lambda))
        weightedQRes(Q) = Sum_all_lambda_for_particular_Q(Counts(lambda)* qRes(lambda))
        ResQ(Q) = weightedQRes(Q)/C(Q)
        Richard suggested:
        ResQMerged(Q) = (weightedQRes_FRONT(Q)*scale + weightedQRes_REAR(Q))/
                        (C_FRONT(Q)*scale + C_REAR(Q))
        Note that we drop the shift here.
        The Q Resolution functionality only exists currently
        for 1D, ie when only one spectrum is present.
        @param count_ws_front: the front counts
        @param count_ws_rear: the rear counts
        @param output_ws: the output workspace
        """
        self._comment(output_ws, "Internal Step: q-resolution transferred from input workspaces")

        if count_ws_rear.getNumberHistograms() != 1:
            return

        if count_ws_front.getNumberHistograms() != 1:
            return

        # We require both count workspaces to contain the DX value
        if not count_ws_rear.hasDx(0) or not count_ws_front.hasDx(0):
            return

        q_resolution_front = count_ws_front.readDx(0)
        q_resolution_rear = count_ws_rear.readDx(0)
        counts_front = count_ws_front.readY(0)
        counts_rear = count_ws_rear.readY(0)

        # We need to make sure that the workspaces match in length
        if (len(q_resolution_front) != len(q_resolution_rear)) or (len(counts_front) != len(counts_rear)):
            return

        # Get everything for the FRONT detector
        q_res_front_norm_free = self._multiply_q_resolution_by_counts(q_resolution_front, counts_front)
        q_res_front_norm_free = q_res_front_norm_free * scale
        counts_front = counts_front * scale

        # Get everything for the REAR detector
        q_res_rear_norm_free = self._multiply_q_resolution_by_counts(q_resolution_rear, counts_rear)

        # Now add and divide
        new_q_res = np.add(q_res_front_norm_free, q_res_rear_norm_free)
        new_counts = np.add(counts_front, counts_rear)
        q_resolution = self._divide_q_resolution_by_counts(new_q_res, new_counts)

        # Set the dx error
        output_ws.setDx(0, q_resolution)

    def _comment(self, ws, message):
        comment = AlgorithmManager.createUnmanaged("Comment")
        comment.initialize()
        comment.setChild(True)
        comment.setProperty("Workspace", ws)
        comment.setProperty("Text", message)
        comment.execute()


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSStitch)
