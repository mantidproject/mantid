# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import DirectILL_common as common
from mantid.api import AlgorithmFactory, FileAction, MatrixWorkspaceProperty, \
    MultipleFileProperty, PropertyMode, PythonAlgorithm, \
    WorkspaceGroupProperty
from mantid.kernel import Direction, FloatArrayProperty, FloatArrayOrderedPairsValidator, \
    FloatBoundedValidator, IntArrayProperty, Property, PropertyManagerProperty, \
    RebinParamsValidator, StringListValidator
from mantid.simpleapi import *

from os import path


def get_run_number(value):
    """
    Extracts the run number from the first run out of the string value of a
    multiple file property of numors
    """
    return path.splitext(path.basename(value.split(',')[0].split('+')[0]))[0]


class DirectILLAutoProcess(PythonAlgorithm):

    instrument = None
    sample = None
    process = None
    reduction_type = None
    incident_energy_calibration = None
    incident_energy_ws = None
    elastic_channel_ws = None
    masking = None
    ebinning_params = None  # energy binning
    output = None
    flat_bkg_scaling = None
    flat_background = None

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

        run_no_err = 'Wrong number of {0} runs: {1}. Provide one or as many as '\
                     'sample runs: {2}.'
        runs_sample = len(self.getPropertyValue('Runs'))
        if not self.getProperty('EmptyContainerWorkspace').isDefault:
            runs_container = mtd[self.getPropertyValue('EmptyContainerWorkspace')].getNumberOfEntries()
            if runs_container != 1 and runs_container > runs_sample:
                issues['BeamRuns'] = run_no_err.format('EmptyContainerWorkspace', runs_container, runs_sample)

        grouping_err_msg = 'Only one grouping method can be specified.'
        if self.getProperty('DetectorGrouping').isDefault:
            if not self.getProperty('GroupPixelsBy').isDefault \
                    and not self.getProperty(common.PROP_GROUPING_ANGLE_STEP).isDefault:
                issues['GroupPixelsBy'] = grouping_err_msg
                issues[common.PROP_GROUPING_ANGLE_STEP] = grouping_err_msg
        else:
            if not self.getProperty('GroupPixelsBy').isDefault:
                issues['DetectorGrouping'] = grouping_err_msg
                issues['GroupPixelsBy'] = grouping_err_msg
            if not self.getProperty(common.PROP_GROUPING_ANGLE_STEP).isDefault:
                issues['DetectorGrouping'] = grouping_err_msg
                issues[common.PROP_GROUPING_ANGLE_STEP] = grouping_err_msg

        if self.getProperty('IncidentEnergyCalibration').value and self.getProperty('IncidentEnergy').isDefault:
            issues['IncidentEnergy'] = 'Please provide a value for the incident energy in meV.'

        if self.getProperty('ElasticChannelCalibration').value and self.getProperty('ElasticChannelEnergy').isDefault:
            issues['ElasticChannelEnergy'] = 'Please provide a value for the elastic channel energy in meV.'

        if self.getProperty('MaskWithVanadium').value and self.getProperty('VanadiumWorkspace').isDefault:
            issues['VanadiumWorkspace'] = 'Please provide a vanadium input for a masking reference.'

        if self.getPropertyValue('AbsorptionCorrection') != 'None':
            if self.getProperty('SampleMaterial').isDefault:
                issues['SampleMaterial'] = 'Please define sample material.'
            if self.getProperty('SampleShape').isDefault:
                issues['SampleShape'] = 'Please define sample shape.'
            if self.getProperty('SampleGeometry').isDefault:
                issues['SampleGeometry'] = 'Please define sample geometry.'
            if self.getProperty('ContainerMaterial').isDefault:
                issues['ContainerMaterial'] = 'Please define container material.'
            if self.getProperty('ContainerGeometry').isDefault:
                issues['ContainerGeometry'] = 'Please define container geometry.'

        return issues

    def setUp(self):
        self.sample = self.getPropertyValue('Runs').split(',')
        self.output = self.getPropertyValue('OutputWorkspace')
        self.process = self.getPropertyValue('ProcessAs')
        self.reduction_type = self.getPropertyValue('ReductionType')
        if self.getProperty('IncidentEnergyCalibration').value:
            self.incident_energy_calibration = 'Energy Calibration ON'
            self.incident_energy_ws = 'incident_energy_ws'
            CreateSingleValuedWorkspace(DataValue=self.getProperty('IncidentEnergy').value,
                                        OutputWorkspace=self.incident_energy_ws)
        if self.getProperty('ElasticChannelCalibration').value:
            self.elastic_channel_ws = 'elastic_channel_ws'
            CreateSingleValuedWorkspace(DataValue=self.getProperty('ElasticChannelCalibration').value,
                                        OutputWorkspace=self.elastic_channel_ws)

        if (self.getProperty('MaskWorkspace').isDefault and self.getProperty('MaskedTubes').isDefault
                and self.getProperty('MaskThreshold').isDefault and self.getProperty('MaskedAngles').isDefault
                and self.getProperty('MaskWithVanadium').isDefault):
            self.masking = False
        else:
            self.masking = True
        self.flat_bkg_scaling = self.getProperty('FlatBkgScaling')
        self.ebinning_params = self.getProperty('EnergyBinning')

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

        self.declareProperty(MatrixWorkspaceProperty('CadmiumWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Cadmium absorber workspace.')

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

        self.declareProperty(IntArrayProperty(name='MaskedTubes', values=[], direction=Direction.Input),
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

        self.declareProperty(PropertyManagerProperty('SampleMaterial', dict()),
                             doc='Sample material definitions.')

        self.declareProperty(name='SampleShape',
                             defaultValue='None',
                             validator=StringListValidator(['FlatPlate', 'Cylinder', 'Annulus']),
                             doc='Sample material.')

        self.declareProperty(PropertyManagerProperty('SampleGeometry', dict()),
                             doc="Dictionary for the sample geometry.")

        self.declareProperty(PropertyManagerProperty('ContainerMaterial', dict()),
                             doc='Container material definitions.')

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
        self.setUp()
        sample_runs = self.getPropertyValue('Runs').split(',')
        output_samples = []
        for sample in sample_runs:
            ws = self._collect_data(sample, vanadium=self.process == 'Vanadium')
            if self.process in ['Empty', 'Cadmium']:
                pass
            elif self.process == 'Vanadium':
                pass
            elif self.process == 'Sample':
                ws = self._process_sample(ws)
                output_samples.append(ws)
        GroupWorkspaces(InputWorkspaces=output_samples,
                        OutputWorkspace=self.output)
        self.setProperty('OutputWorkspace', mtd[self.output])

    def _collect_data(self, sample, vanadium=False):
        """Loads data if the corresponding workspace does not exist in the ADS."""
        ws = "{}_{}".format(get_run_number(sample), 'raw')
        kwargs = dict()
        if not self.getProperty('FlatBackground').isDefault:
            kwargs['FlatBkgWorkspace'] = self.getPropertyValue('FlatBackground')
            kwargs['FlatBkgScaling'] = self.flat_bkg_scaling
        if ws not in mtd:
            DirectILLCollectData(Run=sample, OutputWorkspace=ws,
                                 IncidentEnergyCalibration=self.incident_energy_calibration,
                                 IncidentEnergyWorkspace=self.incident_energy_ws,
                                 ElasticChannelWorkspace=self.elastic_channel_ws,
                                 **kwargs)
            instrument = mtd[ws].getInstrument().getName()
            if self.instrument and instrument != self.instrument:
                self.log().error("Sample data: {} comes from different instruments that the rest of the data:"
                                 " {} and {}".format(sample, instrument, self.instrument))
            else:
                self.instrument = instrument
        return ws

    def _process_sample(self, ws):
        return ws


AlgorithmFactory.subscribe(DirectILLAutoProcess)
