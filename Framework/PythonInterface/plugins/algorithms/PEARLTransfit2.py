# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Transfit v2 - translated from Fortran77 to Python/Mantid
Original Author: Christopher J Ridley
Last updated: 2nd October 2020

Program fits a Voigt function to PEARL transmission data, using the doppler broadening
of the gaussian component to determine the sample temperature


- References:
    - 'Temperature measurement in a Paris-Edinburgh cell by neutron resonance spectroscopy' - Journal Of Applied Physics 98,
       064905 (2005).'
    - 'Remote determination of sample temperature by neutron resonance spectroscopy' - Nuclear Instruments and Methods in
       Physics Research A 547 (2005) 601-615.'
"""

from mantid.kernel import Direction, StringListValidator, EnabledWhenProperty, PropertyCriterion
from mantid.api import (
    PythonAlgorithm,
    MultipleFileProperty,
    AlgorithmFactory,
    ITableWorkspaceProperty,
    ITableWorkspace,
    WorkspaceFactory,
    PropertyMode,
    MatrixWorkspaceProperty,
)
from scipy import constants
import numpy as np
from mantid.fitfunctions import FunctionWrapper
from typing import Sequence, TYPE_CHECKING
from dataclasses import dataclass

if TYPE_CHECKING:
    from mantid.api import MatrixWorkspace, IFunction1D

"""
 Resonance constants broadly consistent with those reported from
 'Neutron cross sections Vol 1, Resonance Parameters'
 S.F. Mughabghab and D.I. Garber
 June 1973, BNL report 325
 Mass in atomic units, En in eV, temperatures in K, gamma factors in eV
