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

import numpy as np


def get_run_number(value):
    """
    Extracts the run number from the first run out of the string value of a
    multiple file property of numors
    """
    return path.splitext(path.basename(value.split(',')[0].split('+')[0]))[0]


def get_vanadium_corrections(vanadium_ws):
    """
    Extracts vanadium integral and vanadium diagnostics workspaces from the provided list. If the provided group
    has only one workspace, then it is assumed it contains vanadium integral. Assumed is the following order
    of vanadium workspaces for each numor in the group: SofQ, SofTW, diagnostics, integral.
    :param vanadium_ws: workspace group with processed vanadium
    :return: vanadium integral and vanadium diagnostics (if exist)
    """
    diagnostics = []
    integrals = []
    nentries = mtd[vanadium_ws].getNumberOfEntries()
    if nentries == 1:
        integrals.append(mtd[vanadium_ws][0].name())
    else:
        for index in range(0, nentries, 4):
            diagnostics.append(mtd[vanadium_ws][index+2].name())
            integrals.append(mtd[vanadium_ws][index+3].name())
    return diagnostics, integrals


class DirectILLAutoProcess(PythonAlgorithm):

    instrument = None
    sample = None
    process = None
    reduction_type = None
    incident_energy = None
    incident_energy_calibration = None
    incident_energy_ws = None
    elastic_channel_ws = None
    masking = None
    mask_ws = None
    ebinning_params = None  # energy binning
    output = None
    vanadium = None
    vanadium_epp = None
    vanadium_diagnostics = None
    vanadium_integral = None
    empty = None
    flat_bkg_scaling = None
    flat_background = None
    to_clean = None
    absorption_corr = None
    save_output = None

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

        if self.getProperty('MaskWithVanadium').value and self.getProperty('VanadiumWorkspace').isDefault:
            issues['VanadiumWorkspace'] = 'Please provide a vanadium input for a masking reference.'

        if self.getPropertyValue('AbsorptionCorrection') != 'None':
            if self.getProperty('SampleMaterial').isDefault:
                issues['SampleMaterial'] = 'Please define sample material.'
            if self.getProperty('SampleGeometry').isDefault:
                issues['SampleGeometry'] = 'Please define sample geometry.'

        return issues

    def setUp(self):
        self.sample = self.getPropertyValue('Runs').split(',')
        self.output = self.getPropertyValue('OutputWorkspace')
        self.process = self.getPropertyValue('ProcessAs')
        self.reduction_type = self.getPropertyValue('ReductionType')
        self.to_clean = []
        if self.getProperty('IncidentEnergyCalibration').value:
            self.incident_energy_calibration = 'Energy Calibration ON'
        if not self.getProperty('IncidentEnergy').isDefault:
            self.incident_energy = self.getProperty('IncidentEnergy').value
            self.incident_energy_ws = 'incident_energy_ws'
            CreateSingleValuedWorkspace(DataValue=self.incident_energy,
                                        OutputWorkspace=self.incident_energy_ws)
            self.to_clean.append(self.incident_energy_ws)
        if not self.getProperty('ElasticChannelEnergy').isDefault:
            self.elastic_channel_ws = 'elastic_channel_energy_ws'
            CreateSingleValuedWorkspace(DataValue=self.getProperty('ElasticChannelEnergy').value,
                                        OutputWorkspace=self.elastic_channel_ws)
            self.to_clean.append(self.elastic_channel_ws)
        if (self.getProperty('MaskWorkspace').isDefault and self.getProperty('MaskedTubes').isDefault
                and self.getProperty('MaskThreshold').isDefault and self.getProperty('MaskedAngles').isDefault
                and self.getProperty('MaskWithVanadium').isDefault):
            self.masking = False
        else:
            self.masking = True
        self.flat_bkg_scaling = self.getProperty('FlatBkgScaling').value
        self.ebinning_params = self.getProperty('EnergyBinning').value
        self.empty = self.getPropertyValue('EmptyContainerWorkspace')
        self.vanadium = self.getPropertyValue('VanadiumWorkspace')
        if self.vanadium:
            self.vanadium_diagnostics, self.vanadium_integral = get_vanadium_corrections(self.vanadium)
        self.save_output = self.getProperty('SaveOutput').value

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
        self.setPropertyGroup('CadmiumWorkspace', additional_inputs_group)
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

        self.declareProperty(name="SaveOutput",
                             defaultValue=True,
                             doc="Whether to save the output directly after processing.")

        self.declareProperty(name='ClearCache',
                             defaultValue=False,
                             doc='Whether to clear intermediate workspaces.')

    def PyExec(self):
        self.setUp()
        sample_runs = self.getPropertyValue('Runs').split(',')
        output_samples = []
        if self.masking:
            self.mask_ws = self._prepare_masks()

        for sample_no, sample in enumerate(sample_runs):
            current_it_output = []  # output of the current iteration of reduction
            ws = self._collect_data(sample, vanadium=self.process == 'Vanadium')
            if self.process == 'Vanadium':
                ws_sofq, ws_softw, ws_diag, ws_integral = self._process_vanadium(ws)
                current_it_output = [ws_sofq, ws_softw, ws_diag, ws_integral]
                output_samples.extend(current_it_output)
            elif self.process == 'Sample':
                sample_sofq, sample_softw = self._process_sample(ws, sample_no)
                current_it_output = np.array([sample_sofq, sample_softw])
                current_it_output = current_it_output[[isinstance(elem, str) for elem in current_it_output]]
                output_samples.extend(current_it_output)
            if self.save_output:
                self._save_output(current_it_output)

        GroupWorkspaces(InputWorkspaces=output_samples,
                        OutputWorkspace=self.output)
        if self.getProperty('ClearCache').value:
            self._final_cleanup()
        self.setProperty('OutputWorkspace', mtd[self.output])

    def _collect_data(self, sample, vanadium=False):
        """Loads data if the corresponding workspace does not exist in the ADS."""
        ws = "{}_{}".format(get_run_number(sample), 'raw')
        kwargs = dict()
        if not self.getProperty('FlatBackground').isDefault:
            kwargs['FlatBkgWorkspace'] = self.getPropertyValue('FlatBackground')
            kwargs['FlatBkgScaling'] = self.flat_bkg_scaling
        if vanadium:
            kwargs['EPPCreationMethod'] = 'Calculate EPP'
            kwargs['ElasticChannel'] = 'Elastic Channel AUTO'
            kwargs['FlatBkg'] = 'Flat Bkg ON'
        if ws not in mtd:
            if vanadium:
                self.vanadium_epp = "{}_epp".format(ws)
                kwargs['OutputEPPWorkspace'] = self.vanadium_epp
                self.to_clean.append(self.vanadium_epp)
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

    def _final_cleanup(self):
        """Performs the clean up of intermediate workspaces that are created and used throughout the code."""
        if self.to_clean:
            DeleteWorkspaces(WorkspaceList=self.to_clean)

    def _save_output(self, ws_to_save):
        """Saves the output workspaces to an external file."""
        for ws_name in ws_to_save:
            if self.reduction_type == 'SingleCrystal':
                Psi = mtd[ws_name].run().getProperty('a3.value').value
            SaveNXSPE(
                InputWorkspace=ws_name,
                Filename='{}.nxspe'.format(ws_name),
                Psi=Psi
            )

    def _prepare_masks(self):
        """Builds a masking workspace from the provided inputs. Masking using threshold cannot be prepared ahead."""
        existing_masks = []
        mask = self.getPropertyValue('MaskWorkspace')
        if mask != str():
            mask = self.getPropertyValue('MaskWorkspace')
            if mask not in mtd:
                LoadNexusProcessed(Filename=mask, OutputWorkspace=mask)
            existing_masks.append(mask)
        mask_tubes = self.getPropertyValue('MaskedTubes')
        if mask_tubes != str():
            MaskBTP(Instrument=self.instrument, Tube=self.getPropertyValue(mask_tubes))
            tube_mask_ws = "{}_masked_tubes".format(self.instrument)
            RenameWorkspace(InputWorkspace='{}MaskBTP'.format(self.instrument), OutputWorkspace=tube_mask_ws)
            existing_masks.append(tube_mask_ws)

        mask_angles = self.getProperty('MaskedAngles')
        if mask_angles != list():
            masked_angles_ws = '{}_masked_angles'.format(self.instrument)
            LoadEmptyInstrument(Filename=self.instrument, OutputWorkspace=masked_angles_ws)
            MaskAngles(Workspace=masked_angles_ws, MinAngle=mask_angles[0], MaxAngle=mask_angles[1])
            existing_masks.append(masked_angles_ws)

        mask_with_vanadium = self.getProperty('MaskWithVanadium').value
        if mask_with_vanadium:
            existing_masks.append(self.vanadium_diagnostics)

        mask_ws = 'mask_ws'
        if len(existing_masks) > 1:
            MergeRuns(InputWorkspaces=existing_masks, OutputWorkspace=mask_ws)
        else:
            RenameWorkspace(InputWorkspace=existing_masks[0], OutputWorkspace=mask_ws)
        return mask_ws

    def _apply_mask(self, ws):
        """Applies selected masks."""
        MaskWorkspace(InputWorkspace=ws, OutputWorkspace=ws)
        # masks bins below the chosen threshold, this has to be applied for each ws and cannot be created ahead:
        if not self.getProperty('MaskThreshold').isDefault:
            MaskBinsIf(InputWorkspace=ws, OutputWorkspace=ws,
                       Criterion='y < {}'.format(self.getPropertyValue('MaskThreshold')))
        return ws

    def _subtract_background(self, ws):
        pass

    def _process_vanadium(self, ws):
        """Processes vanadium and creates workspaces with diagnostics, integrated vanadium, and reduced vanadium."""
        to_remove = [ws]
        numor = ws[:ws.rfind('_')]
        vanadium_diagnostics = '{}_diag'.format(numor)
        DirectILLDiagnostics(InputWorkspace=ws,
                             OutputWorkspace=vanadium_diagnostics,
                             BeamStopDiagnostics="Beam Stop Diagnostics OFF")

        if self.instrument == 'IN5':
            van_flat_ws = "{}_flat".format(numor)
            to_remove.append(van_flat_ws)
            DirectILLTubeBackground(InputWorkspace=ws,
                                    DiagnosticsWorkspace=vanadium_diagnostics,
                                    EPPWorkspace=self.vanadium_epp,
                                    OutputWorkspace=van_flat_ws)
            Minus(LHSWorkspace=ws, RHSWorkspace=van_flat_ws,
                  OutputWorkspace=ws, EnableLogging=False)

        if self.empty:
            self._subtract_background(ws)

        vanadium_integral = '{}_integral'.format(numor)
        DirectILLIntegrateVanadium(InputWorkspace=ws,
                                   OutputWorkspace=vanadium_integral,
                                   EPPWorkspace=self.vanadium_epp)

        sofq_output = '{}_SofQ'.format(numor)
        softw_output = '{}_SofTW'.format(numor)
        DirectILLReduction(InputWorkspace=ws,
                           OutputWorkspace=sofq_output,
                           OutputSofThetaEnergyWorkspace=softw_output,
                           IntegratedVanadiumWorkspace=vanadium_integral,
                           DiagnosticsWorkspace=vanadium_diagnostics
                           )

        if self.getProperty('ClearCache').value:
            DeleteWorkspaces(WorkspaceList=to_remove)
        return sofq_output, softw_output, vanadium_diagnostics, vanadium_integral

    def _normalise_sample(self, sample_ws, sample_no, numor):
        """
        Normalises sample using vanadium integral, if it has been provided.
        :param sample_ws: sample being processed
        :return: Either normalised sample or the input, if vanadium is not provided
        """
        normalised_ws = '{}_norm'.format(numor)
        if self.vanadium_integral:
            nintegrals = len(self.vanadium_integral)
            vanadium_no = sample_no
            if nintegrals == 1:
                vanadium_no = 0
            elif sample_no > nintegrals:
                vanadium_no = sample_no % nintegrals
            Divide(
                LHSWorkspace=sample_ws,
                RHSWorkspace=self.vanadium_integral[vanadium_no],
                OutputWorkspace=normalised_ws
            )
        else:
            normalised_ws = sample_ws
            self.log().warning("Vanadium integral workspace not found.")

        return normalised_ws

    def _prepare_self_attenuation_ws(self, ws):
        """Creates a self-attenuation workspace using either a MonteCarlo approach or numerical integration."""
        sample_geometry = self.getProperty('SampleGeometry').value
        sample_material = self.getProperty('SampleMaterial').value
        container_geometry = self.getProperty('ContainerGeometry').value \
            if not self.getProperty('SampleGeometry').isDefault else ""
        container_material = self.getProperty('ContainerMaterial').value \
            if not self.getProperty('ContainerMaterial').isDefault else ""
        self.absorption_corr = "{}_abs_corr".format(ws)
        self.to_clean.append(self.absorption_corr)
        SetSample(
            InputWorkspace=ws,
            Geometry=sample_geometry,
            Material=sample_material,
            ContainerGeometry=container_geometry,
            ContainerMaterial=container_material
        )
        if self.getProperty('SelfAttenuationMethod').value == 'MonteCarlo':
            PaalmanPingsMonteCarloAbsorption(
                InputWorkspace=ws,
                CorrectionsWorkspace=self.absorption_corr,
                SparseInstrument=True,
                NumberOfDetectorRows=5,
                NumberOfDetectorColumns=10
            )
        else:
            PaalmanPingsAbsorptionCorrection(
                InputWorkspace=ws,
                OutputWorkspace=self.absorption_corr
            )

    def _correct_self_attenuation(self, ws, sample_no):
        """Creates, if necessary, a self-attenuation workspace and uses it to correct the provided sample workspace."""
        if sample_no == 0:
            self._prepare_self_attenuation_ws(ws)

        if self.absorption_corr:
            ApplyPaalmanPingsCorrection(
                    SampleWorkspace=ws,
                    OutputWorkspace=ws,
                    CorrectionsWorkspace=self.absorption_corr
            )

    def _process_sample(self, ws, sample_no):
        """Does the sample data reduction for single crystal."""
        to_remove = [ws]
        if self.masking:
            ws = self._apply_mask(ws)
        if self.empty:
            self._subtract_background(ws)
        numor = ws[:ws.rfind('_')]
        processed_sample_tw = None
        if self.reduction_type == 'SingleCrystal':
            # normalises to vanadium integral
            normalised_ws = self._normalise_sample(ws, sample_no, numor)
            to_remove.append(normalised_ws)
            # converts to energy
            corrected_ws = '{}_ene'.format(numor)
            ConvertUnits(InputWorkspace=normalised_ws, EFixed=self.incident_energy,
                         Target='DeltaE', EMode='Direct', OutputWorkspace=corrected_ws)
            to_remove.append(corrected_ws)

            # transforms the distribution into dynamic structure factor
            CorrectKiKf(InputWorkspace=corrected_ws, EFixed=self.incident_energy,
                        OutputWorkspace=corrected_ws)

            # corrects for detector efficiency
            DetectorEfficiencyCorUser(InputWorkspace=corrected_ws, IncidentEnergy=self.incident_energy,
                                      OutputWorkspace=corrected_ws)

            # rebin in energy or momentum transfer
            processed_sample = '{}_reb'.format(numor)
            if self.ebinning_params:
                Rebin(InputWorkspace=corrected_ws, Params=self.ebinning_params, OutputWorkspace=processed_sample)
            else:
                RenameWorkspace(InputWorkspace=corrected_ws, OutputWorkspace=processed_sample)
                to_remove.pop()
            # saving of the output is omitted at this point: it is handled by Drill interface
        else:
            processed_sample = 'SofQW_{}'.format(ws[:ws.rfind('_')])  # name should contain only SofQW and numor
            processed_sample_tw = 'SofTW_{}'.format(ws[:ws.rfind('_')])  # name should contain only SofTW and numor
            if self.getPropertyValue('AbsorptionCorrection') != 'None':
                self._correct_self_attenuation(ws, sample_no)
            vanadium_integral = self.vanadium_integral[0] if self.vanadium_integral else ""
            vanadium_diagnostics = self.vanadium_diagnostics[0] if self.vanadium_diagnostics else ""
            DirectILLReduction(
                InputWorkspace=ws,
                OutputWorkspace=processed_sample,
                OutputSofThetaEnergyWorkspace=processed_sample_tw,
                IntegratedVanadiumWorkspace=vanadium_integral,
                DiagnosticsWorkspace=vanadium_diagnostics
            )
        if len(to_remove) > 0 and self.getProperty('ClearCache').value:
            DeleteWorkspaces(WorkspaceList=to_remove)

        return processed_sample, processed_sample_tw


AlgorithmFactory.subscribe(DirectILLAutoProcess)
