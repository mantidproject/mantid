# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (AlgorithmFactory, IPeaksWorkspaceProperty,
                        PythonAlgorithm, PropertyMode, ADSValidator,
                        WorkspaceGroup)
from mantid.kernel import (Direction, IntArrayProperty,
                           IntArrayLengthValidator, StringArrayProperty)
from mantid.simpleapi import (mtd, IntegrateMDHistoWorkspace,
                              CreatePeaksWorkspace, DeleteWorkspace,
                              AnalysisDataService, SetGoniometer,
                              ConvertMDHistoToMatrixWorkspace,
                              ConvertToPointData, Fit,
                              CombinePeaksWorkspaces,
                              HB3AAdjustSampleNorm,
                              CentroidPeaksMD, RenameWorkspace)
import numpy as np


class HB3AIntegrateDetectorPeaks(PythonAlgorithm):

    def category(self):
        return "Crystal\\Integration"

    def seeAlso(self):
        return ["HB3AAdjustSampleNorm", "HB3AIntegratePeaks"]

    def name(self):
        return "HB3AIntegrateDetectorPeaks"

    def summary(self):
        return "Integrate rocking-scan peak in detector space"

    def PyInit(self):
        self.declareProperty(StringArrayProperty("InputWorkspace", direction=Direction.Input, validator=ADSValidator()),
                             doc="Workspace or comma-separated workspace list containing input MDHisto scan data.")

        self.declareProperty(IntArrayProperty("LowerLeft", [128, 128], IntArrayLengthValidator(2),
                                              direction=Direction.Input), doc="ROI lower-left")
        self.declareProperty(IntArrayProperty("UpperRight", [384, 384], IntArrayLengthValidator(2),
                                              direction=Direction.Input), doc="ROI upper_right")

        self.declareProperty("ScaleFactor", 1.0, doc="scale the integrated intensity by this value")
        self.declareProperty("ChiSqMax", 10.0, doc="Fitting resulting in chisq higher than this won't be added to the output")
        self.declareProperty("ApplyLorentz", True, doc="If to apply Lorentz Correction to intensity")

        self.declareProperty("OutputFitResults", False, doc="This will include the fitting result workspace")

        self.declareProperty("OptimizeQVector", True,
                             doc="This will convert the data to q and optimize the peak location using CentroidPeaksdMD")

        self.declareProperty(IPeaksWorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc="Output MDWorkspace in Q-space, name is prefix if multiple input files were provided.")

    def PyExec(self):
        input_workspaces = self._expand_groups()
        outWS = self.getPropertyValue("OutputWorkspace")
        CreatePeaksWorkspace(OutputType='LeanElasticPeak',
                             InstrumentWorkspace=input_workspaces[0],
                             NumberOfPeaks=0,
                             OutputWorkspace=outWS)

        scale = self.getProperty("ScaleFactor").value
        chisqmax = self.getProperty("ChiSqMax").value
        ll = self.getProperty("LowerLeft").value
        ur = self.getProperty("UpperRight").value
        use_lorentz = self.getProperty("ApplyLorentz").value
        optmize_q = self.getProperty("OptimizeQVector").value
        output_fit = self.getProperty("OutputFitResults").value

        if output_fit:
            fit_results = WorkspaceGroup()
            AnalysisDataService.addOrReplace(outWS+"_fit_results", fit_results)

        for inWS in input_workspaces:
            tmp_inWS = '__tmp_' + inWS
            IntegrateMDHistoWorkspace(InputWorkspace=inWS,
                                      P1Bin=f'{ll[1]},{ur[1]}',
                                      P2Bin=f'{ll[0]},{ur[0]}',
                                      OutputWorkspace=tmp_inWS)
            ConvertMDHistoToMatrixWorkspace(tmp_inWS, OutputWorkspace=tmp_inWS)
            data = ConvertToPointData(tmp_inWS, OutputWorkspace=tmp_inWS)

            run = mtd[inWS].getExperimentInfo(0).run()
            scan_log = 'omega' if np.isclose(run.getTimeAveragedStd('phi'), 0.0) else 'phi'
            scan_axis = run[scan_log].value
            data.setX(0, scan_axis)

            fit_result = fit_gaussian(data, output_fit)
            if fit_result.OutputStatus == 'success' and fit_result.OutputChi2overDoF < chisqmax:
                __tmp_pw = CreatePeaksWorkspace(OutputType='LeanElasticPeak',
                                                InstrumentWorkspace=inWS,
                                                NumberOfPeaks=0)

                _, A, x, s, _ = fit_result.OutputParameters.toDict()['Value']
                _, errA, _, errs, _ = fit_result.OutputParameters.toDict()['Error']

                if scan_log == 'omega':
                    SetGoniometer(Workspace=__tmp_pw, Axis0=f'{x},0,1,0,-1', Axis1='chi,0,0,1,-1', Axis2='phi,0,1,0,-1')
                else:
                    SetGoniometer(Workspace=__tmp_pw, Axis0='omega,0,1,0,-1', Axis1='chi,0,0,1,-1', Axis2=f'{x},0,1,0,-1')

                peak = __tmp_pw.createPeakHKL([run['h'].getStatistics().median,
                                               run['k'].getStatistics().median,
                                               run['l'].getStatistics().median])
                peak.setWavelength(float(run['wavelength'].value))

                integrated_intensity = A * s * np.sqrt(2*np.pi) * scale
                peak.setIntensity(integrated_intensity)

                # Convert correlation back into covariance
                cor_As = (fit_result.OutputNormalisedCovarianceMatrix.cell(1,4)/100
                          * fit_result.OutputParameters.cell(1,2) * fit_result.OutputParameters.cell(3,2))
                # σ^2 = 2π (A^2 σ_s^2 + σ_A^2 s^2 + 2 A s σ_As)
                integrated_intensity_error = np.sqrt(2*np.pi * (A**2 * errs**2 +  s**2 * errA**2 + 2*A*s*cor_As)) * scale
                peak.setSigmaIntensity(integrated_intensity_error)

                __tmp_pw.addPeak(peak)

                if use_lorentz:
                    peak = __tmp_pw.getPeak(0)
                    lorentz = abs(np.sin(peak.getScattering() * np.cos(peak.getAzimuthal())))
                    peak.setIntensity(peak.getIntensity() * lorentz)
                    peak.setSigmaIntensity(peak.getSigmaIntensity() * lorentz)

                # correct q-vector using CentroidPeaksdMD
                if optmize_q:
                    __tmp_q_ws = HB3AAdjustSampleNorm(InputWorkspaces=inWS, NormaliseBy='None')
                    __tmp_pw = CentroidPeaksMD(__tmp_q_ws, __tmp_pw)
                    DeleteWorkspace(__tmp_q_ws)

                CombinePeaksWorkspaces(outWS, __tmp_pw, OutputWorkspace=outWS)
                DeleteWorkspace(__tmp_pw)

                if output_fit:
                    fit_results.addWorkspace(RenameWorkspace(tmp_inWS+'_Workspace', inWS+'_Workspace'))
                    fit_results.addWorkspace(RenameWorkspace(tmp_inWS+'_Parameters', inWS+'_Parameters'))
                    fit_results.addWorkspace(RenameWorkspace(tmp_inWS+'_NormalisedCovarianceMatrix', inWS+'_NormalisedCovarianceMatrix'))
                    fit_results.addWorkspace(IntegrateMDHistoWorkspace(InputWorkspace=inWS,
                                                                       P1Bin=f'{ll[1]},0,{ur[1]}',
                                                                       P2Bin=f'{ll[0]},0,{ur[0]}',
                                                                       P3Bin='0,{}'.format(mtd[inWS].getDimension(2).getNBins()),
                                                                       OutputWorkspace=inWS+"_ROI"))
                else:
                    DeleteWorkspace(tmp_inWS+'_Parameters')
                    DeleteWorkspace(tmp_inWS+'_NormalisedCovarianceMatrix')
            DeleteWorkspace(tmp_inWS)

        self.setProperty("OutputWorkspace", mtd[outWS])

    def _expand_groups(self):
        """expand workspace groups"""
        workspaces = self.getProperty("InputWorkspace").value
        input_workspaces = []
        for wsname in workspaces:
            wks = AnalysisDataService.retrieve(wsname)
            if isinstance(wks, WorkspaceGroup):
                input_workspaces.extend(wks.getNames())
            else:
                input_workspaces.append(wsname)
        return input_workspaces


def fit_gaussian(ws, output_fit):
    y = ws.extractY()
    x = ws.extractX()
    function = f"name=FlatBackground, A0={y.min()}; name=Gaussian, PeakCentre={x[0, y.argmax()]}, Height={y.max()-y.min()}, Sigma=0.25"
    fit_result = Fit(function, ws, Output=str(ws), OutputParametersOnly=not output_fit, OutputCompositeMembers=True)
    return fit_result


AlgorithmFactory.subscribe(HB3AIntegrateDetectorPeaks)
