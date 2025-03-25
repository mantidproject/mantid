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
from mantid.kernel import Direction, StringListValidator, EnabledWhenProperty, PropertyCriterion, LogicOperator
from mantid.api import PythonAlgorithm, MultipleFileProperty, AlgorithmFactory, AnalysisDataService as ADS
from scipy import constants
import numpy as np
from mantid.fitfunctions import FunctionWrapper
from typing import Sequence


class PEARLTransfit(PythonAlgorithm):
    # Resonance constants broadly consistent with those reported from
    # 'Neutron cross sections Vol 1, Resonance Parameters'
    # S.F. Mughabghab and D.I. Garber
    # June 1973, BNL report 325
    # Mass in atomic units, En in eV, temperatures in K, gamma factors in eV
    res_params_dict = {
        # Hf01
        "Hf01_Mass": 177.0,
        "Hf01_En": 1.098,
        "Hf01_TD": 252.0,
        "Hf01_TwogG": 0.00192,
        "Hf01_Gg": 0.0662,
        "Hf01_startE": 0.6,
        "Hf01_endE": 1.7,
        # Hf02
        "Hf02_Mass": 177.0,
        "Hf02_En": 2.388,
        "Hf02_TD": 252.0,
        "Hf02_TwogG": 0.009,
        "Hf02_Gg": 0.0608,
        "Hf02_startE": 2.0,
        "Hf02_endE": 2.7,
        # Ta10
        "Ta10_Mass": 181.0,
        "Ta10_En": 10.44,
        "Ta10_TD": 240.0,
        "Ta10_TwogG": 0.00335,
        "Ta10_Gg": 0.0069,
        "Ta10_startE": 9.6,
        "Ta10_endE": 11.4,
        # Irp6
        "Irp6_Mass": 191.0,
        "Irp6_En": 0.6528,
        "Irp6_TD": 420.0,
        "Irp6_TwogG": 0.000547,
        "Irp6_Gg": 0.072,
        "Irp6_startE": 0.1,
        "Irp6_endE": 0.9,
        # Iro5
        "Iro5_Mass": 191.0,
        "Iro5_En": 5.36,
        "Iro5_TD": 420.0,
        "Iro5_TwogG": 0.006,
        "Iro5_Gg": 0.082,
        "Iro5_startE": 4.9,
        "Iro5_endE": 6.3,
        # Iro9
        "Iro9_Mass": 191.0,
        "Iro9_En": 9.3,
        "Iro9_TD": 420.0,
        "Iro9_TwogG": 0.0031,
        "Iro9_Gg": 0.082,
        "Iro9_startE": 8.7,
        "Iro9_endE": 9.85,
    }

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
        self.declareProperty(name="Ediv", defaultValue=0.0025, direction=Direction.Input, doc="Energy bin-width in eV")
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
        self.declareProperty(
            name="RebinInEnergy",
            defaultValue=True,
            direction=Direction.Input,
            doc="Estimate background parameters from data.",
        )
        # disable properties
        is_cal = EnabledWhenProperty("Calibration", PropertyCriterion.IsNotDefault)
        is_not_cal = EnabledWhenProperty("Calibration", PropertyCriterion.IsDefault)
        self.setPropertySettings("Debug", is_not_cal)
        self.setPropertySettings("EstimateBackground", is_cal)
        is_bg_estimated = EnabledWhenProperty("EstimateBackground", PropertyCriterion.IsNotDefault)
        enable_user_bg = EnabledWhenProperty(is_bg_estimated, is_cal, LogicOperator.And)
        for prop in ["Bg0guessFraction", "Bg1guess", "Bg2guess"]:
            self.setPropertySettings(prop, enable_user_bg)
        enable_Ediv = EnabledWhenProperty("RebinInEnergy", PropertyCriterion.IsDefault)
        self.setPropertySettings("Ediv", enable_Ediv)

    def PyExec(self):
        # ----------------------------------------------------------
        # Imports resonance parameters from ResParam dictionary depending on the selected foil type.
        # ----------------------------------------------------------
        files = self.get_file_list(self.getProperty("Files").value)
        foil_type = self.getProperty("FoilType").value
        is_calib = self.getProperty("Calibration").value
        ref_temp = float(self.getProperty("ReferenceTemp").value)
        is_debug = self.getProperty("Debug").value
        estimate_background = self.getProperty("EstimateBackground").value

        if not is_calib:
            if not ADS.doesExist("S_fit_Parameters"):
                self.log().warning(
                    "No calibration files found. Please run this algorithm will 'Calibration' ticked to generate the calibration workspace."
                )
                return

        ws = self.load_and_average_moinitor_from_files(files, foil_type)
        if self.getProperty("RebinInEnergy").value:
            # Rebin with constant width has similar effect to normalising raw data by bin-width
            # so hack here is to pre-scale intensities by inverse (i.e. multiply by bin-width)
            # this means that Rebin is effectively performing a mean rather than sum
            ws.setDistribution(True)
            self.exec_child_alg("ConvertFromDistribution", Workspace=ws)
            bin_width = 1000 * float(self.getProperty("Ediv").value)  # meV
            ws = self.exec_child_alg("Rebin", InputWorkspace=ws, Params=f"{bin_width}", FullBinsOnly=True)

        # Define the gaussian width at the reference temperature
        # Factor of 1e3 converting to meV for Mantid
        energy = self.res_params_dict[foil_type + "_En"]
        mass = self.res_params_dict[foil_type + "_Mass"]
        gauss_fwhm_ref_temp = 1000.0 * np.sqrt(4 * energy * constants.k * ref_temp * mass / (constants.e * (1 + mass) ** 2))

        # make initial function with constraints
        func = FunctionWrapper("PEARLTransVoigt")
        func.constrain("LorentzianFWHM>1")
        func.constrain(f"GaussianFWHM>{gauss_fwhm_ref_temp}")

        # set initial parameters
        if is_calib:
            # use tabulated values (note energies are in eV)
            out_ws_name = "S_fit"
            func["Amplitude"] = 1.6
            two_gGamma = self.res_params_dict[foil_type + "_TwogG"]
            gamma_g = self.res_params_dict[foil_type + "_Gg"]
            func["LorentzianFWHM"] = 1000.0 * (0.5 * two_gGamma + gamma_g)
            func["GaussianFWHM"] = gauss_fwhm_ref_temp
            func["Position"] = energy * 1000  # take peak position starting guess from tabulated value
            bg_pars = np.zeros(3)
            if estimate_background:
                bg_pars[:2] = self.estimate_linear_background(ws.readX(0), ws.readY(0))
            else:
                bg_pars[0] = self.getProperty("Bg0guessFraction").value * ws.readY(0)[0]
                bg_pars[1] = self.getProperty("Bg1guess").value
                bg_pars[2] = self.getProperty("Bg2guess").value
            for ipar, par in enumerate(bg_pars):
                func[f"Bg{ipar}"] = par
        else:
            out_ws_name = "T_fit"
            # retrieve parameters from previous calibration
            calib_param_table = ADS.retrieve("S_fit_Parameters")
            for ipar in range(func.nParams()):
                row = calib_param_table.row(ipar)
                func[row["Name"]] = row["Value"]

        fit_kwargs = {
            "InputWorkspace": ws,
            "MaxIterations": 200,
            "CreateOutput": True,
            "Output": out_ws_name,
            "IgnoreInvalidData": True,
            "StepSizeMethod": "Sqrt epsilon",
        }
        if is_calib:
            # perform initial fit keeping Gaussian width constant
            func.fix("GaussianFWHM")
            fit_res = self.exec_fit(Function=func.fun, **fit_kwargs)
            func = fit_res["Function"]
            func.untie("GaussianFWHM")
        else:
            func.fix("LorentzianFWHM")
        fit_res = self.exec_fit(Function=func.fun, **fit_kwargs)

        if fit_res["OutputStatus"] != "success":
            self.log().warning(f"Fit failed with status: {fit_res['OutputStatus']}")

        if not is_calib:
            # calculate effective temperature using fitted widths
            # get instrument contribution to Gausssian width in calibration
            if func["GaussianFWHM"] > gauss_fwhm_ref_temp:
                # get fwhm d width of resolution (add in quadrature for convolution of Gaussians)
                gauss_fwhm_inst = np.sqrt(func["GaussianFWHM"] ** 2 - gauss_fwhm_ref_temp**2)
            else:
                gauss_fwhm_inst = 0  # shouldn't be the case due to calibration fit constraint
            final_func = fit_res["Function"]
            fwhm = np.sqrt(final_func["GaussianFWHM"] ** 2 - gauss_fwhm_inst**2)
            fwhm_err = final_func.fun.getError("GaussianFWHM")
            energy = final_func["Position"]
            energy_err = final_func.fun.getError("Position")
            # get element from correlation matrix (called normalised covar in mantid)
            irow = final_func.fun.getParameterIndex("Position")
            corr = fit_res["OutputNormalisedCovarianceMatrix"].row(irow)["GaussianFWHM"]
            temp_eff, temp_eff_err = self.calc_effective_temp_and_error(fwhm, fwhm_err, energy, energy_err, corr, mass)
            # calculate sample temp using ideal gas formulation
            temp_debye = self.res_params_dict[foil_type + "_TD"]  # Debye temp.
            temp_sample, temp_sample_err = self.calc_sample_temp_and_error_from_effective(temp_eff, temp_eff_err, temp_debye)

            # print info
            self.log().information(f"Sample temperature is: {temp_sample:.2f} +/- {temp_sample_err:.2f} K")
            if is_debug:
                self.log().information("-----------------------------")
                self.log().information(f"The Debye temperature is {temp_debye:.2f} K")
                self.log().information(f"The effective temperature is: {temp_eff:.2f} +/- {temp_eff_err:.2f} K")
                self.log().information(f"Fitted in range {ws.readX(0)[0]:.2f} < Energy (meV) < {ws.readX(0)[-1]:.2f}")
                self.log().information(f"Gaussian width at this reference temperature is: {gauss_fwhm_ref_temp:.2f} meV")
                self.log().information(f"Lorentzian FWHM is fixed: {final_func['LorentzianFWHM']:.2f} meV")
                self.log().information(f"Gaussian FWHM is fitted as: {final_func['GaussianFWHM']:.2f} meV")
                self.log().information(f"Instrumental contribution is: {gauss_fwhm_inst:.2f} meV")
                self.log().information(f"Temperature contribution is: {fwhm:.2f} meV")
                self.log().information("-----------------------------")

        # clean up ADS
        self.exec_child_alg("DeleteWorkspace", Workspace=fit_res["OutputNormalisedCovarianceMatrix"])

    @staticmethod
    def get_file_list(input_files):
        # MultipleFileProperty returns a list of list(s) of files, or a list containing a single (string) file
        # Condense into one list of file(s) in either case
        file_list = []
        for files in input_files:
            if isinstance(files, str):
                file_list.append(files)
            else:
                file_list.extend(files)
        return file_list

    def exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props

    def exec_fit(self, **kwargs):
        alg = self.createChildAlgorithm("Fit", enableLogging=False)
        alg.setAlwaysStoreInADS(True)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        return {
            "Function": FunctionWrapper(alg.getProperty("Function").value),
            "OutputStatus": alg.getProperty("OutputStatus").value,
            "OutputNormalisedCovarianceMatrix": ADS.retrieve(alg.getPropertyValue("OutputNormalisedCovarianceMatrix")),
        }

    def calc_effective_temp_and_error(
        self, fwhm: float, fwhm_err: float, energy: float, energy_err: float, correlation: float, mass: float
    ) -> (float, float):
        const = ((1e-3) * constants.e * ((1 + mass) ** 2)) / (4 * constants.k * mass)
        temp = const * (fwhm**2) / energy
        # propagate errors
        dtemp_by_dfwhm = 2 * fwhm / energy
        dtemp_by_denergy = -((fwhm / energy) ** 2)
        # calc. term due to covariance
        # corr = 100*cij/sqrt(cii*cjj); cii = error_i^2
        covar = (correlation * fwhm_err * energy_err) / 100
        covar_term = 2 * dtemp_by_dfwhm * dtemp_by_denergy * covar
        temp_err = const * np.sqrt((dtemp_by_dfwhm * fwhm_err) ** 2 + (dtemp_by_denergy * energy_err) ** 2 + covar_term)
        return temp, temp_err

    def calc_sample_temp_and_error_from_effective(self, temp_eff: float, temp_eff_err: float, temp_debye: float) -> (float, float):
        if 8 * temp_eff < 3 * temp_debye:
            self.log().warning("The effective temperature is currently too far below the Debye temperature to give an accurate measure.")
            return temp_eff, temp_eff_err
        # use free gas formulation
        log_term = np.log((8 * temp_eff + 3 * temp_debye) / (8 * temp_eff - 3 * temp_debye))
        temp_sample = 3 * temp_debye / (4 * log_term)
        # calculate error using derivative
        temp_sample_err = abs((36 * temp_debye**2) / ((9 * temp_debye**2 - 64 * temp_eff**2) * log_term**2)) * temp_eff_err
        return temp_sample, temp_sample_err

    # ----------------------------------------------------------
    # Define function for importing raw monitor data, summing, normalising, converting units, and cropping
    # ----------------------------------------------------------
    def _load_monitor_from_single_file(self, filepath: str):
        ws, *_ = self.exec_child_alg("Load", Filename=filepath)
        ws = self.exec_child_alg("ExtractSingleSpectrum", InputWorkspace=ws, WorkspaceIndex=3)
        ws = self.exec_child_alg("CropWorkspace", InputWorkspace=ws, XMin=100, XMax=19990)
        ws = self.exec_child_alg("NormaliseByCurrent", InputWorkspace=ws)
        ws = self.exec_child_alg("ConvertUnits", InputWorkspace=ws, Target="Energy")
        return ws

    def load_and_average_moinitor_from_files(self, filepaths: Sequence[str], foil_type: str):
        ws = self._load_monitor_from_single_file(filepaths[0])
        for filepath in filepaths[1:]:
            ws = ws + self._load_monitor_from_single_file(filepath)
        if len(filepaths) > 1:
            ws = ws / len(filepaths)
        # crop
        xstart = 1000 * self.res_params_dict[foil_type + "_startE"]
        xend = 1000 * self.res_params_dict[foil_type + "_endE"]
        ws = self.exec_child_alg("CropWorkspaceRagged", InputWorkspace=ws, XMin=xstart, XMax=xend)
        return ws

    @staticmethod
    def estimate_linear_background(x: np.ndarray, y: np.ndarray, nbg: int = 3) -> (float, float):
        if len(y) < 2 * nbg:
            # not expected as trying to fit a function with 7 parameters
            return 2 * [0.0]
        dy = y[:nbg].mean() - y[-nbg:].mean()
        dx = x[:nbg].mean() - x[-nbg:].mean()
        gradient = dy / dx
        slope = y[:nbg].mean() - gradient * x[:nbg].mean()
        return gradient, slope


AlgorithmFactory.subscribe(PEARLTransfit)
