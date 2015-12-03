#pylint: disable=no-init
from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode
from mantid.kernel import Direction, Property, StringListValidator, UnitFactory


class SANSStitch1D(DataProcessorAlgorithm):

    def category(self):
        return 'DataHandling\\Logs'


    def summary(self):
        return 'Add multiple sample logs to a workspace'


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('HABSample', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='High angle bank sample workspace in Q')

        self.declareProperty(MatrixWorkspaceProperty('HABNorm', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='High angle bank normalization workspace in Q')

        self.declareProperty(MatrixWorkspaceProperty('LABSample', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='Low angle bank sample workspace in Q')

        self.declareProperty(MatrixWorkspaceProperty('LABNorm', '', optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='Low angle bank normalization workspace in Q')

        self.declareProperty('ScaleFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc='Optional scaling factor')

        self.declareProperty('ShiftFactor', defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc='Optional shift factor')

        self.declareProperty('QMin', defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc='Optional q-min')

        self.declareProperty('QMax', defaultValue=Property.EMPTY_DBL, direction=Direction.Input, doc='Optional q-max')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='Stitched high and low Q 1-D data')

        allowedModes = StringListValidator(['ScaleOnly', 'ShiftOnly', 'Both', 'None'])

        self.declareProperty('Mode', 'None', validator=allowedModes, direction=Direction.Input, doc='What to fit')


    def PyExec(self):
        pass


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
        if mode_property.value == 'None':
            if scale_factor_property.isDefault:
                errors[scale_factor_property.name] = 'ScaleFactor required'
            if shift_factor_property.isDefault:
                errors[shift_factor_property.name] = 'ShiftFactor required'
        elif mode_property.value == 'ScaleOnly':
            if shift_factor_property.isDefault:
                errors[shift_factor_property.name] = 'ShiftFactor required'
        elif mode_property.value == 'ShiftOnly':
            if shift_factor_property.isDefault:
                errors[scale_factor_property.name] = 'ScaleFactor required'


        # 1d data check
        message_when_not_1d = 'Wrong number of spectra. Must be 1D input'
        if not self._validateIs1DFromPropertyName('HABSample'):
            errors['HABSample'] = message_when_not_1d
        if not self._validateIs1DFromPropertyName('LABSample'):
            errors['LABSample'] = message_when_not_1d
        if not self._validateIs1DFromPropertyName('HABNorm'):
            errors['HABNorm'] = message_when_not_1d
        if not self._validateIs1DFromPropertyName('LABNorm'):
            errors['LABNorm'] = message_when_not_1d

        # Units check
        message_when_not_in_q = 'Workspace must have units of momentum transfer'
        if not self._validateIsInQ('HABSample'):
            errors['HABSample'] = message_when_not_in_q
        if not self._validateIsInQ('LABSample'):
            errors['LABSample'] = message_when_not_in_q
        if not self._validateIsInQ('HABNorm'):
            errors['HABNorm'] = message_when_not_in_q
        if not self._validateIsInQ('LABNorm'):
            errors['LABNorm'] = message_when_not_in_q




        return errors





# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSStitch1D)
