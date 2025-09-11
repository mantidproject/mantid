# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# import mantid algorithms, numpy and matplotlib
import numpy as np
from mantid.kernel import V3D
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, ITableWorkspaceProperty, PythonAlgorithm, PropertyMode
from mantid.kernel import (
    Direction,
    FloatBoundedValidator,
    FloatArrayProperty,
    FloatArrayLengthValidator,
    PropertyCriterion,
    EnabledWhenProperty,
)
from mantid.geometry import ReflectionGenerator, CrystalStructure
from typing import Optional


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
            "ReadoutColumn",
            defaultValue="I",
            direction=Direction.Input,
            doc="The column from the Peak Parameter Workspace which should be used for populating the final column in the output table",
        )
        self.declareProperty(
            FloatArrayProperty("Reflection", [0, 0, 0], FloatArrayLengthValidator(3), direction=Direction.Input),
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
        self.declareProperty(
            "ApplyScatteringPowerCorrection",
            defaultValue=True,
            direction=Direction.Input,
            doc="Flag for determining whether the provided intensities should be corrected for scattering power",
        )
        self.declareProperty(
            FloatArrayProperty("ScatteringVolumePosition", [0.0, 0.0, 0.0], FloatArrayLengthValidator(3), direction=Direction.Input),
            doc="Position where the diffraction vectors should be calculated from, defaults as the origin",
        )
        self.declareProperty(
            FloatArrayProperty(
                "AxesTransform", [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0], FloatArrayLengthValidator(9), direction=Direction.Input
            ),
            doc="Algorithm assumes the initial orientation of the sample has the "
            "two in plane axes of the pole figure aligned with X and Z of the laboratory frame. "
            "This can be overridden by providing a flattened axes transform matrix, "
            "which transforms X and Z to the desired initial vectors",
        )
        # disable properties which don't apply when there is no peak parameter workspace
        self.setPropertySettings("ReadoutColumn", EnabledWhenProperty("PeakParameterWorkspace", PropertyCriterion.IsNotDefault))
        self.setPropertySettings("Chi2Threshold", EnabledWhenProperty("PeakParameterWorkspace", PropertyCriterion.IsNotDefault))
        self.setPropertySettings("PeakPositionThreshold", EnabledWhenProperty("PeakParameterWorkspace", PropertyCriterion.IsNotDefault))
        # disable properties which don't apply when there is no reflection
        self.setPropertySettings("ApplyScatteringPowerCorrection", EnabledWhenProperty("Reflection", PropertyCriterion.IsNotDefault))

    def validateInputs(self):
        issues = dict()
        ws = self.getProperty("InputWorkspace").value

        # validate the input ws exists
        if ws is None:
            issues["InputWorkspace"] = "InputWorkspace must be set"

        # check peak parameter ws
        peak_ws = self.getProperty("PeakParameterWorkspace").value
        chi2_thresh = self.getProperty("Chi2Threshold").value
        x0_thresh = self.getProperty("PeakPositionThreshold").value
        readout_column = self.getProperty("ReadoutColumn").value
        # if the chi2_threshold is set to allow all peaks, no chi2 column is needed
        self.no_chi2_needed = np.allclose(chi2_thresh, 0)
        # if the x0_threshold is set to allow all peaks, no x0 column is needed
        self.no_x0_needed = np.allclose(x0_thresh, 0)

        ax_trans = np.asarray(self.getProperty("AxesTransform").value).reshape((3, 3))
        if not np.allclose(np.abs(np.linalg.det(ax_trans)), 1):
            issues["AxesTransform"] = (
                "Algorithm currently expects axes transforms to volume-preserving, as such the determinant must equal 1 (or -1)"
            )

        if peak_ws:
            # check num rows in table matches num spectra in ws
            if peak_ws.rowCount() != ws.getNumberHistograms():
                issues["PeakParameterWorkspace"] = "PeakParameterWorkspace must have same number of rows as Input Workspace has spectra"

            def check_col(col_name):
                try:
                    peak_ws.column(col_name)
                except RuntimeError:
                    issues["PeakParameterWorkspace"] = f"PeakParameterWorkspace must have column: '{col_name}'"

            # if peak parameter ws is given, expect these columns:

            # the specified readout column
            check_col(readout_column)

            if not self.no_x0_needed:
                # x0, if needed
                check_col("X0")

            if not self.no_chi2_needed:
                # chi2, if needed
                check_col("chi2")

        if not self.getProperty("Reflection").isDefault:
            # if a hkl is given, scattering power will be calculated, check that CrystalStructure has been defined
            if not ws.sample().hasCrystalStructure():
                issues["InputWorkspace"] = "If reflection is specified: InputWorkspace sample must have a CrystalStructure"
        return issues

    def PyExec(self):
        # get inputs
        ws = self.getProperty("InputWorkspace").value
        peak_param_ws = self.getProperty("PeakParameterWorkspace").value
        readout_column = self.getProperty("ReadoutColumn").value
        hkl = self.getProperty("Reflection").value
        hkl_is_set = not self.getProperty("Reflection").isDefault
        chi_thresh = self.getProperty("Chi2Threshold").value
        x0_thresh = self.getProperty("PeakPositionThreshold").value
        apply_scatt_corr = self.getProperty("ApplyScatteringPowerCorrection").value
        sample_pos = np.asarray(self.getProperty("ScatteringVolumePosition").value)
        ax_trans = np.asarray(self.getProperty("AxesTransform").value).reshape((3, 3))

        # generate a detector table to get detector positions
        det_table = self.exec_child_alg(
            "CreateDetectorTable", InputWorkspace=ws, IncludeDetectorPosition=True, DetectorTableWorkspace="det_table"
        )
        # get the detector position relative to the sample
        det_pos = np.asarray(det_table.column("Position")) - sample_pos

        # get the normalised source pos for qi
        source_pos = ws.getInstrument().getSource().getPos()
        source_pos = np.asarray(source_pos) / source_pos.norm()
        qi = sample_pos - source_pos

        # get the rotation matrix of the sample
        rot_mat = ws.run().getGoniometer().getR() @ ax_trans

        # matrix to put detectors in sample frame
        to_pole_view = np.linalg.inv(rot_mat)

        # get Qs
        qds = det_pos / np.linalg.norm(det_pos, axis=1)[:, None]
        # normalise source pos then take away from sample pos for ki
        Qs = qds - qi
        Qs = Qs / np.linalg.norm(Qs, axis=1)[:, None]

        # put detectors in sample reference frame
        sample_frame_Qs = to_pole_view @ Qs.T

        # calculate alpha and beta for the Qs in the sample ref frame
        ab_arr = _get_alpha_beta_from_cart(sample_frame_Qs)

        # set default values
        default_intensity = 1.0
        use_default_intensity = True
        intensities = None
        chi2 = None
        x0s = None
        peak = None
        scat_power = 1.0

        if peak_param_ws:
            # assume that the peak_param_ws has only the peak of interest fitted
            intensities = np.asarray(peak_param_ws.column(readout_column))
            use_default_intensity = False
            if not self.no_x0_needed:
                x0s = np.asarray(peak_param_ws.column("X0"))
                # if no hkl provided, peak will be set to mean x0
                peak = np.mean(x0s)
            if not self.no_chi2_needed:
                chi2 = np.asarray(peak_param_ws.column("chi2"))
        else:
            self.no_x0_needed = True
            self.no_chi2_needed = True

        if hkl_is_set:
            hkl = V3D(*hkl)
            xtal = ws.sample().getCrystalStructure()
            peak = xtal.getUnitCell().d(hkl)
            if apply_scatt_corr:
                scat_power = _calc_scattering_power(xtal, hkl)

        # create the table to hold the data
        table_ws = self.exec_child_alg("CreateEmptyTableWorkspace", OutputWorkspace="_tmp")
        table_ws.addColumn(type="double", name="Beta", plottype=2)
        table_ws.addColumn(type="double", name="Alpha", plottype=2)
        table_ws.addColumn(type="double", name=readout_column, plottype=2)

        # check which spectra meet inclusion thresholds
        valid_spec_mask = self._thresh_criteria_mask(ab_arr.shape[0], chi_thresh, x0_thresh, chi2, x0s, peak)
        for spec_index, is_valid in enumerate(valid_spec_mask):
            if is_valid:
                # amend intensity for any scattering correction
                intensity = intensities[spec_index] / scat_power if not use_default_intensity else default_intensity
                # add data to table
                table_ws.addRow({"Beta": ab_arr[spec_index, 1], "Alpha": ab_arr[spec_index, 0], readout_column: intensity})

        self.setProperty("OutputWorkspace", table_ws)

    def exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props

    def _thresh_criteria_mask(
        self,
        n_rows: int,
        chi2_thresh: float,
        x0_thresh: float,
        chi2_vals: Optional[np.ndarray] = None,
        x0_vals: Optional[np.ndarray] = None,
        peak: Optional[float] = None,
    ) -> np.ndarray:
        mask = np.ones(n_rows, dtype=bool)
        if not self.no_chi2_needed:
            mask[chi2_vals >= chi2_thresh] = False
        if not self.no_x0_needed:
            mask[np.abs(x0_vals - peak) >= x0_thresh] = False
        return mask


def _calc_scattering_power(xtal: CrystalStructure, hkl: V3D) -> float:
    """
    Calculate the scattering power of a reflection as described in:
    Malamud, F., Santisteban, J. R., Vicente Alvarez, M. A., Bolmaro, R., Kelleher, J., Kabra,
    S. & Kockelmann, W. (2014). Texture analysis with a time-of-flight neutron
    strain scanner. J. Appl. Cryst. 47, https://doi.org/10.1107/S1600576714012710

    """
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


def _get_alpha_beta_from_cart(q_sample_cart: np.ndarray) -> np.ndarray:
    """
    get spherical angles from cartesian coordinates
    alpha is angle from positive x towards positive z
    beta is angle from positive y
    """
    q_sample_cart = np.clip(q_sample_cart, -1.0, 1.0)  # numerical inaccuracies outside this range will give nan in the trig funcs
    q_sample_cart = np.where(q_sample_cart[1] < 0, -q_sample_cart, q_sample_cart)  # invert the southern points
    alphas = np.arctan2(q_sample_cart[2], q_sample_cart[0])
    betas = np.arccos(q_sample_cart[1])
    return np.concatenate([alphas[:, None], betas[:, None]], axis=1)


AlgorithmFactory.subscribe(CreatePoleFigureTableWorkspace)
