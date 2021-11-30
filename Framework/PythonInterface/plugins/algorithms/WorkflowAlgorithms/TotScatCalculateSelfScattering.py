# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import (CalculatePlaczekSelfScattering, ConvertUnits, CreateWorkspace,
                              DeleteWorkspace, Divide, ExtractSpectra, FitIncidentSpectrum,
                              LoadCalFile, SetSample, GroupDetectors, Rebin)
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, FileAction, FileProperty, WorkspaceProperty)
from mantid.kernel import Direction
import numpy as np


class TotScatCalculateSelfScattering(DataProcessorAlgorithm):

    def name(self):
        return 'TotScatCalculateSelfScattering  CalculateSelfScatteringCorrection'

    def category(self):
        return "Workflow\\Diffraction"

    def seeAlso(self):
        return []

    def summary(self):
        return "Calculates the self scattering correction factor for total scattering data."

    def checkGroups(self):
        return False

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(WorkspaceProperty('InputWorkspace', '', direction=Direction.Input),
                             doc='Raw workspace.')
        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='Focused corrected workspace.')
        self.declareProperty(FileProperty("CalFileName", "",
                                          direction=Direction.Input,
                                          action=FileAction.Load),
                             doc='File path for the instrument calibration file.')
        self.declareProperty(name='SampleGeometry', defaultValue={},
                             doc='Geometry of the sample material.')
        self.declareProperty(name='SampleMaterial', defaultValue={},
                             doc='Chemical formula for the sample material.')
        self.declareProperty(name='CrystalDensity', defaultValue=0.0,
                             doc='The crystalographic density of the material.')

    def PyExec(self):
        raw_ws = self.getProperty('InputWorkspace').value
        sample_geometry = self.getPropertyValue('SampleGeometry')
        sample_material = self.getPropertyValue('SampleMaterial')
        cal_file_name = self.getPropertyValue('CalFileName')
        crystal_density = self.getPropertyValue('CrystalDensity')
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
                                                                    IncidentSpectra=fit_spectra,
                                                                    CrystalDensity=crystal_density,
                                                                    Version=1)
        # Convert to Q
        self_scattering_correction = ConvertUnits(InputWorkspace=self_scattering_correction,
                                                  Target="MomentumTransfer", EMode='Elastic')
        cal_workspace = LoadCalFile(InputWorkspace=self_scattering_correction,
                                    CalFileName=cal_file_name,
                                    Workspacename='cal_workspace',
                                    MakeOffsetsWorkspace=False,
                                    MakeMaskWorkspace=False,
                                    MakeGroupingWorkspace=True)
        ssc_min_x, ssc_max_x = float('inf'), float('-inf')
        for index in range(self_scattering_correction.getNumberHistograms()):
            spec_info = self_scattering_correction.spectrumInfo()
            if not spec_info.isMasked(index) and not spec_info.isMonitor(index):
                ssc_x_data = np.ma.masked_invalid(self_scattering_correction.dataX(index))
                if np.min(ssc_x_data) < ssc_min_x:
                    ssc_min_x = np.min(ssc_x_data)
                if np.max(ssc_x_data) > ssc_max_x:
                    ssc_max_x = np.max(ssc_x_data)
        ssc_width_x = (ssc_max_x - ssc_min_x) / ssc_x_data.size
        # TO DO: calculate rebin parameters per group
        # and run GroupDetectors on each separately
        self_scattering_correction = Rebin(InputWorkspace=self_scattering_correction,
                                           Params=[ssc_min_x, ssc_width_x, ssc_max_x],
                                           IgnoreBinErrors=True)
        self_scattering_correction = GroupDetectors(InputWorkspace=self_scattering_correction,
                                                    CopyGroupingFromWorkspace='cal_workspace_group')
        n_pixel = np.zeros(self_scattering_correction.getNumberHistograms())
        for i in range(cal_workspace.getNumberHistograms()):
            grouping = cal_workspace.dataY(i)
            if grouping[0] > 0:
                n_pixel[int(grouping[0] - 1)] += 1
        correction_ws = CreateWorkspace(DataY=n_pixel, DataX=[0, 1],
                                        NSpec=self_scattering_correction.getNumberHistograms())
        self_scattering_correction = Divide(LHSWorkspace=self_scattering_correction, RHSWorkspace=correction_ws)
        DeleteWorkspace('cal_workspace_group')
        DeleteWorkspace(correction_ws)
        DeleteWorkspace(fit_spectra)
        DeleteWorkspace(monitor)
        DeleteWorkspace(raw_ws)
        self.setProperty('OutputWorkspace', self_scattering_correction)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(TotScatCalculateSelfScattering)
