# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import shutil
import systemtesting
from mantid import config
from mantid.api import AnalysisDataService as ADS
from Engineering.texture.TextureUtils import run_abs_corr, fit_all_peaks
from mantid.simpleapi import LoadEmptyInstrument, CreateSampleShape, SetSampleMaterial, Load, ExtractSingleSpectrum, ConvertUnits
from Engineering.common.xml_shapes import get_cube_xml
import numpy as np

DATA_DIR = config["datasearch.directories"].split(";")[0]
CWDIR = os.path.join(config["datasearch.directories"].split(";")[0], "Texture")
config.appendDataSearchSubDir(CWDIR)

# Disclaimer: These tests mainly operate under the assumption that the workflow was working correctly
# at the point of writing the tests. If you now have a failure, consider the nature of your change and
# then consider the implication that the need for this change has on the validity of that initial assumption.

# Most of the functionality is System Tested elsewhere: Calibration and Focusing in EnginXScriptTest, and
# Pole Figure creation in CreatePoleFigureTableWorkspace


class AbsCorrMixin(object):
    def setup_absorption_correction_inputs(self):
        self.shape_xml = get_cube_xml("test_cube", 0.01)
        LoadEmptyInstrument(InstrumentName="ENGINX", OutputWorkspace="ref_ws")
        CreateSampleShape(InputWorkspace="ref_ws", ShapeXML=self.shape_xml)
        SetSampleMaterial(InputWorkspace="ref_ws", ChemicalFormula="Fe")

        self.input_raw = Load(Filename=os.path.join(DATA_DIR, "ENGINX299080.nxs"), OutputWorkspace="raw_ENGINX299080")
        # tests can just be run on the first spectrum for speed
        self.input_ws = ExtractSingleSpectrum(InputWorkspace="raw_ENGINX299080", WorkspaceIndex=0, OutputWorkspace="ENGINX299080")

        self.expected_files = [os.path.join(CWDIR, "AbsorptionCorrection", "Corrected_ENGINX299080.nxs")]

        self.default_kwargs = {
            "wss": ["ENGINX299080"],
            "ref_ws": "ref_ws",
            "copy_ref": True,
            "include_abs_corr": True,
            "monte_carlo_args": "SparseInstrument:False",
            "gauge_vol_preset": "4mmCube",
            "include_atten_table": False,
            "eval_point": "2.00",
            "eval_units": "dSpacing",
            "root_dir": CWDIR,
            "clear_ads_after": False,
        }

    def validate_expected_files(self):
        [self.assertTrue(os.path.exists(ef)) for ef in self.expected_files]


class RunAStandardAbsorptionCorrectionWithAttenuationTable(systemtesting.MantidSystemTest, AbsCorrMixin):
    def runTest(self):
        self.setup_absorption_correction_inputs()
        kwargs = self.default_kwargs
        kwargs["include_atten_table"] = True
        run_abs_corr(**kwargs)

        self.corr_ws = ADS.retrieve("Corrected_ENGINX299080")
        self.attenuation_table = ADS.retrieve("ENGIN-X_299080_attenuation_coefficient_2.00_dSpacing")

    def validate(self):
        self.assertEqual(self.attenuation_table.rowCount(), self.corr_ws.getNumberHistograms())  # row per spectra
        self.assertAlmostEqual(self.attenuation_table.cell(0, 0), 0.2671205699443817, places=5)  # expected attenuation coefficient
        self.expected_files += [
            os.path.join(CWDIR, "AttenuationTables", "ENGIN-X_299080_attenuation_coefficient_2.00_dSpacing.nxs"),
        ]
        self.tolerance = 1e-6
        self.validate_expected_files()
        return self.corr_ws.name(), os.path.join(CWDIR, "Corrected_ENGINX299080_1cmFeCube_unrotated.nxs")

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["AbsorptionCorrection", "AttenuationTables", "User"])


