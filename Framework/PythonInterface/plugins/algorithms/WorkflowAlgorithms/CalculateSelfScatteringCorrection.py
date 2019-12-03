# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import (CalculatePlaczekSelfScattering, ConvertToDistribution, ConvertUnits, CreateWorkspace,
                              DeleteWorkspace, DiffractionFocussing, Divide, ExtractSpectra, FitIncidentSpectrum,
                              LoadCalFile, SetSample)
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, WorkspaceProperty)
from mantid.kernel import Direction
import numpy as np


class CalculateSelfScatteringCorrection(DataProcessorAlgorithm):

    def category(self):
        return "Workflow\\Diffraction"

    def seeAlso(self):
        return [""]

    def summary(self):
        return "Calculates the self scattering correction factor for total scattering data."

    def checkGroups(self):
        return False

    def PyInit(self):
        self.declareProperty(WorkspaceProperty('RawWorkspace', '', direction=Direction.Input),
                             doc='Raw workspace')
        self.declareProperty(WorkspaceProperty('CorrectionWorkspace', '', direction=Direction.Output),
                             doc='Focused corrected workspace')
        self.declareProperty(name='CalFileName', defaultValue='',
                             doc='Chemical formula for the sample material')
        self.declareProperty(name='SampleGeometry', defaultValue={},
                             doc='Geometry of the sample material')
        self.declareProperty(name='SampleMaterial', defaultValue={},
                             doc='Chemical formula for the sample material')

    def PyExec(self):
        raw_ws = self.getProperty('RawWorkspace').value
        sample_geometry = self.getPropertyValue('SampleGeometry')
        sample_material = self.getPropertyValue('SampleMaterial')
        cal_file_name = self.getPropertyValue('CalFileName')
        SetSample(InputWorkspace=raw_ws,
                  Geometry=sample_geometry,
                  Material=sample_material)
        # find the closest monitor to the sample for incident spectrum
        raw_spec_info = raw_ws.spectrumInfo()
        incident_index = None
        for i in range(raw_spec_info.size()):
            if raw_spec_info.isMonitor(i):
                l2 = raw_spec_info.position(i)[2]
                if not incident_index:
                    incident_index = i
                else:
                    if raw_spec_info.position(incident_index)[2] < l2 < 0:
                        incident_index = i
        monitor = ExtractSpectra(InputWorkspace=raw_ws, WorkspaceIndexList=[incident_index])
        monitor = ConvertUnits(InputWorkspace=monitor, Target="Wavelength")
        x_data = monitor.dataX(0)
        min_x = np.min(x_data)
        max_x = np.max(x_data)
        width_x = (max_x - min_x) / x_data.size
        fit_spectra = FitIncidentSpectrum(InputWorkspace=monitor,
                                          BinningForCalc=[min_x, 1 * width_x, max_x],
                                          BinningForFit=[min_x, 10 * width_x, max_x],
                                          FitSpectrumWith="CubicSpline")
        self_scattering_correction = CalculatePlaczekSelfScattering(InputWorkspace=raw_ws,
                                                                    IncidentSpecta=fit_spectra)
        cal_workspace = LoadCalFile(InputWorkspace=self_scattering_correction,
                                    CalFileName=cal_file_name,
                                    Workspacename='cal_workspace',
                                    MakeOffsetsWorkspace=False,
                                    MakeMaskWorkspace=False)
        self_scattering_correction = DiffractionFocussing(InputWorkspace=self_scattering_correction,
                                                          GroupingFilename=cal_file_name)

        n_pixel = np.zeros(self_scattering_correction.getNumberHistograms())

        for i in range(cal_workspace.getNumberHistograms()):
            grouping = cal_workspace.dataY(i)
            if grouping[0] > 0:
                n_pixel[int(grouping[0] - 1)] += 1
        correction_ws = CreateWorkspace(DataY=n_pixel, DataX=[0, 1],
                                        NSpec=self_scattering_correction.getNumberHistograms())
        self_scattering_correction = Divide(LHSWorkspace=self_scattering_correction, RHSWorkspace=correction_ws)
        ConvertToDistribution(Workspace=self_scattering_correction)
        self_scattering_correction = ConvertUnits(InputWorkspace=self_scattering_correction,
                                                  Target="MomentumTransfer", EMode='Elastic')
        DeleteWorkspace('cal_workspace_group')
        DeleteWorkspace(correction_ws)
        DeleteWorkspace(fit_spectra)
        DeleteWorkspace(monitor)
        DeleteWorkspace(raw_ws)
        self.setProperty('CorrectionWorkspace', self_scattering_correction)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CalculateSelfScatteringCorrection)
