# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-arguments,too-few-public-methods

from mantid.api import (
    AlgorithmFactory,
    AlgorithmManager,
    DataProcessorAlgorithm,
    MatrixWorkspaceProperty,
    PropertyMode,
    AnalysisDataService,
)
from mantid.kernel import Direction, Property, StringListValidator, UnitFactory
import numpy as np


class Mode(object):
    class ShiftOnly(object):
        pass

    class ScaleOnly(object):
        pass

    class BothFit(object):
        pass

    class NoneFit(object):
        pass


class SANSFitShiftScale(DataProcessorAlgorithm):
    def _make_mode_map(self):
        return {"ShiftOnly": Mode.ShiftOnly, "ScaleOnly": Mode.ScaleOnly, "Both": Mode.BothFit, "None": Mode.NoneFit}

    def category(self):
        return "SANS"

    def summary(self):
        return "Fits the high angle workspace and to the low angle bank workspace and provides the required shift and scale"

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("HABWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="High angle bank workspace in Q",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("LABWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="Low angle bank workspace in Q",
        )

        allowedModes = StringListValidator(list(self._make_mode_map().keys()))

        self.declareProperty("Mode", "None", validator=allowedModes, direction=Direction.Input, doc="What to fit. Free parameter(s).")

        self.declareProperty("ScaleFactor", defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc="Optional scaling factor")

        self.declareProperty("ShiftFactor", defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc="Optional shift factor")

        self.declareProperty("FitMin", defaultValue=0.0, direction=Direction.Input, doc="Optional minimum q for fit")

        self.declareProperty("FitMax", defaultValue=1000.0, direction=Direction.Input, doc="Optional maximum q for fit")

        self.declareProperty("OutScaleFactor", defaultValue=Property.EMPTY_DBL, direction=Direction.Output, doc="Applied scale factor")
        self.declareProperty("OutShiftFactor", defaultValue=Property.EMPTY_DBL, direction=Direction.Output, doc="Applied shift factor")

    def PyExec(self):
        enum_map = self._make_mode_map()
        mode = enum_map[self.getProperty("Mode").value]
        hab = self.getProperty("HABWorkspace").value
        lab = self.getProperty("LABWorkspace").value
        shift_factor = self.getProperty("ShiftFactor").value
        scale_factor = self.getProperty("ScaleFactor").value
        fit_min = self.getProperty("FitMin").value
        fit_max = self.getProperty("FitMax").value

        if fit_min < min(hab.dataX(0)):
            fit_min = min(hab.dataX(0))
        if fit_max > max(lab.dataX(0)):
            fit_max = max(lab.dataX(0))

        if not mode == Mode.NoneFit:
            shift_factor, scale_factor = self._determine_factors(
                hab, lab, mode, scale=scale_factor, shift=shift_factor, fit_min=fit_min, fit_max=fit_max
            )

        self.setProperty("OutScaleFactor", scale_factor)
        self.setProperty("OutShiftFactor", shift_factor)

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

        workspace_property_names = ["HABWorkspace", "LABWorkspace"]
        # 1d data check
        self._validate_1D(workspace_property_names, errors, mode)

        # Units check
        self._validate_units(workspace_property_names, errors)
        return errors

    def _validate_units(self, workspace_property_names, errors):
        for property_name in workspace_property_names:
            if not self._validateIsInQ(property_name):
                errors[property_name] = "Workspace must have units of momentum transfer"

    def _validateIsInQ(self, property_name):
        ws = self.getProperty(property_name).value
        if not ws:
            return True  # Mandatory validators to take care of this. Early exit.

        targetUnit = UnitFactory.create("MomentumTransfer")
        return targetUnit.caption() == ws.getAxis(0).getUnit().caption()

    def _validate_1D(self, workspace_property_names, errors, mode):
        if mode != Mode.NoneFit:
            for property_name in workspace_property_names:
                if not self._validateIs1DFromPropertyName(property_name):
                    errors[property_name] = "Wrong number of spectra. Must be 1D input"

    def _validateIs1DFromPropertyName(self, property_name):
        ws = self.getProperty(property_name).value
        if not ws:
            return True  # Mandatory validators to take care of this. Early exit.
        return ws.getNumberHistograms() == 1

    def _determine_factors(self, q_high_angle, q_low_angle, mode, scale, shift, fit_min, fit_max):
        # We need to make suret that the fitting only occurs in the y direction
        constant_x_shift_and_scale = ", f0.Shift=0.0, f0.XScaling=1.0"

        # Determine the StartQ and EndQ values
        q_min, q_max = self._get_start_q_and_end_q_values(rear_data=q_low_angle, front_data=q_high_angle, fit_min=fit_min, fit_max=fit_max)

        # We need to transfer the errors from the front data to the rear data, as we are using the front data as a model, but
        # we want to take into account the errors of both workspaces.
        error_correction = ErrorTransferFromModelToData()
        front_data_corrected, rear_data_corrected = error_correction.get_error_corrected(
            rear_data=q_low_angle, front_data=q_high_angle, q_min=q_min, q_max=q_max
        )

        # The front_data_corrected data set is used as the fit model. Setting the IgnoreInvalidData on the Fit algorithm
        # will not have any ignore Nans in the model, but only in the data. Hence this will lead to unreadable
        # error messages of the fit algorithm. We need to catch this before the algorithm starts
        y_model = front_data_corrected.dataY(0)
        y_data = rear_data_corrected.dataY(0)
        if any([np.isnan(element) for element in y_model]) or any([np.isnan(element) for element in y_data]):
            raise RuntimeError(
                "Trying to merge the two reduced data sets for HAB and LAB failed. "
                "You seem to have Nan values in your reduced HAB or LAB data set. This is most likely "
                "caused by a too small Q binning. Try to increase the Q bin width."
            )

        # We currently have to put the front_data into the ADS so that the TabulatedFunction has access to it
        front_data_corrected = AnalysisDataService.addOrReplace("front_data_corrected", front_data_corrected)
        front_in_ads = AnalysisDataService.retrieve("front_data_corrected")

        function = 'name=TabulatedFunction, Workspace="' + str(front_in_ads.name()) + '"' + ";name=FlatBackground"

        fit = self.createChildAlgorithm("Fit")
        fit.setProperty("Function", function)
        fit.setProperty("InputWorkspace", rear_data_corrected)

        constant_x_shift_and_scale = "f0.Shift=0.0, f0.XScaling=1.0"
        if mode == Mode.BothFit:
            fit.setProperty("Ties", constant_x_shift_and_scale)
        elif mode == Mode.ShiftOnly:
            fit.setProperty("Ties", "f0.Scaling=" + str(scale) + "," + constant_x_shift_and_scale)
        elif mode == Mode.ScaleOnly:
            fit.setProperty("Ties", "f1.A0=" + str(shift) + "*f0.Scaling," + constant_x_shift_and_scale)
        else:
            raise RuntimeError("Unknown fitting mode requested.")

        fit.setProperty("StartX", q_min)
        fit.setProperty("EndX", q_max)
        fit.setProperty("CreateOutput", True)
        fit.execute()
        param = fit.getProperty("OutputParameters").value
        AnalysisDataService.remove(front_in_ads.name())

        # The outparameters are:
        # 1. Scaling in y direction
        # 2. Shift in x direction
        # 3. Scaling in x direction
        # 4. Shift in y direction

        scale = param.row(0)["Value"]

        if scale == 0.0:
            raise RuntimeError("Fit scaling as part of stitching evaluated to zero")

        # In order to determine the shift, we need to remove the scale factor
        shift = param.row(3)["Value"] / scale

        return (shift, scale)

    def _get_start_q_and_end_q_values(self, rear_data, front_data, fit_min, fit_max):
        min_q = None
        max_q = None

        front_dataX = front_data.readX(0)

        front_size = len(front_dataX)
        front_q_min = None
        front_q_max = None
        if front_size > 0:
            front_q_min = front_dataX[0]
            front_q_max = front_dataX[front_size - 1]
        else:
            raise RuntimeError("The FRONT detector does not seem to contain q values")

        rear_dataX = rear_data.readX(0)

        rear_size = len(rear_dataX)
        rear_q_min = None
        rear_q_max = None
        if rear_size > 0:
            rear_q_min = rear_dataX[0]
            rear_q_max = rear_dataX[rear_size - 1]
        else:
            raise RuntimeError("The REAR detector does not seem to contain q values")

        if rear_q_max < front_q_min:
            raise RuntimeError("The min value of the FRONT detector data set is larger than the max value of the REAR detector data set")

        # Get the min and max range
        min_q = max(rear_q_min, front_q_min, fit_min)
        max_q = min(rear_q_max, front_q_max, fit_max)

        return min_q, max_q