class RunAStandardAbsorptionCorrection(systemtesting.MantidSystemTest, AbsCorrMixin):
    def runTest(self):
        self.setup_absorption_correction_inputs()
        kwargs = self.default_kwargs
        run_abs_corr(**kwargs)

        self.corr_ws = ADS.retrieve("Corrected_ENGINX299080")

    def validate(self):
        self.assertTrue(not ADS.doesExist("ENGIN-X_299080_attenuation_coefficient_2.00_dSpacing"))  # no attenuation table
        self.tolerance = 1e-6
        self.validate_expected_files()
        return self.corr_ws.name(), os.path.join(CWDIR, "Corrected_ENGINX299080_1cmFeCube_unrotated.nxs")

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["AbsorptionCorrection"])


class RunAStandardAbsorptionCorrectionEulerGoniometer(systemtesting.MantidSystemTest, AbsCorrMixin):
    def runTest(self):
        self.setup_absorption_correction_inputs()
        orientation_file = os.path.join(CWDIR, "rotation_as_euler.txt")
        kwargs = self.default_kwargs
        kwargs["orientation_file"] = orientation_file
        kwargs["orient_file_is_euler"] = True
        kwargs["euler_scheme"] = "YXY"
        kwargs["euler_axes_sense"] = "-1,-1,-1"
        run_abs_corr(**kwargs)

        self.corr_ws = ADS.retrieve("Corrected_ENGINX299080")

    def validate(self):
        self.tolerance = 1e-6
        self.validate_expected_files()
        return self.corr_ws.name(), os.path.join(CWDIR, "Corrected_ENGINX299080_1cmFeCube_euler_rotated.nxs")

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["AbsorptionCorrection"])


class RunAStandardAbsorptionCorrectionProvideGoniometerMatrix(systemtesting.MantidSystemTest, AbsCorrMixin):
    def runTest(self):
        self.setup_absorption_correction_inputs()
        orientation_file = os.path.join(CWDIR, "rotation_as_matrix.txt")
        kwargs = self.default_kwargs
        kwargs["orientation_file"] = orientation_file
        kwargs["orient_file_is_euler"] = False
        run_abs_corr(**kwargs)

        self.corr_ws = ADS.retrieve("Corrected_ENGINX299080")

    def validate(self):
        self.tolerance = 1e-6
        self.validate_expected_files()
        return self.corr_ws.name(), os.path.join(CWDIR, "Corrected_ENGINX299080_1cmFeCube_matrix_rotated.nxs")

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["AbsorptionCorrection"])


class RunAStandardAbsorptionCorrectionWithCustomGaugeVolume(systemtesting.MantidSystemTest, AbsCorrMixin):
    def runTest(self):
        self.setup_absorption_correction_inputs()
        gv_file = os.path.join(CWDIR, "custom_gauge_volume.xml")
        kwargs = self.default_kwargs
        kwargs["gauge_vol_preset"] = "Custom"
        kwargs["gauge_vol_shape_file"] = gv_file
        run_abs_corr(**kwargs)

        self.corr_ws = ADS.retrieve("Corrected_ENGINX299080")

    def validate(self):
        self.tolerance = 1e-6
        self.validate_expected_files()
        return self.corr_ws.name(), os.path.join(CWDIR, "Corrected_ENGINX299080_1cmFeCube_unrotated.nxs")

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["AbsorptionCorrection"])


class RunAStandardAbsorptionCorrectionWithDivergenceCorrection(systemtesting.MantidSystemTest, AbsCorrMixin):
    def runTest(self):
        self.setup_absorption_correction_inputs()
        kwargs = self.default_kwargs
        kwargs["gauge_vol_preset"] = "4mmCube"
        kwargs["include_div_corr"] = True
        kwargs["div_hoz"] = 0.012
        kwargs["div_vert"] = 0.014
        kwargs["det_hoz"] = 0.002
        run_abs_corr(**kwargs)

        self.corr_ws = ADS.retrieve("Corrected_ENGINX299080")

    def validate(self):
        self.tolerance = 1e-3
        self.validate_expected_files()
        return self.corr_ws.name(), os.path.join(CWDIR, "Corrected_ENGINX299080_1cmFeCube_unrotated_divcorr.nxs")

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["AbsorptionCorrection"])


