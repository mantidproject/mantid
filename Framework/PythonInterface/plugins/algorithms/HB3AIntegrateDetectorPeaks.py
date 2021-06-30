# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (AlgorithmFactory, IPeaksWorkspaceProperty,
                        PythonAlgorithm, PropertyMode, ADSValidator,
                        WorkspaceGroup)
from mantid.kernel import (Direction, EnabledWhenProperty, IntArrayProperty, IntBoundedValidator,
                           Property, PropertyCriterion, IntArrayLengthValidator, StringArrayProperty,
                           StringListValidator)
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
        return "Integrate four-circle rocking-scan peak in detector space from HB3A"

    def PyInit(self):
        self.declareProperty(StringArrayProperty("InputWorkspace", direction=Direction.Input, validator=ADSValidator()),
                             doc="Workspace or comma-separated workspace list containing input MDHisto scan data.")

        self.declareProperty("Method", direction=Direction.Input, defaultValue="Fitted",
                             validator=StringListValidator(["Counts", "CountsWithFitting", "Fitted"]),
                             doc="Integration method to use")

        self.declareProperty("NumBackgroundPts", direction=Direction.Input, defaultValue=3,
                             validator=IntBoundedValidator(lower=0),
                             doc="Number of background points from beginning and end of scan to use for background estimation")
        self.setPropertySettings("NumBackgroundPts", EnabledWhenProperty("Method", PropertyCriterion.IsEqualTo, "Counts"))

        self.declareProperty("N", direction=Direction.Input, defaultValue=2,
                             validator=IntBoundedValidator(lower=0),
                             doc="Set of measurements around motor position (+/- N/2*FWHM)")
        self.setPropertySettings("N",
                                 EnabledWhenProperty("Method", PropertyCriterion.IsEqualTo, "CountsWithFitting"))

        self.declareProperty(IntArrayProperty("LowerLeft", [128, 128], IntArrayLengthValidator(2),
                                              direction=Direction.Input), doc="Region of interest lower-left corner, in detector pixels")
        self.declareProperty(IntArrayProperty("UpperRight", [384, 384], IntArrayLengthValidator(2),
                                              direction=Direction.Input), doc="Region of interest upper-right corner, in detector pixels")

        self.declareProperty("StartX", Property.EMPTY_DBL,
                             doc="The start of the scan axis fitting range in degrees, either omega or chi axis.")
        self.declareProperty("EndX", Property.EMPTY_DBL,
                             doc="The end of the scan axis fitting range in degrees, either omega or chi axis.")

        self.declareProperty("ScaleFactor", 1.0, doc="scale the integrated intensity by this value")
        self.declareProperty("ChiSqMax", 10.0, doc="Fitting resulting in chi-sqaured higher than this won't be added to the output")
        self.declareProperty("SignalNoiseMin", 1.0,
                             doc="Minimum Signal/Noice ratio (Intensity/SigmaIntensity) of peak to be added to the output")
        self.declareProperty("ApplyLorentz", True, doc="If to apply Lorentz Correction to intensity")

        self.declareProperty("OutputFitResults", False, doc="This will output the fitting result workspace and a ROI workspace")

        self.declareProperty("OptimizeQVector", True,
                             doc="This will convert the data to q and optimize the peak location using CentroidPeaksdMD")

        self.declareProperty(IPeaksWorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc="Output Peaks Workspace")

    def PyExec(self):
        input_workspaces = self._expand_groups()
        outWS = self.getPropertyValue("OutputWorkspace")
        CreatePeaksWorkspace(OutputType='LeanElasticPeak',
                             InstrumentWorkspace=input_workspaces[0],
                             NumberOfPeaks=0,
                             OutputWorkspace=outWS, EnableLogging=False)

        scale = self.getProperty("ScaleFactor").value
        chisqmax = self.getProperty("ChiSqMax").value
        signalNoiseMin = self.getProperty("SignalNoiseMin").value
        ll = self.getProperty("LowerLeft").value
        ur = self.getProperty("UpperRight").value
        startX = self.getProperty('StartX').value
        endX = self.getProperty('EndX').value
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
                                      OutputWorkspace=tmp_inWS,
                                      EnableLogging=False)
            ConvertMDHistoToMatrixWorkspace(tmp_inWS, OutputWorkspace=tmp_inWS, EnableLogging=False)
            data = ConvertToPointData(tmp_inWS, OutputWorkspace=tmp_inWS, EnableLogging=False)

            run = mtd[inWS].getExperimentInfo(0).run()
            scan_log = 'omega' if np.isclose(run.getTimeAveragedStd('phi'), 0.0) else 'phi'
            scan_axis = run[scan_log].value
            data.setX(0, scan_axis)

            x = data.extractX().flatten()

            integrated_intensity, integrated_intensity_error = self._fitted_integration(inWS, data, startX, endX,
                                                                                        chisqmax, scale, output_fit)
            if integrated_intensity == -1 or integrated_intensity_error == -1:
                continue

            __tmp_pw = CreatePeaksWorkspace(OutputType='LeanElasticPeak',
                                            InstrumentWorkspace=inWS,
                                            NumberOfPeaks=0,
                                            EnableLogging=False)

            if scan_log == 'omega':
                SetGoniometer(Workspace=__tmp_pw, Axis0=f'{x},0,1,0,-1', Axis1='chi,0,0,1,-1', Axis2='phi,0,1,0,-1',
                              EnableLogging=False)
            else:
                SetGoniometer(Workspace=__tmp_pw, Axis0='omega,0,1,0,-1', Axis1='chi,0,0,1,-1', Axis2=f'{x},0,1,0,-1',
                              EnableLogging=False)

            peak = __tmp_pw.createPeakHKL([run['h'].getStatistics().median,
                                           run['k'].getStatistics().median,
                                           run['l'].getStatistics().median])
            peak.setWavelength(float(run['wavelength'].value))
            peak.setIntensity(integrated_intensity)
            peak.setSigmaIntensity(integrated_intensity_error)

            if integrated_intensity / integrated_intensity_error > signalNoiseMin:
                __tmp_pw.addPeak(peak)

                # correct q-vector using CentroidPeaksMD
                if optmize_q:
                    __tmp_q_ws = HB3AAdjustSampleNorm(InputWorkspaces=inWS, NormaliseBy='None', EnableLogging=False)
                    __tmp_pw = CentroidPeaksMD(__tmp_q_ws, __tmp_pw, EnableLogging=False)
                    DeleteWorkspace(__tmp_q_ws, EnableLogging=False)

                if use_lorentz:
                    # ILL Neutron Data Booklet, Second Edition, Section 2.9, Part 4.1, Equation 7
                    peak = __tmp_pw.getPeak(0)
                    lorentz = abs(np.sin(peak.getScattering() * np.cos(peak.getAzimuthal())))
                    peak.setIntensity(peak.getIntensity() * lorentz)
                    peak.setSigmaIntensity(peak.getSigmaIntensity() * lorentz)

                CombinePeaksWorkspaces(outWS, __tmp_pw, OutputWorkspace=outWS, EnableLogging=False)
                DeleteWorkspace(__tmp_pw, EnableLogging=False)

                if output_fit:
                    fit_results.addWorkspace(RenameWorkspace(tmp_inWS + '_Workspace', outWS + "_" + inWS + '_Workspace',
                                                             EnableLogging=False))
                    fit_results.addWorkspace(
                        RenameWorkspace(tmp_inWS + '_Parameters', outWS + "_" + inWS + '_Parameters',
                                        EnableLogging=False))
                    fit_results.addWorkspace(RenameWorkspace(tmp_inWS + '_NormalisedCovarianceMatrix',
                                                             outWS + "_" + inWS + '_NormalisedCovarianceMatrix',
                                                             EnableLogging=False))
                    fit_results.addWorkspace(IntegrateMDHistoWorkspace(InputWorkspace=inWS,
                                                                       P1Bin=f'{ll[1]},0,{ur[1]}',
                                                                       P2Bin=f'{ll[0]},0,{ur[0]}',
                                                                       P3Bin='0,{}'.format(
                                                                           mtd[inWS].getDimension(2).getNBins()),
                                                                       OutputWorkspace=outWS + "_" + inWS + "_ROI",
                                                                       EnableLogging=False))
            else:
                self.log().warning("Skipping peak from {} because Signal/Noise={:.3f} which is less than {}"
                                   .format(inWS, integrated_intensity/integrated_intensity_error, signalNoiseMin))

            for tmp_ws in (tmp_inWS, tmp_inWS+'_Workspace', tmp_inWS+'_Parameters', tmp_inWS+'_NormalisedCovarianceMatrix'):
                if mtd.doesExist(tmp_ws):
                    DeleteWorkspace(tmp_ws, EnableLogging=False)

        self.setProperty("OutputWorkspace", mtd[outWS])

    def _fitted_integration(self, inWS, ws, startX, endX, chisqmax, scale_factor, output_fit):
        y = ws.extractY().flatten()
        x = ws.extractX().flatten()

        integrated_intensity = -1
        integrated_intensity_error = -1

        function = f"name=FlatBackground, A0={np.nanmin(y)};" \
                   f"name=Gaussian, PeakCentre={x[np.nanargmax(y)]}, Height={np.nanmax(y) - np.nanmin(y)}, Sigma=0.25"
        constraints = f"f0.A0 > 0, f1.Height > 0, {x.min()} < f1.PeakCentre < {x.max()}"
        try:
            fit_result = Fit(function, ws, Output=str(ws),
                             IgnoreInvalidData=True,
                             OutputParametersOnly=not output_fit,
                             Constraints=constraints,
                             StartX=startX, EndX=endX,
                             EnableLogging=False)
        except RuntimeError as e:
            self.log().warning("Failed to fit workspace {}: {}".format(inWS, e))
            return -1, -1

        if fit_result.OutputStatus == 'success' and fit_result.OutputChi2overDoF < chisqmax:
            _, A, x, s, _ = fit_result.OutputParameters.toDict()['Value']
            _, errA, _, errs, _ = fit_result.OutputParameters.toDict()['Error']

            integrated_intensity = A * s * np.sqrt(2 * np.pi) * scale_factor

            # Convert correlation back into covariance
            cor_As = (fit_result.OutputNormalisedCovarianceMatrix.cell(1, 4) / 100
                      * fit_result.OutputParameters.cell(1, 2) * fit_result.OutputParameters.cell(3, 2))
            # σ^2 = 2π (A^2 σ_s^2 + σ_A^2 s^2 + 2 A s σ_As)
            integrated_intensity_error = np.sqrt(
                2 * np.pi * (A ** 2 * errs ** 2 + s ** 2 * errA ** 2 + 2 * A * s * cor_As)) * scale_factor
        else:
            self.log().warning("Failed to fit workspace {}: Output Status={}, ChiSq={}".format(inWS,
                                                                                               fit_result.OutputStatus,
                                                                                               fit_result.OutputChi2overDoF))

        return integrated_intensity, integrated_intensity_error

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


AlgorithmFactory.subscribe(HB3AIntegrateDetectorPeaks)