class ErrorTransferFromModelToData(object):
    """
    Handles the error transfer from the model workspace to the
    data workspace
    """

    def __init__(self):
        super(ErrorTransferFromModelToData, self).__init__()

    def get_error_corrected(self, front_data, rear_data, q_min, q_max):
        self._comment(front_data, "Internal Step: Front data errors corrected as sqrt(rear_error^2 + front_error^2)")
        self._comment(rear_data, "Internal Step: Rear data errors corrected as sqrt(rear_error^2 + front_error^2)")

        front_data_cropped = self._crop_to_x_range(front_data, q_min, q_max)

        # For the rear data set
        rear_data_cropped = self._crop_to_x_range(rear_data, q_min, q_max)

        # Now transfer the error from front data to the rear data workspace
        # This works only if we have a single QMod spectrum in the workspaces
        for i in range(0, front_data_cropped.getNumberHistograms()):
            front_error = front_data_cropped.dataE(i)
            rear_error = rear_data_cropped.dataE(i)

            rear_error_squared = rear_error * rear_error
            front_error_squared = front_error * front_error

            corrected_error_squared = rear_error_squared + front_error_squared
            corrected_error = np.sqrt(corrected_error_squared)
            rear_error[0 : len(rear_error)] = corrected_error[0 : len(rear_error)]

        return front_data_cropped, rear_data_cropped

    def _crop_to_x_range(self, ws, x_min, x_max):
        crop = AlgorithmManager.createUnmanaged("CropWorkspace")
        crop.initialize()
        crop.setChild(True)
        crop.setProperty("InputWorkspace", ws)
        crop.setProperty("XMin", x_min)
        crop.setProperty("XMax", x_max)
        crop.setProperty("OutputWorkspace", "cropped_ws")
        crop.execute()
        return crop.getProperty("OutputWorkspace").value

    def _comment(self, ws, message):
        comment = AlgorithmManager.createUnmanaged("Comment")
        comment.initialize()
        comment.setChild(True)
        comment.setProperty("Workspace", ws)
        comment.setProperty("Text", message)
        comment.execute()


AlgorithmFactory.subscribe(SANSFitShiftScale)