"""
RES_PARAMS = {
    #       (mass,  En,  TD,  TwogG,  Gg,  startE,  endE)
    "Hf01": (177.0, 1.098, 252.0, 0.00192, 0.0662, 0.6, 1.7),
    "Hf02": (177.0, 2.388, 252.0, 0.009, 0.0608, 2.0, 2.7),
    "Ta10": (181.0, 10.44, 240.0, 0.00335, 0.0069, 9.6, 11.4),
    "Irp6": (191.0, 0.6528, 420.0, 0.000547, 0.072, 0.1, 0.9),
    "Iro5": (191.0, 5.36, 420.0, 0.006, 0.082, 4.9, 6.3),
    "Iro9": (191.0, 9.3, 420.0, 0.0031, 0.082, 8.7, 9.85),
}
TABLE_COLUMN_NAME = ["Name", "Value", "Error"]
TABLE_COLUMN_TYPES = ["str", *["float"] * 2]
FIT_TABLE_PARAMS = ["Position", "LorentzianFWHM", "GaussianFWHM", "Amplitude", "Bg0", "Bg1", "Bg2"]
eV_TO_meV: float = 1000.0


@dataclass(frozen=True)
class FoilParameters:
    Foil: str
    Mass: float
    En: float
    TD: float
    TwogG: float
    Gg: float
    StartE: float
    EndE: float


class PEARLTransfit(PythonAlgorithm):
    x_range: tuple[float, float] = (100, 19990)
    trans_monitor_index: int = 3
    res_params: FoilParameters = None
    default_peak_amplitude: float = 1.6
    is_calib: bool = True
    gauss_fwhm_ref_temp: float = 0.0
    run_numbers: list[str] = None

    def version(self):
        return 2

    def name(self):
        return "PEARLTransfit"

    def category(self):
        return "Diffraction\\Fitting"

    def summary(self):
        return (
            " Version 2 of PEARLTransfit. Reads high-energy neutron resonances from the downstream monitor data on the PEARL instrument,"
            " then fits a Voigt function to them to determine the sample temperature. A calibration must be run for"
            " each sample pressure. Can be used on a single file, or multiple files, in which case workspaces"
            " are summed and the average taken."
        )

    def PyInit(self):
        self.declareProperty(
            MultipleFileProperty("Files", extensions=[".raw", ".s0x", ".nxs"]),
            doc="File paths or comma separated run numbers for target runs (numors). Must be detector scans.",
        )

        self.declareProperty(
            name="FoilType",
            defaultValue="Hf01",
            validator=StringListValidator(["Hf01", "Hf02", "Ta10", "Irp6", "Iro5", "Iro9"]),
            direction=Direction.Input,
            doc="Type of foil included with the sample",
        )

        self.declareProperty(
            name="Calibration",
            defaultValue=False,
            direction=Direction.Input,
            doc="Calibration flag, default is False in which case temperature measured",
        )

        self.declareProperty(
            ITableWorkspaceProperty(
                name="InputCalibrationParameters", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional
            ),
            doc="Table with fit parameters for 'PEARLTransVoigt' function: 'Position', 'LorentzianFWHM', 'GaussianFWHM', 'Amplitude', 'Bg0'"
            "'Bg1', 'Bg2'. The calculated temperature is added in non-calibration runs. This property is mandatory when calibration"
            " is set to False. If a 'InputCalibrationParameters' is used in calibration runs: 'Bg0', 'Bg1' and 'Bg2' will "
            " be used for background estimation",
        )

        self.declareProperty(name="ReferenceTemp", defaultValue=290.0, direction=Direction.Input, doc="Enter reference temperature in K")

        self.declareProperty(
            name="EstimateBackground",
            defaultValue=True,
            direction=Direction.Input,
            doc="Estimate background parameters from data.",
        )

        self.declareProperty(
            name="CreateDebugTable",
            defaultValue=False,
            direction=Direction.Input,
            doc="Create an output table with debug parameters in non calibration runs",
        )

        self.declareProperty(
            name="Output",
            defaultValue="",
            direction=Direction.Input,
            doc="Output basename on ADS. If empty, `S_fit` will be the basename for calibration runs and `T_fit` for non-calibration, "
            "adding"
            " the foil type and run numbers as suffixes.",
        )

        self.setup_property_rules()

    def validateInputs(self):
        issues = dict()
        is_calib = self.getProperty("Calibration").value
        fit_table = self.getProperty("InputCalibrationParameters").value
        if not (is_calib or fit_table):
            issues["InputCalibrationParameters"] = "InputCalibrationParameters is missing."
        elif fit_table:
            if "Name" not in fit_table.getColumnNames():
                issues["InputCalibrationParameters"] = "'Name' column for parameter names is missing in InputCalibrationParameters."
            else:
                parameters = fit_table.column("Name")
                missing = []
                # we only need bg info from table if it is a calibration run
                fit_params = FIT_TABLE_PARAMS if not is_calib else FIT_TABLE_PARAMS[-3:]
                for param in fit_params:
                    if param not in parameters:
                        missing.append(f"Parameter {param} missing from InputCalibrationParameters.")
                if missing:
                    issues["InputCalibrationParameters"] = "\n".join(missing)
        return issues

    def setup_property_rules(self):
        is_cal = EnabledWhenProperty("Calibration", PropertyCriterion.IsNotDefault)
        is_not_cal = EnabledWhenProperty("Calibration", PropertyCriterion.IsDefault)

        self.setPropertySettings("EstimateBackground", is_cal)
        self.setPropertySettings("CreateDebugTable", is_not_cal)

    def PyExec(self):
        files = self.get_file_list(self.getProperty("Files").value)
        self.run_numbers = []

        # Imports resonance parameters from ResParam dictionary depending on the selected foil type.
        foil_type = self.getProperty("FoilType").value
        self.res_params = FoilParameters(foil_type, *RES_PARAMS[foil_type])
        self.is_calib = self.getProperty("Calibration").value

        ref_temp = self.getProperty("ReferenceTemp").value
        fit_table = self.getProperty("InputCalibrationParameters").value
        estimate_background = self.getProperty("EstimateBackground").value or not fit_table

        ws = self._load_and_average_monitors_from_files(files)
        # Define the gaussian width at the reference temperature
        self.gauss_fwhm_ref_temp = eV_TO_meV * np.sqrt(
            4 * self.res_params.En * constants.k * ref_temp * self.res_params.Mass / (constants.e * (1 + self.res_params.Mass) ** 2)
        )

        func = self._prepare_fit_func(ws, fit_table, estimate_background)
        fit_res = self._fit(ws, func)
        debug_table = self._create_debug_info(fit_res, ws, func["GaussianFWHM"]) if not self.is_calib else None

        self._prepare_outputs(fit_res["OutputWorkspace"], fit_res["OutputTable"], debug_table)

    def _prepare_fit_func(self, ws: "MatrixWorkspace", fit_table: ITableWorkspace, estimate_background: bool) -> "IFunction1D":
        # make initial function with constraints
        func = FunctionWrapper("PEARLTransVoigt")
        func.constrain("LorentzianFWHM>1")
        func.constrain(f"GaussianFWHM>{self.gauss_fwhm_ref_temp}")

        # set initial parameters
        if self.is_calib:
            # use tabulated values (note energies are in eV)
            func["Amplitude"] = self.default_peak_amplitude
            func["LorentzianFWHM"] = eV_TO_meV * (0.5 * self.res_params.TwogG + self.res_params.Gg)
            func["GaussianFWHM"] = self.gauss_fwhm_ref_temp
            func["Position"] = eV_TO_meV * self.res_params.En  # take peak position starting guess from tabulated value
            bg_pars = np.zeros(3)
            if estimate_background:
                bg_pars[:2] = self.estimate_linear_background(ws.readX(0), ws.readY(0))
            else:
                param_names = fit_table.column(0)
                bg_pars[0] = fit_table.row(param_names.index("Bg0"))["Value"]
                bg_pars[1] = fit_table.row(param_names.index("Bg1"))["Value"]
                bg_pars[2] = fit_table.row(param_names.index("Bg2"))["Value"]
            for i_par, par in enumerate(bg_pars):
                func[f"Bg{i_par}"] = par
        else:
            # retrieve parameters from previous calibration
            for i_par in range(func.nParams()):
                row = fit_table.row(i_par)
                if (name := row["Name"]) in FIT_TABLE_PARAMS:
                    func[name] = row["Value"]

        return func

    def _create_output_basename(self):
        run_numbers = ""
        if self.run_numbers:
            run_numbers = f"_{self.run_numbers[0]}" if len(self.run_numbers) == 1 else f"_{self.run_numbers[0]}_{self.run_numbers[-1]}"
        suffix = f"_{self.res_params.Foil}{run_numbers}"
        self.setPropertyValue("Output", "S_fit" + suffix if self.is_calib else "T_fit" + suffix)

    def _prepare_outputs(self, ws: "MatrixWorkspace", table: ITableWorkspace, debug_table: ITableWorkspace | None):
        if self.getProperty("Output").isDefault:
            self._create_output_basename()
        out_name = self.getPropertyValue("Output")
        prop_names = ["OutputWorkspace", "OutputFitParameters", "OutputDebugTable"]
        prop_callers = [MatrixWorkspaceProperty, ITableWorkspaceProperty, ITableWorkspaceProperty]
        docs = ["Fitted Calibration Run.", "Fitted Calibration Parameters.", "Debug Parameters table."]
        suffixes = ["_Workspace", "_Parameters", "_DebugParameters"]
        data_items = [ws, table, debug_table]

        p_len = len(prop_names)
        props_len = p_len if debug_table else p_len - 1
        for idx in range(props_len):
            if not self.existsProperty(propName := prop_names[idx]):
                self.declareProperty(prop_callers[idx](name=propName, defaultValue="", direction=Direction.Output), doc=docs[idx])
            self.setPropertyValue(prop_names[idx], f"{out_name}{suffixes[idx]}")
            self.setProperty(prop_names[idx], data_items[idx])

    def _fit(self, ws: "MatrixWorkspace", func: "IFunction1D"):
        fit_kwargs = {
            "InputWorkspace": ws,
            "MaxIterations": 200,
            "CreateOutput": True,
            "Output": "fit_out",
            "IgnoreInvalidData": True,
            "StepSizeMethod": "Sqrt epsilon",
        }

        if self.is_calib:
            # perform initial fit keeping Gaussian width constant
            func.fix("GaussianFWHM")
            fit_res = self._exec_fit(Function=func.fun, **fit_kwargs)
            func = fit_res["Function"]
            func.untie("GaussianFWHM")
        else:
            func.fix("LorentzianFWHM")

        # Execute fit
        fit_res = self._exec_fit(Function=func.fun, **fit_kwargs)

        if fit_res["OutputStatus"] != "success":
            self.log().warning(f"Fit failed with status: {fit_res['OutputStatus']}")
        return fit_res

    def _exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props

    def _exec_fit(self, **kwargs):
        alg = self.createChildAlgorithm("Fit", enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        return {
            "Function": FunctionWrapper(alg.getProperty("Function").value),
            "OutputWorkspace": alg.getProperty("OutputWorkspace").value,
            "OutputTable": alg.getProperty("OutputParameters").value,
            "OutputStatus": alg.getProperty("OutputStatus").value,
            "OutputNormalisedCovarianceMatrix": alg.getProperty("OutputNormalisedCovarianceMatrix").value,
        }

    def _load_monitor_from_single_file(self, filepath: str):
        """
        Imports raw monitor data; extracts monitor spectrum and crops it.  Normalise by current and convert units to energy.
        :param filepath: file with raw data
        :return: workspace
        """
        ws, *_ = self._exec_child_alg("Load", Filename=filepath)
        ws = self._exec_child_alg("ExtractSingleSpectrum", InputWorkspace=ws, WorkspaceIndex=self.trans_monitor_index)
        ws = self._exec_child_alg("CropWorkspace", InputWorkspace=ws, XMin=self.x_range[0], XMax=self.x_range[1])
        ws = self._exec_child_alg("NormaliseByCurrent", InputWorkspace=ws)
        ws = self._exec_child_alg("ConvertUnits", InputWorkspace=ws, Target="Energy")

        if (runno := ws.getRunNumber()) and str(runno) not in self.run_numbers:
            self.run_numbers.append(str(runno))
        return ws

    def _load_and_average_monitors_from_files(self, filepaths: Sequence[str]):
        ws = self._load_monitor_from_single_file(filepaths[0])
        for filepath in filepaths[1:]:
            ws = ws + self._load_monitor_from_single_file(filepath)
        if len(filepaths) > 1:
            ws = ws / len(filepaths)
        # crop
        x_start = eV_TO_meV * self.res_params.StartE
        x_end = eV_TO_meV * self.res_params.EndE
        ws = self._exec_child_alg("CropWorkspaceRagged", InputWorkspace=ws, XMin=x_start, XMax=x_end)
        ws.setDistribution(True)
        return ws

    def _create_debug_info(self, fit_res: dict[str, any], ws: "MatrixWorkspace", gauss_fwhm: float):
        """
        Calculates effective temperature using fitted widths
        Get instrument contribution to Gausssian width in calibration
        """

        # get fwhm d width of resolution (add in quadrature for convolution of Gaussians), 0 shouldn't be the case due to fit constraint
        gauss_fwhm_inst = np.sqrt(gauss_fwhm**2 - self.gauss_fwhm_ref_temp**2) if gauss_fwhm > self.gauss_fwhm_ref_temp else 0.0
        final_func = fit_res["Function"]
        fwhm = np.sqrt(final_func["GaussianFWHM"] ** 2 - gauss_fwhm_inst**2)
        fwhm_err = final_func.fun.getError("GaussianFWHM")
        energy = final_func["Position"]
        energy_err = final_func.fun.getError("Position")
        # get element from correlation matrix (called normalised covar in mantid)
        irow = final_func.fun.getParameterIndex("Position")
        corr = fit_res["OutputNormalisedCovarianceMatrix"].row(irow)["GaussianFWHM"]
        temp_eff, temp_eff_err = self._calc_effective_temp_and_error(fwhm, fwhm_err, energy, energy_err, corr, self.res_params.Mass)
        # calculate sample temp using ideal gas formulation
        temp_debye = self.res_params.TD  # Debye temp.
        temp_sample, temp_sample_err = self._calc_sample_temp_and_error_from_effective(temp_eff, temp_eff_err, temp_debye)
        self.log().information(f"Sample temperature is: {temp_sample:.2f} +/- {temp_sample_err:.2f} K")
        fit_res["OutputTable"].addRow({"Name": "Sample Temperature", "Value": temp_sample, "Error": temp_sample_err})

        debug_table = None
        if self.getProperty("CreateDebugTable").value:
            # Generate Debug Table
            debug_params = {
                "Debye Temp. (K)": temp_debye,
                "Eff. Temp. (K)": (temp_eff, temp_eff_err),
                "Fit. Min. (meV)": ws.readX(0)[0],
                "Fit. Max. (meV)": ws.readX(0)[-1],
                "Gaussian Width at Reference Temp (meV)": self.gauss_fwhm_ref_temp,
                "Instrumental contribution (meV)": gauss_fwhm_inst,
                "Temperature Contribution (meV)": fwhm,
                "Sample Temperature (K)": (temp_sample, temp_sample_err),
            }
            debug_table = self.generate_table(debug_params)
        return debug_table

    def _calc_effective_temp_and_error(
        self, fwhm: float, fwhm_err: float, energy: float, energy_err: float, correlation: float, mass: float
    ) -> tuple[float, float]:
        const = ((1 / eV_TO_meV) * constants.e * ((1 + mass) ** 2)) / (4 * constants.k * mass)
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

    def _calc_sample_temp_and_error_from_effective(self, temp_eff: float, temp_eff_err: float, temp_debye: float) -> tuple[float, float]:
        if 8 * temp_eff < 3 * temp_debye:
            self.log().warning("The effective temperature is currently too far below the Debye temperature to give an accurate measure.")
            return temp_eff, temp_eff_err
        # use free gas formulation
        log_term = np.log((8 * temp_eff + 3 * temp_debye) / (8 * temp_eff - 3 * temp_debye))
        temp_sample = 3 * temp_debye / (4 * log_term)
        # calculate error using derivative
        temp_sample_err = abs((36 * temp_debye**2) / ((9 * temp_debye**2 - 64 * temp_eff**2) * log_term**2)) * temp_eff_err
        return temp_sample, temp_sample_err

    @staticmethod
    def estimate_linear_background(x: np.ndarray, y: np.ndarray, nbg: int = 3) -> tuple[float, float]:
        if len(y) < 2 * nbg:
            # not expected as trying to fit a function with 7 parameters
            return 2 * (0.0,)
        dy = y[:nbg].mean() - y[-nbg:].mean()
        dx = x[:nbg].mean() - x[-nbg:].mean()
        gradient = dy / dx
        slope = y[:nbg].mean() - gradient * x[:nbg].mean()
        return gradient, slope

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

    @staticmethod
    def generate_table(params_dict: dict[str, any]) -> ITableWorkspace:
        table = WorkspaceFactory.Instance().createTable()
        for n, t in zip(TABLE_COLUMN_NAME, TABLE_COLUMN_TYPES):
            table.addColumn(name=n, type=t)
        for k, v in params_dict.items():
            if isinstance(v, tuple):
                d = {"Name": k, "Value": v[0], "Error": v[1]}
            else:
                d = {"Name": k, "Value": v, "Error": 0}
            table.addRow(d)
        return table


AlgorithmFactory.subscribe(PEARLTransfit)
