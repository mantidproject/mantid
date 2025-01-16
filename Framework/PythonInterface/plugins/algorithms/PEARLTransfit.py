# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# -----------------------------------------------------------------------
# Transfit v2 - translated from Fortran77 to Python/Mantid
# Original Author: Christopher J Ridley
# Last updated: 2nd October 2020
#
# Program fits a Voigt function to PEARL transmission data, using the doppler broadening
# of the gaussian component to determine the sample temperature
#
#
# References:
# 'Temperature measurement in a Paris-Edinburgh cell by neutron resonance spectroscopy' - Journal Of Applied Physics 98,
# 064905 (2005)
# 'Remote determination of sample temperature by neutron resonance spectroscopy' - Nuclear Instruments and Methods in
# Physics Research A 547 (2005) 601-615
# -----------------------------------------------------------------------
from mantid.kernel import Direction, StringListValidator, EnabledWhenProperty, PropertyCriterion
from mantid.api import PythonAlgorithm, MultipleFileProperty, AlgorithmFactory, mtd
from mantid.simpleapi import (
    Load,
    DeleteWorkspace,
    CropWorkspace,
    NormaliseByCurrent,
    ExtractSingleSpectrum,
    ConvertUnits,
    CreateWorkspace,
    Fit,
    Plus,
    Multiply,
    RenameWorkspace,
    CreateSingleValuedWorkspace,
)
from os import path
from scipy import constants
import numpy as np


