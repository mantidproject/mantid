#pylint: disable=no-init
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
        return {'ShiftOnly':Mode.ShiftOnly, 'ScaleOnly':Mode.ScaleOnly,
                'Both':Mode.BothFit,'None':Mode.NoneFit}


    def category(self):
        return 'DataHandling\\Logs'


    def summary(self):
        return 'Stitch the high angle and low angle banks of a workspace together'


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('HABCountsSample', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='High angle bank sample workspace in Q')

        self.declareProperty(MatrixWorkspaceProperty('HABNormSample', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='High angle bank normalization workspace in Q')

        self.declareProperty(MatrixWorkspaceProperty('LABCountsSample', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='Low angle bank sample workspace in Q')

        self.declareProperty(MatrixWorkspaceProperty('LABNormSample', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='Low angle bank normalization workspace in Q')


        self.declareProperty('ProcessCan', defaultValue=False, direction=Direction.Input, doc='Process the can')

        self.declareProperty(MatrixWorkspaceProperty('HABCountsCan', '', optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='High angle bank sample workspace in Q')

        self.declareProperty(MatrixWorkspaceProperty('HABNormCan', '', optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='High angle bank normalization workspace in Q')

        self.declareProperty(MatrixWorkspaceProperty('LABCountsCan', '', optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='Low angle bank sample workspace in Q')

        self.declareProperty(MatrixWorkspaceProperty('LABNormCan', '', optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='Low angle bank normalization workspace in Q')

        allowedModes = StringListValidator(self._make_mode_map().keys())

        self.declareProperty('Mode', 'None', validator=allowedModes, direction=Direction.Input, doc='What to fit')

        self.declareProperty('ScaleFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc='Optional scaling factor')

        self.declareProperty('ShiftFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc='Optional shift factor')

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

    def PyExec(self):
        enum_map = self._make_mode_map()

        mode = enum_map[self.getProperty('Mode').value]


        scale_factor = 0;
        shift_factor = 0;
        if mode == Mode.NoneFit:
            shift_factor = 1
            scale_factor = 1






    def _validateIs1DFromPropertyName(self, property_name):
        ws = self.getProperty(property_name).value
        if not ws:
            return True # Mandatory validators to take care of this. Early exit.
        return ws.getNumberHistograms() == 1

    def _validateIsInQ(self, property_name):
        ws = self.getProperty(property_name).value
        if not ws:
            return True # Mandatory validators to take care of this. Early exit.

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

        # 1d data check
        message_when_not_1d = 'Wrong number of spectra. Must be 1D input'
        if not self._validateIs1DFromPropertyName('HABCountsSample'):
            errors['HABCountsSample'] = message_when_not_1d
        if not self._validateIs1DFromPropertyName('LABCountsSample'):
            errors['LABCountsSample'] = message_when_not_1d
        if not self._validateIs1DFromPropertyName('HABNormSample'):
            errors['HABNormSample'] = message_when_not_1d
        if not self._validateIs1DFromPropertyName('LABNormSample'):
            errors['LABNormSample'] = message_when_not_1d

        # Units check
        message_when_not_in_q = 'Workspace must have units of momentum transfer'
        if not self._validateIsInQ('HABCountsSample'):
            errors['HABCountsSample'] = message_when_not_in_q
        if not self._validateIsInQ('LABCountsSample'):
            errors['LABCountsSample'] = message_when_not_in_q
        if not self._validateIsInQ('HABNormSample'):
            errors['HABNormSample'] = message_when_not_in_q
        if not self._validateIsInQ('LABNormSample'):
            errors['LABNormSample'] = message_when_not_in_q

        process_can = self.getProperty('ProcessCan')
        hab_counts_can = self.getProperty('HABCountsCan')
        hab_norm_can = self.getProperty('HABNormCan')
        lab_counts_can = self.getProperty('LABCountsCan')
        lab_norm_can = self.getProperty('LABNormCan')
        mesage_when_can_required = 'This workspace is required in order to process the can'
        if bool(process_can.value):
            if hab_counts_can.isDefault:
                errors[hab_counts_can.name] = mesage_when_can_required
            if lab_counts_can.isDefault:
                errors[lab_counts_can.name] = mesage_when_can_required
            if hab_norm_can.isDefault:
                errors[hab_norm_can.name] = mesage_when_can_required
            if lab_norm_can.isDefault:
                errors[lab_norm_can.name] = mesage_when_can_required




        return errors





# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSStitch1D)
