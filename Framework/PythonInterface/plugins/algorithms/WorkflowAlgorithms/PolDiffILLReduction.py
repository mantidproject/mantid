# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import FileProperty, MatrixWorkspaceProperty, MultipleFileProperty, \
    PropertyMode, Progress, PythonAlgorithm, WorkspaceGroupProperty, FileAction, \
    AlgorithmFactory
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator, \
    LogicOperator, PropertyCriterion, PropertyManagerProperty, StringListValidator
from mantid.simpleapi import *

from scipy.constants import physical_constants
import numpy as np
import math


class PolDiffILLReduction(PythonAlgorithm):

    _mode = 'Monochromatic'
    _method_data_structure = None # measurement method determined from the data
    _instrument = None
    _sampleAndEnvironmentProperties = None

    _DEG_2_RAD =  np.pi / 180.0

    def category(self):
        return 'ILL\\Diffraction'

    def summary(self):
        return 'Performs polarized diffraction and spectroscopy data reduction for the D7 instrument at the ILL.'

    def seeAlso(self):
        return ['D7YIGPositionCalibration', 'D7AbsoluteCrossSections']

    def name(self):
        return 'PolDiffILLReduction'

    def validateInputs(self):
        issues = dict()
        process = self.getPropertyValue('ProcessAs')
        if process == 'Transmission' and self.getProperty('BeamInputWorkspace').isDefault:
            issues['BeamInputWorkspace'] = 'Beam input workspace is mandatory for transmission calculation.'

        if process == 'Quartz' and self.getProperty('TransmissionInputWorkspace').isDefault:
            issues['TransmissionInputWorkspace'] = 'Quartz transmission is mandatory for polarisation correction calculation.'

        if process == 'Sample' or process == 'Vanadium':
            if len(self.getProperty('SampleAndEnvironmentProperties').value) == 0:
                issues['SampleAndEnvironmentProperties'] = 'Sample parameters need to be defined.'

            sampleAndEnvironmentProperties = self.getProperty('SampleAndEnvironmentProperties').value
            geometry_type = self.getPropertyValue('SampleGeometry')
            required_keys = ['FormulaUnits', 'SampleMass', 'FormulaUnitMass']
            if geometry_type != 'None':
                required_keys += ['SampleChemicalFormula', 'SampleDensity', 'BeamHeight', 'BeamWidth', 'ContainerDensity']
            if geometry_type == 'FlatPlate':
                required_keys += ['Height', 'SampleWidth', 'SampleThickness', 'ContainerFrontThickness', 'ContainerBackThickness']
            if geometry_type == 'Cylinder':
                required_keys += ['Height', 'SampleRadius', 'ContainerRadius']
            if geometry_type == 'Annulus':
                required_keys += ['Height', 'SampleInnerRadius', 'SampleOuterRadius', 'ContainerInnerRadius', 'ContainerOuterRadius']

            for key in required_keys:
                if key not in sampleAndEnvironmentProperties:
                    issues['SampleAndEnvironmentProperties'] = '{} needs to be defined.'.format(key)

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        options = ['Absorber', 'EmptyBeam', 'BeamWithAbsorber', 'Transmission', 'Container', 'Quartz', 'Vanadium', 'Sample']

        self.declareProperty(name='ProcessAs',
                             defaultValue='Sample',
                             validator=StringListValidator(options),
                             doc='Choose the process type.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output workspace based on the value of ProcessAs.')

        absorber = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Absorber')
        beam = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'EmptyBeam')
        container = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Container')
        sample = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Sample')
        quartz = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Quartz')
        transmission = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Transmission')
        vanadium = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Vanadium')
        reduction = EnabledWhenProperty(quartz, EnabledWhenProperty(vanadium, sample, LogicOperator.Or), LogicOperator.Or)
        scan = EnabledWhenProperty(reduction, EnabledWhenProperty(absorber, container, LogicOperator.Or), LogicOperator.Or)

        self.declareProperty(WorkspaceGroupProperty('AbsorberInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the absorber workspace group.')

        self.setPropertySettings('AbsorberInputWorkspace',
                                 EnabledWhenProperty(quartz,
                                                     EnabledWhenProperty(vanadium, sample, LogicOperator.Or),
                                                     LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('BeamInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the empty beam input workspace.')

        self.setPropertySettings('BeamInputWorkspace', transmission)

        self.declareProperty(MatrixWorkspaceProperty('AbsorberTransmissionInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the absorber transmission input workspace.')

        self.setPropertySettings('AbsorberTransmissionInputWorkspace', EnabledWhenProperty(transmission, beam, LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('TransmissionInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the transmission input workspace.')

        self.setPropertySettings('TransmissionInputWorkspace', reduction)

        self.declareProperty(WorkspaceGroupProperty('ContainerInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the container workspace.')

        self.setPropertySettings('ContainerInputWorkspace', reduction)

        self.declareProperty(WorkspaceGroupProperty('QuartzInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the polarisation efficiency correction workspace.')

        self.setPropertySettings('QuartzInputWorkspace',
                                 EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty(name="OutputTreatment",
                             defaultValue="Individual",
                             validator=StringListValidator(["Individual", "Average", "Sum"]),
                             direction=Direction.Input,
                             doc="Which treatment of the provided scan should be used to create output.")

        self.setPropertySettings('OutputTreatment', scan)

        self.declareProperty('ClearCache', True,
                             doc='Whether or not to clear the cache of intermediate workspaces.')

        self.declareProperty(name="SampleGeometry",
                             defaultValue="None",
                             validator=StringListValidator(["None", "FlatPlate", "Cylinder", "Annulus"]),
                             direction=Direction.Input,
                             doc="Sample geometry for self-attenuation correction to be applied.")

        self.setPropertySettings('SampleGeometry', EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty(PropertyManagerProperty('SampleAndEnvironmentProperties', dict()),
                             doc="Dictionary for the geometry used for self-attenuation correction.")

        self.setPropertySettings('SampleAndEnvironmentProperties',
                                 EnabledWhenProperty('SampleGeometry', PropertyCriterion.IsNotEqualTo, 'Custom'))

        self.setPropertySettings('SampleAndEnvironmentProperties',
                                 EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty(FileProperty(name="SampleGeometryFile",
                             defaultValue="",
                             action=FileAction.OptionalLoad),
                             doc="The path to the custom geometry for self-attenuation correction to be applied.")

        self.setPropertySettings('SampleGeometryFile', EnabledWhenProperty('SampleGeometry', PropertyCriterion.IsEqualTo, 'Custom'))

        self.declareProperty(name="OutputUnits",
                             defaultValue="TwoTheta",
                             validator=StringListValidator(["TwoTheta", "Q"]),
                             direction=Direction.Input,
                             doc="The choice to display the reduced data either as a function of the raw data units, the detector twoTheta,"
                                 +" or the momentum exchange.")

        self.setPropertySettings('OutputUnits', EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty(name="ScatteringAngleBinSize",
                             defaultValue=0.5,
                             validator=FloatBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="Scattering angle bin size in degrees used for expressing scan data on a single TwoTheta axis.")

        self.setPropertySettings("ScatteringAngleBinSize", EnabledWhenProperty('OutputTreatment', PropertyCriterion.IsEqualTo, 'Sum'))

        self.declareProperty(FileProperty('InstrumentCalibration', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['.xml']),
                             doc='The path to the calibrated Instrument Parameter File.')

        self.setPropertySettings('InstrumentCalibration', scan)

    @staticmethod
    def _normalise(ws):
        """Normalises the provided WorkspaceGroup to the monitor 1."""
        for entry_no, entry in enumerate(mtd[ws]):
            mon = ws + '_mon'
            ExtractMonitors(InputWorkspace=entry, DetectorWorkspace=entry,
                            MonitorWorkspace=mon)
            if 0 in mtd[mon].readY(0):
                raise RuntimeError('Cannot normalise to monitor; monitor has 0 counts.')
            else:
                CreateSingleValuedWorkspace(DataValue=mtd[mon].readY(0)[0], ErrorValue=mtd[mon].readE(0)[0],
                                            OutputWorkspace=mon)
                Divide(LHSWorkspace=entry, RHSWorkspace=mon, OutputWorkspace=entry)
                DeleteWorkspace(Workspace=mon)
        return ws

    @staticmethod
    def _calculate_transmission(ws, beam_ws):
        """Calculates transmission based on the measurement of the current sample and empty beam."""
        # extract Monitor2 values
        if 0 in mtd[ws][0].readY(0):
            raise RuntimeError('Cannot calculate transmission; monitor has 0 counts.')
        if 0 in mtd[beam_ws].readY(0):
            raise RuntimeError('Cannot calculate transmission; beam monitor has 0 counts.')
        Divide(LHSWorkspace=ws, RHSWorkspace=beam_ws, OutputWorkspace=ws)
        return ws

    @staticmethod
    def _enforce_uniform_units(origin_ws, target_ws):
        for entry_tuple in zip(mtd[origin_ws], mtd[target_ws]):
            entry_origin, entry_target = entry_tuple
            if entry_origin.YUnit() != entry_target.YUnit():
                entry_target.setYUnit(entry_origin.YUnit())

    def _figure_out_measurement_method(self, ws):
        """Figures out the measurement method based on the structure of the input files."""
        entries_per_numor = mtd[ws].getNumberOfEntries() / len(self.getPropertyValue('Run').split(','))
        if entries_per_numor == 10:
            self._method_data_structure = '10p'
        elif entries_per_numor == 6:
            self._method_data_structure = 'XYZ'
        elif entries_per_numor == 2:
            self._method_data_structure = 'Uniaxial'
        else:
            if self.getPropertyValue("ProcessAs") not in ['EmptyBeam', 'BeamWithAbsorber', 'Transmission']:
                raise RuntimeError("The analysis options are: Uniaxial, XYZ, and 10p. "
                                   + "The provided input does not fit in any of these measurement types.")

    def _merge_polarisations(self, ws, average_detectors=False):
        """Merges workspaces with the same polarisation inside the provided WorkspaceGroup either by using SumOverlappingTubes
        or averaging entries for each detector depending on the status of the sumOverDetectors flag."""
        pol_directions = set()
        numors = set()
        for name in mtd[ws].getNames():
            last_underscore = name.rfind("_")
            numors.add(name[:last_underscore])
            pol_directions.add(name[last_underscore+1:])
        if len(numors) > 1:
            names_list = []
            for direction in sorted(list(pol_directions)):
                name = '{0}_{1}'.format(ws, direction)
                list_pol = []
                for numor in numors:
                    if average_detectors:
                        try:
                            Plus(LHSWorkspace=name, RHSWorkspace=mtd[numor + '_' + direction], OutputWorkspace=name)
                        except ValueError:
                            CloneWorkspace(InputWorkspace=mtd[numor + '_' + direction], OutputWorkspace=name)
                    else:
                        list_pol.append('{0}_{1}'.format(numor, direction))
                if average_detectors:
                    CreateSingleValuedWorkspace(DataValue=len(numors), OutputWorkspace='norm')
                    Divide(LHSWorkspace=name, RHSWorkspace='norm', OutputWorkspace=name)
                    DeleteWorkspace(Workspace='norm')
                else:
                    SumOverlappingTubes(','.join(list_pol), OutputWorkspace=name,
                                        OutputType='1D', ScatteringAngleBinning=self.getProperty('ScatteringAngleBinSize').value,
                                        Normalise=True, HeightAxis='-0.1,0.1')
                names_list.append(name)
            DeleteWorkspaces(WorkspaceList=ws)
            GroupWorkspaces(InputWorkspaces=names_list, OutputWorkspace=ws)
        return ws

    def _subtract_background(self, ws, container_ws, transmission_ws):
        """Subtracts empty container and absorber scaled by transmission."""
        absorber_ws = self.getPropertyValue('AbsorberInputWorkspace')
        if absorber_ws == "":
            return ws
        unit_ws = 'unit_ws'
        CreateSingleValuedWorkspace(DataValue=1.0, OutputWorkspace=unit_ws)
        background_ws = 'background_ws'
        tmp_names = [unit_ws, background_ws]
        nMeasurements = self._data_structure_helper()
        singleContainerPerPOL = mtd[container_ws].getNumberOfEntries() < mtd[ws].getNumberOfEntries()
        singleAbsorberPerPOL = mtd[container_ws].getNumberOfEntries() < mtd[ws].getNumberOfEntries()
        for entry_no, entry in enumerate(mtd[ws]):
            if singleContainerPerPOL:
                container_entry = mtd[container_ws][entry_no % nMeasurements].name()
            else:
                container_entry = mtd[container_ws][entry_no].name()
            if singleAbsorberPerPOL:
                absorber_entry = mtd[absorber_ws][entry_no % nMeasurements].name()
            else:
                absorber_entry = mtd[absorber_ws][entry_no].name()
            mtd[container_entry].setYUnit('')
            mtd[transmission_ws].setYUnit('')
            mtd[absorber_entry].setYUnit('')
            entry.setYUnit('')
            container_corr = container_entry + '_corr'
            tmp_names.append(container_corr)
            Multiply(LHSWorkspace=transmission_ws, RHSWorkspace=container_entry, OutputWorkspace=container_corr)
            transmission_corr = transmission_ws + '_corr'
            tmp_names.append(transmission_corr)
            Minus(LHSWorkspace=unit_ws, RHSWorkspace=transmission_ws, OutputWorkspace=transmission_corr)
            absorber_corr = absorber_entry + '_corr'
            tmp_names.append(absorber_corr)
            Multiply(LHSWorkspace=transmission_corr, RHSWorkspace=absorber_entry, OutputWorkspace=absorber_corr)
            Plus(LHSWorkspace=container_corr, RHSWorkspace=absorber_corr, OutputWorkspace=background_ws)
            Minus(LHSWorkspace=entry,
                  RHSWorkspace=background_ws,
                  OutputWorkspace=entry)
        DeleteWorkspaces(WorkspaceList=tmp_names)
        return ws

    def _calculate_polarising_efficiencies(self, ws):
        """Calculates the polarising efficiencies using quartz data."""
        flipper_eff = 1.0 # this could be extracted from data if 4 measurements are done
        flipper_corr_ws = 'flipper_corr_ws'
        CreateSingleValuedWorkspace(DataValue=(2*flipper_eff-1), OutputWorkspace=flipper_corr_ws)
        nMeasurementsPerPOL = 2
        tmp_names = []
        names_to_delete = [flipper_corr_ws]
        index = 0

        if self.getProperty('OutputTreatment').value == 'Average':
            ws = self._merge_polarisations(ws, average_detectors=True)
        for entry_no in range(1, mtd[ws].getNumberOfEntries()+1, nMeasurementsPerPOL):
            # two polarizer-analyzer states, fixed flipper_eff
            ws_00 = mtd[ws][entry_no].name()
            ws_01 = mtd[ws][entry_no-1].name()
            tmp_name = '{0}_{1}_{2}'.format(ws[2:], mtd[ws_00].getRun().getLogData('POL.actual_state').value, index)
            Minus(LHSWorkspace=ws_00, RHSWorkspace=ws_01, OutputWorkspace='nominator')
            ws_00_corr = ws_00 + '_corr'
            names_to_delete.append(ws_00_corr)
            Multiply(LHSWorkspace=flipper_corr_ws, RHSWorkspace=ws_00, OutputWorkspace=ws_00_corr)
            Plus(LHSWorkspace=ws_00_corr, RHSWorkspace=ws_01, OutputWorkspace='denominator')
            Divide(LHSWorkspace='nominator',
                   RHSWorkspace='denominator',
                   OutputWorkspace=tmp_name)
            tmp_names.append(tmp_name)
            if self._method_data_structure == 'Uniaxial' and entry_no % 2 == 1:
                index += 1
            elif self._method_data_structure == 'XYZ' and entry_no % 6 == 5:
                index += 1
            elif self._method_data_structure == '10p' and entry_no % 10 == 9:
                index += 1
        names_to_delete += ['nominator', 'denominator']
        GroupWorkspaces(InputWorkspaces=tmp_names, OutputWorkspace='tmp')
        DeleteWorkspaces(WorkspaceList=names_to_delete)
        DeleteWorkspace(Workspace=ws)
        RenameWorkspace(InputWorkspace='tmp', OutputWorkspace=ws)
        return ws

    def _detector_analyser_energy_efficiency(self, ws):
        """Corrects for the detector and analyser energy efficiency."""
        for entry in mtd[ws]:
            DetectorEfficiencyCorUser(InputWorkspace=entry, OutputWorkspace=entry,
                                      IncidentEnergy=self._sampleAndEnvironmentProperties['InitialEnergy'].value)
        return ws

    def _apply_polarisation_corrections(self, ws, pol_eff_ws):
        """Applies the polarisation correction based on the output from quartz reduction."""
        nPolarisations = None
        singleCorrectionPerPOL = False
        if mtd[ws].getNumberOfEntries() != 2*mtd[pol_eff_ws].getNumberOfEntries():
            singleCorrectionPerPOL = True
            nMeasurements = self._data_structure_helper()
            nPolarisations = math.floor(nMeasurements / 2.0)
            if mtd[pol_eff_ws].getNumberOfEntries() != nPolarisations:
                raise RuntimeError("Incompatible number of polarisations between quartz input and sample.")

        CreateSingleValuedWorkspace(DataValue=1.0, OutputWorkspace='unity')
        CreateSingleValuedWorkspace(DataValue=2.0, OutputWorkspace='double_fp')
        to_clean = ['unity', 'double_fp']

        for entry_no in range(mtd[ws].getNumberOfEntries()):
            if entry_no % 2 != 0:
                continue
            polarisation_entry_no = int(entry_no/2)
            if singleCorrectionPerPOL:
                polarisation_entry_no = int(polarisation_entry_no % nPolarisations)
            phi = mtd[pol_eff_ws][polarisation_entry_no].name()
            intensity_0 = mtd[ws][entry_no].name()
            intensity_1 = mtd[ws][entry_no+1].name()
            tmp_names = [intensity_0 + '_tmp', intensity_1 + '_tmp']
            # helper ws
            Minus(LHSWorkspace='unity', RHSWorkspace=phi, Outputworkspace='one_m_pol')
            Plus(LHSWorkspace='unity', RHSWorkspace=phi, Outputworkspace='one_p_pol')
            # spin-flip:
            Multiply(LHSWorkspace=intensity_0, RHSWorkspace='one_p_pol', OutputWorkspace='lhs_nominator')
            Multiply(LHSWorkspace=intensity_1, RHSWorkspace='one_m_pol', OutputWorkspace='rhs_nominator')
            Minus(LHSWorkspace='lhs_nominator', RHSWorkspace='rhs_nominator', OutputWorkspace='nominator')
            Multiply(LHSWorkspace=phi, RHSWorkspace='double_fp', OutputWorkspace='denominator')
            Divide(LHSWorkspace='nominator', RHSWorkspace='denominator', OutputWorkspace=tmp_names[0])
            # non-spin-flip:
            Multiply(LHSWorkspace=intensity_0, RHSWorkspace='one_m_pol', OutputWorkspace='lhs_nominator')
            Multiply(LHSWorkspace=intensity_1, RHSWorkspace='one_p_pol', OutputWorkspace='rhs_nominator')
            Minus(LHSWorkspace='rhs_nominator', RHSWorkspace='lhs_nominator', OutputWorkspace='nominator')
            Divide(LHSWorkspace='nominator', RHSWorkspace='denominator', OutputWorkspace=tmp_names[1])
            RenameWorkspace(tmp_names[0], intensity_0)
            RenameWorkspace(tmp_names[1], intensity_1)

        to_clean = ['one_m_pol', 'one_p_pol', 'lhs_nominator', 'rhs_nominator', 'nominator', 'denominator']
        DeleteWorkspaces(WorkspaceList=to_clean)
        return ws

    def _read_experiment_properties(self, ws):
        """Reads the user-provided dictionary that contains sample geometry (type, dimensions) and experimental conditions,
         such as the beam size and calculates derived parameters."""
        self._sampleAndEnvironmentProperties = self.getProperty('SampleAndEnvironmentProperties').value
        if 'InitialEnergy' not in self._sampleAndEnvironmentProperties:
            h = physical_constants['Planck constant'][0]  # in m^2 kg^2 / s^2
            neutron_mass = physical_constants['neutron mass'][0]  # in kg
            wavelength = mtd[ws][0].getRun().getLogData('monochromator.wavelength').value * 1e-10  # in m
            joules_to_mev = 1e3 / physical_constants['electron volt'][0]
            self._sampleAndEnvironmentProperties['InitialEnergy'] = joules_to_mev * math.pow(h / wavelength, 2) / (2 * neutron_mass)

        if 'NMoles' not in self._sampleAndEnvironmentProperties:
            sample_mass = self._sampleAndEnvironmentProperties['SampleMass'].value
            formula_units = self._sampleAndEnvironmentProperties['FormulaUnits'].value
            formula_unit_mass = self._sampleAndEnvironmentProperties['FormulaUnitMass'].value
            self._sampleAndEnvironmentProperties['NMoles'] = (sample_mass / formula_unit_mass) * formula_units

    def _apply_self_attenuation_correction(self, sample_ws, container_ws):
        """Applies the self-attenuation correction based on the Palmaan-Pings Monte-Carlo calculation, taking into account
        the sample's material, shape, and dimensions."""
        geometry_type = self.getPropertyValue('SampleGeometry')

        kwargs = {}
        kwargs['BeamHeight'] = self._sampleAndEnvironmentProperties['BeamHeight'].value
        kwargs['BeamWidth'] = self._sampleAndEnvironmentProperties['BeamWidth'].value
        kwargs['SampleDensityType'] = 'Number Density'
        kwargs['SampleNumberDensityUnit'] = 'Formula Units'
        kwargs['ContainerDensityType'] = 'Number Density'
        kwargs['ContainerNumberDensityUnit'] = 'Formula Units'
        kwargs['SampleDensity'] = self._sampleAndEnvironmentProperties['SampleDensity'].value
        kwargs['Height'] = self._sampleAndEnvironmentProperties['Height'].value
        kwargs['SampleChemicalFormula'] = self._sampleAndEnvironmentProperties['SampleChemicalFormula'].value
        if 'ContainerChemicalFormula' in self._sampleAndEnvironmentProperties:
            kwargs['ContainerChemicalFormula'] = self._sampleAndEnvironmentProperties['ContainerChemicalFormula'].value
            kwargs['ContainerDensity'] = self._sampleAndEnvironmentProperties['ContainerDensity'].value
        if geometry_type == 'FlatPlate':
            kwargs['SampleWidth'] = self._sampleAndEnvironmentProperties['SampleWidth'].value
            kwargs['SampleThickness'] = self._sampleAndEnvironmentProperties['SampleThickness'].value
            if 'ContainerChemicalFormula' in self._sampleAndEnvironmentProperties:
                kwargs['ContainerFrontThickness'] = self._sampleAndEnvironmentProperties['ContainerFrontThickness'].value
                kwargs['ContainerBackThickness'] = self._sampleAndEnvironmentProperties['ContainerBackThickness'].value
        elif geometry_type == 'Cylinder':
            kwargs['SampleRadius'] = self._sampleAndEnvironmentProperties['SampleRadius'].value
            if 'ContainerChemicalFormula' in self._sampleAndEnvironmentProperties:
                kwargs['ContainerRadius'] = self._sampleAndEnvironmentProperties['ContainerRadius'].value
        elif geometry_type == 'Annulus':
            kwargs['SampleInnerRadius'] = self._sampleAndEnvironmentProperties['SampleInnerRadius'].value
            kwargs['SampleOuterRadius']  = self._sampleAndEnvironmentProperties['SampleOuterRadius'].value
            if 'ContainerChemicalFormula' in self._sampleAndEnvironmentProperties:
                kwargs['ContainerInnerRadius'] = self._sampleAndEnvironmentProperties['ContainerInnerRadius'].value
                kwargs['ContainerOuterRadius'] = self._sampleAndEnvironmentProperties['ContainerOuterRadius'].value
        if 'EventsPerPoint' in self._sampleAndEnvironmentProperties:
            kwargs['EventsPerPoint'] = self._sampleAndEnvironmentProperties['EventsPerPoint'].value
        else:
            kwargs['EventsPerPoint'] = 5000

        self._enforce_uniform_units(sample_ws, container_ws)
        if geometry_type == 'Custom':
            raise RuntimeError('Custom geometry treatment has not been implemented yet.')
        else:
            for entry_no, entry in enumerate(mtd[sample_ws]):
                correction_ws = '{}_corr'.format(geometry_type)
                if ( (self._method_data_structure == 'Uniaxial' and entry_no % 2 == 0)
                     or (self._method_data_structure == 'XYZ' and entry_no % 6 == 0)
                     or (self._method_data_structure == '10p' and entry_no % 10 == 0) ):
                    PaalmanPingsMonteCarloAbsorption(SampleWorkspace=entry,
                                                     Shape=geometry_type,
                                                     CorrectionsWorkspace=correction_ws,
                                                     ContainerWorkspace=mtd[container_ws][entry_no],
                                                     **kwargs)
                ApplyPaalmanPingsCorrection(SampleWorkspace=entry,
                                            CorrectionsWorkspace=correction_ws,
                                            CanWorkspace=mtd[container_ws][entry_no],
                                            OutputWorkspace=entry)

        return sample_ws

    def _data_structure_helper(self):
        nMeasurements = 0
        if self._method_data_structure == '10p':
            nMeasurements = 10
        elif self._method_data_structure == 'XYZ':
            nMeasurements = 6
        elif self._method_data_structure == 'Uniaxial':
            nMeasurements = 2

        return nMeasurements

    def _normalise_vanadium(self, ws):
        """Performs normalisation of the vanadium data to the expected cross-section."""
        if self.getPropertyValue('OutputTreatment') == 'Sum':
            vanadium_expected_cross_section = 0.404 # barns
            CreateSingleValuedWorkspace(DataValue=vanadium_expected_cross_section
                                        * self._sampleAndEnvironmentProperties['NMoles'].value,
                                        OutputWorkspace='norm')
            tmp_name = '{}_1'.format(self.getPropertyValue('OutputWorkspace'))
            RenameWorkspace(InputWorkspace=mtd[ws][0].name(), OutputWorkspace=tmp_name)
            to_remove = ['norm']
            for entry_no in range(1, mtd[ws].getNumberOfEntries()):
                ws_name = mtd[ws][entry_no].name()
                Plus(LHSWorkspace=tmp_name, RHSWorkspace=ws_name, OutputWorkspace=tmp_name)
                to_remove.append(ws_name)
            Divide(LHSWorkspace='norm', RHSWorkspace=tmp_name, OutputWorkspace=tmp_name)
            DeleteWorkspaces(WorkspaceList=to_remove)
            GroupWorkspaces(InputWorkspaces=tmp_name, OutputWorkspace=ws)

        elif self.getProperty('OutputTreatment').value == 'Average':
                self._merge_polarisations(ws, average_detectors=True)

        return ws

    def _set_units(self, ws, process):
        output_unit = self.getPropertyValue('OutputUnits')
        if output_unit == 'TwoTheta':
            if (process == 'Sample' and len(self.getPropertyValue('Run').split(',')) > 1
                    and self.getProperty('OutputTreatment').value == 'Sum'):
                self._merge_polarisations(ws)
            else:
                ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=ws, Target='SignedTheta', OrderAxis=False)
            ConvertAxisByFormula(InputWorkspace=ws, OutputWorkspace=ws, Axis='Y', Formula='-y')
        elif output_unit == 'Q':
            ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=ws, Target='ElasticQ',
                                EFixed=self._sampleAndEnvironmentProperties['InitialEnergy'].value,
                                OrderAxis=False)
        for entry in mtd[ws]:
            unit = 'dSigma / dOmega'
            if output_unit == 'TwoTheta':
                unit += ' (TwoTheta)'
            elif output_unit == 'Q':
                unit += ' (Q)'
            entry.setYUnit(unit)
            entry.setYUnitLabel(unit)
        return ws

    def _finalize(self, ws, process, progress):
        ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws, NaNValue=0,
                             NaNError=0, InfinityValue=0, InfinityError=0)
        mtd[ws][0].getRun().addProperty('ProcessedAs', process, True)
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        self.setProperty('OutputWorkspace', mtd[ws[2:]])

    def PyExec(self):
        process = self.getPropertyValue('ProcessAs')
        processes = ['Absorber', 'EmptyBeam', 'BeamWithAbsorber', 'Transmission', 'Container', 'Quartz', 'Vanadium', 'Sample']
        nReports = [3, 2, 2, 3, 3, 3, 10, 10]
        progress = Progress(self, start=0.0, end=1.0, nreports=nReports[processes.index(process)])
        ws = '__' + self.getPropertyValue('OutputWorkspace')

        calibration_setting = 'YIGFile'
        if self.getProperty('InstrumentCalibration').isDefault:
            calibration_setting = 'None'
        progress.report('Loading data')
        Load(Filename=self.getPropertyValue('Run'), LoaderName='LoadILLPolarizedDiffraction',
             PositionCalibration=calibration_setting, YIGFileName=self.getPropertyValue('InstrumentCalibration'),
             OutputWorkspace=ws)

        self._instrument = mtd[ws][0].getInstrument().getName()
        run = mtd[ws][0].getRun()
        if run['acquisition_mode'].value == 1:
            raise RuntimeError("TOF data reduction is not supported at the moment.")
        self._figure_out_measurement_method(ws)

        if process in ['EmptyBeam', 'BeamWithAbsorber', 'Transmission']:
            if mtd[ws].getNumberOfEntries() > 1:
                self._merge_polarisations(ws, average_detectors=True)
            absorber_transmission_ws = self.getPropertyValue('AbsorberTransmissionInputWorkspace')
            if absorber_transmission_ws:
                Minus(LHSWorkspace=ws, RHSWorkspace=absorber_transmission_ws, OutputWorkspace=ws)
            monID = 100001 # monitor 2
            ExtractSpectra(InputWorkspace=ws, DetectorList=monID, OutputWorkspace=ws)
            if process in ['Transmission']:
                beam_ws = self.getPropertyValue('BeamInputWorkspace')
                progress.report('Calculating transmission')
                self._calculate_transmission(ws, beam_ws)
        else:
            progress.report('Normalising to monitor')
            self._normalise(ws)

        if process in ['Quartz', 'Vanadium', 'Sample']:
            container_ws = self.getPropertyValue('ContainerInputWorkspace')
            if not self.getProperty('ContainerInputWorkspace').isDefault and not self.getProperty('TransmissionInputWorkspace').isDefault:
                # Subtracts background if the workspaces for container and transmission are provided
                transmission_ws = self.getPropertyValue('TransmissionInputWorkspace')
                progress.report('Subtracting backgrounds')
                self._subtract_background(ws, container_ws, transmission_ws)

            if process == 'Quartz':
                progress.report('Calculating polarising efficiencies')
                self._calculate_polarising_efficiencies(ws)

            if process in ['Vanadium', 'Sample']:
                pol_eff_ws = self.getPropertyValue('QuartzInputWorkspace')
                if pol_eff_ws:
                    progress.report('Applying polarisation corrections')
                    self._apply_polarisation_corrections(ws, pol_eff_ws)
                self._read_experiment_properties(ws)
                if self.getPropertyValue('SampleGeometry') != 'None' and container_ws != '' :
                    progress.report('Applying self-attenuation correction')
                    self._apply_self_attenuation_correction(ws, container_ws)
                if process == 'Vanadium':
                    progress.report('Normalising vanadium output')
                    self._normalise_vanadium(ws)
                self._set_units(ws, process)
                Transpose(InputWorkspace=ws, OutputWorkspace=ws)

        self._finalize(ws, process, progress)


AlgorithmFactory.subscribe(PolDiffILLReduction)
