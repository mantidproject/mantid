# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import FileProperty, MatrixWorkspaceProperty, MultipleFileProperty, \
    NumericAxis, PropertyMode, Progress, PythonAlgorithm, WorkspaceGroup, WorkspaceGroupProperty, \
    FileAction, AlgorithmFactory
from mantid.kernel import Direction, EnabledWhenProperty, IntBoundedValidator, LogicOperator, \
    PropertyCriterion, StringListValidator

from mantid.simpleapi import *

from scipy.constants import physical_constants
import numpy as np
import math


class PolDiffILLReduction(PythonAlgorithm):

    _mode = 'Monochromatic'
    _method = 'Uniaxial'
    _instrument = None
    _DEG_2_RAD =  np.pi / 180.0

    _sampleGeometry = None

    @staticmethod
    def _max_value_per_detector(ws):
        max_values = np.zeros(mtd[ws].getItem(0).getNumberOfHistograms())
        for entry in mtd[ws]:
            for spectrum_no in range(entry.getNumberOfHistograms()):
                dataY = entry.readY(spectrum_no)
                if dataY > max_values[spectrum_no]:
                    max_values = dataY
        return max_values

    def category(self):
        return 'ILL\\Diffraction'

    def summary(self):
        return 'Performs polarized diffraction data reduction at the ILL.'

    def seeAlso(self):
        return ['PolDIffILLAutoProcess']

    def name(self):
        return 'PolDiffILLReduction'

    def validateInputs(self):
        issues = dict()
        process = self.getPropertyValue('ProcessAs')
        if process == 'Transmission' and self.getProperty('BeamInputWorkspace').isDefault:
            issues['BeamInputWorkspace'] = 'Beam input workspace is mandatory for transmission calculation.'
            issues['CadmiumTransmissionInputWorkspace'] = 'Cadmium transmission input workspace is mandatory for transmission calculation.'

        if ( (not self.getProperty('AbsorberInputWorkspace').isDefault and self.getProperty('ContainerInputWorkspace').isDefault)
             or (not self.getProperty('AbsorberInputWorkspace').isDefault and self.getProperty('ContainerInputWorkspace').isDefault) ):
            issues['AbsorberInputWorkspace'] = 'Both Container and Absorber input workspaces are mandatory for background subtraction.'
            issues['ContainerInputWorkspace'] = 'Both Container and Absorber input workspaces are mandatory for background subtraction.'

        if process == 'Quartz' and self.getProperty('TransmissionInputWorkspace').isDefault:
            issues['TransmissionInputWorkspace'] = 'Quartz transmission is mandatory for polarization correction calculation.'

        if process == 'Vanadium' and self.getProperty('TransmissionInputWorkspace').isDefault :
            issues['TransmissionInputWorkspace'] = 'Vanadium transmission is mandatory for vanadium data reduction.'

        if process == 'Sample' or process == 'Vanadium':
            if self.getProperty('SampleGeometry') != 'None' and self.getProperty('ChemicalFormula').isDefault:
                issues['ChemicalFormula'] = 'Chemical formula of the sample is required for self-attenuation correction.'
            if self.getProperty('SampleGeometryDictionary').isDefault:
                issues['SampleGeometryDictionary'] = 'Sample parameters need to be defined.'

        if process == 'Sample':
            if self.getProperty('TransmissionInputWorkspace').isDefault :
                issues['TransmissionInputWorkspace'] = 'Sample transmission is mandatory for sample data reduction.'

            if (self.getProperty('DetectorEfficiencyCalibration') == 'Vanadium'
                    and self.getProperty('VanadiumInputWorkspace').isDefault):
                issues['VanadiumInputWorkspace'] = 'Vanadium input workspace is mandatory for sample data reduction when \
                    detector efficiency calibration is based "Vanadium".'

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
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc='The output workspace based on the value of ProcessAs.')

        sample = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Sample')

        transmission = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Transmission')

        container = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Container')

        absorber = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Absorber')

        quartz = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Quartz')

        vanadium = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Vanadium')

        reduction = EnabledWhenProperty(quartz, EnabledWhenProperty(vanadium, sample, LogicOperator.Or), LogicOperator.Or)

        scan = EnabledWhenProperty(reduction, EnabledWhenProperty(absorber, container, LogicOperator.Or), LogicOperator.Or)

        self.declareProperty(WorkspaceGroupProperty('AbsorberInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the absorber workspace.')

        self.setPropertySettings('AbsorberInputWorkspace',
                                 EnabledWhenProperty(quartz,
                                                     EnabledWhenProperty(vanadium, sample, LogicOperator.Or),
                                                     LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('BeamInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the empty beam input workspace.')

        self.setPropertySettings('BeamInputWorkspace', transmission)

        self.declareProperty(MatrixWorkspaceProperty('CadmiumTransmissionInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the cadmium transmission input workspace.')

        self.setPropertySettings('CadmiumTransmissionInputWorkspace', transmission)

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

        self.declareProperty('SumScan', False,
                             doc='Whether or not to sum the multiple scan steps into a single distribution')

        self.setPropertySettings('SumScan', scan)

        self.declareProperty('AbsoluteUnitsNormalisation', True,
                             doc='Whether or not express the output in absolute units.')

        self.setPropertySettings('AbsoluteUnitsNormalisation', EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty('ClearCache', True,
                             doc='Whether or not to clear the cache of intermediate workspaces.')

        # self-attenuation group:
        self.declareProperty(name="SampleGeometry",
                             defaultValue="FlatPlate",
                             validator=StringListValidator(["FlatPlate", "Cylinder", "Annulus", "Custom", "None"]),
                             direction=Direction.Input,
                             doc="Sample geometry for self-attenuation correction to be applied.")

        self.setPropertySettings('SampleGeometry', EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty(name="SampleGeometryDictionary",
                             defaultValue="",
                             direction=Direction.Input,
                             doc="Dictionary for the geometry used for self-attenuation correction.")

        self.setPropertySettings('SampleGeometryDictionary',
                                 EnabledWhenProperty('SampleGeometry', PropertyCriterion.IsNotEqualTo, 'Custom'))

        self.declareProperty(FileProperty(name="SampleGeometryFile",
                             defaultValue="",
                             action=FileAction.OptionalLoad),
                             doc="The path to the custom geometry for self-attenuation correction to be applied.")

        self.setPropertySettings('SampleGeometryFile', EnabledWhenProperty('SampleGeometry', PropertyCriterion.IsEqualTo, 'Custom'))

        self.declareProperty(name="ChemicalFormula",
                             defaultValue="",
                             direction=Direction.Input,
                             doc="Chemical formula of the sample")

        self.setPropertySettings('ChemicalFormula', EnabledWhenProperty('SampleGeometry', PropertyCriterion.IsNotEqualTo, 'None'))

        self.declareProperty(name="DetectorEfficiencyCalibration",
                             defaultValue="Incoherent",
                             validator=StringListValidator(["Incoherent", "Vanadium",  "Paramagnetic"]),
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
                                                                               LogicOperator.Or))

        self.declareProperty(name="TOFUnits",
                             defaultValue="TimeChannels",
                             validator=StringListValidator(["TimeChannels", "UncalibratedTime", "Energy"]),
                             direction=Direction.Input,
                             doc="The choice to display the TOF data either as a function of the time channel or the uncalibrated time.\
                             It has no effect if the measurement mode is monochromatic.")

        self.setPropertySettings('SampleGeometry', EnabledWhenProperty(vanadium, sample, LogicOperator.Or))

        self.declareProperty(FileProperty('InstrumentParameterFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['.xml']),
                             doc='The path to the calibrated Instrument Parameter File.')

        self.setPropertySettings('InstrumentParameterFile', scan)

    def _figure_measurement_method(self, ws):
        """Figures out the measurement method based on the structure of the input files."""
        nEntriesPerNumor = mtd[ws].getNumberOfEntries() / len(self.getPropertyValue('Run').split(','))
        if nEntriesPerNumor == 10:
            self._method = '10-p'
        elif nEntriesPerNumor == 6:
            self._method = 'XYZ'
        elif nEntriesPerNumor == 2:
            self._method = 'Uniaxial'
        else:
            if self.getPropertyValue("ProcessAs") not in ['Beam', 'Transmission']:
                raise RuntimeError("The analysis options are: Uniaxial, XYZ, and 10-point. "
                                   +"The provided input does not fit in any of these measurement types.")

    def _merge_polarisations(self, ws):
        """Merges workspaces with the same polarisation inside the provided WorkspaceGroup."""
        pol_directions = set()
        numors = set()
        for name in mtd[ws].getNames():
            numors.add(name[:-2])
            pol_directions.add(name[-1])
        if len(numors) > 1:
            names_list = []
            for direction in sorted(list(pol_directions)):
                list_pol = []
                for numor in numors:
                    list_pol.append('{0}_{1}'.format(numor, direction))
                SumOverlappingTubes(','.join(list_pol), OutputWorkspace='{0}_{1}'.format(ws[2:], direction),
                                    OutputType='1D', ScatteringAngleBinning=0.5, Normalise=True, HeightAxis='-0.1,0.1')
                names_list.append('{0}_{1}'.format(ws[2:], direction))
            GroupWorkspaces(InputWorkspaces=names_list, OutputWorkspace=ws)
        return ws

    def _normalise(self, ws):
        """Normalises the provided WorkspaceGroup to the monitor 1."""
        monID = 100000
        monitorIndices = "{},{}".format(mtd[ws].getItem(0).getNumberHistograms()-2,
                                        mtd[ws].getItem(0).getNumberHistograms()-1)
        for entry_no, entry in enumerate(mtd[ws]):
            mon = ws + '_mon'
            ExtractSpectra(InputWorkspace=entry, DetectorList=monID, OutputWorkspace=mon)
            if 0 in mtd[mon].readY(0):
                raise RuntimeError('Cannot normalise to monitor; monitor has 0 counts.')
            else:
                if self._mode == 'TOF':
                    Integration(InputWorkspace=mon, OutputWorkspace=mon)
                Divide(LHSWorkspace=entry, RHSWorkspace=mon, OutputWorkspace=entry)
                RemoveSpectra(entry, WorkspaceIndices=monitorIndices, OutputWorkspace=entry)
                DeleteWorkspace(mon)
        return ws

    def _calculate_transmission(self, ws, ws_beam, ws_cadmium):
        """Calculates transmission based on the measurement of the current sample, empty beam, and cadmium."""
        # extract Monitor2 values
        monID = 100001
        mon = ws + '_mon'
        ExtractSpectra(InputWorkspace=ws, DetectorList=monID, OutputWorkspace=mon)
        if 0 in mtd[mon].getItem(0).readY(0):
            raise RuntimeError('Cannot calculate transmission; monitor has 0 counts.')
        beam_mon = ws_beam + '_mon'
        ExtractSpectra(InputWorkspace=ws_beam, DetectorList=monID, OutputWorkspace=beam_mon)
        if 0 in mtd[beam_mon].readY(0):
            raise RuntimeError('Cannot calculate transmission; beam monitor has 0 counts.')
        cadmium_mon = ws_cadmium + '_mon'
        ExtractSpectra(InputWorkspace=ws_cadmium, DetectorList=monID, OutputWorkspace=cadmium_mon)
        if 0 in mtd[beam_mon].readY(0):
            raise RuntimeError('Cannot calculate transmission; beam monitor has 0 counts.')
        else:
            Divide(LHSWorkspace=mtd[mon]-mtd[cadmium_mon], RHSWorkspace=mtd[beam_mon]-mtd[cadmium_mon], OutputWorkspace=ws)
            DeleteWorkspace(mon)
            DeleteWorkspace(beam_mon)
            DeleteWorkspace(cadmium_mon)
        return ws

    def _subtract_background(self, ws, container_ws, transmission_ws):
        """Subtracts empty container and cadmium scaled by transmission."""
        if self._mode != 'TOF':
            absorber_ws = self.getPropertyValue('AbsorberInputWorkspace')
            if absorber_ws == "":
                raise RuntimeError("Absorber input workspace needs to be provided for non-TOF background subtraction.")
        for entry_no, entry in enumerate(mtd[ws]):
            container_entry = mtd[container_ws].getItem(entry_no).name()
            mtd[container_entry].setYUnit('Counts/Counts')
            mtd[transmission_ws].setYUnit('Counts/Counts')
            if self._mode != 'TOF':
                absorber_entry = mtd[absorber_ws].getItem(entry_no).name()
                mtd[absorber_entry].setYUnit('Counts/Counts')
                Minus(LHSWorkspace=entry,
                      RHSWorkspace=mtd[transmission_ws] * mtd[container_entry] + (1-mtd[transmission_ws]) * mtd[absorber_entry],
                      OutputWorkspace=entry)
            else:
                raise RuntimeError("TOF requires elastic channel definition")
        return ws

    def _calculate_polarizing_efficiencies(self, ws):
        """Calculates the polarizing efficiencies using quartz data."""
        flipper_eff = 1.0 # this could be extracted from data if 4 measurements are done
        nMeasurementsPerPOL = 2
        tmp_names = []
        index = 0
        for entry_no in range(1, mtd[ws].getNumberOfEntries()+1, nMeasurementsPerPOL):
            # two polarizer-analyzer states, fixed flipper_eff
            ws_00 = mtd[ws].getItem(entry_no).name()
            ws_01 = mtd[ws].getItem(entry_no-1).name()
            tmp_name = '{0}_{1}_{2}'.format(ws[2:], mtd[ws_00].getRun().getLogData('POL.actual_state').value, index)
            Divide(LHSWorkspace=mtd[ws_00]-mtd[ws_01],
                   RHSWorkspace=(2*flipper_eff-1)*mtd[ws_00]+mtd[ws_01],
                   OutputWorkspace=tmp_name)
            tmp_names.append(tmp_name)
            if self._method == 'Uniaxial' and entry_no % 2 == 1:
                index += 1
            elif self._method == 'XYZ' and entry_no % 6 == 5:
                index += 1
            elif self._method == '10-p' and entry_no % 10 == 9:
                index += 1
        GroupWorkspaces(InputWorkspaces=tmp_names, OutputWorkspace='tmp')
        DeleteWorkspaces(ws)
        RenameWorkspace(InputWorkspace='tmp', OutputWorkspace=ws)
        return ws

    def _detector_analyser_energy_efficiency(self, ws):
        """Corrects for the detector and analyser energy efficiency."""
        correction_formula = "1.0 - exp(-13.153/sqrt(e))"
        h = physical_constants['Planck constant'][0] # in m^2 kg^2 / s^2
        neutron_mass = physical_constants['neutron mass'][0] # in kg
        wavelength = mtd[ws].getItem(0).getRun().getLogData('monochromator.wavelength').value * 1e-10 # in m
        joules_to_mev = 1e3 / physical_constants['electron volt'][0]
        initialEnergy = joules_to_mev * math.pow(h / wavelength, 2) / (2 * neutron_mass)
        for entry in mtd[ws]:
            SetInstrumentParameter(entry, ParameterName="formula_eff", Value=correction_formula)
            DetectorEfficiencyCorUser(InputWorkspace=entry, OutputWorkspace=entry, IncidentEnergy=initialEnergy)
        return ws

    def _frame_overlap_correction(self, ws):
        pass

    def _apply_polarization_corrections(self, ws, pol_eff_ws):
        """Applies the polarisation correction based on the output from quartz reduction."""
        fp = 1 # flipper efficiency, placeholder
        for entry_no in range(mtd[ws].getNumberOfEntries()):
            if entry_no % 2 != 0:
                continue
            phi = mtd[pol_eff_ws].getItem(int(entry_no/2)).name()
            intensity_0 = mtd[ws].getItem(entry_no).name()
            intensity_1 = mtd[ws].getItem(entry_no+1).name()
            tmp_names = [intensity_0 + '_tmp', intensity_1 + '_tmp']

            Divide(LHSWorkspace=((1.0-mtd[phi])*(1.0-fp) + fp*(1+mtd[phi]))*mtd[intensity_0]
                   -(1.0-mtd[phi])*mtd[intensity_1],
                   RHSWorkspace=2.0 * fp * mtd[phi],
                   OutputWorkspace=tmp_names[0])
            Divide(LHSWorkspace=(1+mtd[phi])*mtd[intensity_1]
                   - ( (1+mtd[phi])*(1-fp) - fp*(1-mtd[phi]) )*mtd[intensity_0],
                   RHSWorkspace=2.0 * fp * mtd[phi],
                   OutputWorkspace=tmp_names[1])

            RenameWorkspace(tmp_names[0], intensity_0)
            RenameWorkspace(tmp_names[1], intensity_1)

        return ws

    def _read_sample_geometry(self):
        """Reads the user-provided dictionary that contains sample geometry (type, dimensions) and experimental conditions,
         such as the beam size."""
        raw_list = (self.getPropertyValue('SampleGeometryDictionary')).split(';')
        self._sampleGeometry = { item.split('=')[0] : item.split('=')[1] for item in raw_list }
        geometry_type = self.getPropertyValue('SampleGeometry')
        required_keys = ['mass', 'density', 'number_density', 'beam_height', 'beam_width', 'formula_units', 'container_density']

        if geometry_type == 'FlatPlate':
            required_keys += ['height', 'width', 'thickness', 'container_front_thickness', 'container_back_thickness']
        if geometry_type == 'Cylinder':
            required_keys += ['height', 'radius', 'container_radius']
        if geometry_type == 'Annulus':
            required_keys += ['height', 'inner_radius', 'outer_radius', 'container_inner_radius', 'container_outer_radius']

        for key in required_keys:
            if key not in self._sampleGeometry:
                raise RuntimeError('{} needs to be defined in the dictionary'.format(key))

        self._sampleGeometry['n_atoms'] = float(self._sampleGeometry['number_density']) \
            * float(self._sampleGeometry['mass']) \
            / float(self._sampleGeometry['density'])

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
        kwargs['BeamHeight'] = self._sampleGeometry['beam_height']
        kwargs['BeamWidth'] = self._sampleGeometry['beam_width']
        kwargs['SampleDensity'] = self._sampleGeometry['density']
        kwargs['Height'] = self._sampleGeometry['height']
        kwargs['SampleChemicalFormula'] = self.getPropertyValue('ChemicalFormula')
        if 'container_formula' in self._sampleGeometry:
            kwargs['ContainerChemicalFormula'] = self._sampleGeometry['container_formula']
            kwargs['ContainerDensity'] = self._sampleGeometry['container_density']
        if geometry_type == 'FlatPlate':
            kwargs['SampleWidth'] = self._sampleGeometry['width']
            kwargs['SampleThickness'] = self._sampleGeometry['thickness']
            if 'container_formula' in self._sampleGeometry:
                kwargs['ContainerFrontThickness'] = self._sampleGeometry['container_front_thickness']
                kwargs['ContainerBackThickness'] = self._sampleGeometry['container_back_thickness']
        elif geometry_type == 'Cylinder':
            kwargs['SampleRadius'] = self._sampleGeometry['radius']
            if 'container_formula' in self._sampleGeometry:
                kwargs['ContainerRadius'] = self._sampleGeometry['container_radius']
        elif geometry_type == 'Annulus':
            kwargs['SampleInnerRadius'] = self._sampleGeometry['inner_radius']
            kwargs['SampleOuterRadius']  = self._sampleGeometry['outer_radius']
            if 'container_formula' in self._sampleGeometry:
                kwargs['ContainerInnerRadius'] = self._sampleGeometry['container_inner_radius']
                kwargs['ContainerOuterRadius'] = self._sampleGeometry['container_outer_radius']

        self._enforce_uniform_units(sample_ws, container_ws)

        if geometry_type == 'Custom':
            raise RuntimeError('Custom geometry treatment has not been implemented yet.')
        else:
            for entry_no, entry in enumerate(mtd[sample_ws]):
                correction_ws = '{}_corr'.format(geometry_type)
                if ( (self._method == 'Uniaxial' and entry_no % 2 == 0)
                     or (self._method == 'XYZ' and entry_no % 6 == 0)
                     or (self._method == '10-p' and entry_no % 10 == 0) ):
                    PaalmanPingsMonteCarloAbsorption(SampleWorkspace=entry,
                                                     Shape=geometry_type,
                                                     CorrectionsWorkspace=correction_ws,
                                                     ContainerWorkspace=mtd[container_ws].getItem(entry_no),
                                                     **kwargs)
                ApplyPaalmanPingsCorrection(SampleWorkspace=entry,
                                            CorrectionsWorkspace=correction_ws,
                                            CanWorkspace=mtd[container_ws].getItem(entry_no),
                                            OutputWorkspace=entry)

        return sample_ws

    def _component_separation(self, ws):
        """Separates coherent, incoherent, and magnetic components based on spin-flip and non-spin-flip intensities of the
        current sample. The method used is based on either the user's choice or the provided data structure."""
        tmpNames = []
        nMeasurements = 0
        nComponents = 0
        componentNames = ['Coherent', 'Incoherent', 'Magnetic']
        if self._method == '10-p':
            nMeasurements = 10
            nComponents = 3
        elif self._method == 'XYZ':
            nMeasurements = 6
            nComponents = 3
        elif self._method == 'Uniaxial':
            nMeasurements = 2
            nComponents = 2

        for entry_no in range(0, mtd[ws].getNumberOfEntries(), nMeasurements):
            dataY_nuclear = np.zeros(shape=(mtd[ws].getItem(entry_no).getNumberHistograms(), mtd[ws].getItem(entry_no).blocksize()))
            dataY_incoherent = np.zeros(shape=(mtd[ws].getItem(entry_no).getNumberHistograms(), mtd[ws].getItem(entry_no).blocksize()))
            dataY_magnetic = np.zeros(shape=(mtd[ws].getItem(entry_no).getNumberHistograms(), mtd[ws].getItem(entry_no).blocksize()))
            for spectrum in range(mtd[ws].getItem(entry_no).getNumberHistograms()):
                sigma_z_sf = mtd[ws].getItem(entry_no).readY(spectrum)
                sigma_z_nsf = mtd[ws].getItem(entry_no+1).readY(spectrum)
                if nMeasurements == 2:
                    dataY_nuclear[spectrum] = 2.0 * sigma_z_nsf - sigma_z_sf  # Nuclear coherent
                    dataY_incoherent[spectrum] = 2.0 * sigma_z_sf - sigma_z_nsf # Incoherent
                    dataY_magnetic[spectrum] = 0 # Magnetic
                elif nMeasurements == 6 or nMeasurements == 10:
                    sigma_y_sf = mtd[ws].getItem(entry_no+2).readY(spectrum)
                    sigma_y_nsf = mtd[ws].getItem(entry_no+3).readY(spectrum)
                    sigma_x_sf = mtd[ws].getItem(entry_no+4).readY(spectrum)
                    sigma_x_nsf = mtd[ws].getItem(entry_no+5).readY(spectrum)
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
                        sigma_xmy_sf = mtd[ws].getItem(entry_no+6).readY(spectrum)
                        sigma_xmy_nsf = mtd[ws].getItem(entry_no+7).readY(spectrum)
                        sigma_xpy_sf = mtd[ws].getItem(entry_no+8).readY(spectrum)
                        sigma_xpy_nsf = mtd[ws].getItem(entry_no+9).readY(spectrum)
                        # Magnetic component
                        try:
                            theta_0 = self._sampleGeometry['theta_offset']
                        except KeyError:
                            raise RuntimeError("The value for theta_0 needs to be defined for the component separation in 10-p method.")
                        theta = mtd[ws].getItem(entry_no).detectorInfo().twoTheta(spectrum)
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
                dataX = mtd[ws].getItem(entry_no).readX(0)
                dataE = np.sqrt(abs(dataY[component]))
                tmp_name = str(mtd[ws].getItem(entry_no).name())[:-1] + componentNames[component]
                tmp_names.append(tmp_name)
                CreateWorkspace(DataX=dataX, DataY=dataY[component], dataE=dataE,
                                Nspec=mtd[ws].getItem(entry_no).getNumberHistograms(),
                                OutputWorkspace=tmp_name)
        output_name = self.getPropertyValue('ProcessAs') + '_component_separation'
        GroupWorkspaces(tmp_names, OutputWorkspace=output_name)
        return output_name

    def _conjoin_components(self, ws):
        """Conjoins the component workspaces coming from a theta scan."""
        components = [[], []]
        # list_incoherent = []
        componentNames = ['Incoherent', 'Coherent', 'Magnetic']
        if self._method in ['10-p', 'XYZ']:
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
        conjoined_components = self._conjoin_components(component_ws)
        calibrationType = self.getPropertyValue('DetectorEfficiencyCalibration')
        normaliseToAbsoluteUnits = self.getProperty('AbsoluteUnitsNormalisation')
        tmp_name = 'det_eff'
        tmp_names = []
        if calibrationType == 'Vanadium':
            vanadium_ws = self.getPropertyValue('VanadiumInputWorkspace')
            if normaliseToAbsoluteUnits:
                normFactor = self._sampleGeometry['formula_units']
                CreateSingleValuedWorkspace(DataValue=normFactor, OutputWorkspace='normalisation_ws')
            else:
                normalisationFactors = self._max_values_per_detector(vanadium_ws)
                dataE = np.sqrt(normalisationFactors)
                CreateWorkspace(dataX=mtd[vanadium_ws].getItem(0).readX(0), dataY=normalisationFactors, dataE=dataE,
                                NSpec=mtd[vanadium_ws].getItem(0).getNumberOfHistograms(), OutputWorkspace='normalisation_ws')
            Divide(LHSWorkspace=vanadium_ws,
                   RHSWorkspace='normalisation_ws',
                   OutputWorkspace='det_efficiency')
        elif calibrationType in  ['Paramagnetic', 'Incoherent']:
            if calibrationType == 'Paramagnetic':
                if self._mode == 'TOF':
                    raise RuntimeError('Paramagnetic calibration is not valid in the TOF mode.')
                if self._method == 'Uniaxial':
                    raise RuntimeError('Paramagnetic calibration is not valid in the Uniaxial measurement mode.')
                for entry_no, entry in enumerate(mtd[ws]):
                    ws_name = '{0}_{1}'.format(tmp_name, entry_no)
                    tmp_names.append(ws_name)
                    const = (2.0/3.0) * math.pow(physical_constants['neutron gyromag. ratio']
                                                 * physical_constants['classical electron radius'], 2)
                    paramagneticComponent = mtd[conjoined_components].getItem(2)
                    Divide(LHSWorkspace=const * entry * (entry+1),
                           RHSWorkspace=paramagneticComponent,
                           OutputWorkspace=ws_name)
            else: # Incoherent
                if self._mode == 'TOF':
                    raise RuntimeError('Incoherent calibration is not valid in the TOF mode.')
                for spectrum_no in range(mtd[conjoined_components].getItem(1).getNumberOfHistograms()):
                    if normaliseToAbsoluteUnits:
                        normFactor = float(self.getPropertyValue('IncoherentCrossSection'))
                        CreateSingleValuedWorkspace(DataValue=normFactor, OutputWorkspace='normalisation_ws')
                    else:
                        normalisationFactors = self._max_values_per_detector(mtd[conjoined_components].getItem(1).name())
                        dataE = np.sqrt(normalisationFactors)
                        CreateWorkspace(dataX=mtd[conjoined_components].getItem(1).readX(0), dataY=normalisationFactors,
                                        dataE=dataE,
                                        NSpec=mtd[conjoined_components].getItem(1).getNumberOfHistograms(),
                                        OutputWorkspace='normalisation_ws')
                    ws_name = '{0}_{1}'.format(tmp_name, entry_no)
                    tmp_names.append(ws_name)

                Divide(LHSWorkspace='normalisation_ws',
                       RHSWorkspace=component_ws,
                       OutputWorkspace=ws_name)

            GroupWorkspaces(tmp_names, OutputWorkspace='det_efficiency')

        for entry_no, entry in enumerate(mtd[ws]):
            Multiply(LHSWorkspace=entry,
                     RHSWorkspace=mtd['det_efficiency'].getItem(entry_no),
                     OutputWorkspace=entry)
        return ws

    def _sum_TOF_data(self, ws):
        """Integrates intensities over all time channels or time-of-flight bins."""
        tofUnits = self.getPropertyValue('TOFUnits')
        if tofUnits == 'UncalibratedTime':
            timeBinWidth = mtd[ws].getItem(0).getRun().getLogData('Detector.time_of_flight_0')
            Multiply(LHSWorkspace=ws, RHSWorkspace=timeBinWidth, OutputWorkspace=ws)
        if tofUnits == 'Energy':
            energyBinWidth = 0 # placeholder
            Multiply(LHSWorkspace=ws, RHSWorkspace=energyBinWidth, OutputWorkspace=ws)
        Integration(InputWorkspace=ws, OutputWorkspace=ws)
        return ws

    def _output_vanadium(self, ws, n_atoms):
        """Performs normalisation of the vanadium data to the expected cross-section."""
        if self._mode == 'TOF':
            ws = self._sum_TOF_data(ws)
        Divide(LHSWorkspace=CreateSingleValuedWorkspace(0.404*n_atoms), RHSWorkspace=ws, OutputWorkspace=ws)
        return ws

    def _finalize(self, ws, process):
        ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws, NaNValue=0,
                             NaNError=0, InfinityValue=0, InfinityError=0)
        mtd[ws].getItem(0).getRun().addProperty('ProcessedAs', process, True)
        if self.getProperty('SumScan').value and isinstance(mtd[ws], WorkspaceGroup) and mtd[ws].getNumberOfEntries() > 1:
            self._merge_polarisations(ws)
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        self.setProperty('OutputWorkspace', mtd[ws[2:]])

    def PyExec(self):
        process = self.getPropertyValue('ProcessAs')
        processes = ['Absorber', 'Beam', 'Transmission', 'Container', 'Quartz', 'Vanadium', 'Sample']
        progress = Progress(self, start=0.0, end=1.0, nreports=processes.index(process) + 1)
        ws = '__' + self.getPropertyValue('OutputWorkspace')

        calibration_setting = 'YIGFile'
        if self.getProperty('InstrumentParameterFile').isDefault:
            calibration_setting = 'None'

        Load(Filename=self.getPropertyValue('Run').replace('+',','), LoaderName='LoadILLPolarizedDiffraction',
             PositionCalibration=calibration_setting, YIGFileName=self.getPropertyValue('InstrumentParameterFile'),
             TOFUnits=self.getPropertyValue('TOFUnits'), OutputWorkspace=ws)

        self._instrument = mtd[ws].getItem(0).getInstrument().getName()
        run = mtd[ws].getItem(0).getRun()
        if run['acquisition_mode'].value == 1:
            self._mode = 'TOF'
        self._figure_measurement_method(ws)
        progress.report()
        if process in ['Transmission']:
            beam_ws = self.getPropertyValue('BeamInputWorkspace')
            cadmium_ws = self.getPropertyValue('CadmiumTransmissionInputWorkspace')
            self._calculate_transmission(ws, beam_ws, cadmium_ws)
            progress.report()
        elif process not in ['Beam']:
            self._normalise(ws)

        if process in ['Quartz', 'Vanadium', 'Sample']:
            if not self.getProperty('ContainerInputWorkspace').isDefault and not self.getProperty('TransmissionInputWorkspace').isDefault:
                # Subtracts background if the workspaces for container and transmission are provided
                container_ws = self.getPropertyValue('ContainerInputWorkspace')
                transmission_ws = self.getPropertyValue('TransmissionInputWorkspace')
                self._subtract_background(ws, container_ws, transmission_ws)
                progress.report()

            if process == 'Quartz':
                ws = self._calculate_polarizing_efficiencies(ws)
                progress.report()

            if process in ['Vanadium', 'Sample']:
                if self._mode == 'TOF' and process == 'Sample':
                    self._frame_overlap_correction(ws)
                    progress.report()
                pol_eff_ws = self.getPropertyValue('QuartzInputWorkspace')
                if pol_eff_ws:
                    self._apply_polarization_corrections(ws, pol_eff_ws)
                    progress.report()
                if self._mode == 'TOF':
                    self._detector_analyser_energy_efficiency(ws)
                    progress.report()
                self._read_sample_geometry()
                if self.getPropertyValue('SampleGeometry') != 'None' and self._mode != 'TOF':
                    self._apply_self_attenuation_correction(ws, container_ws)
                progress.report()
                component_ws = self._component_separation(ws)
                progress.report()
                if process == 'Vanadium':
                    self._output_vanadium(ws, self._sampleGeometry['n_atoms'])
                else:
                    self._detector_efficiency_correction(ws, component_ws)
                    progress.report()

        self._finalize(ws, process)


AlgorithmFactory.subscribe(PolDiffILLReduction)