class PeakFitMixin(object):
    def setup_fit_peaks_inputs(self):
        raw_ws = Load(Filename=os.path.join(DATA_DIR, "ENGINX_280625_focused_bank_1.nxs"), OutputWorkspace="ENGINX_280625_focused_bank_1")
        self.input_ws = ConvertUnits(InputWorkspace=raw_ws, OutputWorkspace="ENGINX_280625_focused_bank_1_dSpacing", Target="dSpacing")
        self.fit_dir = os.path.join(CWDIR, "FitParameters")
        self.peaks = (1.8, 1.44)
        self.reference_columns = ["wsindex", "I_est", "I", "I_err", "A", "A_err", "B", "B_err", "X0", "X0_err", "S", "S_err"]
        self.default_kwargs = {
            "wss": ["ENGINX_280625_focused_bank_1_dSpacing"],
            "peaks": self.peaks,
            "peak_window": 0.03,
            "save_dir": self.fit_dir,
        }
        self.peak_1_vals = [
            0,
            54.04933925,
            58.52875482,
            1.75884296,
            130891.0,
            0.0,
            0.0117147,
            0.0,
            1.79720022,
            0.00016396,
            103.64888962,
            3.80138506,
        ]
        self.peak_2_vals = [
            0,
            44.85399793,
            55.94547931,
            2.75051451,
            304147.0,
            0.0,
            0.0184249,
            0.0,
            1.43621627,
            0.00028254,
            160.00458428,
            6.80774229,
        ]

    def validate_table(self, out_table, expected_dict):
        expected_cols = list(expected_dict.keys())
        for c in out_table.getColumnNames():
            print(c, ": ", np.nan_to_num(out_table.column(c)), ", validation value: ", expected_dict[c])

        for c in out_table.getColumnNames():
            self.assertIn(c, expected_cols)
            np.testing.assert_allclose(np.nan_to_num(out_table.column(c)), expected_dict[c], rtol=1e-3)

    def validate_missing_peaks_vals(self, peak_1_vals, peak_2_vals):
        param_table1 = ADS.retrieve("ENGINX_280625_2.3_GROUP_Fit_Parameters")
        param_table2 = ADS.retrieve("ENGINX_280625_2.5_GROUP_Fit_Parameters")
        expected_files = [
            os.path.join(CWDIR, "FitParameters", "GROUP", "2.3", "ENGINX_280625_2.3_GROUP_Fit_Parameters.nxs"),
            os.path.join(CWDIR, "FitParameters", "GROUP", "2.5", "ENGINX_280625_2.5_GROUP_Fit_Parameters.nxs"),
        ]

        self.validate_table(param_table1, dict(zip(self.reference_columns, peak_1_vals)))
        self.validate_table(param_table2, dict(zip(self.reference_columns, peak_2_vals)))
        [self.assertTrue(os.path.exists(ef)) for ef in expected_files]


class TestFittingPeaksOfFocusedData(systemtesting.MantidSystemTest, PeakFitMixin):
    def runTest(self):
        self.setup_fit_peaks_inputs()
        fit_all_peaks(**self.default_kwargs)

    def validate(self):
        param_table1 = ADS.retrieve("ENGINX_280625_1.8_GROUP_Fit_Parameters")
        param_table2 = ADS.retrieve("ENGINX_280625_1.44_GROUP_Fit_Parameters")
        expected_files = [
            os.path.join(CWDIR, "FitParameters", "GROUP", "1.8", "ENGINX_280625_1.8_GROUP_Fit_Parameters.nxs"),
            os.path.join(CWDIR, "FitParameters", "GROUP", "1.44", "ENGINX_280625_1.44_GROUP_Fit_Parameters.nxs"),
        ]

        self.validate_table(param_table1, dict(zip(self.reference_columns, self.peak_1_vals)))
        self.validate_table(param_table2, dict(zip(self.reference_columns, self.peak_2_vals)))
        [self.assertTrue(os.path.exists(ef)) for ef in expected_files]

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["FitParameters"])


