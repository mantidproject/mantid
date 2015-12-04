# pylint: disable=no-init
from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode
from mantid.kernel import Direction, Property, StringListValidator, UnitFactory, \
    EnabledWhenProperty, PropertyCriterion


class Mode:
    class ShiftOnly: pass

    class ScaleOnly: pass

    class BothFit: pass

    class NoneFit: pass


class SANSStitch1D(DataProcessorAlgorithm):
    def _make_mode_map(self):
        return {'ShiftOnly': Mode.ShiftOnly, 'ScaleOnly': Mode.ScaleOnly,
                'Both': Mode.BothFit, 'None': Mode.NoneFit}

    def category(self):
        return 'DataHandling\\Logs'

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

        self.declareProperty('Mode', 'None', validator=allowedModes, direction=Direction.Input, doc='What to fit')

        self.declareProperty('ScaleFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='Optional scaling factor')

        self.declareProperty('ShiftFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Input,
                             doc='Optional shift factor')

        self.declareProperty('QMin', defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc='Optional q-min')

        self.declareProperty('QMax', defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc='Optional q-max')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='Stitched high and low Q 1-D data')

        self.setPropertyGroup("Mode", 'Fitting')
        self.setPropertyGroup("ScaleFactor", 'Fitting')
        self.setPropertyGroup("ShiftFactor", 'Fitting')

        self.setPropertyGroup("HABCountsSample", 'Sample')
        self.setPropertyGroup("HABNormSample", 'Sample')
        self.setPropertyGroup("LABCountsSample", 'Sample')
        self.setPropertyGroup("LABNormSample", 'Sample')

        can_settings = EnabledWhenProperty('ProcessCan', PropertyCriterion.IsNotDefault)

        self.setPropertyGroup("HABCountsCan", 'Can')
        self.setPropertyGroup("HABNormCan", 'Can')
        self.setPropertyGroup("LABCountsCan", 'Can')
        self.setPropertyGroup("LABNormCan", 'Can')
        self.setPropertySettings("HABCountsCan", can_settings)
        self.setPropertySettings("HABNormCan", can_settings)
        self.setPropertySettings("LABCountsCan", can_settings)
        self.setPropertySettings("LABNormCan", can_settings)

    def _divide(self, a, b):
        divide = self.createChildAlgorithm('Divide')
        divide.setProperty('LHSWorkspace', a)
        divide.setProperty('RHSWorkspace', b)
        divide.execute()
        return divide.getProperty('OutputWorkspace').value

    def _multiply(self, a, b):
        multiply = self.createChildAlgorithm('Multiply')
        multiply.setProperty('LHSWorkspace', a)
        multiply.setProperty('RHSWorkspace', b)
        multiply.execute()
        return multiply.getProperty('OutputWorkspace').value

    def _add(self, a, b):
        plus = self.createChildAlgorithm('Plus')
        plus.setProperty('LHSWorkspace', a)
        plus.setProperty('RHSWorkspace', b)
        plus.execute()
        return plus.getProperty('OutputWorkspace').value

    def _crop_to_x_range(self, ws, x_min, x_max):
        crop = self.createChildAlgorithm('CropWorkspace')
        crop.setProperty('InputWorkspace', ws)
        crop.setProperty('XMin', x_min)
        crop.setProperty('XMax', x_max)
        crop.execute()
        return crop.getProperty('OutputWorkspace').value


    def PyExec(self):
        enum_map = self._make_mode_map()

        mode = enum_map[self.getProperty('Mode').value]

        scale_factor = 0
        shift_factor = 0
        if mode == Mode.NoneFit:
            shift_factor = self.getProperty('ShiftFactor').value
            scale_factor = self.getProperty('ScaleFactor').value

        cF = self.getProperty('HABCountsSample').value
        cR = self.getProperty('LABCountsSample').value
        nF = self.getProperty('HABNormSample').value
        nR = self.getProperty('LABNormSample').value

        min_q = min(min(cF.dataX(0)), min(cR.dataX(0)))
        max_q = max(max(cF.dataX(0)), max(cR.dataX(0)))

        # Crop our input workspaces
        cF = self._crop_to_x_range(cF, min_q, max_q)
        cR = self._crop_to_x_range(cR, min_q, max_q)
        nF = self._crop_to_x_range(nF, min_q, max_q)
        nR = self._crop_to_x_range(nR, min_q, max_q)


        # We want: (Cf+shift*Nf+Cr)/(Nf/scale + Nr)
        scale = self.createChildAlgorithm('Scale')
        scale.setProperty('InputWorkspace', nF)
        scale.setProperty('Operation', 'Multiply')
        scale.setProperty('Factor', shift_factor)
        scale.execute()
        shifted_norm_front = scale.getProperty('OutputWorkspace').value

        scale.setProperty('InputWorkspace', nF)
        scale.setProperty('Operation', 'Multiply')
        scale.setProperty('Factor', 1.0/scale_factor)
        scale.execute()
        scaled_norm_front = scale.getProperty('OutputWorkspace').value

        numerator = self._add(self._add(cF , shifted_norm_front), cR)
        denominator = self._add(scaled_norm_front, nR)

        merged_q = self._divide(numerator, denominator)

        self.setProperty('OutputWorkspace', merged_q)


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
        if enum_map[mode_property.value] == Mode.NoneFit:
            if scale_factor_property.isDefault:
                errors[scale_factor_property.name] = 'ScaleFactor required'
            if shift_factor_property.isDefault:
                errors[shift_factor_property.name] = 'ShiftFactor required'
        elif enum_map[mode_property.value] == Mode.ScaleOnly:
            if shift_factor_property.isDefault:
                errors[shift_factor_property.name] = 'ShiftFactor required'
        elif enum_map[mode_property.value] == Mode.ShiftOnly:
            if shift_factor_property.isDefault:
                errors[scale_factor_property.name] = 'ScaleFactor required'

        workspace_property_names = ['HABCountsSample', 'LABCountsSample', 'HABNormSample', 'LABNormSample']
        # 1d data check
        self._validate_1D(workspace_property_names, errors)

        # Units check
        self._validate_units(workspace_property_names, errors)

        process_can = self.getProperty('ProcessCan')
        if bool(process_can.value):
            workspace_property_names = ['HABCountsCan', 'HABNormCan', 'LABCountsCan', 'LABNormCan']
            # Check existance
            self._validate_provided(workspace_property_names, errors)
            # Check Q units
            self._validate_units(workspace_property_names, errors)

        return errors

    def _validate_units(self, workspace_property_names, errors):
        for property_name in workspace_property_names:
            if not self._validateIsInQ(property_name):
                errors[property_name] = 'Workspace must have units of momentum transfer'

    def _validate_1D(self, workspace_property_names, errors):
        for property_name in workspace_property_names:
            if not self._validateIs1DFromPropertyName(property_name):
                errors[property_name] = 'Wrong number of spectra. Must be 1D input'

    def _validate_provided(self, workspace_property_names, errors):
        for property_name in workspace_property_names:
            if not self.getProperty(property_name).value:
                errors[property_name] = 'This workspace is required in order to process the can'


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSStitch1D)
