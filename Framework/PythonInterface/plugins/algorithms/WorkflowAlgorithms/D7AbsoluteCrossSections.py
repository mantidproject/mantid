# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, PropertyMode, PythonAlgorithm, \
    WorkspaceGroupProperty, WorkspaceGroup
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator, \
    PropertyCriterion, PropertyManagerProperty, StringListValidator

from mantid.simpleapi import *

from scipy.constants import physical_constants
import numpy as np
import math


class D7AbsoluteCrossSections(PythonAlgorithm):

    _sampleAndEnvironmentProperties = None

    @staticmethod
    def _max_value_per_detector(ws):
        if isinstance(mtd[ws], WorkspaceGroup):
            max_values = np.zeros(shape=(mtd[ws][0].getNumberHistograms(),
                                         mtd[ws].getNumberOfEntries()))
            for entry_no, entry in enumerate(mtd[ws]):
                max_values[:, entry_no] = entry.extractY().T
        else:
            max_values = mtd[ws].extractY().T
        return np.amax(max_values, axis=1)

    @staticmethod
    def _set_as_distribution(ws):
        for entry in mtd[ws]:
            entry.setDistribution(True)
        return ws

    def category(self):
        return 'ILL\\Diffraction'

    def summary(self):
        return 'Separates magnetic, nuclear coherent, and incoherent components for diffraction and spectroscopy data,' \
               'and corrects the sample data for detector efficiency and normalises it to the chosen standard.'

    def seeAlso(self):
        return ['D7YIGPositionCalibration', 'PolDiffILLReduction']

    def name(self):
        return 'D7AbsoluteCrossSections'

    def validateInputs(self):
        issues = dict()

        normalisation_method = self.getPropertyValue('NormalisationMethod')

        if normalisation_method == 'Vanadium' and self.getProperty('VanadiumInputWorkspace').isDefault:
            issues['VanadiumInputWorkspace'] = 'Vanadium input workspace is mandatory for when detector efficiency calibration' \
                                                    ' is "Vanadium".'

        if ( (normalisation_method == 'Incoherent' or normalisation_method == 'Paramagnetic')
             and self.getProperty('CrossSectionSeparationMethod').isDefault):
            issues['NormalisationMethod'] = 'Chosen sample normalisation requires input from the cross-section separation.'
            issues['CrossSectionSeparationMethod'] = 'Chosen sample normalisation requires input from the cross-section separation.'

        if normalisation_method != 'None' or self.getPropertyValue('CrossSectionSeparationMethod') == '10p':
            sampleAndEnvironmentProperties = self.getProperty('SampleAndEnvironmentProperties').value
            if len(sampleAndEnvironmentProperties) == 0:
                issues['SampleAndEnvironmentProperties'] = 'Sample parameters need to be defined.'
            else:
                required_keys = []
                if normalisation_method == 'Incoherent':
                    required_keys = ['FormulaUnits', 'SampleMass', 'FormulaUnitMass']
                elif normalisation_method == 'Incoherent' and self.getProperty('AbsoluteUnitsNormalisation').value:
                    required_keys.append('IncoherentCrossSection')
                elif normalisation_method == 'Paramagnetic':
                    required_keys.append('SampleSpin')

                if self.getPropertyValue('CrossSectionSeparationMethod') == '10p':
                    required_keys.append('ThetaOffset')

                for key in required_keys:
                    if key not in sampleAndEnvironmentProperties:
                        issues['SampleAndEnvironmentProperties'] = '{} needs to be defined.'.format(key)

        return issues

    def PyInit(self):

        self.declareProperty(WorkspaceGroupProperty('InputWorkspace', '',
                                                    direction=Direction.Input),
                             doc='The input workspace with spin-flip and non-spin-flip data.')

        self.declareProperty(WorkspaceGroupProperty('RotatedXYZWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The workspace used in 10p method when data is taken as two XYZ'
                                 +' measurements rotated by 45 degress.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc='The output workspace.')

        self.declareProperty(WorkspaceGroupProperty('CrossSectionsOutputWorkspace', '',
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc='The output workspace with separated cross-sections.')

        self.declareProperty(name="CrossSectionSeparationMethod",
                             defaultValue="None",
                             validator=StringListValidator(["None", "Uniaxial", "XYZ", "10p"]),
                             direction=Direction.Input,
                             doc="What type of cross-section separation to perform.")

        self.declareProperty(name="OutputUnits",
                             defaultValue="TwoTheta",
                             validator=StringListValidator(["TwoTheta", "Q"]),
                             direction=Direction.Input,
                             doc="The choice to display the output either as a function of detector twoTheta,"
                                 +" or the momentum exchange.")

        self.declareProperty(name="NormalisationMethod",
                             defaultValue="None",
                             validator=StringListValidator(["None", "Vanadium", "Incoherent",  "Paramagnetic"]),
                             direction=Direction.Input,
                             doc="Method to correct detector efficiency and normalise data.")

        self.declareProperty(name="OutputTreatment",
                             defaultValue="Individual",
                             validator=StringListValidator(["Individual", "Sum"]),
                             direction=Direction.Input,
                             doc="Which treatment of the provided scan should be used to create output.")

        self.declareProperty(PropertyManagerProperty('SampleAndEnvironmentProperties', dict()),
                             doc="Dictionary for the information about sample and its environment.")

        self.declareProperty(name="ScatteringAngleBinSize",
                             defaultValue=0.5,
                             validator=FloatBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="Scattering angle bin size in degrees used for expressing scan data on a single TwoTheta axis.")

        self.setPropertySettings("ScatteringAngleBinSize", EnabledWhenProperty('OutputTreatment', PropertyCriterion.IsEqualTo, 'Sum'))

        self.declareProperty(WorkspaceGroupProperty('VanadiumInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the vanadium workspace.')

        self.setPropertySettings('VanadiumInputWorkspace', EnabledWhenProperty('NormalisationMethod',
                                                                               PropertyCriterion.IsEqualTo, 'Vanadium'))

        self.declareProperty('AbsoluteUnitsNormalisation', True,
                             doc='Whether or not express the output in absolute units.')

    def _data_structure_helper(self, ws):
        user_method = self.getPropertyValue('CrossSectionSeparationMethod')
        measurements = set()
        for name in mtd[ws].getNames():
            last_underscore = name.rfind("_")
            measurements.add(name[last_underscore+1:])
        nMeasurements = len(measurements)
        error_msg = "The provided data cannot support {} measurement component separation."
        if nMeasurements == 10:
            nComponents = 3
        elif nMeasurements == 6:
            nComponents = 3
            if user_method == '10p' and self.getProperty('RotatedXYZWorkspace').isDefault:
                raise RunTimeError(error_msg.format(user_method))
        elif nMeasurements == 2:
            nComponents = 2
            if user_method == '10p':
                raise RunTimeError(error_msg.format(user_method))
            if user_method == 'XYZ':
                raise RunTimeError(error_msg.format(user_method))
        else:
            raise RuntimeError("The analysis options are: Uniaxial, XYZ, and 10p. "
                               + "The provided input does not fit in any of these measurement types.")
        return nMeasurements, nComponents

    def _read_experiment_properties(self, ws):
        """Reads the user-provided dictionary that contains sample geometry (type, dimensions) and experimental conditions,
         such as the beam size and calculates derived parameters."""
        self._sampleAndEnvironmentProperties = self.getProperty('SampleAndEnvironmentProperties').value
        if 'InitialEnergy' not in self._sampleAndEnvironmentProperties:
            h = physical_constants['Planck constant'][0]  # in m^2 kg^2 / s^2
            neutron_mass = physical_constants['neutron mass'][0]  # in0 kg
            wavelength = mtd[ws][0].getRun().getLogData('monochromator.wavelength').value * 1e-10  # in m
            joules_to_mev = 1e3 / physical_constants['electron volt'][0]
            self._sampleAndEnvironmentProperties['InitialEnergy'] = joules_to_mev * math.pow(h / wavelength, 2) / (2 * neutron_mass)

        if self.getPropertyValue('NormalisationMethod') != 'None' and 'NMoles' not in self._sampleAndEnvironmentProperties:
            sample_mass = self._sampleAndEnvironmentProperties['SampleMass'].value
            formula_units = self._sampleAndEnvironmentProperties['FormulaUnits'].value
            formula_unit_mass = self._sampleAndEnvironmentProperties['FormulaUnitMass'].value
            self._sampleAndEnvironmentProperties['NMoles'] = (sample_mass / formula_unit_mass) * formula_units

    def _cross_section_separation(self, ws):
        """Separates coherent, incoherent, and magnetic components based on spin-flip and non-spin-flip intensities of the
        current sample. The method used is based on either the user's choice or the provided data structure."""
        DEG_2_RAD =  np.pi / 180.0
        nMeasurements, nComponents = self._data_structure_helper(ws)
        user_method = self.getPropertyValue('CrossSectionSeparationMethod')
        componentNames = ['Coherent', 'Incoherent', 'Magnetic']
        number_histograms = mtd[ws][0].getNumberHistograms()
        block_size = mtd[ws][0].blocksize()
        double_xyz_method = False
        if not self.getProperty('RotatedXYZWorkspace').isDefault:
            double_xyz_method = True
            second_xyz_ws = self.getPropertyValue('RotatedXYZWorkspace')
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
                    if nMeasurements == 6 and user_method == 'XYZ':
                        # Magnetic component
                        magnetic_component = 2.0 * (2.0 * sigma_z_nsf - sigma_x_nsf - sigma_y_nsf )
                        dataY_magnetic[spectrum] = magnetic_component
                        # Nuclear coherent component
                        dataY_nuclear[spectrum] = (2.0*(sigma_x_nsf + sigma_y_nsf + sigma_z_nsf)
                                                   - (sigma_x_sf + sigma_y_sf + sigma_z_sf)) / 6.0
                        # Incoherent component
                        dataY_incoherent[spectrum] = 0.5 * (sigma_x_sf + sigma_y_sf + sigma_z_sf) - magnetic_component
                    else:
                        if not double_xyz_method:
                            sigma_xmy_sf = mtd[ws][entry_no+6].readY(spectrum)
                            sigma_xmy_nsf = mtd[ws][entry_no+7].readY(spectrum)
                            sigma_xpy_sf = mtd[ws][entry_no+8].readY(spectrum)
                            sigma_xpy_nsf = mtd[ws][entry_no+9].readY(spectrum)
                        else:
                            # assumed is averaging of twice measured Z-axis:
                            sigma_z_sf = 0.5 * (sigma_z_sf + mtd[second_xyz_ws][entry_no].readY(spectrum))
                            sigma_z_nsf = 0.5 * (sigma_z_nsf + mtd[second_xyz_ws][entry_no+1].readY(spectrum))
                            sigma_xmy_sf = mtd[second_xyz_ws][entry_no+2].readY(spectrum)
                            sigma_xmy_nsf = mtd[second_xyz_ws][entry_no+3].readY(spectrum)
                            sigma_xpy_sf = mtd[second_xyz_ws][entry_no+4].readY(spectrum)
                            sigma_xpy_nsf = mtd[second_xyz_ws][entry_no+5].readY(spectrum)
                        # Magnetic component
                        theta_0 = DEG_2_RAD * self._sampleAndEnvironmentProperties['ThetaOffset'].value
                        theta = mtd[ws][entry_no].detectorInfo().twoTheta(spectrum)
                        alpha = theta - 0.5*np.pi - theta_0
                        c0 = math.pow(math.cos(alpha), 2)
                        c4 = math.pow(math.cos(alpha - np.pi/4.0), 2)
                        magnetic_cos2alpha = (2*c0-4)*sigma_x_nsf + (2*c0+2)*sigma_y_nsf + (2-4*c0)*sigma_z_nsf
                        magnetic_sin2alpha = (2*c4-4)*sigma_xpy_nsf + (2*c4+2)*sigma_xmy_nsf + (2-4*c4)*sigma_z_nsf
                        magnetic_component = magnetic_cos2alpha * math.cos(2*alpha) + magnetic_sin2alpha * math.sin(2*alpha)
                        dataY_magnetic[spectrum] = magnetic_component
                        # Nuclear coherent component
                        dataY_nuclear[spectrum] = (2.0 * (sigma_x_nsf + sigma_y_nsf + 2*sigma_z_nsf + sigma_xpy_nsf + sigma_xmy_nsf)
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
                                ParentWorkspace=mtd[ws][entry_no],
                                OutputWorkspace=tmp_name)
        output_name = ws + '_separated_cs'
        GroupWorkspaces(tmp_names, OutputWorkspace=output_name)
        return output_name

    def _detector_efficiency_correction(self, cross_section_ws):
        """Calculates detector efficiency using either vanadium data, incoherent,
        or paramagnetic scattering cross-sections."""

        calibrationType = self.getPropertyValue('NormalisationMethod')
        normaliseToAbsoluteUnits = self.getProperty('AbsoluteUnitsNormalisation').value
        det_efficiency_ws = cross_section_ws + '_det_efficiency'
        norm_ws = 'normalisation_ws'
        tmp_name = 'det_eff'
        tmp_names = []
        to_clean = []
        if calibrationType == 'Vanadium':
            if normaliseToAbsoluteUnits:
                normFactor = self._sampleAndEnvironmentProperties['NMoles'].value
                CreateSingleValuedWorkspace(DataValue=normFactor, OutputWorkspace=norm_ws)
            else:
                normalisationFactors = self._max_value_per_detector(cross_section_ws)
                dataE = np.sqrt(normalisationFactors)
                entry0 = mtd[cross_section_ws][0]
                CreateWorkspace(dataX=entry0.readX(0), dataY=normalisationFactors, dataE=dataE,
                                NSpec=entry0.getNumberHistograms(), OutputWorkspace=norm_ws)
                mtd[norm_ws].getAxis(0).setUnit(entry0.getAxis(0).getUnit().unitID())
                mtd[norm_ws].getAxis(1).setUnit(entry0.getAxis(1).getUnit().unitID())
            to_clean.append(norm_ws)
            Divide(LHSWorkspace=cross_section_ws,
                   RHSWorkspace=norm_ws,
                   OutputWorkspace=det_efficiency_ws)
        elif calibrationType in  ['Paramagnetic', 'Incoherent']:
            if calibrationType == 'Paramagnetic':
                spin = self._sampleAndEnvironmentProperties['SampleSpin'].value
                for entry_no, entry in enumerate(mtd[cross_section_ws]):
                    ws_name = '{0}_{1}'.format(tmp_name, entry_no)
                    tmp_names.append(ws_name)
                    const = (2.0/3.0) * math.pow(physical_constants['neutron gyromag. ratio'][0]
                                                 * physical_constants['classical electron radius'][0], 2)
                    paramagneticComponent = mtd[cross_section_ws][2]
                    normalisation_name = 'normalisation_{}'.format(ws_name)
                    to_clean.append(normalisation_name)
                    CreateSingleValuedWorkspace(DataValue=const * spin * (spin+1), OutputWorkspace=normalisation_name)
                    Divide(LHSWorkspace=paramagneticComponent,
                           RHSWorkspace=normalisation_name,
                           OutputWorkspace=ws_name)
            else: # Incoherent
                for spectrum_no in range(mtd[cross_section_ws][1].getNumberHistograms()):
                    if normaliseToAbsoluteUnits:
                        normFactor = self._sampleAndEnvironmentProperties['IncoherentCrossSection'].value
                        CreateSingleValuedWorkspace(DataValue=normFactor, OutputWorkspace=norm_ws)
                    else:
                        normalisationFactors = self._max_value_per_detector(mtd[cross_section_ws].name())
                        dataE = np.sqrt(normalisationFactors)
                        if len(normalisationFactors) == 1:
                            CreateSingleValuedWorkspace(DataValue=normalisationFactors[0],
                                                        ErrorValue=dataE[0],
                                                        OutputWorkspace='normalisation_ws')
                        else:
                            CreateWorkspace(dataX=mtd[cross_section_ws][1].readX(0), dataY=normalisationFactors,
                                            dataE=dataE,
                                            NSpec=mtd[cross_section_ws][1].getNumberHistograms(),
                                            OutputWorkspace=norm_ws)
                    ws_name = '{0}_{1}'.format(tmp_name, spectrum_no)
                    tmp_names.append(ws_name)

                    Divide(LHSWorkspace=cross_section_ws,
                           RHSWorkspace=norm_ws,
                           OutputWorkspace=ws_name)

            GroupWorkspaces(tmp_names, OutputWorkspace=det_efficiency_ws)

        if len(to_clean) != 0:
            DeleteWorkspaces(to_clean)

        return det_efficiency_ws

    def _normalise_sample_data(self, ws, det_efficiency_ws):
        """Normalises the sample data using the detector efficiency calibration workspace"""
        nMeasurements, _ = self._data_structure_helper(ws)
        single_efficiency_per_POL = False
        if mtd[ws].getNumberOfEntries() != mtd[det_efficiency_ws].getNumberOfEntries():
            single_efficiency_per_POL = True
        tmp_names = []
        normalisation_method = self.getPropertyValue('NormalisationMethod')
        for entry_no, entry in enumerate(mtd[ws]):
            det_eff_entry_no = int(entry_no / 2)
            if normalisation_method == 'Vanadium':
                det_eff_entry_no = 0
            elif single_efficiency_per_POL:
                det_eff_entry_no = int(entry_no / 2)
                if entry_no % 2 != 0:
                    det_eff_entry_no -= 1
            ws_name = entry.name() + '_normalised'
            tmp_names.append(ws_name)

            Divide(LHSWorkspace=entry,
                   RHSWorkspace=mtd[det_efficiency_ws][det_eff_entry_no],
                   OutputWorkspace=ws_name)
        output_name = self.getPropertyValue('OutputWorkspace')
        GroupWorkspaces(InputWorkspaces=tmp_names, Outputworkspace=output_name)
        return output_name

    def _set_units(self, ws):
        output_unit = self.getPropertyValue('OutputUnits')
        unit_symbol = 'barn / sr / formula unit'
        unit = 'd sigma / d Omega ({0})'
        if output_unit == 'TwoTheta':
            unit = unit.format('TwoTheta')
            if mtd[ws].getNumberOfEntries() > 1 and self.getPropertyValue('OutputTreatment') == 'Sum':
                self._call_sum_data(ws)
                ConvertAxisByFormula(InputWorkspace=ws, OutputWorkspace=ws, Axis='X', Formula='-x')
            elif self.getProperty('OutputTreatment').value == 'Individual':
                ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=ws, Target='SignedTheta', OrderAxis=False)
                ConvertAxisByFormula(InputWorkspace=ws, OutputWorkspace=ws, Axis='Y', Formula='-y')
                Transpose(InputWorkspace=ws, OutputWorkspace=ws)
        elif output_unit == 'Q':
            unit = unit.format('Q')
            if self.getPropertyValue('OutputTreatment') == 'Sum':
                self._merge_polarisations(ws)
                ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target='ElasticQ',
                             EFixed=self._sampleAndEnvironmentProperties['InitialEnergy'].value)
            else:
                ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=ws, Target='ElasticQ',
                                    EFixed=self._sampleAndEnvironmentProperties['InitialEnergy'].value,
                                    OrderAxis=False)
                Transpose(InputWorkspace=ws, OutputWorkspace=ws)

        for entry in mtd[ws]:
            entry.setYUnitLabel("{} ({})".format(unit, unit_symbol))
        return ws

    def _call_sum_data(self, ws):
        SumOverlappingTubes(InputWorkspaces=ws, OutputWorkspace=ws,
                            OutputType='1D', ScatteringAngleBinning=self.getProperty('ScatteringAngleBinSize').value,
                            Normalise=True, HeightAxis='-0.1,0.1')
        return ws

    def _call_cross_section_separation(self, input_ws):
        component_ws = self._cross_section_separation(input_ws)
        self._set_as_distribution(component_ws)
        if not self.getProperty('CrossSectionsOutputWorkspace').isDefault:
            self.setProperty('CrossSectionsOutputWorkspace', mtd[component_ws])
        return component_ws

    def PyExec(self):
        input_ws = self.getPropertyValue('InputWorkspace')
        self._read_experiment_properties(input_ws)
        normalisation_method = self.getPropertyValue('NormalisationMethod')
        if normalisation_method != 'None':
            if normalisation_method =='Vanadium':
                det_efficiency_input = self.getPropertyValue('VanadiumInputWorkspace')
            elif normalisation_method in ['Paramagnetic', 'Incoherent']:
                det_efficiency_input = self._call_cross_section_separation(input_ws)

            det_efficiency_ws = self._detector_efficiency_correction(det_efficiency_input)
            output_ws = self._normalise_sample_data(input_ws, det_efficiency_ws)
            self._set_units(output_ws)
            self._set_as_distribution(output_ws)
            # clean-up
            DeleteWorkspace(det_efficiency_ws)
            if self.getProperty('CrossSectionsOutputWorkspace').isDefault:
                DeleteWorkspace(det_efficiency_input)
            else:
                self._set_units(det_efficiency_input)
            self.setProperty('OutputWorkspace', mtd[output_ws])

        if ( self.getPropertyValue('CrossSectionSeparationMethod') != 'None'
             and normalisation_method not in ['Paramagnetic', 'Incoherent'] ):
            if mtd[input_ws].getNumberOfEntries() != 1 and self.getProperty('OutputTreatment') == 'Sum':
                self._call_sum_data(input_ws)
            component_ws = self._call_cross_section_separation(input_ws)
            self._set_units(component_ws)


AlgorithmFactory.subscribe(D7AbsoluteCrossSections)