class PEARLTransfit(PythonAlgorithm):
    # Resonance constants broadly consistent with those reported from
    # 'Neutron cross sections Vol 1, Resonance Parameters'
    # S.F. Mughabghab and D.I. Garber
    # June 1973, BNL report 325
    # Mass in atomic units, En in eV, temperatures in K, gamma factors in eV
    ResParamsDict = {
        # Hf01
        "Hf01_Mass": 177.0,
        "Hf01_En": 1.098,
        "Hf01_TD": 252.0,
        "Hf01_TwogG": 0.00192,
        "Hf01_Gg": 0.0662,
        "Hf01_startE": 0.6,
        "Hf01_Ediv": 0.01,
        "Hf01_endE": 1.7,
        # Hf02
        "Hf02_Mass": 177.0,
        "Hf02_En": 2.388,
        "Hf02_TD": 252.0,
        "Hf02_TwogG": 0.009,
        "Hf02_Gg": 0.0608,
        "Hf02_startE": 2.0,
        "Hf02_Ediv": 0.01,
        "Hf02_endE": 2.7,
        # Ta10
        "Ta10_Mass": 181.0,
        "Ta10_En": 10.44,
        "Ta10_TD": 240.0,
        "Ta10_TwogG": 0.00335,
        "Ta10_Gg": 0.0069,
        "Ta10_startE": 9.6,
        "Ta10_Ediv": 0.01,
        "Ta10_endE": 11.4,
        # Irp6
        "Irp6_Mass": 191.0,
        "Irp6_En": 0.6528,
        "Irp6_TD": 420.0,
        "Irp6_TwogG": 0.000547,
        "Irp6_Gg": 0.072,
        "Irp6_startE": 0.1,
        "Irp6_Ediv": 0.01,
        "Irp6_endE": 0.9,
        # Iro5
        "Iro5_Mass": 191.0,
        "Iro5_En": 5.36,
        "Iro5_TD": 420.0,
        "Iro5_TwogG": 0.006,
        "Iro5_Gg": 0.082,
        "Iro5_startE": 4.9,
        "Iro5_Ediv": 0.01,
        "Iro5_endE": 6.3,
        # Iro9
        "Iro9_Mass": 191.0,
        "Iro9_En": 9.3,
        "Iro9_TD": 420.0,
        "Iro9_TwogG": 0.0031,
        "Iro9_Gg": 0.082,
        "Iro9_startE": 8.7,
        "Iro9_Ediv": 0.01,
        "Iro9_endE": 9.85,
    }

    # Physical constants
    k = constants.k
    e = constants.e
    Pi = np.pi

    def version(self):
        return 1

    def name(self):
        return "PEARLTransfit"

    def category(self):
        return "Diffraction\\Fitting"

    def summary(self):
        return (
            "Reads high-energy neutron resonances from the downstream monitor data on the PEARL instrument,"
            "then fits a Voigt function to them to determine the sample temperature. A calibration must be run for"
            " each sample pressure. Can be used on a single file, or multiple files, in which case workspaces"
            " are summed and the average taken."
        )

    def PyInit(self):
        self.declareProperty(
            MultipleFileProperty("Files", extensions=[".raw", ".s0x", ".nxs"]),
            doc="Files of calibration runs (numors). Must be detector scans.",
        )
        self.declareProperty(
            name="FoilType",
            defaultValue="Hf01",
            validator=StringListValidator(["Hf01", "Hf02", "Ta10", "Irp6", "Iro5", "Iro9"]),
            direction=Direction.Input,
            doc="Type of foil included with the sample",
        )
        self.declareProperty(name="Ediv", defaultValue="0.0025", direction=Direction.Input, doc="Energy step in eV")
        self.declareProperty(name="ReferenceTemp", defaultValue="290", direction=Direction.Input, doc="Enter reference temperature in K")
        self.declareProperty(
            name="Calibration",
            defaultValue=False,
            direction=Direction.Input,
            doc="Calibration flag, default is False in which case temperature measured",
        )
        self.declareProperty(
            name="Debug",
            defaultValue=False,
            direction=Direction.Input,
            doc="True/False - provides more verbose output of procedure for debugging purposes",
        )
        self.declareProperty(
            name="EstimateBackground",
            defaultValue=True,
            direction=Direction.Input,
            doc="Estimate background parameters from data.",
        )

        self.declareProperty(
            name="Bg0guessFraction",
            defaultValue=0.84,
            direction=Direction.Input,
            doc="Starting guess for constant term of polynomial background is calculated as Bg0guessScaleFactor*y0 where"
            "y0 is the intensity in the first bin of the data to be fitted.",
        )
        self.declareProperty(
            name="Bg1guess",
            defaultValue=0.0173252,
            direction=Direction.Input,
            doc="Starting guess for linear term of polynomial background.",
        )
        self.declareProperty(
            name="Bg2guess",
            defaultValue=0.0000004,
            direction=Direction.Input,
            doc="Starting guess for quadratic term of polynomial background.",
        )
        enable_bg_params = EnabledWhenProperty("EstimateBackground", PropertyCriterion.IsNotDefault)
        for prop in ["Bg0guessFraction", "Bg1guess", "Bg2guess"]:
            self.setPropertySettings(prop, enable_bg_params)

    def validateFileInputs(self, filesList):
        # MultipleFileProperty returns a list of list(s) of files, or a list containing a single (string) file
        # Condense into one list of file(s) in either case
        returnList = []
        for files in filesList:
            if isinstance(files, str):
                returnList.append(files)
            else:
                returnList.extend(files)
        return returnList

    def PyExec(self):
        # ----------------------------------------------------------
        # Imports resonance parameters from ResParam dictionary depending on the selected foil type.
        # ----------------------------------------------------------
        files = self.getProperty("Files").value
        files = self.validateFileInputs(files)
        foilType = self.getProperty("FoilType").value
        divE = float(self.getProperty("Ediv").value)
        isCalib = self.getProperty("Calibration").value
        mass = self.ResParamsDict[foilType + "_Mass"]
        TD = self.ResParamsDict[foilType + "_TD"]
        # Energy parameters are in eV
        energy = self.ResParamsDict[foilType + "_En"]
        TwogG = self.ResParamsDict[foilType + "_TwogG"]
        Gg = self.ResParamsDict[foilType + "_Gg"]
        startE = self.ResParamsDict[foilType + "_startE"]
        endE = self.ResParamsDict[foilType + "_endE"]
        refTemp = float(self.getProperty("ReferenceTemp").value)
        isDebug = self.getProperty("Debug").value
        estimate_backround = self.getProperty("EstimateBackground").value

        if not isCalib:
            if "S_fit_Parameters" not in mtd:
                self.log().warning(
                    "No calibration files found. Please run this algorithm will 'Calibration' ticked to generate the calibration workspace."
                )
                return

        self.monitorTransfit(files, foilType, divE)
        file = files[0]
        discard, fileName = path.split(file)
        fnNoExt = path.splitext(fileName)[0]

        # Define the gaussian width at the reference temperature
        # Factor of 1e3 converting to meV for Mantid

        width_300 = 1000.0 * np.sqrt(4 * energy * self.k * refTemp * mass / (self.e * (1 + mass) ** 2))

        # ----------------------------------------------------------
        # Perform fits based on whether isCalib is flagged (calibration or measurement)
        # ----------------------------------------------------------
        if isCalib:
            lorentzFWHM = 1000.0 * (0.5 * TwogG + Gg)
            # Take peak position starting guess from tabulated value
            peakPosGuess = energy * 1000
            fileName_monitor = fnNoExt + "_monitor"
            # For guessing initial background values, read in y-data and use starting value as value for b0
            wsBgGuess = mtd[fileName_monitor]
            if estimate_backround:
                bg1guess, bg0guess = estimate_linear_background(wsBgGuess.readX(0), wsBgGuess.readY(0))
                bg2guess = 0.0
            else:
                bg0guess = self.getProperty("Bg0guessFraction").value * wsBgGuess.readY(0)[0]
                bg1guess = self.getProperty("Bg1guess").value
                bg2guess = self.getProperty("Bg2guess").value
            # New Voigt function as from Igor pro function
            Fit(
                Function="name=PEARLTransVoigt,Position="
                + str(peakPosGuess)
                + ",LorentzianFWHM="
                + str(lorentzFWHM)
                + ",GaussianFWHM="
                + str(width_300)
                + ",Amplitude=1.6,Bg0="
                + str(bg0guess)
                + ",Bg1="
                + str(bg1guess)
                + ",Bg2="
                + str(bg2guess)
                + ",constraints=(1<LorentzianFWHM,"
                + str(width_300)
                + "<GaussianFWHM)",
                InputWorkspace=fileName_monitor,
                MaxIterations=200,
                Output="S_fit",
            )
            #
            DeleteWorkspace("S_fit_NormalisedCovarianceMatrix")
            DeleteWorkspace(fileName_monitor)

        else:
            S_fit = mtd["S_fit_Parameters"]
            lorentzFWHM = S_fit.column(1)[1]
            gaussianFWHM = S_fit.column(1)[2]

            # Calculates the gaussian resolution contribution, sometimes constraints are broken and the value can drop
            # below that from width_300 alone, in which case the instrument contribution is set to zero.
            if gaussianFWHM > width_300:
                gaussianFWHM_inst = np.sqrt(gaussianFWHM**2 - width_300**2)
            else:
                gaussianFWHM_inst = 0
            #
            # New Voigt function as from Igor pro function
            Fit(
                Function="name=PEARLTransVoigt,Position="
                + str(S_fit.column(1)[0])
                + ",LorentzianFWHM="
                + str(lorentzFWHM)
                + ",GaussianFWHM="
                + str(gaussianFWHM)
                + ",Amplitude="
                + str(S_fit.column(1)[3])
                + ",Bg0="
                + str(S_fit.column(1)[4])
                + ",Bg1="
                + str(S_fit.column(1)[5])
                + ",Bg2="
                + str(S_fit.column(1)[6])
                + ",constraints=("
                + str(gaussianFWHM)
                + "<GaussianFWHM),ties=(LorentzianFWHM="
                + str(lorentzFWHM)
                + ")",
                InputWorkspace=fnNoExt + "_monitor",
                MaxIterations=200,
                Output="T_fit",
            )
            #
            DeleteWorkspace("T_fit_NormalisedCovarianceMatrix")
            DeleteWorkspace(fnNoExt + "_monitor")
            T_fit = mtd["T_fit_Parameters"]
            gaussian_FWHM_fitted = T_fit.column(1)[2]
            width_T = np.sqrt(gaussian_FWHM_fitted**2 - gaussianFWHM_inst**2)
            # Factor of 1e-3 converts back from meV to eV
            Teff = (((width_T * 1e-3) ** 2) * self.e * ((1 + mass) ** 2)) / (4 * 1e-3 * T_fit.column(1)[0] * self.k * mass)
            Teff_low = ((((width_T - T_fit.column(2)[2]) * 1e-3) ** 2) * self.e * ((1 + mass) ** 2)) / (
                4 * 1e-3 * (T_fit.column(1)[0] + T_fit.column(2)[0]) * self.k * mass
            )
            Teff_high = ((((width_T + T_fit.column(2)[2]) * 1e-3) ** 2) * self.e * ((1 + mass) ** 2)) / (
                4 * 1e-3 * (T_fit.column(1)[0] - T_fit.column(2)[0]) * self.k * mass
            )
            errTeff = 0.5 * (Teff_high - Teff_low)
            # ----------------------------------------------------------
            # If the temperature is too far below the Debye temperature, then the result is inaccurate. Else the
            # temperature is calculated assuming free gas formulation
            # ----------------------------------------------------------
            if 8 * Teff < 3 * TD:
                self.log().information(
                    "The effective temperature is currently too far below the Debye temperature togive an accurate measure."
                )
                Tactual = Teff
                Terror = errTeff
            else:
                Tactual = 3 * TD / (4 * np.log((8 * Teff + 3 * TD) / (8 * Teff - 3 * TD)))
                Tactual_high = 3 * TD / (4 * np.log((8 * (Teff + errTeff) + 3 * TD) / (8 * (Teff + errTeff) - 3 * TD)))
                Tactual_low = 3 * TD / (4 * np.log((8 * (Teff - errTeff) + 3 * TD) / (8 * (Teff - errTeff) - 3 * TD)))
                Terror = 0.5 * (np.abs(Tactual - Tactual_high) + np.abs(Tactual - Tactual_low))
            # Introduce a catchment for unphysically small determined errors, setting to defualt value if below a set
            # threshold
            Terror_flag = 0
            if Terror < 5.0:
                Terror_flag = 1
                Terror = 10.0
            if isDebug:
                self.log().information("-----------------------------")
                self.log().information("Debugging....")
                self.log().information("The Debye temperature is " + str(TD) + " K")
                self.log().information("The effective temperature is: {:.1f}".format(Teff) + "+/- {:.1f}".format(errTeff) + " K")
                self.log().information("Energy bin width set to " + str(1000 * divE) + " meV")
                self.log().information("E range is between " + str(startE) + " and " + str(endE))
                self.log().information("Gaussian width at this reference temperature is: {:.2f}".format(width_300) + " meV")
                self.log().information("Lorentzian FWHM is fixed: {:.2f}".format(lorentzFWHM) + " meV")
                self.log().information("Gaussian FWHM is fitted as: {:.2f}".format(gaussian_FWHM_fitted) + " meV")
                self.log().information("Instrumental contribution is: {:.2f}".format(gaussianFWHM_inst) + " meV")
                self.log().information("Temperature contribution is: {:.2f}".format(width_T) + " meV")
                self.log().information("-----------------------------")

            self.log().information("Sample temperature is: {:.1f}".format(Tactual) + " +/- {:.1f}".format(Terror) + " K")
            if Terror_flag == 1:
                self.log().information("(the default error, as determined error unphysically small)")

    # ----------------------------------------------------------
    # Define function for importing raw monitor data, summing, normalising, converting units, and cropping
    # ----------------------------------------------------------
    def monitorTransfit(self, files, foilType, divE):
        isFirstFile = True
        isSingleFile = len(files) == 1
        firstFileName = ""
        for file in files:
            discard, fileName = path.split(file)
            fnNoExt = path.splitext(fileName)[0]
            if isFirstFile:
                firstFileName = fnNoExt
            fileName_format = fnNoExt + "_" + fileName.split(".")[-1]
            fileName_3 = fnNoExt + "_3"
            Load(Filename=file, OutputWorkspace=fileName_format)
            CropWorkspace(InputWorkspace=fileName_format, OutputWorkspace=fileName_format, XMin=100, XMax=19990)
            NormaliseByCurrent(InputWorkspace=fileName_format, OutputWorkspace=fileName_format)
            ExtractSingleSpectrum(InputWorkspace=fileName_format, OutputWorkspace=fileName_3, WorkspaceIndex=3)
            DeleteWorkspace(fileName_format)
            ConvertUnits(InputWorkspace=fileName_3, Target="Energy", OutputWorkspace=fileName_3)
            self.TransfitRebin(fileName_3, fileName_3, foilType, divE)
            if not isFirstFile:
                Plus(LHSWorkspace=firstFileName + "_3", RHSWorkspace=fileName_3, OutputWorkspace=firstFileName + "_3")
                DeleteWorkspace(fileName_3)
            else:
                isFirstFile = False
        if isSingleFile:
            RenameWorkspace(InputWorkspace=firstFileName + "_3", OutputWorkspace=firstFileName + "_monitor")
        else:
            noFiles = len(files) ** (-1)
            CreateSingleValuedWorkspace(OutputWorkspace="scale", DataValue=noFiles)
            Multiply(LHSWorkspace=firstFileName + "_3", RHSWorkspace="scale", OutputWorkspace=firstFileName + "_monitor")
            DeleteWorkspace("scale")
            DeleteWorkspace(firstFileName + "_3")

    # ----------------------------------------------------------
    # Define function for improved rebinning of the data
    # ----------------------------------------------------------
    def TransfitRebin(self, inputWS, outputWSName, foilType, divE):
        ws2D = mtd[inputWS]
        # Expand the limits for rebinning to prevent potential issues at the boundaries
        startE = self.ResParamsDict[foilType + "_startE"]
        endE = self.ResParamsDict[foilType + "_endE"]
        startEp = 0.99 * startE
        endEp = 1.01 * endE
        CropWorkspace(InputWorkspace=ws2D, OutputWorkspace=ws2D, XMin=1000 * startEp, XMax=1000 * endEp)
        numPts = int(((endEp - startEp) / divE) + 1)
        xData_out = []
        xData_in = ws2D.readX(0)
        yData_in = ws2D.readY(0)
        # Deals with issues in Mantid where Xdata mark start of bin, not true x position
        # calculates the bin width and then takes middle of bin as x value
        current_bin_widths = []
        xActual = []
        for x in range(0, len(xData_in) - 1):
            current_bin_widths.append(xData_in[x + 1] - xData_in[x])
            xActual.append(xData_in[x] + 0.5 * (xData_in[x + 1] - xData_in[x]))
        # Make xData with uniform binning defined by divE
        for j in range(numPts):
            xData_out.append(1000 * (startEp + j * divE))
        yData_out = [0] * (len(xData_out))
        # Normalise output ydata accordingly based on surrounding values
        yNorm = [0] * (len(xData_out))
        for j in range(0, len(yData_in)):
            currentBin = int((xActual[j] - startEp * 1000) / (divE * 1000))
            scale1 = 1 - ((xActual[j] - xData_out[currentBin]) / (divE * 1000))
            yData_out[currentBin] += yData_in[j] * scale1
            yNorm[currentBin] += scale1
            if currentBin < (len(xData_out) - 1):
                yData_out[currentBin + 1] += yData_in[j] * (1 - scale1)
                yNorm[currentBin + 1] += 1 - scale1
        # Apply the normalisation, with a catch for any potential divide by zero errors
        for i in range(len(yData_out)):
            if yNorm[i] != 0:
                yData_out[i] = yData_out[i] / yNorm[i]
            else:
                print("Empty bin")

        outputWS = CreateWorkspace(DataX=xData_out, DataY=yData_out, NSpec=1, UnitX="meV")
        CropWorkspace(InputWorkspace=outputWS, OutputWorkspace=outputWS, XMin=1000 * startE, XMax=1000 * endE)
        RenameWorkspace(InputWorkspace=outputWS, OutputWorkspace=outputWSName)


def estimate_linear_background(x, y, nbg=3):
    if len(y) < 2 * nbg:
        # not expected as trying to fit a function with 7 parameters
        return 2 * [0.0]
    dy = y[:nbg].mean() - y[-nbg:].mean()
    dx = x[:nbg].mean() - x[-nbg:].mean()
    gradient = dy / dx
    slope = y[:nbg].mean() - gradient * x[:nbg].mean()
    return gradient, slope


AlgorithmFactory.subscribe(PEARLTransfit)
