# pylint: disable=no-init,invalid-name,too-many-arguments,too-few-public-methods

from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode, AnalysisDataService
from mantid.kernel import Direction, Property, StringListValidator, UnitFactory, \
    EnabledWhenProperty, PropertyCriterion
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


class SANSStitch1D(DataProcessorAlgorithm):
    def _make_mode_map(self):
        return {'ShiftOnly': Mode.ShiftOnly, 'ScaleOnly': Mode.ScaleOnly,
                'Both': Mode.BothFit, 'None': Mode.NoneFit}

    def category(self):
        return 'SANS'

    def summary(self):
        return 'Stitch the high angle and low angle banks of a workspace together'

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty('HABCountsSample', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc='High angle bank sample workspace in Q')

        self.declareProperty(
            MatrixWorkspaceProperty('HABNormSample', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc='High angle bank normalization workspace in Q')

        self.declareProperty(
            MatrixWorkspaceProperty('LABCountsSample', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc='Low angle bank sample workspace in Q')

        self.declareProperty(
            MatrixWorkspaceProperty('LABNormSample', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc='Low angle bank normalization workspace in Q')

        self.declareProperty('ProcessCan', defaultValue=False, direction=Direction.Input, doc='Process the can')

        self.declareProperty(
            MatrixWorkspaceProperty('HABCountsCan', '', optional=PropertyMode.Optional, direction=Direction.Input),
            doc='High angle bank sample workspace in Q')

        self.declareProperty(
            MatrixWorkspaceProperty('HABNormCan', '', optional=PropertyMode.Optional, direction=Direction.Input),
            doc='High angle bank normalization workspace in Q')

        self.declareProperty(
            MatrixWorkspaceProperty('LABCountsCan', '', optional=PropertyMode.Optional, direction=Direction.Input),
            doc='Low angle bank sample workspace in Q')

        self.declareProperty(
            MatrixWorkspaceProperty('LABNormCan', '', optional=PropertyMode.Optional, direction=Direction.Input),
            doc='Low angle bank normalization workspace in Q')

        allowedModes = StringListValidator(self._make_mode_map().keys())

        self.declareProperty('Mode', 'None', validator=allowedModes, direction=Direction.Input,
                             doc='What to fit. Free parameter(s).')

        self.declareProperty('ScaleFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='Optional scaling factor')

        self.declareProperty('ShiftFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='Optional shift factor')

        self.declareProperty('QMin', defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc='Optional q-min')

        self.declareProperty('QMax', defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc='Optional q-max')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='Stitched high and low Q 1-D data')

        self.declareProperty('OutScaleFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Output,
                             doc='Applied scale factor')
        self.declareProperty('OutShiftFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Output,
                             doc='Applied shift factor')

        self.setPropertyGroup("Mode", 'Fitting')
        self.setPropertyGroup("ScaleFactor", 'Fitting')
        self.setPropertyGroup("ShiftFactor", 'Fitting')

        self.setPropertyGroup("HABCountsSample", 'Sample')
        self.setPropertyGroup("HABNormSample", 'Sample')
        self.setPropertyGroup("LABCountsSample", 'Sample')
        self.setPropertyGroup("LABNormSample", 'Sample')

        self.setPropertyGroup("OutputWorkspace", 'Output')
        self.setPropertyGroup("OutScaleFactor", 'Output')
        self.setPropertyGroup("OutShiftFactor", 'Output')

        can_settings = EnabledWhenProperty('ProcessCan', PropertyCriterion.IsNotDefault)

        self.setPropertyGroup("HABCountsCan", 'Can')
        self.setPropertyGroup("HABNormCan", 'Can')
        self.setPropertyGroup("LABCountsCan", 'Can')
        self.setPropertyGroup("LABNormCan", 'Can')
        self.setPropertySettings("HABCountsCan", can_settings)
        self.setPropertySettings("HABNormCan", can_settings)
        self.setPropertySettings("LABCountsCan", can_settings)
        self.setPropertySettings("LABNormCan", can_settings)

    def _divide(self, left, right):
        divide = self.createChildAlgorithm('Divide')
        divide.setProperty('LHSWorkspace', left)
        divide.setProperty('RHSWorkspace', right)
        divide.execute()
        return divide.getProperty('OutputWorkspace').value

    def _multiply(self, left, right):
        multiply = self.createChildAlgorithm('Multiply')
        multiply.setProperty('LHSWorkspace', left)
        multiply.setProperty('RHSWorkspace', right)
        multiply.execute()
        return multiply.getProperty('OutputWorkspace').value

    def _add(self, left, right):
        plus = self.createChildAlgorithm('Plus')
        plus.setProperty('LHSWorkspace', left)
        plus.setProperty('RHSWorkspace', right)
        plus.execute()
        return plus.getProperty('OutputWorkspace').value

    def _subract(self, left, right):
        minus = self.createChildAlgorithm('Minus')
        minus.setProperty('LHSWorkspace', left)
        minus.setProperty('RHSWorkspace', right)
        minus.execute()
        return minus.getProperty('OutputWorkspace').value

    def _crop_to_x_range(self, ws, x_min, x_max):
        crop = self.createChildAlgorithm('Rebin')
        crop.setProperty('InputWorkspace', ws)
        step = ws.readX(0)[1] - ws.readX(0)[0]
        crop.setProperty('Params', [x_min, step, x_max])
        crop.execute()
        return crop.getProperty('OutputWorkspace').value

    def _scale(self, ws, factor, operation='Multiply'):
        scale = self.createChildAlgorithm('Scale')
        scale.setProperty('InputWorkspace', ws)
        scale.setProperty('Operation', operation)
        scale.setProperty('Factor', factor)
        scale.execute()
        scaled = scale.getProperty('OutputWorkspace').value
        return scaled

    def _comment(self, ws, message):
        comment = self.createChildAlgorithm('Comment')
        comment.setProperty('Workspace', ws)
        comment.setProperty('Text', message)
        comment.execute()

    def _calculate_merged_q(self, cF, nF, cR, nR, shift_factor, scale_factor):
        # We want: (Cf+shift*Nf+Cr)/(Nf/scale + Nr)
        shifted_norm_front = self._scale(nF, shift_factor)
        scaled_norm_front = self._scale(nF, 1.0 / scale_factor)
        numerator = self._add(self._add(cF, shifted_norm_front), cR)
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

    def _get_error_corrected(self, front_data, rear_data, q_min, q_max):

        self._comment(front_data, 'Internal Step: Front data errors corrected as sqrt(rear_error^2 + front_error^2)')
        self._comment(rear_data, 'Internal Step: Rear data errors corrected as sqrt(rear_error^2 + front_error^2)')

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
            rear_error[0:len(rear_error)] = corrected_error[0:len(rear_error)]

        return front_data_cropped, rear_data_cropped

    def _get_start_q_and_end_q_values(self, rear_data, front_data):

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
            raise RuntimeError("The min value of the FRONT detector data set is larger"
                               "than the max value of the REAR detector data set")

        # Get the min and max range
        min_q = max(rear_q_min, front_q_min)
        max_q = min(rear_q_max, front_q_max)

        return min_q, max_q

    def _correct_q_resolution_for_merged(self, count_ws_front, count_ws_rear,
                                         output_ws, scale):
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

        self._comment(output_ws, 'Internal Step: q-resolution transferred from input workspaces')

        def divide_q_resolution_by_counts(q_res, counts):
            # We are dividing DX by Y. Note that len(DX) = len(Y) + 1
            # Unfortunately, we need some knowlege about the Q1D algorithm here.
            # The two last entries of DX are duplicate in Q1D and this is how we
            # treat it here.
            q_res_buffer = np.divide(q_res[0:-1], counts)
            q_res_buffer = np.append(q_res_buffer, q_res_buffer[-1])
            return q_res_buffer

        def multiply_q_resolution_by_counts(q_res, counts):
            # We are dividing DX by Y. Note that len(DX) = len(Y) + 1
            # Unfortunately, we need some knowlege about the Q1D algorithm here.
            # The two last entries of DX are duplicate in Q1D and this is how we
            # treat it here.
            q_res_buffer = np.multiply(q_res[0:-1], counts)
            q_res_buffer = np.append(q_res_buffer, q_res_buffer[-1])
            return q_res_buffer

        if count_ws_rear.getNumberHistograms() != 1:
            return

        # We require both count workspaces to contain the DX value
        if not count_ws_rear.hasDx(0) or not count_ws_front.hasDx(0):
            return

        q_resolution_front = count_ws_front.readDx(0)
        q_resolution_rear = count_ws_rear.readDx(0)
        counts_front = count_ws_front.readY(0)
        counts_rear = count_ws_rear.readY(0)

        # We need to make sure that the workspaces match in length
        if ((len(q_resolution_front) != len(q_resolution_rear)) or
                (len(counts_front) != len(counts_rear))):
            return

        # Get everything for the FRONT detector
        q_res_front_norm_free = multiply_q_resolution_by_counts(q_resolution_front, counts_front)
        q_res_front_norm_free = q_res_front_norm_free * scale
        counts_front = counts_front * scale

        # Get everything for the REAR detector
        q_res_rear_norm_free = multiply_q_resolution_by_counts(q_resolution_rear, counts_rear)

        # Now add and divide
        new_q_res = np.add(q_res_front_norm_free, q_res_rear_norm_free)
        new_counts = np.add(counts_front, counts_rear)
        q_resolution = divide_q_resolution_by_counts(new_q_res, new_counts)

        # Set the dx error
        output_ws.setDx(0, q_resolution)

    def _determine_factors(self, q_high_angle, q_low_angle, mode, scale, shift):

        # We need to make suret that the fitting only occurs in the y direction
        constant_x_shift_and_scale = ', f0.Shift=0.0, f0.XScaling=1.0'

        # Determine the StartQ and EndQ values
        q_min, q_max = self._get_start_q_and_end_q_values(rear_data=q_low_angle, front_data=q_high_angle,
                                                          )

        # We need to transfer the errors from the front data to the rear data, as we are using the the front data as a model, but
        # we want to take into account the errors of both workspaces.
        front_data_corrected, rear_data_corrected = self._get_error_corrected(rear_data=q_low_angle,
                                                                              front_data=q_high_angle,
                                                                              q_min=q_min, q_max=q_max)

        fit = self.createChildAlgorithm('Fit')

        # We currently have to put the front_data into the ADS so that the TabulatedFunction has access to it
        front_data_corrected = AnalysisDataService.addOrReplace('front_data_corrected', front_data_corrected)
        front_in_ads = AnalysisDataService.retrieve('front_data_corrected')

        function = 'name=TabulatedFunction, Workspace="' + str(
            front_in_ads.name()) + '"' + ";name=FlatBackground"

        fit.setProperty('Function', function)
        fit.setProperty('InputWorkspace', rear_data_corrected)

        constant_x_shift_and_scale = 'f0.Shift=0.0, f0.XScaling=1.0'
        if mode == Mode.BothFit:
            fit.setProperty('Ties', constant_x_shift_and_scale)
        elif mode == Mode.ShiftOnly:
            fit.setProperty('Ties', 'f0.Scaling=' + str(scale) + ',' + constant_x_shift_and_scale)
        elif mode == Mode.ScaleOnly:
            fit.setProperty('Ties', 'f1.A0=' + str(shift) + '*f0.Scaling,' + constant_x_shift_and_scale)
        else:
            raise RuntimeError('Unknown fitting mode requested.')

        fit.setProperty('StartX', q_min)
        fit.setProperty('EndX', q_max)
        fit.setProperty('CreateOutput', True)
        fit.execute()
        param = fit.getProperty('OutputParameters').value
        AnalysisDataService.remove(front_in_ads.name())

        # The outparameters are:
        # 1. Scaling in y direction
        # 2. Shift in x direction
        # 3. Scaling in x direction
        # 4. Shift in y direction
        # 5. Chi^2 value
        row0 = param.row(0).items()
        row3 = param.row(3).items()

        scale = row0[1][1]

        if scale == 0.0:
            raise RuntimeError('Fit scaling as part of stitching evaluated to zero')

        # In order to determine the shift, we need to remove the scale factor
        shift = row3[1][1] / scale

        return (shift, scale)

    def PyExec(self):
        enum_map = self._make_mode_map()

        mode = enum_map[self.getProperty('Mode').value]

        cF = self.getProperty('HABCountsSample').value
        cR = self.getProperty('LABCountsSample').value
        nF = self.getProperty('HABNormSample').value
        nR = self.getProperty('LABNormSample').value
        q_high_angle = self._divide(cF, nF)
        q_low_angle = self._divide(cR, nR)
        if self.getProperty('ProcessCan').value:
            cF_can = self.getProperty('HABCountsCan').value
            cR_can = self.getProperty('LABCountsCan').value
            nF_can = self.getProperty('HABNormCan').value
            nR_can = self.getProperty('LABNormCan').value

            q_high_angle_can = self._divide(cF_can, nF_can)
            q_low_angle_can = self._divide(cR_can, nR_can)

            # Now we can do the can subraction.
            q_high_angle = self._subract(q_high_angle, q_high_angle_can)
            q_low_angle = self._subract(q_low_angle, q_low_angle_can)

        shift_factor = self.getProperty('ShiftFactor').value
        scale_factor = self.getProperty('ScaleFactor').value
        if not mode == Mode.NoneFit:
            shift_factor, scale_factor = self._determine_factors(q_high_angle, q_low_angle, mode, scale=scale_factor,
                                                                 shift=shift_factor)

        min_q = min(min(cF.dataX(0)), min(cR.dataX(0)))
        max_q = max(max(cF.dataX(0)), max(cR.dataX(0)))

        # Crop our input workspaces
        cF = self._crop_to_x_range(cF, min_q, max_q)
        cR = self._crop_to_x_range(cR, min_q, max_q)
        nF = self._crop_to_x_range(nF, min_q, max_q)
        nR = self._crop_to_x_range(nR, min_q, max_q)

        # We want: (Cf+shift*Nf+Cr)/(Nf/scale + Nr)
        merged_q = self._calculate_merged_q(cF=cF, nF=nF, cR=cR, nR=nR, scale_factor=scale_factor,
                                            shift_factor=shift_factor)

        if self.getProperty('ProcessCan').value:
            cF_can = self.getProperty('HABCountsCan').value
            cR_can = self.getProperty('LABCountsCan').value
            nF_can = self.getProperty('HABNormCan').value
            nR_can = self.getProperty('LABNormCan').value

            # Calculate merged q for the can
            merged_q_can = self._calculate_merged_q_can(cF=cF_can, nF=nF_can, cR=cR_can, nR=nR_can,
                                                    scale_factor=scale_factor)
            # Subtract it from the sample
            merged_q = self._subract(merged_q, merged_q_can)

        if not mode == Mode.NoneFit:
            self._correct_q_resolution_for_merged(count_ws_front=cF, count_ws_rear=cR, output_ws=merged_q,
                                                  scale=scale_factor)

        self.setProperty('OutputWorkspace', merged_q)
        self.setProperty('OutScaleFactor', scale_factor)
        self.setProperty('OutShiftFactor', shift_factor)

    def _validateIs1DFromPropertyName(self, property_name):
        ws = self.getProperty(property_name).value
        if not ws:
            return True  # Mandatory validators to take care of this. Early exit.
        return ws.getNumberHistograms() == 1

    def _validateIsInQ(self, property_name):
        ws = self.getProperty(property_name).value
        if not ws:
            return True  # Mandatory validators to take care of this. Early exit.

        targetUnit = UnitFactory.create('MomentumTransfer')
        return targetUnit.caption() == ws.getAxis(0).getUnit().caption()

    def validateInputs(self):
        q_min_property_name = 'QMin'
        q_max_property_name = 'QMax'
        q_min_property = self.getProperty(q_min_property_name)
        q_max_property = self.getProperty(q_max_property_name)
        q_min = q_min_property.value
        q_max = q_max_property.value

        errors = dict()
        if not q_min_property.isDefault and not q_max_property.isDefault and q_min >= q_max:
            errors[q_min_property_name] = 'QMin must be < QMax'
            errors[q_max_property_name] = 'QMin must be < QMax'


        # Mode compatibility checks
        scale_factor_property = self.getProperty('ScaleFactor')
        shift_factor_property = self.getProperty('ShiftFactor')
        mode_property = self.getProperty('Mode')
        enum_map = self._make_mode_map()
        mode = enum_map[mode_property.value]
        if mode == Mode.NoneFit:
            if scale_factor_property.isDefault:
                errors[scale_factor_property.name] = 'ScaleFactor required'
            if shift_factor_property.isDefault:
                errors[shift_factor_property.name] = 'ShiftFactor required'
        elif mode == Mode.ScaleOnly:
            if shift_factor_property.isDefault:
                errors[shift_factor_property.name] = 'ShiftFactor required'
        elif mode == Mode.ShiftOnly:
            if scale_factor_property.isDefault:
                errors[scale_factor_property.name] = 'ScaleFactor required'

        workspace_property_names = ['HABCountsSample', 'LABCountsSample', 'HABNormSample', 'LABNormSample']
        # 1d data check
        self._validate_1D(workspace_property_names, errors, mode)

        # Units check
        self._validate_units(workspace_property_names, errors)

        process_can = self.getProperty('ProcessCan')
        if bool(process_can.value):
            workspace_property_names = ['HABCountsCan', 'HABNormCan', 'LABCountsCan', 'LABNormCan']
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
                errors[property_name] = 'Workspace must have units of momentum transfer'

    def _validate_1D(self, workspace_property_names, errors, mode):
        if mode != Mode.NoneFit:
            for property_name in workspace_property_names:
                if not self._validateIs1DFromPropertyName(property_name):
                    errors[property_name] = 'Wrong number of spectra. Must be 1D input'

    def _validate_provided(self, workspace_property_names, errors):
        for property_name in workspace_property_names:
            if not self.getProperty(property_name).value:
                errors[property_name] = 'This workspace is required in order to process the can'


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSStitch1D)
