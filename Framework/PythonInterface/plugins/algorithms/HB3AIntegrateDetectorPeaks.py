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
                              ConvertMDHistoToMatrixWorkspace, HFIRCalculateGoniometer,
                              ConvertToPointData, Fit, CombinePeaksWorkspaces)
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
        #self.declareProperty(IMDHistoWorkspaceProperty("InputWorkspace", defaultValue="", direction=Direction.Input,
        #                                               optional=PropertyMode.Optional),
        #                     doc="Workspace or comma-separated workspace list containing input MDHisto scan data.")

        self.declareProperty(IntArrayProperty("LowerLeft", [128, 128], IntArrayLengthValidator(2),
                                              direction=Direction.Input), doc="ROI lower-left")
        self.declareProperty(IntArrayProperty("UpperRight", [384, 384], IntArrayLengthValidator(2),
                                              direction=Direction.Input), doc="ROI upper_right")

        self.declareProperty("ScaleFactor", 1.0, doc="scale the integrated intensity by this value")
        self.declareProperty("ChiSqMax", 10.0, doc="Fitting resulting in chisq higher than this won't be added to the output")

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

            fit_result = fit_gaussian(data)
            if fit_result.OutputStatus == 'success' and fit_result.OutputChi2overDoF < chisqmax:
                __tmp_pw = CreatePeaksWorkspace(OutputType='LeanElasticPeak',
                                                InstrumentWorkspace=inWS,
                                                NumberOfPeaks=0)

                _, A, x, s, _ = fit_result.OutputParameters.toDict()['Value']
                _, eA, _, es, _ = fit_result.OutputParameters.toDict()['Error']

                if scan_log == 'omega':
                    SetGoniometer(Workspace=__tmp_pw, Axis0=f'{x},0,1,0,-1', Axis1='chi,0,0,1,-1', Axis2='phi,0,1,0,-1')
                else:
                    SetGoniometer(Workspace=__tmp_pw, Axis0='omega,0,1,0,-1', Axis1='chi,0,0,1,-1', Axis2=f'{x},0,1,0,-1')

                peak = __tmp_pw.createPeakHKL([run['h'].getStatistics().median,
                                               run['k'].getStatistics().median,
                                               run['l'].getStatistics().median])

                integrated_intensity = A * s * np.sqrt(2*np.pi) * scale
                peak.setIntensity(integrated_intensity)
                # σ^2 = 2π (A^2 σ_s^2 + σ_A^2 s^2 + 2 A s σ_As)
                integrated_intensity_error = np.sqrt(2*np.pi * (A**2 * es**2 +  s**2 * eA**2)) * scale  # FIX THIS
                peak.setSigmaIntensity(integrated_intensity_error)
                __tmp_pw.addPeak(peak)
                HFIRCalculateGoniometer(__tmp_pw)
                CombinePeaksWorkspaces(outWS, __tmp_pw, OutputWorkspace=outWS)
                DeleteWorkspace(__tmp_pw)

            DeleteWorkspace(tmp_inWS)
            DeleteWorkspace(tmp_inWS+'_Parameters')
            DeleteWorkspace(tmp_inWS+'_NormalisedCovarianceMatrix')

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


def fit_gaussian(ws):
    y = ws.extractY()
    x = ws.extractX()
    function = f"name=FlatBackground, A0={y.min()}; name=Gaussian, PeakCentre={x[0, y.argmax()]}, Height={y.max()-y.min()}, Sigma=0.25"
    fit_result = Fit(function, ws, Output=str(ws), OutputParametersOnly=True)
    print(fit_result)
    return fit_result


AlgorithmFactory.subscribe(HB3AIntegrateDetectorPeaks)
