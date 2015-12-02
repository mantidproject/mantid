#pylint: disable=no-init
from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, PropertyMode
from mantid.kernel import Direction, Property, StringListValidator


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


        scale_factor_property = self.getProperty('ScaleFactor')
        shift_factor_property = self.getProperty('ShiftFactor')
        mode_property = self.getProperty('Mode')

        if(mode_property.value == 'None'):
            if scale_factor_property.isDefault:
                errors[scale_factor_property.name] = 'ScaleFactor required'
            if shift_factor_property.isDefault:
                errors[shift_factor_property.name] = 'ShiftFactor required'

        return errors


    # 1D data
    # Units in Q
    # Scale modes require inputs



# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSStitch1D)
