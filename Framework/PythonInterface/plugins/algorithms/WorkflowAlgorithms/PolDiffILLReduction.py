# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import FileProperty, MatrixWorkspaceProperty, MultipleFileProperty, \
    NumericAxis, PropertyMode, Progress, PythonAlgorithm, WorkspaceGroup, WorkspaceGroupProperty, \
    FileAction, AlgorithmFactory
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator, IntBoundedValidator, \
    LogicOperator, PropertyCriterion, PropertyManagerProperty, StringListValidator

from mantid.simpleapi import *

from scipy.constants import physical_constants
import numpy as np
import math


class PolDiffILLReduction(PythonAlgorithm):

    _mode = 'Monochromatic'
    _method_data_structure = None # measurement method determined from the data
    _user_method = None
    _instrument = None
    _sampleAndEnvironmentProperties = None

    _DEG_2_RAD =  np.pi / 180.0

    @staticmethod
    def _max_value_per_detector(ws):
        max_values = np.zeros(shape=(mtd[ws][0].getNumberHistograms(),
                                     mtd[ws].getNumberOfEntries()))
        for entry_no, entry in enumerate(mtd[ws]):
            max_values[:, entry_no] = entry.extractY().T
        return np.amax(max_values, axis=1)

    def category(self):
        return 'ILL\\Diffraction'

    def summary(self):
        return 'Performs polarized diffraction and spectroscopy data reduction for the D7 instrument at the ILL.'

    def name(self):
        return 'PolDiffILLReduction'

    def validateInputs(self):
        issues = dict()
        process = self.getPropertyValue('ProcessAs')
        if process == 'Transmission' and self.getProperty('BeamInputWorkspace').isDefault:
            issues['BeamInputWorkspace'] = 'Beam input workspace is mandatory for transmission calculation.'

        if ( (not self.getProperty('AbsorberInputWorkspace').isDefault and self.getProperty('ContainerInputWorkspace').isDefault)
             or (not self.getProperty('AbsorberInputWorkspace').isDefault and self.getProperty('ContainerInputWorkspace').isDefault) ):
            issues['AbsorberInputWorkspace'] = 'Both Container and Absorber input workspaces are mandatory for background subtraction.'
            issues['ContainerInputWorkspace'] = 'Both Container and Absorber input workspaces are mandatory for background subtraction.'

        if process == 'Quartz' and self.getProperty('TransmissionInputWorkspace').isDefault:
            issues['TransmissionInputWorkspace'] = 'Quartz transmission is mandatory for polarisation correction calculation.'

        if process == 'Vanadium' and self.getProperty('TransmissionInputWorkspace').isDefault :
            issues['TransmissionInputWorkspace'] = 'Vanadium transmission is mandatory for vanadium data reduction.'

        if process == 'Sample' or process == 'Vanadium':
            if len(self.getProperty('SampleAndEnvironmentPropertiesDictionary').value) == 0:
                issues['SampleAndEnvironmentPropertiesDictionary'] = 'Sample parameters need to be defined.'

            sampleAndEnvironmentProperties = self.getProperty('SampleAndEnvironmentPropertiesDictionary').value
            geometry_type = self.getPropertyValue('SampleGeometry')
            required_keys = ['Mass', 'FormulaUnits', 'ChemicalFormula']
            if geometry_type != 'None':
                required_keys += ['BeamHeight', 'BeamWidth', 'ContainerDensity',]
            if geometry_type == 'FlatPlate':
                required_keys += ['Height', 'Width', 'Thickness', 'ContainerFrontThickness', 'ContainerBackThickness']
            if geometry_type == 'Cylinder':
                required_keys += ['Height', 'Radius', 'ContainerRadius']
            if geometry_type == 'Annulus':
                required_keys += ['Height', 'InnerRadius', 'OuterRadius', 'ContainerInnerRadius', 'ContainerOuterRadius']

            if self.getPropertyValue('DetectorEfficiencyCalibration') == 'Incoherent':
                required_keys.append('SampleSpin')

            for key in required_keys:
                if key not in sampleAndEnvironmentProperties:
                    issues['SampleAndEnvironmentPropertiesDictionary'] = '{} needs to be defined.'.format(key)

            if 'Density' not in sampleAndEnvironmentProperties and 'NumberDensity' not in sampleAndEnvironmentProperties:
                issues['SampleAndEnvironmentPropertiesDictionary'] = 'Either Density of NumberDensity needs to be defined.'

        if process == 'Sample':
            if self.getProperty('TransmissionInputWorkspace').isDefault :
                issues['TransmissionInputWorkspace'] = 'Sample transmission is mandatory for sample data reduction.'

            if (self.getProperty('DetectorEfficiencyCalibration') == 'Vanadium'
                    and self.getProperty('VanadiumInputWorkspace').isDefault):
                issues['VanadiumInputWorkspace'] = 'Vanadium input workspace is mandatory for sample data reduction when \
                    detector efficiency calibration is based "Vanadium".'

            if ( (self.getProperty('DetectorEfficiencyCalibration') == 'Incoherent'
                  or self.getProperty('DetectorEfficiencyCalibration') == 'Paramagnetic')
                 and self.getProperty('ComponentSeparationMethod').isDefault):
                issues['DetectorEfficiencyCalibration'] = 'Chosen sample normalisation requires input from the component separation.'
                issues['ComponentSeparationMethod'] = 'Chosen sample normalisation requires input from the component separation.'

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        options = ['Absorber', 'Beam', 'Transmission', 'Container', 'Quartz', 'Vanadium', 'Sample']

        self.declareProperty(name='ProcessAs',
                             defaultValue='Sample',
                             validator=StringListValidator(options),
                             doc='Choose the process type.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output workspace based on the value of ProcessAs.')

        absorber = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Absorber')
        beam = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Beam')
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

        self.declareProperty(WorkspaceGroupProperty('VanadiumInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the vanadium workspace.')

        self.setPropertySettings('VanadiumInputWorkspace', EnabledWhenProperty('DetectorEfficiencyCalibration',
                                                                               PropertyCriterion.IsEqualTo, 'Vanadium'))

        self.declareProperty(name="OutputTreatment",
                             defaultValue="OutputIndividualScans",
                             validator=StringListValidator(["OutputIndividualScans", "AverageScans", "SumScans"]),
                             direction=Direction.Input,
                             doc="Which treatment of the provided scan should be used to create output.")

        self.setPropertySettings('OutputTreatment', scan)

        self.declareProperty('AbsoluteUnitsNormalisation', True,
                             doc='Whether or not express the output in absolute units.')

        self.setPropertySettings('AbsoluteUnitsNormalisation', EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty('ClearCache', True,
                             doc='Whether or not to clear the cache of intermediate workspaces.')

        self.declareProperty(name="MeasurementTechnique",
                             defaultValue="Powder",
                             validator=StringListValidator(["Powder", "SingleCrystal", "TOF"]),
                             direction=Direction.Input,
                             doc="Which measurement technique was used to measure the sample.")

        self.declareProperty(name="ComponentSeparationMethod",
                             defaultValue="None",
                             validator=StringListValidator(["None", "Uniaxial", "XYZ", "10p"]),
                             direction=Direction.Input,
                             doc="Whether to perform component separation and what type of method to use.")

        self.declareProperty(name="SampleGeometry",
                             defaultValue="None",
                             validator=StringListValidator(["None", "FlatPlate", "Cylinder", "Annulus", "Custom"]),
                             direction=Direction.Input,
                             doc="Sample geometry for self-attenuation correction to be applied.")

        self.setPropertySettings('SampleGeometry', EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty(PropertyManagerProperty('SampleAndEnvironmentPropertiesDictionary', dict()),
                             doc="Dictionary for the geometry used for self-attenuation correction.")

        self.setPropertySettings('SampleAndEnvironmentPropertiesDictionary',
                                 EnabledWhenProperty('SampleGeometry', PropertyCriterion.IsNotEqualTo, 'Custom'))

        self.declareProperty(FileProperty(name="SampleGeometryFile",
                             defaultValue="",
                             action=FileAction.OptionalLoad),
                             doc="The path to the custom geometry for self-attenuation correction to be applied.")

        self.setPropertySettings('SampleGeometryFile', EnabledWhenProperty('SampleGeometry', PropertyCriterion.IsEqualTo, 'Custom'))

        self.declareProperty(name="DetectorEfficiencyCalibration",
                             defaultValue="None",
                             validator=StringListValidator(["None", "Vanadium", "Incoherent",  "Paramagnetic"]),
                             direction=Direction.Input,
                             doc="Detector efficiency calibration type.")

        self.setPropertySettings('DetectorEfficiencyCalibration', sample)

        self.declareProperty(name="IncoherentCrossSection",
                             defaultValue=0,
                             validator=IntBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="Cross-section for incoherent scattering, necessary for setting the output on the absolute scale.")

        incoherent = EnabledWhenProperty('DetectorEfficiencyCalibration', PropertyCriterion.IsEqualTo, 'Incoherent')

        absoluteNormalisation = EnabledWhenProperty('AbsoluteUnitsNormalisation', PropertyCriterion.IsDefault)

        self.setPropertySettings('IncoherentCrossSection', EnabledWhenProperty(incoherent, absoluteNormalisation,
                                                                               LogicOperator.And))

        self.declareProperty(name="OutputUnits",
                             defaultValue="Wavelength/TOF",
                             validator=StringListValidator(["Wavelength/TOF", "TwoTheta", "Q"]),
                             direction=Direction.Input,
                             doc="The choice to display the reduced data either as a function of the raw data units, the detector twoTheta,"
                                 +" or the momentum exchange.")

        self.setPropertySettings('OutputUnits', EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty(name="TOFUnits",
                             defaultValue="TimeChannels",
                             validator=StringListValidator(["TimeChannels", "UncalibratedTime", "Energy"]),
                             direction=Direction.Input,
                             doc="The choice to display the TOF data either as a function of the time channel or the uncalibrated time.\
                             It has no effect if the measurement mode is monochromatic.")

        tofMeasurement = EnabledWhenProperty('MeasurementTechnique', PropertyCriterion.IsEqualTo, 'TOF')

        self.setPropertySettings('TOFUnits', EnabledWhenProperty(tofMeasurement,
                                                                 EnabledWhenProperty(vanadium, sample, LogicOperator.Or),
                                                                 LogicOperator.And))

        self.declareProperty(name="ScatteringAngleBinSize",
                             defaultValue=0.5,
                             validator=FloatBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="Scattering angle bin size in degrees used for expressing scan data on a single TwoTheta axis.")

        self.setPropertySettings("ScatteringAngleBinSize", EnabledWhenProperty('OutputTreatment', PropertyCriterion.IsEqualTo, 'SumScans'))

        self.declareProperty(FileProperty('InstrumentCalibration', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['.xml']),
                             doc='The path to the calibrated Instrument Parameter File.')

        self.setPropertySettings('InstrumentCalibration', scan)

    def _figure_out_measurement_method(self, ws):
        """Figures out the measurement method based on the structure of the input files."""
        entries_per_numor = mtd[ws].getNumberOfEntries() / len(self.getPropertyValue('Run').split(','))
        if entries_per_numor == 10:
            self._method_data_structure = '10p'
        elif entries_per_numor == 6:
            self._method_data_structure = 'XYZ'
            if self._user_method == '10p':
                raise RunTimeError("The provided data cannot support 10-point measurement component separation.")
        elif entries_per_numor == 2:
            self._method_data_structure = 'Uniaxial'
            if self._user_method == '10p':
                raise RunTimeError("The provided data cannot support 10-point measurement component separation.")
            if self._user_method == 'XYZ':
                raise RunTimeError("The provided data cannot support XYZ measurement component separation.")
        else:
            if self.getPropertyValue("ProcessAs") not in ['Beam', 'Transmission']:
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

    def _normalise(self, ws):
        """Normalises the provided WorkspaceGroup to the monitor 1."""
        monID = 100000 # monitor 1
        monitor_indices = "{},{}".format(mtd[ws][0].getNumberHistograms()-2,
                                         mtd[ws][0].getNumberHistograms()-1)
        for entry_no, entry in enumerate(mtd[ws]):
            mon = ws + '_mon'
            ExtractSpectra(InputWorkspace=entry, DetectorList=monID, OutputWorkspace=mon)
            if 0 in mtd[mon].readY(0):
                raise RuntimeError('Cannot normalise to monitor; monitor has 0 counts.')
            else:
                if self._mode == 'TOF':
                    Integration(InputWorkspace=mon, OutputWorkspace=mon)
                Divide(LHSWorkspace=entry, RHSWorkspace=mon, OutputWorkspace=entry)
                RemoveSpectra(entry, WorkspaceIndices=monitor_indices, OutputWorkspace=entry)
                DeleteWorkspace(Workspace=mon)
        return ws

    def _calculate_transmission(self, ws, beam_ws):
        """Calculates transmission based on the measurement of the current sample and empty beam."""
        # extract Monitor2 values
        if 0 in mtd[ws][0].readY(0):
            raise RuntimeError('Cannot calculate transmission; monitor has 0 counts.')
        if 0 in mtd[beam_ws].readY(0):
            raise RuntimeError('Cannot calculate transmission; beam monitor has 0 counts.')
        Divide(LHSWorkspace=ws, RHSWorkspace=beam_ws, OutputWorkspace=ws)
        return ws

    def _subtract_background(self, ws, container_ws, transmission_ws):
        """Subtracts empty container and absorber scaled by transmission."""
        if self._mode != 'TOF':
            absorber_ws = self.getPropertyValue('AbsorberInputWorkspace')
            if absorber_ws == "":
                raise RuntimeError("Absorber input workspace needs to be provided for non-TOF background subtraction.")
        unit_ws = 'unit_ws'
        CreateSingleValuedWorkspace(DataValue=1.0, OutputWorkspace=unit_ws)
        background_ws = 'background_ws'
        tmp_names = [unit_ws, background_ws]
        for entry_no, entry in enumerate(mtd[ws]):
            container_entry = mtd[container_ws][entry_no].name()
            mtd[container_entry].setYUnit('Counts/Counts')
            mtd[transmission_ws].setYUnit('Counts/Counts')
            if self._mode != 'TOF':
                absorber_entry = mtd[absorber_ws][entry_no].name()
                mtd[absorber_entry].setYUnit('Counts/Counts')
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
            else:
                raise RuntimeError("TOF requires elastic channel definition")
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

        if self.getProperty('OutputTreatment').value == 'AverageScans':
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

    def _frame_overlap_correction(self, ws):
        pass

    def _apply_polarisation_corrections(self, ws, pol_eff_ws):
        """Applies the polarisation correction based on the output from quartz reduction."""
        fp = 1 # flipper efficiency, placeholder
        nPolarisations = None
        singleCorrectionPerPOL = False
        if mtd[ws].getNumberOfEntries() != 2*mtd[pol_eff_ws].getNumberOfEntries():
            singleCorrectionPerPOL = True
            nMeasurements, _ = self._data_structure_helper()
            nPolarisations = math.floor(nMeasurements / 2.0)
            if mtd[pol_eff_ws].getNumberOfEntries() != nPolarisations:
                raise RuntimeError("Incompatible number of polarisations between quartz input and sample.")

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

            nominator = ((1.0-mtd[phi])*(1.0-fp) + fp*(1+mtd[phi])) * mtd[intensity_0] \
                - (1.0-mtd[phi]) * mtd[intensity_1]
            denominator = 2.0 * fp * mtd[phi]
            Divide(LHSWorkspace=nominator, RHSWorkspace=denominator,
                   OutputWorkspace=tmp_names[0])
            nominator = (1+mtd[phi])*mtd[intensity_1] \
                - ( (1+mtd[phi])*(1-fp) - fp*(1-mtd[phi]) )*mtd[intensity_0]
            denominator = 2.0 * fp * mtd[phi]
            Divide(LHSWorkspace=nominator, RHSWorkspace=denominator,
                   OutputWorkspace=tmp_names[1])

            RenameWorkspace(tmp_names[0], intensity_0)
            RenameWorkspace(tmp_names[1], intensity_1)

        DeleteWorkspaces(WorkspaceList=[nominator, denominator])
        return ws

    def _read_experiment_properties(self, ws):
        """Reads the user-provided dictionary that contains sample geometry (type, dimensions) and experimental conditions,
         such as the beam size and calculates derived parameters."""
        self._sampleAndEnvironmentProperties = self.getProperty('SampleAndEnvironmentPropertiesDictionary').value
        if 'n_atoms' not in self._sampleAndEnvironmentProperties:
            self._sampleAndEnvironmentProperties['NAtoms'] = self._sampleAndEnvironmentProperties['NumberDensity'].value \
                * self._sampleAndEnvironmentProperties['Mass'].value \
                / float(self._sampleAndEnvironmentProperties['Density'].value)
        if 'initial_energy' not in self._sampleAndEnvironmentProperties:
            h = physical_constants['Planck constant'][0]  # in m^2 kg^2 / s^2
            neutron_mass = physical_constants['neutron mass'][0]  # in kg
            wavelength = mtd[ws][0].getRun().getLogData('monochromator.wavelength').value * 1e-10  # in m
            joules_to_mev = 1e3 / physical_constants['electron volt'][0]
            self._sampleAndEnvironmentProperties['InitialEnergy'] = joules_to_mev * math.pow(h / wavelength, 2) / (2 * neutron_mass)

    def _enforce_uniform_units(self, origin_ws, target_ws):
        for entry_tuple in zip(mtd[origin_ws], mtd[target_ws]):
            entry_origin, entry_target = entry_tuple
            if entry_origin.YUnit() != entry_target.YUnit():
                entry_target.setYUnit(entry_origin.YUnit())

    def _apply_self_attenuation_correction(self, sample_ws, container_ws):
        """Applies the self-attenuation correction based on the Palmaan-Pings Monte-Carlo calculation, taking into account
        the sample's material, shape, and dimensions."""
        geometry_type = self.getPropertyValue('SampleGeometry')

        kwargs = {}
        kwargs['BeamHeight'] = self._sampleAndEnvironmentProperties['BeamHeight'].value
        kwargs['BeamWidth'] = self._sampleAndEnvironmentProperties['BeamWidth'].value
        kwargs['SampleDensity'] = self._sampleAndEnvironmentProperties['Density'].value
        kwargs['Height'] = self._sampleAndEnvironmentProperties['Height'].value
        kwargs['SampleChemicalFormula'] = self._sampleAndEnvironmentProperties['ChemicalFormula'].value
        if 'container_formula' in self._sampleAndEnvironmentProperties:
            kwargs['ContainerChemicalFormula'] = self._sampleAndEnvironmentProperties['ContainerFormula'].value
            kwargs['ContainerDensity'] = self._sampleAndEnvironmentProperties['ContainerDensity'].value
        if geometry_type == 'FlatPlate':
            kwargs['SampleWidth'] = self._sampleAndEnvironmentProperties['Width'].value
            kwargs['SampleThickness'] = self._sampleAndEnvironmentProperties['Thickness'].value
            if 'container_formula' in self._sampleAndEnvironmentProperties:
                kwargs['ContainerFrontThickness'] = self._sampleAndEnvironmentProperties['ContainerFrontThickness'].value
                kwargs['ContainerBackThickness'] = self._sampleAndEnvironmentProperties['ContainerBackThickness'].value
        elif geometry_type == 'Cylinder':
            kwargs['SampleRadius'] = self._sampleAndEnvironmentProperties['Radius'].value
            if 'container_formula' in self._sampleAndEnvironmentProperties:
                kwargs['ContainerRadius'] = self._sampleAndEnvironmentProperties['ContainerRadius'].value
        elif geometry_type == 'Annulus':
            kwargs['SampleInnerRadius'] = self._sampleAndEnvironmentProperties['InnerRadius'].value
            kwargs['SampleOuterRadius']  = self._sampleAndEnvironmentProperties['OuterRadius'].value
            if 'container_formula' in self._sampleAndEnvironmentProperties:
                kwargs['ContainerInnerRadius'] = self._sampleAndEnvironmentProperties['ContainerInnerRadius'].value
                kwargs['ContainerOuterRadius'] = self._sampleAndEnvironmentProperties['ContainerOuterRadius'].value

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
        nComponents = 0
        if self._user_method == 'None':
            if self._method_data_structure == '10p':
                nMeasurements = 10
            elif self._method_data_structure == 'XYZ':
                nMeasurements = 6
            elif self._method_data_structure == 'Uniaxial':
                nMeasurements = 2
        else:
            if self._user_method == '10p':
                nMeasurements = 10
                nComponents = 3
            elif self._user_method == 'XYZ':
                nMeasurements = 6
                nComponents = 3
            elif self._user_method == 'Uniaxial':
                nMeasurements = 2
                nComponents = 2

        return nMeasurements, nComponents

    def _component_separation(self, ws):
        """Separates coherent, incoherent, and magnetic components based on spin-flip and non-spin-flip intensities of the
        current sample. The method used is based on either the user's choice or the provided data structure."""

        nMeasurements, nComponents = self._data_structure_helper()
        componentNames = ['Coherent', 'Incoherent', 'Magnetic']
        number_histograms = mtd[ws][0].getNumberHistograms()
        block_size = mtd[ws][0].blocksize()
        tmp_names = []

        for entry_no in range(0, mtd[ws].getNumberOfEntries(), nMeasurements):
            dataY_nuclear = np.zeros(shape=(number_histograms, block_size))
            dataY_incoherent = np.zeros(shape=(number_histograms, block_size))
            dataY_magnetic = np.zeros(shape=(number_histograms, block_size))
            for spectrum in range(number_histograms):
                sigma_z_sf = mtd[ws][entry_no].readY(spectrum)
                sigma_z_nsf = mtd[ws][entry_no+1].readY(spectrum)
                if nMeasurements == 2:
                    dataY_nuclear[spectrum] = 2.0 * sigma_z_nsf - sigma_z_sf  # Nuclear coherent
                    dataY_incoherent[spectrum] = 2.0 * sigma_z_sf - sigma_z_nsf # Incoherent
                    dataY_magnetic[spectrum] = 0 # Magnetic
                elif nMeasurements == 6 or nMeasurements == 10:
                    sigma_y_sf = mtd[ws][entry_no+2].readY(spectrum)
                    sigma_y_nsf = mtd[ws][entry_no+3].readY(spectrum)
                    sigma_x_sf = mtd[ws][entry_no+4].readY(spectrum)
                    sigma_x_nsf = mtd[ws][entry_no+5].readY(spectrum)
                    if nMeasurements == 6:
                        # Magnetic component
                        magnetic_component = 2.0 * (2.0 * sigma_z_nsf - sigma_x_nsf - sigma_y_nsf )
                        dataY_magnetic[spectrum] = magnetic_component
                        # Nuclear coherent component
                        dataY_nuclear[spectrum] = (2.0*(sigma_x_nsf + sigma_y_nsf + sigma_z_nsf)
                                                   - (sigma_x_sf + sigma_y_sf + sigma_z_sf)) / 6.0
                        # Incoherent component
                        dataY_incoherent[spectrum] = 0.5 * (sigma_x_sf + sigma_y_sf + sigma_z_sf) - magnetic_component
                    else:
                        sigma_xmy_sf = mtd[ws][entry_no+6].readY(spectrum)
                        sigma_xmy_nsf = mtd[ws][entry_no+7].readY(spectrum)
                        sigma_xpy_sf = mtd[ws][entry_no+8].readY(spectrum)
                        sigma_xpy_nsf = mtd[ws][entry_no+9].readY(spectrum)
                        # Magnetic component
                        try:
                            theta_0 = self._sampleAndEnvironmentProperties['ThetaOffset'].value
                        except KeyError:
                            raise RuntimeError("The value for theta_0 needs to be defined for the component separation in 10p method.")
                        theta = mtd[ws][entry_no].detectorInfo().twoTheta(spectrum)
                        alpha = theta - 0.5*np.pi - theta_0
                        c0 = math.pow(math.cos(alpha), 2)
                        c4 = math.pow(math.cos(alpha - np.pi/4.0), 2)
                        magnetic_cos2alpha = (2*c0-4)*sigma_x_nsf + (2*c0+2)*sigma_y_nsf + (2-4*c0)*sigma_z_nsf
                        magnetic_sin2alpha = (2*c4-4)*sigma_xpy_nsf + (2*c4+2)*sigma_xmy_nsf + (2-4*c4)*sigma_z_nsf
                        magnetic_component = magnetic_cos2alpha * math.cos(2*alpha) + magnetic_sin2alpha * math.sin(2*alpha)
                        dataY_magnetic[spectrum] = magnetic_component
                        # Nuclear coherent component
                        dataY_nuclear[spectrum] = (2.0 * (sigma_x_nsf + sigma_y_nsf + 2*sigma_z_nsf + sigma_xpy_nsf + sigma_xmy_nfs)
                                                   - (sigma_x_sf + sigma_y_sf + 2*sigma_z_sf + sigma_xpy_sf + sigma_xmy_sf)) / 12.0
                        # Incoherent component
                        dataY_incoherent[spectrum] = 0.25 * (sigma_x_sf + sigma_y_sf + 2*sigma_z_sf + sigma_xpy_sf + sigma_xmy_sf) \
                            - magnetic_component

            dataY = [dataY_nuclear, dataY_incoherent, dataY_magnetic]
            for component in range(nComponents):
                dataX = mtd[ws][entry_no].readX(0)
                dataE = np.sqrt(abs(dataY[component]))
                tmp_name = str(mtd[ws][entry_no].name())[:-1] + componentNames[component]
                tmp_names.append(tmp_name)
                CreateWorkspace(DataX=dataX, DataY=dataY[component], dataE=dataE,
                                Nspec=mtd[ws][entry_no].getNumberHistograms(),
                                OutputWorkspace=tmp_name)
        output_name = self.getPropertyValue('ProcessAs') + '_component_separation'
        GroupWorkspaces(tmp_names, OutputWorkspace=output_name)
        return output_name

    def _conjoin_components(self, ws):
        """Conjoins the component workspaces coming from a theta scan."""
        components = [[], []]
        componentNames = ['Incoherent', 'Coherent', 'Magnetic']
        if self._user_method in ['10p', 'XYZ']:
            components.append([])
        for entry in mtd[ws]:
            entryName = entry.name()
            ConvertToPointData(InputWorkspace=entry, OutputWorkspace=entry)
            for component_no, componentName in enumerate(componentNames):
                if componentName in entryName:
                    components[component_no].append(entryName)

        x_axis = NumericAxis.create(len(components[0]))
        for index in range(len(components[0])):
            x_axis.setValue(index, index)
        x_axis.setUnit("Label").setLabel('Scan step', '')

        ws_names = []
        for component_no, compList in enumerate(components):
            ws_name = '{}_component'.format(componentNames[component_no])
            ws_names.append(ws_name)
            ConjoinXRuns(InputWorkspaces=compList, OutputWorkspace=ws_name)
            mtd[ws_name].replaceAxis(0, x_axis)
        output_name = '{}_conjoined_components'.format(self.getPropertyValue('ProcessAs'))
        GroupWorkspaces(ws_names, OutputWorkspace=output_name)
        return output_name

    def _detector_efficiency_correction(self, ws, component_ws):
        """Corrects detector efficiency and normalises data to either vanadium, incoherent scattering, or paramagnetic scattering."""

        nMeasurements, _ = self._data_structure_helper()
        if component_ws:
            conjoined_components = self._conjoin_components(component_ws)
        calibrationType = self.getPropertyValue('DetectorEfficiencyCalibration')
        normaliseToAbsoluteUnits = self.getProperty('AbsoluteUnitsNormalisation').value
        tmp_name = 'det_eff'
        tmp_names = []
        if calibrationType == 'Vanadium':
            vanadium_ws = self.getPropertyValue('VanadiumInputWorkspace')
            if normaliseToAbsoluteUnits:
                normFactor = self._sampleAndEnvironmentProperties['FormulaUnits'].value
                CreateSingleValuedWorkspace(DataValue=normFactor, OutputWorkspace='normalisation_ws')
            else:
                normalisationFactors = self._max_value_per_detector(vanadium_ws)
                dataE = np.sqrt(normalisationFactors)
                entry0 = mtd[vanadium_ws][0]
                CreateWorkspace(dataX=entry0.readX(0), dataY=normalisationFactors, dataE=dataE,
                                NSpec=entry0.getNumberHistograms(), OutputWorkspace='normalisation_ws')
            Divide(LHSWorkspace=vanadium_ws,
                   RHSWorkspace='normalisation_ws',
                   OutputWorkspace='det_efficiency')
        elif calibrationType in  ['Paramagnetic', 'Incoherent']:
            if calibrationType == 'Paramagnetic':
                if self._mode == 'TOF':
                    raise RuntimeError('Paramagnetic calibration is not valid in the TOF mode.')
                if self._user_method == 'Uniaxial':
                    raise RuntimeError('Paramagnetic calibration is not valid in the Uniaxial measurement mode.')
                for entry_no, entry in enumerate(mtd[ws]):
                    ws_name = '{0}_{1}'.format(tmp_name, entry_no)
                    tmp_names.append(ws_name)
                    const = (2.0/3.0) * math.pow(physical_constants['neutron gyromag. ratio']
                                                 * physical_constants['classical electron radius'], 2)
                    paramagneticComponent = mtd[conjoined_components][2]
                    spin = self._sampleAndEnvironmentProperties['SampleSpin'].value
                    Divide(LHSWorkspace=const * spin * (spin+1),
                           RHSWorkspace=paramagneticComponent,
                           OutputWorkspace=ws_name)
            else: # Incoherent
                if self._mode == 'TOF':
                    raise RuntimeError('Incoherent calibration is not valid in the TOF mode.')
                for spectrum_no in range(mtd[conjoined_components][1].getNumberHistograms()):
                    if normaliseToAbsoluteUnits:
                        normFactor = float(self.getPropertyValue('IncoherentCrossSection'))
                        CreateSingleValuedWorkspace(DataValue=normFactor, OutputWorkspace='normalisation_ws')
                    else:
                        normalisationFactors = self._max_value_per_detector(mtd[conjoined_components][1].name())
                        dataE = np.sqrt(normalisationFactors)
                        CreateWorkspace(dataX=mtd[conjoined_components][1].readX(0), dataY=normalisationFactors,
                                        dataE=dataE,
                                        NSpec=mtd[conjoined_components][1].getNumberHistograms(),
                                        OutputWorkspace='normalisation_ws')
                    ws_name = '{0}_{1}'.format(tmp_name, entry_no)
                    tmp_names.append(ws_name)

                Divide(LHSWorkspace='normalisation_ws',
                       RHSWorkspace=component_ws,
                       OutputWorkspace=ws_name)

            GroupWorkspaces(tmp_names, OutputWorkspace='det_efficiency')

        single_efficiency_per_POL = False
        if mtd[ws].getNumberOfEntries() != mtd['det_efficiency'].getNumberOfEntries():
            single_efficiency_per_POL = True
        for entry_no, entry in enumerate(mtd[ws]):
            det_eff_entry_no = entry_no
            if single_efficiency_per_POL:
                det_eff_entry_no = det_eff_entry_no % nMeasurements
            Multiply(LHSWorkspace=entry,
                     RHSWorkspace=mtd['det_efficiency'][det_eff_entry_no],
                     OutputWorkspace=entry)
        return ws

    def _sum_TOF_data(self, ws):
        """Integrates intensities over all time channels or time-of-flight bins."""
        tofUnits = self.getPropertyValue('TOFUnits')
        if tofUnits == 'UncalibratedTime':
            timeBinWidth = mtd[ws][0].getRun().getLogData('Detector.time_of_flight_0')
            Multiply(LHSWorkspace=ws, RHSWorkspace=timeBinWidth, OutputWorkspace=ws)
        if tofUnits == 'Energy':
            energyBinWidth = 0 # placeholder
            Multiply(LHSWorkspace=ws, RHSWorkspace=energyBinWidth, OutputWorkspace=ws)
        Integration(InputWorkspace=ws, OutputWorkspace=ws)
        return ws

    def _output_vanadium(self, ws):
        """Performs normalisation of the vanadium data to the expected cross-section."""
        if self._mode == 'TOF':
            ws = self._sum_TOF_data(ws)
        CreateSingleValuedWorkspace(DataValue=0.404 * self._sampleAndEnvironmentProperties['NAtoms'].value,
                                    OutputWorkspace='norm')
        Divide(LHSWorkspace='norm', RHSWorkspace=ws, OutputWorkspace=ws)
        DeleteWorkspace(Workspace='norm')
        return ws

    def _set_units(self, ws):
        output_unit = self.getPropertyValue('OutputUnits')
        if output_unit == 'TwoTheta':
            if (self.getProperty('OutputTreatment').value == 'SumScans'
                    and isinstance(mtd[ws], WorkspaceGroup)
                    and mtd[ws].getNumberOfEntries() > 1):
                self._merge_polarisations(ws)
            else:
                ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=ws, Target='SignedTheta')
            ConvertAxisByFormula(InputWorkspace=ws, OutputWorkspace=ws, Axis='Y', Formula='-y')
        elif output_unit == 'Q':
            ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=ws, Target='ElasticQ',
                                EFixed=self._sampleAndEnvironmentProperties['InitialEnergy'].value)
        for entry in mtd[ws]:
            unit = ''
            if output_unit == 'TwoTheta':
                unit = 'S (TwoTheta)'
            elif output_unit == 'Q':
                unit = 'S (Q)'
            entry.setYUnit(unit)
        return ws

    def _finalize(self, ws, process):
        ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws, NaNValue=0,
                             NaNError=0, InfinityValue=0, InfinityError=0)
        mtd[ws][0].getRun().addProperty('ProcessedAs', process, True)
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        self.setProperty('OutputWorkspace', mtd[ws[2:]])

    def PyExec(self):
        process = self.getPropertyValue('ProcessAs')
        processes = ['Absorber', 'Beam', 'Transmission', 'Container', 'Quartz', 'Vanadium', 'Sample']
        progress = Progress(self, start=0.0, end=1.0, nreports=processes.index(process) + 1)
        ws = '__' + self.getPropertyValue('OutputWorkspace')

        calibration_setting = 'YIGFile'
        if self.getProperty('InstrumentCalibration').isDefault:
            calibration_setting = 'None'

        Load(Filename=self.getPropertyValue('Run'), LoaderName='LoadILLPolarizedDiffraction',
             PositionCalibration=calibration_setting, YIGFileName=self.getPropertyValue('InstrumentCalibration'),
             TOFUnits=self.getPropertyValue('TOFUnits'), OutputWorkspace=ws)

        self._instrument = mtd[ws][0].getInstrument().getName()
        self._mode = self.getPropertyValue('MeasurementTechnique')
        run = mtd[ws][0].getRun()
        if run['acquisition_mode'].value == 1 and self._mode != 'TOF':
            raise RuntimeError("Monochromatic measurement method chosen but data contains TOF results.")
        elif self._mode == 'TOF' and run['acquisition_mode'].value == 0:
            raise RuntimeError("TOF measurement method chosen but data contains monochromatic results.")
        self._user_method = self.getPropertyValue('ComponentSeparationMethod')
        self._figure_out_measurement_method(ws)
        progress.report()
        if process in ['Beam', 'Transmission']:
            if mtd[ws].getNumberOfEntries() > 1:
                self._merge_polarisations(ws, average_detectors=True)
            absorber_transmission_ws = self.getPropertyValue('AbsorberTransmissionInputWorkspace')
            if absorber_transmission_ws:
                Minus(LHSWorkspace=ws, RHSWorkspace=absorber_transmission_ws, OutputWorkspace=ws)
            monID = 100001 # monitor 2
            ExtractSpectra(InputWorkspace=ws, DetectorList=monID, OutputWorkspace=ws)
            if process in ['Transmission']:
                beam_ws = self.getPropertyValue('BeamInputWorkspace')
                self._calculate_transmission(ws, beam_ws)
            progress.report()
        else:
            self._normalise(ws)
            progress.report()

        if process in ['Quartz', 'Vanadium', 'Sample']:
            if not self.getProperty('ContainerInputWorkspace').isDefault and not self.getProperty('TransmissionInputWorkspace').isDefault:
                # Subtracts background if the workspaces for container and transmission are provided
                container_ws = self.getPropertyValue('ContainerInputWorkspace')
                transmission_ws = self.getPropertyValue('TransmissionInputWorkspace')
                self._subtract_background(ws, container_ws, transmission_ws)
                progress.report()

            if process == 'Quartz':
                self._calculate_polarising_efficiencies(ws)
                progress.report()

            if process in ['Vanadium', 'Sample']:
                if self._mode == 'TOF' and process == 'Sample':
                    self._frame_overlap_correction(ws)
                    progress.report()
                pol_eff_ws = self.getPropertyValue('QuartzInputWorkspace')
                if pol_eff_ws:
                    self._apply_polarisation_corrections(ws, pol_eff_ws)
                    progress.report()
                if self._mode == 'TOF':
                    self._detector_analyser_energy_efficiency(ws)
                    progress.report()
                self._read_experiment_properties(ws)
                if self.getPropertyValue('SampleGeometry') != 'None' and self._mode != 'TOF':
                    self._apply_self_attenuation_correction(ws, container_ws)
                progress.report()
                if self.getProperty('OutputTreatment').value == 'AverageScans':
                    self._merge_polarisations(ws, average_detectors=True)
                if self._user_method != 'None':
                    component_ws = self._component_separation(ws)
                else:
                    component_ws = ''
                progress.report()
                if process == 'Vanadium':
                    self._output_vanadium(ws)
                else:
                    self._detector_efficiency_correction(ws, component_ws)
                    progress.report()
                self._set_units(ws)

        self._finalize(ws, process)


AlgorithmFactory.subscribe(PolDiffILLReduction)
