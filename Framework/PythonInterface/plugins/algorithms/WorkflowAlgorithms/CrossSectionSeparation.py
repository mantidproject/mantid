# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import PythonAlgorithm, WorkspaceGroupProperty, AlgorithmFactory
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator,\
    StringListValidator, PropertyCriterion

from mantid.simpleapi import *

import numpy as np
import math


class CrossSectionSeparation(PythonAlgorithm):

    def category(self):
        return 'ILL\\Diffraction'

    def summary(self):
        return 'Separates magnetic, nuclear coherent, and incoherent components for diffraction and spectroscopy data.'

    def name(self):
        return 'CrossSectionSeparation'

    def validateInputs(self):
        issues = dict()
        separationMethod = self.getPropertyValue('ComponentSeparationMethod')
        if separationMethod == '10p' and self.getProperty('ThetaOffset').isDefault:
            issues['ThetaOffset'] = "The value for theta_0 needs to be defined for the component separation in 10p method."
        return issues

    def PyInit(self):

        self.declareProperty(WorkspaceGroupProperty('InputWorkspace', '',
                                                    direction=Direction.Input),
                             doc='The input workspace with spin-flip and non-spin-flip data.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output workspace.')

        self.declareProperty(name="ComponentSeparationMethod",
                             defaultValue="Uniaxial",
                             validator=StringListValidator(["Uniaxial", "XYZ", "10p"]),
                             direction=Direction.Input,
                             doc="What type of component separation to perform.")

        self.declareProperty(name="ThetaOffset",
                             defaultValue=0.0,
                             validator=FloatBoundedValidator(lower=-180, upper=180),
                             direction=Direction.Input,
                             doc="Theta_0 offset used in 10p method.")
        self.setPropertySettings('ThetaOffset', EnabledWhenProperty("ComponentSeparationMethod",
                                                                    PropertyCriterion.IsEqualTo, '10p'))

    def PyExec(self):
        input_ws = self.getPropertyValue('InputWorkspace')
        output_name = self._component_separation(input_ws)
        self.setProperty('OutputWorkspace', mtd[output_name])

    def _data_structure_helper(self, ws):
        nComponents = 0
        user_method = self.getProperty('ComponentSeparationMethod')
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

    def _component_separation(self, ws):
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
        output_name = self.getPropertyValue('OutputWorkspace')
        GroupWorkspaces(tmp_names, OutputWorkspace=output_name)
        return output_name


AlgorithmFactory.subscribe(CrossSectionSeparation)
