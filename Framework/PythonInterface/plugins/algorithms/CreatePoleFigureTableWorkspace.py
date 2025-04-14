# import mantid algorithms, numpy and matplotlib
import numpy as np
from mantid.kernel import V3D
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, ITableWorkspaceProperty, PythonAlgorithm, PropertyMode
from mantid.kernel import Direction, FloatBoundedValidator
from mantid.geometry import ReflectionGenerator


class CreatePoleFigureTableWorkspace(PythonAlgorithm):
    def category(self):
        return "Diffraction\\Engineering"

    def name(self):
        return "CreatePoleFigureTableWorkspace"

    def summary(self):
        return "Creates a table storing the pole figure data extracted from the input workspaces."

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", defaultValue="", direction=Direction.Input),
            doc="The workspace containing the input data.",
        )
        self.declareProperty(
            ITableWorkspaceProperty("PeakParameterWorkspace", "", Direction.Input, PropertyMode.Optional),
            doc="Optional workspace containing the fitted peak data.",
        )
        self.declareProperty(
            ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output),
            doc="Output workspace containing the table of alphas, betas and intensities.",
        )
        self.declareProperty(
            name="Reflection",
            defaultValue="",
            direction=Direction.Input,
            doc="Reflection number for peak being fit. If given, algorithm will attempt to use the CrystalStructure on "
            "the InputWorkspace to adjust intensity for scattering power.",
        )
        positive_float_validator = FloatBoundedValidator(lower=0.0)
        self.declareProperty(
            "Chi2Threshold",
            defaultValue=2.0,
            direction=Direction.Input,
            validator=positive_float_validator,
            doc="Minimum chi^2 value to include in table. If set to 0.0, no chi^2 filtering will be performed.",
        )
        self.declareProperty(
            "PeakPositionThreshold",
            defaultValue=0.1,
            direction=Direction.Input,
            validator=positive_float_validator,
            doc="Maximum distance of fitted peak X0 from HKL X0 value to include in table."
            "If set to 0.0, no positional filtering will be performed. "
            "If no Reflection is specified, this tolerance can be ignored, "
            "otherwise it will assume X0 in PeakParameterWorkspace is given in dSpacing.",
        )

    def validateInputs(self):
        issues = dict()
        ws = self.getProperty("InputWorkspace").value

        # validate the input ws exists
        if ws is None:
            issues["InputWorkspace"] = "InputWorkspace must be set"

        # check peak parameter ws
        peak_ws = self.getProperty("PeakParameterWorkspace").value
        chi2_thresh = self.getProperty("Chi2Threshold").value
        # if the chi2_threshold is set to allow all peaks, no chi2 column is needed
        self.no_chi2_needed = chi2_thresh < 1e-6
        if peak_ws:

            def check_col(col_name):
                try:
                    peak_ws.column(col_name)
                except RuntimeError:
                    issues["PeakParameterWorkspace"] = f"PeakParameterWorkspace must have column: '{col_name}'"

            # if peak parameter ws is given, expect these columns
            check_col("I")
            check_col("X0")

            if not self.no_chi2_needed:
                check_col("chi2")

        # check that if a reflection is given it can be parsed to V3D
        if not self.getProperty("Reflection").isDefault:
            try:
                _parse_hkl(self.getProperty("Reflection").value)
            except:
                issues["Reflection"] = "Problem parsing hkl string, ensure H,K,L are separated by commas"

            # if a hkl is given, scattering power will be calculated, check that both a sample and CrystalStructure have
            # been defined
            try:
                ws.sample().getShape().volume()  # python api hasShape doesn't work as expected
                # this will check if there is a valid sample
                if not ws.sample().hasCrystalStructure():
                    issues["InputWorkspace"] = "If reflection is specified: InputWorkspace sample must have a CrystalStructure"
            except RuntimeError:
                issues["InputWorkspace"] = "If reflection is specified: InputWorkspace must have a sample"
        return issues

    def PyExec(self):
        # get inputs
        ws = self.getProperty("InputWorkspace").value
        peak_param_ws = self.getProperty("PeakParameterWorkspace").value
        hkl = self.getProperty("Reflection").value
        hkl_is_set = not self.getProperty("Reflection").isDefault
        chi_thresh = self.getProperty("Chi2Threshold").value
        x0_thresh = self.getProperty("PeakPositionThreshold").value

        # generate a detector table to get detector positions
        det_table = self.exec_child_alg(
            "CreateDetectorTable", InputWorkspace=ws, IncludeDetectorPosition=True, DetectorTableWorkspace="det_table"
        )
        det_pos = np.asarray(det_table.column("Position"))

        # get the source pos for ki
        source_pos = np.asarray(ws.getInstrument().getSource().getPos())

        # get the rotation matrix of the sample
        rot_mat = ws.run().getGoniometer().getR()

        # matrix to put detectors in sample frame
        to_pole_view = np.linalg.inv(rot_mat)

        # get Qs
        qds = det_pos / np.linalg.norm(det_pos, axis=1)[:, None]
        # normalise source pos then take away from sample pos for ki
        qi = np.zeros(3) - source_pos / np.linalg.norm(source_pos)
        Qs = qds - qi
        Qs = Qs / np.linalg.norm(Qs, axis=1)[:, None]

        # put detectors in sample reference frame
        sample_frame_Qs = to_pole_view @ Qs.T

        # calculate alpha and beta for the Qs in the sample ref frame
        ab_arr = _get_alpha_beta_from_cart(sample_frame_Qs)

        if peak_param_ws:
            # assume that the peak_param_ws has only the peak of interest fitted
            intensities = np.asarray(peak_param_ws.column("I"))
            x0s = np.asarray(peak_param_ws.column("X0"))
            if not self.no_chi2_needed:
                chi2 = np.asarray(peak_param_ws.column("chi2"))
            else:
                chi2 = np.zeros((len(ab_arr)))

        else:
            intensities = np.ones((len(ab_arr)))
            chi2 = np.zeros((len(ab_arr)))
            x0s = np.ones((len(ab_arr)))

        if hkl_is_set:
            hkl = _parse_hkl(hkl)
            xtal = ws.sample().getCrystalStructure()
            peak = xtal.getUnitCell().d(hkl)
            scat_power = _calc_scattering_power(xtal, hkl)
        else:
            # if no hkl provided, peak will be set to mean x0 and scattering power will be 1
            peak = np.mean(x0s)
            scat_power = 1.0

        # create the table to hold the data
        table_ws = self.exec_child_alg("CreateEmptyTableWorkspace", OutputWorkspace="_tmp")
        table_ws.addColumn(type="double", name="Alpha", plottype=2)
        table_ws.addColumn(type="double", name="Beta", plottype=2)
        table_ws.addColumn(type="double", name="Intensity", plottype=2)

        # go through each detector group
        for index, (a, b) in enumerate(ab_arr):
            # check chi2 is under threshold (or threshold is set to be ignored) and peak position is also within tol
            if ((chi2[index] < chi_thresh) or chi_thresh == 0.0) and ((np.abs(x0s[index] - peak) < x0_thresh) or x0_thresh == 0.0):
                # amend intensity for any scattering correction
                intensity = intensities[index] / scat_power
                # add data to table
                table_ws.addRow({"Alpha": a, "Beta": b, "Intensity": intensity})

        self.setProperty("OutputWorkspace", table_ws)

    def exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props


def _parse_hkl(hkl: str) -> V3D:
    return V3D(*[int("".join([char for char in part if char.isnumeric()])) for part in hkl.split(",")])


def _calc_scattering_power(xtal, hkl) -> float:
    generator = ReflectionGenerator(xtal)
    d = xtal.getUnitCell().d(hkl)
    f_sq = generator.getFsSquared(
        [
            hkl,
        ]
    )[0]

    pg = xtal.getSpaceGroup().getPointGroup()
    m = len(pg.getEquivalents(hkl))  # multiplicity

    vol = xtal.getUnitCell().volume() / len(xtal.getScatterers())  # volume per atom
    return (m * f_sq * d**4) / vol**2  # Eq 2 of NyRTeX paper


def _get_alpha_beta_from_cart(arr: np.ndarray) -> np.ndarray:
    arr = arr.copy()
    arr = np.where(arr[1] < -0.001, -arr, arr)  # invert the southern points
    alphas = np.arctan2(arr[2], arr[0])
    betas = np.arccos(arr[1])
    return np.concatenate([alphas[:, None], betas[:, None]], axis=1)


AlgorithmFactory.subscribe(CreatePoleFigureTableWorkspace)
