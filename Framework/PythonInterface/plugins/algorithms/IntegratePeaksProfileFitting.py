# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
"""
This is a Python algorithm, with profile
fitting for integrating peaks.
"""

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import numpy as np
import warnings


class IntegratePeaksProfileFitting(PythonAlgorithm):
    def summary(self):
        return "Fits a series of peaks using 3D profile fitting as an Ikeda-Carpenter function by a bivariate gaussian."

    def category(self):
        # defines the category the algorithm will be put in the algorithm browser
        return "Crystal\\Integration"

    def PyInit(self):
        # Declare a property for the output workspace
        self.declareProperty(
            WorkspaceProperty(name="OutputPeaksWorkspace", defaultValue="", direction=Direction.Output),
            doc="PeaksWorkspace with integrated peaks",
        )
        self.declareProperty(
            WorkspaceProperty(name="OutputParamsWorkspace", defaultValue="", direction=Direction.Output),
            doc="MatrixWorkspace with fit parameters",
        )
        self.declareProperty(
            WorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input),
            doc="An input Sample MDHistoWorkspace or MDEventWorkspace in HKL.",
        )
        self.declareProperty(
            WorkspaceProperty(name="PeaksWorkspace", defaultValue="", direction=Direction.Input),
            doc="PeaksWorkspace with peaks to be integrated.",
        )

        self.declareProperty(
            FileProperty(name="UBFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".mat"]),
            doc="File containing the UB Matrix in ISAW format. Leave blank to use loaded UB Matrix.",
        )
        self.declareProperty(
            FileProperty(name="ModeratorCoefficientsFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".dat"]),
            doc="File containing the Pade coefficients describing moderator emission versus energy.",
        )
        self.declareProperty(
            FileProperty("StrongPeakParamsFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".pkl"]),
            doc="File containing strong peaks profiles.  If left blank, strong peaks will be fit first.",
        )
        self.declareProperty("IntensityCutoff", defaultValue=0.0, doc="Minimum number of counts to force a profile")
        edgeDocString = "Pixels within EdgeCutoff from a detector edge will be have a profile forced."
        self.declareProperty("EdgeCutoff", defaultValue=0.0, doc=edgeDocString)
        self.declareProperty(
            "FracStop",
            defaultValue=0.05,
            validator=FloatBoundedValidator(lower=0.0, exclusive=True),
            doc="Fraction of max counts to include in peak selection.",
        )

        self.declareProperty("MinpplFrac", defaultValue=0.9, doc="Min fraction of predicted background level to check")
        self.declareProperty("MaxpplFrac", defaultValue=1.1, doc="Max fraction of predicted background level to check")

        self.declareProperty("DQMax", defaultValue=0.15, doc="Largest total side length (in Angstrom) to consider for profile fitting.")
        self.declareProperty("PeakNumber", defaultValue=-1, doc="Which Peak to fit.  Leave negative for all.")

    def initializeStrongPeakSettings(self, strongPeaksParamsFile, peaks_ws, sampleRun, forceCutoff, edgeCutoff, numDetRows, numDetCols):
        import pickle

        # Strong peak profiles - we set up the workspace and determine which peaks we'll fit.
        strongPeakKeys = ["Phi", "Theta", "Scale3d", "FitPhi", "FitTheta", "SigTheta", "SigPhi", "SigP", "PeakNumber"]
        strongPeakDatatypes = ["float"] * len(strongPeakKeys)
        strongPeakParams_ws = CreateEmptyTableWorkspace(OutputWorkspace="__StrongPeakParameters")
        for key, datatype in zip(strongPeakKeys, strongPeakDatatypes):
            strongPeakParams_ws.addColumn(datatype, key)

        # Either load the provided strong peaks file or set the flag to generate it as we go
        if strongPeaksParamsFile != "":
            strongPeakParams = pickle.load(open(strongPeaksParamsFile, "rb"), encoding="latin1")
            generateStrongPeakParams = False
            # A strong peaks file was provided - we don't need to generate it on the fly so we can fit in order
            runNumbers = np.array(peaks_ws.column("RunNumber"))
            peaksToFit = np.where(runNumbers == sampleRun)[0]
            intensities = np.array(peaks_ws.column("Intens"))
            rows = np.array(peaks_ws.column("Row"))
            cols = np.array(peaks_ws.column("Col"))
            runNumbers = np.array(peaks_ws.column("RunNumber"))
            intensIDX = intensities < forceCutoff
            edgeIDX = np.logical_or.reduce(
                np.array([rows < edgeCutoff, rows > numDetRows - edgeCutoff, cols < edgeCutoff, cols > numDetCols - edgeCutoff])
            )
            needsForcedProfile = np.logical_or(intensIDX, edgeIDX)
            needsForcedProfileIDX = np.where(needsForcedProfile)[0]
            canFitProfileIDX = np.where(~needsForcedProfile)[0]
            numPeaksCanFit = len(canFitProfileIDX)

            # We can populate the strongPeakParams_ws now and use that for initial BVG guesses
            for row in strongPeakParams:
                strongPeakParams_ws.addRow(row)

        else:
            generateStrongPeakParams = True
            # Figure out which peaks to fit without forcing a profile and set those to be fit first
            intensities = np.array(peaks_ws.column("Intens"))
            rows = np.array(peaks_ws.column("Row"))
            cols = np.array(peaks_ws.column("Col"))
            runNumbers = np.array(peaks_ws.column("RunNumber"))
            intensIDX = intensities < forceCutoff
            edgeIDX = np.logical_or.reduce(
                np.array([rows < edgeCutoff, rows > numDetRows - edgeCutoff, cols < edgeCutoff, cols > numDetCols - edgeCutoff])
            )
            needsForcedProfile = np.logical_or(intensIDX, edgeIDX)
            needsForcedProfileIDX = np.where(needsForcedProfile)[0]
            canFitProfileIDX = np.where(~needsForcedProfile)[0]
            numPeaksCanFit = len(canFitProfileIDX)
            peaksToFit = np.append(canFitProfileIDX, needsForcedProfileIDX)  # Will fit in this order
            peaksToFit = peaksToFit[runNumbers[peaksToFit] == sampleRun]

            # Initialize our strong peaks dictionary.  Set BVG Params to be None so that we fall back on
            # instrument defaults until we have fit >=30 peaks.
            strongPeakParams = np.empty([numPeaksCanFit, 9])
            # sigX0Params, sigY0, sigP0Params = None, None, None
        peaksToFit = np.append(peaksToFit, np.where(runNumbers != sampleRun)[0])
        return (
            generateStrongPeakParams,
            strongPeakParams,
            strongPeakParams_ws,
            needsForcedProfile,
            needsForcedProfileIDX,
            canFitProfileIDX,
            numPeaksCanFit,
            peaksToFit,
        )

    def getBVGInitialGuesses(self, peaks_ws, strongPeakParams_ws, minNumberPeaks=30):
        """
        Returns initial guesses for the BVG fit if strongPeakParams_ws contains more than
        minNumberPeaks entries.  If not, we return all None, which will fall back to the
        instrument defaults.
        """
        if strongPeakParams_ws.rowCount() > minNumberPeaks:
            # First, along the scattering direction
            theta = np.abs(strongPeakParams_ws.column("Theta"))
            sigma_theta = np.abs(strongPeakParams_ws.column("SigTheta"))

            CreateWorkspace(DataX=theta, DataY=sigma_theta, OutputWorkspace="__ws_bvg0_scat")
            Fit(
                Function="name=UserFunction,Formula=A/2.0*(exp(((x-x0)/b))+exp( -((x-x0)/b)))+BG,A=0.0025,x0=1.54,b=1,BG=-1.26408e-15",
                InputWorkspace="__ws_bvg0_scat",
                Output="__fitSigX0",
                StartX=np.min(theta),
                EndX=np.max(theta),
            )
            sigX0Params = mtd["__fitSigX0_Parameters"].column(1)[:-1]
            # Second, along the azimuthal.  This is just a constant.
            sigY0 = np.median(strongPeakParams_ws.column("SigPhi"))
            # Finally, the interaction term.  This we just get from the instrument file.
            try:
                sigP0Params = peaks_ws.getInstrument().getStringParameter("sigP0Params")
                sigP0Params = np.array(str(sigP0Params).strip("[]'").split(), dtype=float)
            except:
                logger.warning("Cannot find sigP0Params.  Will use defaults.")
                sigP0Params = [0.1460775, 1.85816592, 0.26850086, -0.00725352]
            return sigX0Params, sigY0, sigP0Params
        else:
            return None, None, None

    def getUBMatrix(self, peaks_ws, UBFile):
        # Load the UB Matrix if one is not already loaded
        if UBFile == "" and peaks_ws.sample().hasOrientedLattice():
            logger.information("Using UB file already available in PeaksWorkspace")
        else:
            try:
                from mantid.simpleapi import LoadIsawUB

                LoadIsawUB(InputWorkspace=peaks_ws, FileName=UBFile)
            except:
                logger.error("peaks_ws does not have a UB matrix loaded.  Must provide a file")
        UBMatrix = peaks_ws.sample().getOrientedLattice().getUB()
        return UBMatrix

    def PyExec(self):
        import ICCFitTools as ICCFT
        import BVGFitTools as BVGFT
        from scipy.ndimage.filters import convolve

        MDdata = self.getProperty("InputWorkspace").value
        peaks_ws = self.getProperty("PeaksWorkspace").value
        fracStop = self.getProperty("FracStop").value
        dQMax = self.getProperty("DQMax").value
        UBFile = self.getProperty("UBFile").value
        padeFile = self.getProperty("ModeratorCoefficientsFile").value
        strongPeaksParamsFile = self.getProperty("StrongPeakParamsFile").value
        forceCutoff = self.getProperty("IntensityCutoff").value
        edgeCutoff = self.getProperty("EdgeCutoff").value
        peakNumberToFit = self.getProperty("PeakNumber").value
        pplmin_frac = self.getProperty("MinpplFrac").value
        pplmax_frac = self.getProperty("MaxpplFrac").value
        sampleRun = peaks_ws.getPeak(0).getRunNumber()

        q_frame = "lab"
        mtd["MDdata"] = MDdata
        zBG = 1.96
        neigh_length_m = 3
        iccFitDict = ICCFT.parseConstraints(peaks_ws)  # Contains constraints and guesses for ICC Fitting
        padeCoefficients = ICCFT.getModeratorCoefficients(padeFile)

        # There are a few instrument specific parameters that we define here.  In some cases,
        # it may improve fitting to set tweak these parameters, but for simplicity we define these here
        # The default values are good for MaNDi - new instruments can be added by adding a different elif
        # statement.
        # If you change these values or add an instrument, documentation should also be changed.
        try:
            numDetRows = peaks_ws.getInstrument().getIntParameter("numDetRows")[0]
            numDetCols = peaks_ws.getInstrument().getIntParameter("numDetCols")[0]
            nPhi = peaks_ws.getInstrument().getIntParameter("numBinsPhi")[0]
            nTheta = peaks_ws.getInstrument().getIntParameter("numBinsTheta")[0]
            nPhi = peaks_ws.getInstrument().getIntParameter("numBinsPhi")[0]
            mindtBinWidth = peaks_ws.getInstrument().getNumberParameter("mindtBinWidth")[0]
            maxdtBinWidth = peaks_ws.getInstrument().getNumberParameter("maxdtBinWidth")[0]
            fracHKL = peaks_ws.getInstrument().getNumberParameter("fracHKL")[0]
            dQPixel = peaks_ws.getInstrument().getNumberParameter("dQPixel")[0]
            peakMaskSize = peaks_ws.getInstrument().getIntParameter("peakMaskSize")[0]
        except:
            logger.error("Cannot find all parameters in instrument parameters file.")
            raise

        UBMatrix = self.getUBMatrix(peaks_ws, UBFile)
        dQ = np.abs(ICCFT.getDQFracHKL(UBMatrix, frac=0.5))
        dQ[dQ > dQMax] = dQMax
        qMask = ICCFT.getHKLMask(UBMatrix, frac=fracHKL, dQPixel=dQPixel, dQ=dQ)

        (
            generateStrongPeakParams,
            strongPeakParams,
            strongPeakParams_ws,
            needsForcedProfile,
            needsForcedProfileIDX,
            canFitProfileIDX,
            numPeaksCanFit,
            peaksToFit,
        ) = self.initializeStrongPeakSettings(strongPeaksParamsFile, peaks_ws, sampleRun, forceCutoff, edgeCutoff, numDetRows, numDetCols)

        if peakNumberToFit > -1:
            peaksToFit = [peakNumberToFit]

        # Create the parameters workspace
        keys = [
            "peakNumber",
            "Alpha",
            "Beta",
            "R",
            "T0",
            "bgBVG",
            "chiSq3d",
            "chiSq",
            "dQ",
            "KConv",
            "MuPH",
            "MuTH",
            "newQ",
            "Scale",
            "scale3d",
            "SigP",
            "SigX",
            "SigY",
            "Intens3d",
            "SigInt3d",
        ]
        datatypes = ["float"] * len(keys)
        datatypes[np.where(np.array(keys) == "newQ")[0][0]] = "V3D"
        params_ws = CreateEmptyTableWorkspace()
        for key, datatype in zip(keys, datatypes):
            params_ws.addColumn(datatype, key)

        # And we're off!
        peaks_ws_out = peaks_ws.clone()
        warnings.filterwarnings("ignore")  # There can be a lot of warnings for bad solutions that get rejected.
        progress = Progress(self, 0.0, 1.0, len(peaksToFit))
        sigX0Params, sigY0, sigP0Params = self.getBVGInitialGuesses(peaks_ws, strongPeakParams_ws)

        for fitNumber, peakNumber in enumerate(peaksToFit):  # range(peaks_ws.getNumberPeaks()):
            peakNumber = int(peakNumber)
            peak = peaks_ws_out.getPeak(peakNumber)
            progress.report(" ")
            if peak.getRunNumber() != MDdata.getExperimentInfo(0).getRunNumber():
                logger.warning(
                    "Peak number %i has run number %i but MDWorkspace is from run number %i.  Skipping this peak."
                    % (peakNumber, peak.getRunNumber(), MDdata.getExperimentInfo(0).getRunNumber())
                )
                continue
            try:
                box = ICCFT.getBoxFracHKL(peak, peaks_ws, MDdata, UBMatrix, peakNumber, dQ, fracHKL=0.5, dQPixel=dQPixel, q_frame=q_frame)
                if ~needsForcedProfile[peakNumber]:
                    strongPeakParamsToSend = None
                else:
                    strongPeakParamsToSend = strongPeakParams

                # Will allow forced weak and edge peaks to be fit using a neighboring peak profile
                Y3D, goodIDX, pp_lambda, params = BVGFT.get3DPeak(
                    peak,
                    peakNumber,
                    peaks_ws,
                    box,
                    padeCoefficients,
                    qMask,
                    nTheta=nTheta,
                    nPhi=nPhi,
                    plotResults=False,
                    zBG=zBG,
                    fracBoxToHistogram=1.0,
                    bgPolyOrder=1,
                    strongPeakParams=strongPeakParamsToSend,
                    q_frame=q_frame,
                    mindtBinWidth=mindtBinWidth,
                    maxdtBinWidth=maxdtBinWidth,
                    pplmin_frac=pplmin_frac,
                    pplmax_frac=pplmax_frac,
                    forceCutoff=forceCutoff,
                    edgeCutoff=edgeCutoff,
                    peakMaskSize=peakMaskSize,
                    iccFitDict=iccFitDict,
                    sigX0Params=sigX0Params,
                    sigY0=sigY0,
                    sigP0Params=sigP0Params,
                    fitPenalty=1.0e7,
                )
                # First we get the peak intensity
                peakIDX = Y3D / Y3D.max() > fracStop
                intensity = np.sum(Y3D[peakIDX])

                # Now the number of background counts under the peak assuming a constant bg across the box
                n_events = box.getNumEventsArray()
                convBox = 1.0 * np.ones([neigh_length_m, neigh_length_m, neigh_length_m]) / neigh_length_m**3
                conv_n_events = convolve(n_events, convBox)
                bgIDX = np.logical_and.reduce(np.array([~goodIDX, qMask, conv_n_events > 0]))
                bgEvents = np.mean(n_events[bgIDX]) * np.sum(peakIDX)

                # Now we consider the variation of the fit.  These are done as three independent fits.  So we need to consider
                # the variance within our fit sig^2 = sum(N*(yFit-yData)) / sum(N) and scale by the number of parameters that go into
                # the fit.  In total: 10 (removing scale variables)
                w_events = n_events.copy()
                w_events[w_events == 0] = 1
                varFit = np.average((n_events[peakIDX] - Y3D[peakIDX]) * (n_events[peakIDX] - Y3D[peakIDX]), weights=(w_events[peakIDX]))

                sigma = np.sqrt(intensity + bgEvents + varFit)

                compStr = "peak {:d}; original: {:4.2f} +- {:4.2f};  new: {:4.2f} +- {:4.2f}".format(
                    peakNumber, peak.getIntensity(), peak.getSigmaIntensity(), intensity, sigma
                )
                logger.information(compStr)

                # Save the results
                params["peakNumber"] = peakNumber
                params["Intens3d"] = intensity
                params["SigInt3d"] = sigma
                params["newQ"] = V3D(params["newQ"][0], params["newQ"][1], params["newQ"][2])
                params_ws.addRow(params)
                peak.setIntensity(intensity)
                peak.setSigmaIntensity(sigma)

                if generateStrongPeakParams and ~needsForcedProfile[peakNumber]:
                    qPeak = peak.getQLabFrame()
                    theta = np.arctan2(qPeak[2], np.hypot(qPeak[0], qPeak[1]))  # 2theta
                    try:
                        p = mtd["__fitSigX0_Parameters"].column(1)[:-1]
                        tol = 0.2  # We should have a good idea now - only allow 20% variation
                    except:
                        p = peaks_ws.getInstrument().getStringParameter("sigSC0Params")
                        p = np.array(str(p).strip("[]'").split(), dtype=float)
                        tol = 5.0  # High tolerance since we don't know what the answer will be
                    predSigX = BVGFT.coshPeakWidthModel(theta, p[0], p[1], p[2], p[3])

                    if np.abs((params["SigX"] - predSigX) / 1.0 / predSigX) < tol:
                        strongPeakParams[fitNumber, 0] = np.arctan2(qPeak[1], qPeak[0])  # phi
                        strongPeakParams[fitNumber, 1] = np.arctan2(qPeak[2], np.hypot(qPeak[0], qPeak[1]))  # theta
                        strongPeakParams[fitNumber, 2] = params["scale3d"]
                        strongPeakParams[fitNumber, 3] = params["MuTH"]
                        strongPeakParams[fitNumber, 4] = params["MuPH"]
                        strongPeakParams[fitNumber, 5] = params["SigX"]
                        strongPeakParams[fitNumber, 6] = params["SigY"]
                        strongPeakParams[fitNumber, 7] = params["SigP"]
                        strongPeakParams[fitNumber, 8] = peakNumber
                        strongPeakParams_ws.addRow(strongPeakParams[fitNumber])
                        sigX0Params, sigY0, sigP0Params = self.getBVGInitialGuesses(peaks_ws, strongPeakParams_ws)

            except KeyboardInterrupt:
                warnings.filterwarnings("default")  # Re-enable on exit
                raise

            except:
                # raise
                logger.warning("Error fitting peak number " + str(peakNumber))
                peak.setIntensity(0.0)
                peak.setSigmaIntensity(1.0)

        # Cleanup
        for wsName in mtd.getObjectNames():
            if "fit_" in wsName or "bvgWS" in wsName or "tofWS" in wsName or "scaleWS" in wsName:
                mtd.remove(wsName)
        warnings.filterwarnings("default")  # Re-enable on exit
        # Set the output
        self.setProperty("OutputPeaksWorkspace", peaks_ws_out)
        self.setProperty("OutputParamsWorkspace", params_ws)


# Register algorith with Mantid
AlgorithmFactory.subscribe(IntegratePeaksProfileFitting)