class TestFittingPeaksOfMissingPeakDataWithFillZero(systemtesting.MantidSystemTest, PeakFitMixin):
    def runTest(self):
        self.setup_fit_peaks_inputs()
        kwargs = self.default_kwargs
        kwargs["peaks"] = (2.3, 2.5)
        # expect no peaks here, set the i over sigma threshold large as well as sigma is ill-defined
        fit_all_peaks(**kwargs, i_over_sigma_thresh=10.0, nan_replacement="zeros")

    def validate(self):
        # expect all params to be zeros (coincidentally including val[0] as this is the wsindex)
        expected_vals1 = [0.0 for _ in self.peak_1_vals]
        expected_vals2 = [0.0 for _ in self.peak_2_vals]
        self.validate_missing_peaks_vals(expected_vals1, expected_vals2)

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["FitParameters"])


class TestFittingPeaksOfMissingPeakDataWithSpecifiedValue(systemtesting.MantidSystemTest, PeakFitMixin):
    def runTest(self):
        self.setup_fit_peaks_inputs()
        kwargs = self.default_kwargs
        kwargs["peaks"] = (2.3, 2.5)
        # expect no peaks here, set the i over sigma threshold large as well as sigma is ill-defined
        fit_all_peaks(**kwargs, i_over_sigma_thresh=10.0, nan_replacement="zeros", no_fit_value_dict={"I_est": 1.0, "I": 0.01})

    def validate(self):
        # expect all params to be zeros (coincidentally including val[0] as this is the wsindex)
        # except I_est which is val[1] and has been set to 1.0 and I which is val[2] and is set as 0.01
        expected_vals1 = [0.0, 1.0, 0.01] + [0.0 for _ in self.peak_1_vals[3:]]
        expected_vals2 = [0.0, 1.0, 0.01] + [0.0 for _ in self.peak_2_vals[3:]]
        self.validate_missing_peaks_vals(expected_vals1, expected_vals2)

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["FitParameters"])


class TestFittingPeaksOfFocusedDataWithGroup(systemtesting.MantidSystemTest, PeakFitMixin):
    def runTest(self):
        self.setup_fit_peaks_inputs()
        self.input_ws.getRun().addProperty("Grouping", "TEST", False)

        fit_all_peaks(**self.default_kwargs)

    def validate(self):
        param_table1 = ADS.retrieve("ENGINX_280625_1.8_TEST_Fit_Parameters")
        param_table2 = ADS.retrieve("ENGINX_280625_1.44_TEST_Fit_Parameters")
        expected_files = [
            os.path.join(CWDIR, "FitParameters", "TEST", "1.8", "ENGINX_280625_1.8_TEST_Fit_Parameters.nxs"),
            os.path.join(CWDIR, "FitParameters", "TEST", "1.44", "ENGINX_280625_1.44_TEST_Fit_Parameters.nxs"),
        ]

        self.validate_table(param_table1, dict(zip(self.reference_columns, self.peak_1_vals)))
        self.validate_table(param_table2, dict(zip(self.reference_columns, self.peak_2_vals)))
        [self.assertTrue(os.path.exists(ef)) for ef in expected_files]

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["FitParameters"])


def _try_delete_dirs(parent_dir, folders):
    for folder in folders:
        rm_dir = os.path.join(parent_dir, folder)
        try:
            if os.path.isdir(rm_dir):
                shutil.rmtree(rm_dir)  # don't use os.remove as we could be passed a non-empty dir
            else:
                os.remove(rm_dir)
        except OSError:
            print("Could not delete output file at: ", rm_dir)
