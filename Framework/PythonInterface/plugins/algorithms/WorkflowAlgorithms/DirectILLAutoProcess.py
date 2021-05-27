# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import DirectILL_common as common
from mantid.api import MatrixWorkspaceProperty, MultipleFileProperty, \
    PropertyMode, PythonAlgorithm, WorkspaceGroupProperty, FileAction, \
    AlgorithmFactory
from mantid.kernel import Direction, FloatArrayProperty, FloatArrayOrderedPairsValidator, \
    FloatBoundedValidator, IntArrayProperty, Property, PropertyManagerProperty, \
    RebinParamsValidator, StringListValidator


class DirectILLAutoProcess(PythonAlgorithm):

    _instrument = None

    def category(self):
        return "{};{}".format(common.CATEGORIES, "ILL\\Auto")

    def summary(self):
        return 'Performs automatic data reduction for the direct geometry TOF spectrometers at ILL.'

    def seeAlso(self):
        return ['DirectILLReduction']

    def name(self):
        return 'DirectILLAutoProcess'

    def validateInputs(self):
        issues = dict()

        return issues

    def PyInit(self):

        positiveFloat = FloatBoundedValidator(0., exclusive=True)
        validRebinParams = RebinParamsValidator(AllowEmpty=True)
        orderedPairsValidator = FloatArrayOrderedPairsValidator()

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output workspace group containing reduced data.')

        self.declareProperty(MultipleFileProperty('Runs',
                                                  action=FileAction.Load,
                                                  extensions=['nxs']),
                             doc='Run(s) to be processed.')

        processes = ['Cadmium', 'Empty', 'Vanadium', 'Sample']
        self.declareProperty(name='ProcessAs',
                             defaultValue='Sample',
                             validator=StringListValidator(processes),
                             doc='Choose the process type.')

        reduction_options = ['Powder', 'SingleCrystal']
        self.declareProperty(name='ReductionType',
                             defaultValue='Powder',
                             validator=StringListValidator(reduction_options),
                             doc='Choose the appropriate reduction type for the data to process.')

        self.declareProperty(WorkspaceGroupProperty('VanadiumWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='Vanadium input workspace.')

        self.declareProperty(MatrixWorkspaceProperty('EmptyContainerWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Empty container workspace.')

        self.declareProperty('EmptyContainerScaling', 1.0,
                             doc='Scaling factor for the empty container.')

        self.declareProperty('FlatBackground', "",
                             doc='File(s) or workspaces containing the source to calculate flat background.')

        self.declareProperty('FlatBkgScaling', 1.0,
                             doc='Scaling parameter for the flat background.')

        self.declareProperty(common.PROP_ABSOLUTE_UNITS, False,
                             doc='Enable or disable normalisation to absolute units.')

        self.declareProperty("IncidentEnergyCalibration", True,
                             doc='Enable or disable incident energy calibration.')

        self.declareProperty("ElasticChannelCalibration", False,
                             doc='Enable or disable calibration of the elastic channel.')

        additional_inputs_group = 'Correction workspaces'
        self.setPropertyGroup('VanadiumWorkspace', additional_inputs_group)
        self.setPropertyGroup('EmptyContainerWorkspace', additional_inputs_group)
        self.setPropertyGroup('EmptyContainerScaling', additional_inputs_group)
        self.setPropertyGroup('FlatBackground', additional_inputs_group)
        self.setPropertyGroup('FlatBkgScaling', additional_inputs_group)
        self.setPropertyGroup('IncidentEnergyCalibration', additional_inputs_group)
        self.setPropertyGroup('ElasticChannelCalibration', additional_inputs_group)
        self.setPropertyGroup(common.PROP_ABSOLUTE_UNITS, additional_inputs_group)

        self.declareProperty(name='IncidentEnergy',
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             doc='Value for the calibrated incident energy (meV).')

        self.declareProperty(FloatArrayProperty(name='IncidentEnergyRange', values=[], validator=orderedPairsValidator),
                             doc='Minimum and maximum energy for neutrons in a time frame (meV).')

        self.declareProperty(name='ElasticChannelEnergy',
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             doc='Value for the energy of the elastic peak (meV).')

        self.declareProperty('SampleAngleOffset', 0.0,
                             doc='Value for the offset parameter in omega scan (degrees).')

        parameters_group = 'Parameters'
        self.setPropertyGroup('IncidentEnergy', parameters_group)
        self.setPropertyGroup('IncidentEnergyRange', parameters_group)
        self.setPropertyGroup('ElasticChannelEnergy', parameters_group)
        self.setPropertyGroup('SampleAngleOffset', parameters_group)

        # The mask workspace replaces MaskWorkspace parameter from PantherSingle and DiagnosticsWorkspace from directred
        self.declareProperty('MaskWorkspace', '',
                             doc='File(s) or workspaces containing the mask.')

        self.declareProperty(IntArrayProperty(name='MaskedTubes', direction=Direction.Input),
                             doc='List of tubes to be masked.')

        self.declareProperty('MaskThreshold', 0.0, doc='Create a mask with threshold from the ELPReference'
                                                       ' to remove empty / background pixels.')

        self.declareProperty(FloatArrayProperty(name='MaskedAngles', values=[], validator=orderedPairsValidator),
                             doc='Mask detectors in the given angular range.')

        self.declareProperty('MaskWithVanadium', False,
                             doc='Whether to mask using vanadium workspace.')

        masking_group_name = 'Masking'
        self.setPropertyGroup('MaskWorkspace', masking_group_name)
        self.setPropertyGroup('MaskedTubes', masking_group_name)
        self.setPropertyGroup('MaskThreshold', masking_group_name)
        self.setPropertyGroup('MaskedAngles', masking_group_name)
        self.setPropertyGroup('MaskWithVanadium', masking_group_name)

        self.declareProperty(FloatArrayProperty(name='EnergyBinning',
                                                validator=validRebinParams),
                             doc='Energy binning parameters.')

        self.declareProperty(FloatArrayProperty(name='MomentumTransferBinning',
                                                validator=validRebinParams),
                             doc='Momentum transfer binning parameters.')

        rebinning_group = 'Binning parameters'
        self.setPropertyGroup('EnergyBinning', rebinning_group)
        self.setPropertyGroup('MomentumTransferBinning', rebinning_group)

        self.declareProperty(name='AbsorptionCorrection',
                             defaultValue='None',
                             validator=StringListValidator(['None', 'Fast', 'Full']),
                             doc='Choice of approach to absorption correction.')

        self.declareProperty(name='SelfAttenuationMethod',
                             defaultValue='MonteCarlo',
                             validator=StringListValidator(['Numerical', 'MonteCarlo']),
                             doc='Choice of calculation method for the attenuation calculation.')

        self.declareProperty('SampleMaterial', "", doc='Sample material.')

        self.declareProperty(name='SampleShape',
                             defaultValue='None',
                             validator=StringListValidator(['FlatPlate', 'Cylinder', 'Annulus']),
                             doc='Sample material.')

        self.declareProperty(PropertyManagerProperty('SampleGeometry', dict()),
                             doc="Dictionary for the sample geometry.")

        self.declareProperty('ContainerMaterial', "", doc='Container material.')

        self.declareProperty(PropertyManagerProperty('ContainerGeometry', dict()),
                             doc="Dictionary for the container geometry.")

        attenuation_group = 'Sample attenuation'
        self.setPropertyGroup('AbsorptionCorrection', attenuation_group)
        self.setPropertyGroup('SelfAttenuationMethod', attenuation_group)
        self.setPropertyGroup('SampleMaterial', attenuation_group)
        self.setPropertyGroup('SampleShape', attenuation_group)
        self.setPropertyGroup('SampleGeometry', attenuation_group)
        self.setPropertyGroup('ContainerMaterial', attenuation_group)
        self.setPropertyGroup('ContainerGeometry', attenuation_group)

        self.declareProperty(name='DetectorGrouping',
                             defaultValue="",
                             doc='Grouping pattern to reduce the granularity of the output.')

        self.declareProperty(name='GroupPixelsBy',
                             defaultValue=1,
                             doc='Step to use when grouping detectors to reduce the granularity of the output.')

        self.declareProperty(name=common.PROP_GROUPING_ANGLE_STEP,
                             defaultValue=Property.EMPTY_DBL,
                             validator=positiveFloat,
                             doc='A scattering angle step to which to group detectors, in degrees.')

        grouping_options_group = 'Grouping options'
        self.setPropertyGroup('DetectorGrouping', grouping_options_group)
        self.setPropertyGroup('GroupPixelsBy', grouping_options_group)
        self.setPropertyGroup(common.PROP_GROUPING_ANGLE_STEP, grouping_options_group)

    def PyExec(self):
        pass


AlgorithmFactory.subscribe(DirectILLAutoProcess)
