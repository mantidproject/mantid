# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import PythonAlgorithm, WorkspaceGroupProperty, AlgorithmFactory, \
    PropertyMode
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator, \
    LogicOperator, PropertyCriterion, StringListValidator

from mantid.simpleapi import *

import numpy as np
import math


class D7AbsoluteScaleNormalisation(PythonAlgorithm):

    def category(self):
        return 'ILL/Diffraction'

    def summary(self):
        return 'Separates magnetic, nuclear coherent, and incoherent components for diffraction and spectroscopy data,' \
               'and corrects the sample data for detector efficiency and normalises it to the chosen standard.'

    def name(self):
        return 'D7AbsoluteScaleNormalisation'

    def validateInputs(self):
        issues = dict()
        separationMethod = self.getPropertyValue('CrossSectionSeparationMethod')
        if separationMethod == '10p' and self.getProperty('ThetaOffset').isDefault:
            issues['ThetaOffset'] = "The value for theta_0 needs to be defined for the component separation in 10p method."

        if self.getPropertyValue('NormalisationMethod') == 'Vanadium':
            if self.getProperty('VanadiumInputWorkspace').isDefault:
                issues['VanadiumInputWorkspace'] = 'Vanadium input workspace is mandatory for when detector efficiency calibration' \
                                                    ' is "Vanadium".'

            if self.getProperty('AbsoluteUnitsNormalisation').value and self.getProperty('FormulaUnits').isDefault:
                issues['FormulaUnits'] = 'Formula units need to be defined for absolute unit normalisation.'

        if ( (self.getPropertyValue('NormalisationMethod') == 'Incoherent'
             or self.getPropertyValue('NormalisationMethod') == 'Paramagnetic')
             and self.getProperty('CrossSectionSeparationMethod').isDefault):
            issues['NormalisationMethod'] = 'Chosen sample normalisation requires input from the cross-section separation.'
            issues['CrossSectionSeparationMethod'] = 'Chosen sample normalisation requires input from the cross-section separation.'

        if self.getProperty('NormalisationMethod') == 'Incoherent':
            if self.getProperty('AbsoluteUnitsNormalisation').value and self.getProperty('IncoherentCrossSection').isDefault:
                issues['IncoherentCrossSection'] = 'Spin-incoherent cross-section is required for the absolute scale normalisation.'

            if self.getProperty('SampleSpin').isDefault:
                issues['SampleSpin'] = 'Sample spin is required for the incoherent-spin sample normalisation.'

        return issues

    def PyInit(self):

        self.declareProperty(WorkspaceGroupProperty('InputWorkspace', '',
                                                    direction=Direction.Input),
                             doc='The input workspace with spin-flip and non-spin-flip data.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output workspace.')

        self.declareProperty(name="CrossSectionSeparationMethod",
                             defaultValue="Uniaxial",
                             validator=StringListValidator(["None", "Uniaxial", "XYZ", "10p"]),
                             direction=Direction.Input,
                             doc="What type of cross-section separation to perform.")

        self.declareProperty(name="ThetaOffset",
                             defaultValue=0.0,
                             validator=FloatBoundedValidator(lower=-180, upper=180),
                             direction=Direction.Input,
                             doc="Theta_0 offset used in 10p method.")
        self.setPropertySettings('ThetaOffset', EnabledWhenProperty("CrossSectionSeparationMethod",
                                                                    PropertyCriterion.IsEqualTo, '10p'))

        self.declareProperty(name="NormalisationMethod",
                             defaultValue="None",
                             validator=StringListValidator(["None", "Vanadium", "Incoherent",  "Paramagnetic"]),
                             direction=Direction.Input,
                             doc="Method to correct detector efficiency and normalise data.")

        self.declareProperty(WorkspaceGroupProperty('VanadiumInputWorkspace', '',
                                                    direction=Direction.Input,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the vanadium workspace.')

        self.setPropertySettings('VanadiumInputWorkspace', EnabledWhenProperty('NormalisationMethod',
                                                                               PropertyCriterion.IsEqualTo, 'Vanadium'))

        self.declareProperty(name="FormulaUnits",
                             defaultValue=0.0,
                             validator=FloatBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="Number of formula units of the sample.")

        self.setPropertySettings('FormulaUnits', EnabledWhenProperty('NormalisationMethod',
                                                                     PropertyCriterion.IsEqualTo, 'Vanadium'))

        incoherent = EnabledWhenProperty('NormalisationMethod', PropertyCriterion.IsEqualTo, 'Incoherent')

        self.declareProperty('AbsoluteUnitsNormalisation', True,
                             doc='Whether or not express the output in absolute units.')

        self.declareProperty(name="IncoherentCrossSection",
                             defaultValue=0.0,
                             validator=FloatBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="Cross-section for incoherent scattering, necessary for setting the output on the absolute scale.")

        incoherent = EnabledWhenProperty('NormalisationMethod', PropertyCriterion.IsEqualTo, 'Incoherent')

        absoluteNormalisation = EnabledWhenProperty('AbsoluteUnitsNormalisation', PropertyCriterion.IsDefault)

        self.setPropertySettings('IncoherentCrossSection', EnabledWhenProperty(incoherent, absoluteNormalisation,
                                                                               LogicOperator.And))

    def _data_structure_helper(self, ws):
        nComponents = 0
        user_method = self.getPropertyValue('CrossSectionSeparationMethod')
        measurements = set()
        for name in mtd[ws].getNames():
            last_underscore = name.rfind("_")
            measurements.add(name[last_underscore:])
        nMeasurements = len(measurements)
        error_msg = "The provided data cannot support {} measurement component separation."
        if nMeasurements == 10:
            nComponents = 3
        elif nMeasurements == 6:
            nComponents = 3
            if user_method == '10p':
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

    def _cross_section_separation(self, ws):
        """Separates coherent, incoherent, and magnetic components based on spin-flip and non-spin-flip intensities of the
        current sample. The method used is based on either the user's choice or the provided data structure."""
        DEG_2_RAD =  np.pi / 180.0
        nMeasurements, nComponents = self._data_structure_helper(ws)
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
                        theta_0 = DEG_2_RAD * self.getProperty('ThetaOffset').value
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
                                OutputWorkspace=tmp_name)
        output_name = ws + '_separated_cs'
        # output_name = self.getPropertyValue('OutputWorkspace')
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

    def _detector_efficiency_correction(self, ws):
        """Calculates detector efficiency using either vanadium, incoherent scattering,
        or paramagnetic scattering."""

        nMeasurements, _ = self._data_structure_helper(ws)
        calibrationType = self.getPropertyValue('NormalisationMethod')
        normaliseToAbsoluteUnits = self.getProperty('AbsoluteUnitsNormalisation').value
        det_efficiency_ws = ws + '_det_efficiency'
        tmp_name = 'det_eff'
        tmp_names = []
        if calibrationType == 'Vanadium':
            vanadium_ws = ws
            if normaliseToAbsoluteUnits:
                normFactor = self.getProperty('FormulaUnits').value
                CreateSingleValuedWorkspace(DataValue=normFactor, OutputWorkspace='normalisation_ws')
            else:
                normalisationFactors = self._max_value_per_detector(vanadium_ws)
                dataE = np.sqrt(normalisationFactors)
                entry0 = mtd[vanadium_ws][0]
                CreateWorkspace(dataX=entry0.readX(0), dataY=normalisationFactors, dataE=dataE,
                                NSpec=entry0.getNumberHistograms(), OutputWorkspace='normalisation_ws')
            Divide(LHSWorkspace=vanadium_ws,
                   RHSWorkspace='normalisation_ws',
                   OutputWorkspace=det_efficiency_ws)
        elif calibrationType in  ['Paramagnetic', 'Incoherent']:
            if calibrationType == 'Paramagnetic':
                spin = self.getProperty('SampleSpin').value
                for entry_no, entry in enumerate(mtd[ws]):
                    ws_name = '{0}_{1}'.format(tmp_name, entry_no)
                    tmp_names.append(ws_name)
                    const = (2.0/3.0) * math.pow(physical_constants['neutron gyromag. ratio']
                                                 * physical_constants['classical electron radius'], 2)
                    paramagneticComponent = mtd[conjoined_components][2]
                    Divide(LHSWorkspace=const * spin * (spin+1),
                           RHSWorkspace=paramagneticComponent,
                           OutputWorkspace=ws_name)
            else: # Incoherent
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

            GroupWorkspaces(tmp_names, OutputWorkspace=det_efficiency_ws)

        return det_efficiency_ws

    def _normalise_sample_data(self, ws, det_efficiency_ws):
        """Normalises the sample data using the detector efficiency calibration workspace"""

        single_efficiency_per_POL = False
        if mtd[ws].getNumberOfEntries() != mtd[det_efficiency_ws].getNumberOfEntries():
            single_efficiency_per_POL = True
        for entry_no, entry in enumerate(mtd[ws]):
            det_eff_entry_no = entry_no
            if single_efficiency_per_POL:
                det_eff_entry_no = det_eff_entry_no % nMeasurements
            Multiply(LHSWorkspace=entry,
                     RHSWorkspace=mtd[det_efficiency_ws][det_eff_entry_no],
                     OutputWorkspace=entry)
        return ws

    def PyExec(self):
        input_ws = self.getPropertyValue('InputWorkspace')
        if self.getPropertyValue('CrossSectionSeparationMethod') != 'None':
            component_ws = self._cross_section_separation(input_ws)
        normalisation_method = self.getPropertyValue('NormalisationMethod')
        if normalisation_method == 'None':
            if self.getPropertyValue('CrossSectionSeparationMethod') != 'None':
                output_ws = component_ws
            else:
                output_ws = input_ws
        else:
            if normalisation_method != 'Vanadium':
                det_efficiency_input = self._conjoin_components(component_ws)
            else:
                det_efficiency_input = self.getPropertyValue('VanadiumInputWorkspace')
            det_efficiency_ws = self._detector_efficiency_correction(det_efficiency_input)
            output_ws = self._normalise_sample_data(input_ws, det_efficiency_ws)

        self.setProperty('OutputWorkspace', mtd[output_ws])


AlgorithmFactory.subscribe(D7AbsoluteScaleNormalisation)
