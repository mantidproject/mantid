# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (AlgorithmFactory, IPeaksWorkspaceProperty,
                        IMDHistoWorkspaceProperty, PythonAlgorithm,
                        PropertyMode)
from mantid.kernel import (Direction, IntArrayProperty, IntArrayLengthValidator)
from mantid.simpleapi import (mtd, IntegrateMDHistoWorkspace,
                              CreatePeaksWorkspace,
                              ConvertMDHistoToMatrixWorkspace,
                              ConvertToPointData, CopySample, Fit)
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
        self.declareProperty(IMDHistoWorkspaceProperty("InputWorkspace", defaultValue="", direction=Direction.Input,
                                                       optional=PropertyMode.Optional),
                             doc="Workspace or comma-separated workspace list containing input MDHisto scan data.")

        self.declareProperty(IntArrayProperty("LowerLeft", [128, 128], IntArrayLengthValidator(2),
                                              direction=Direction.Input), doc="ROI lower-left")
        self.declareProperty(IntArrayProperty("UpperRight", [384, 384], IntArrayLengthValidator(2),
                                              direction=Direction.Input), doc="ROI upper_right")

        self.declareProperty("ScaleFactor", 1.0, doc="scale the integrated intensity by this value")
        self.declareProperty("ChiSqMax", 10.0, doc="Fitting resulting in chisq higher than this won't be added to the output")

        self.declareProperty(IPeaksWorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc="Output MDWorkspace in Q-space, name is prefix if multiple input files were provided.")

    def PyExec(self):
        inWS = self.getProperty("InputWorkspace").value
        outWS_name = self.getPropertyValue("OutputWorkspace")
        print(f'{outWS_name=}')
        if mtd.doesExist(outWS_name):
            outWS = mtd[outWS_name]
        else:
            outWS = CreatePeaksWorkspace(OutputType='LeanElasticPeak', NumberOfPeaks=0, OutputWorkspace=outWS_name)
            CopySample(inWS, outWS, CopyEnvironment=False, CopyName=False, CopyMaterial=False, CopyShape=False)

        scale = self.getProperty("ScaleFactor").value
        chisqmax = self.getProperty("ChiSqMax").value
        ll = self.getProperty("LowerLeft").value
        ur = self.getProperty("UpperRight").value

        data = IntegrateMDHistoWorkspace(InputWorkspace=inWS,
                                         P1Bin=f'{ll[1]},{ur[1]}',
                                         P2Bin=f'{ll[0]},{ur[0]}')
        data = ConvertMDHistoToMatrixWorkspace(data)
        data = ConvertToPointData(data)

        run = inWS.getExperimentInfo(0).run()
        scan_log = 'omega' if np.isclose(run.getTimeAveragedStd('phi'), 0.0) else 'phi'
        scan_axis = run[scan_log].value
        data.setX(0, scan_axis)

        outWS.run().getGoniometer().setR(run.getGoniometer().getR())
        peak = outWS.createPeakHKL([run['h'].value.mean(),
                                    run['k'].value.mean(),
                                    run['l'].value.mean()])

        status, parameters = fit_gaussian('data')
        if status.OutputStatus == 'success' and status.OutputChi2overDoF < chisqmax:
            _, A, _, s, _ = mtd[parameters].toDict()['Value']
            _, eA, _, es, _ = mtd[parameters].toDict()['Error']
            integrated_intensity = A * s * np.sqrt(2*np.pi) * scale
            peak.setIntensity(integrated_intensity)
            # σ^2 = 2π (A^2 σ_s^2 + σ_A^2 s^2 + 2 A s σ_As)
            integrated_intensity_error = np.sqrt(2*np.pi * (A**2 * es**2 +  s**2 * eA**2)) * scale  # Need to fix this
            peak.setSigmaIntensity(integrated_intensity_error)
            outWS.addPeak(peak)

        self.setProperty("OutputWorkspace", outWS)


def fit_gaussian(ws_name):
    y = mtd[ws_name].extractY()
    x = mtd[ws_name].extractX()
    function = f"name=FlatBackground, A0={y.min()}; name=Gaussian, PeakCentre={x[0, y.argmax()]}, Height={y.max()-y.min()}, Sigma=0.25"
    status = Fit(function, ws_name, Output=ws_name+'_fitted')
    return status, ws_name+'_fitted_Parameters'


AlgorithmFactory.subscribe(HB3AIntegrateDetectorPeaks)
