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

DATA_DIR = config["datasearch.directories"].split(";")[0]
CWDIR = os.path.join(config["datasearch.directories"].split(";")[0], "Texture")
config.appendDataSearchSubDir(CWDIR)

# Disclaimer: These tests mainly operate under the assumption that the workflow was working correctly
# at the point of writing the tests. If you now have a failure, consider the nature of your change and
# then consider the implication that the need for this change has on the validity of that initial assumption.

# Most of the functionality is System Tested elsewhere: Calibration and Focusing in EnginXScriptTest, and
# Pole Figure creation in CreatePoleFigureTableWorkspace


class AbsCorrMixin(object):
    def get_cube_xml(self, side_len):
        return f"""
                <cuboid id='some-shape'> \
                <height val='{side_len}'  /> \
                <width val='{side_len}' />  \
                <depth  val='{side_len}' />  \
                <centre x='0.0' y='0.0' z='0.0'  />  \
                </cuboid>  \
                <algebra val='some-shape' /> \\ """

    def setup_absorption_correction_inputs(self):
        self.shape_xml = self.get_cube_xml(0.01)
        LoadEmptyInstrument(InstrumentName="ENGINX", OutputWorkspace="ref_ws")
        CreateSampleShape(InputWorkspace="ref_ws", ShapeXML=self.shape_xml)
        SetSampleMaterial(InputWorkspace="ref_ws", ChemicalFormula="Fe")

        self.input_raw = Load(Filename=os.path.join(DATA_DIR, "ENGINX299080.nxs"), OutputWorkspace="raw_ENGINX299080")
        # tests can just be run on the first spectrum for speed
        self.input_ws = ExtractSingleSpectrum(InputWorkspace="raw_ENGINX299080", WorkspaceIndex=0, OutputWorkspace="ENGINX299080")

        self.expected_files = [os.path.join(CWDIR, "AbsorptionCorrection", "Corrected_ENGINX299080.nxs")]

    def validate_expected_files(self):
        [self.assertTrue(os.path.exists(ef)) for ef in self.expected_files]


class RunAStandardAbsorptionCorrectionWithAttenuationTable(systemtesting.MantidSystemTest, AbsCorrMixin):
    def runTest(self):
        self.setup_absorption_correction_inputs()
        run_abs_corr(
            wss=["ENGINX299080"],
            ref_ws="ref_ws",
            copy_ref=True,
            include_abs_corr=True,
            monte_carlo_args="SparseInstrument:False",
            gauge_vol_preset="4mmCube",
            include_atten_table=True,
            eval_point="2.00",
            eval_units="dSpacing",
            exp_name="Test",
            root_dir=CWDIR,
            clear_ads_after=False,
        )
        self.corr_ws = ADS.retrieve("Corrected_ENGINX299080")
        self.attenuation_table = ADS.retrieve("ENGIN-X_299080_attenuation_coefficient_2.00_dSpacing")

    def validate(self):
        self.assertEqual(self.attenuation_table.rowCount(), self.corr_ws.getNumberHistograms())  # row per spectra
        self.assertAlmostEqual(self.attenuation_table.cell(0, 0), 0.2671205699443817, places=5)  # expected attenuation coefficient
        self.expected_files += [
            os.path.join(CWDIR, "AttenuationTables", "ENGIN-X_299080_attenuation_coefficient_2.00_dSpacing.nxs"),
            os.path.join(CWDIR, "User", "Test", "AttenuationTables", "ENGIN-X_299080_attenuation_coefficient_2.00_dSpacing.nxs"),
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
        run_abs_corr(
            wss=["ENGINX299080"],
            ref_ws="ref_ws",
            copy_ref=True,
            include_abs_corr=True,
            monte_carlo_args="SparseInstrument:False",
            gauge_vol_preset="4mmCube",
            exp_name="Test",
            root_dir=CWDIR,
            clear_ads_after=False,
        )
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
        run_abs_corr(
            wss=["ENGINX299080"],
            ref_ws="ref_ws",
            orientation_file=orientation_file,
            orient_file_is_euler=True,
            euler_scheme="YXY",
            euler_axes_sense="-1,-1,-1",
            copy_ref=True,
            include_abs_corr=True,
            monte_carlo_args="SparseInstrument:False",
            gauge_vol_preset="4mmCube",
            exp_name="Test",
            root_dir=CWDIR,
            clear_ads_after=False,
        )
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
        run_abs_corr(
            wss=["ENGINX299080"],
            ref_ws="ref_ws",
            orientation_file=orientation_file,
            orient_file_is_euler=False,
            copy_ref=True,
            include_abs_corr=True,
            monte_carlo_args="SparseInstrument:False",
            gauge_vol_preset="4mmCube",
            eval_units="dSpacing",
            exp_name="Test",
            root_dir=CWDIR,
            clear_ads_after=False,
        )
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
        run_abs_corr(
            wss=["ENGINX299080"],
            ref_ws="ref_ws",
            copy_ref=True,
            include_abs_corr=True,
            monte_carlo_args="SparseInstrument:False",
            gauge_vol_preset="Custom",
            gauge_vol_shape_file=gv_file,  # same as the preset 4mm cube
            exp_name="Test",
            root_dir=CWDIR,
            clear_ads_after=False,
        )
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
        run_abs_corr(
            wss=["ENGINX299080"],
            ref_ws="ref_ws",
            copy_ref=True,
            include_abs_corr=True,
            monte_carlo_args="SparseInstrument:False",
            gauge_vol_preset="4mmCube",
            gauge_vol_shape_file="",
            exp_name="Test",
            root_dir=CWDIR,
            include_div_corr=True,
            div_hoz=0.012,
            div_vert=0.014,
            det_hoz=0.002,
            clear_ads_after=False,
        )
        self.corr_ws = ADS.retrieve("Corrected_ENGINX299080")

    def validate(self):
        self.tolerance = 1e-6
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
        self.reference_columns = ["wsindex", "A0", "A1", "I", "A", "B", "X0", "S", "chi2", "I_est"]
        self.peak_1_vals = [
            0,
            1500.856889154188,
            -669.0606994822296,
            70.61040576989895,
            174.2113006175971,
            127.43312817687284,
            1.8001619058645897,
            0.0030776621774879147,
            0.16627024534662274,
            41.97759313602705,
        ]
        self.peak_2_vals = [
            0,
            -5072.716721937553,
            3906.2564035129785,
            60.66188215016203,
            161.4951711779007,
            242.50216981399444,
            1.4402945900852524,
            0.007282529342267018,
            0.16767853995308524,
            28.70083944783797,
        ]

    def validate_table(self, out_table, expected_dict):
        expected_cols = list(expected_dict.keys())
        for c in out_table.getColumnNames():
            self.assertIn(c, expected_cols)
            self.assertAlmostEqual(out_table.column(c)[0], expected_dict[c], places=5)


class TestFittingPeaksOfFocusedData(systemtesting.MantidSystemTest, PeakFitMixin):
    def runTest(self):
        self.setup_fit_peaks_inputs()
        fit_all_peaks(
            wss=["ENGINX_280625_focused_bank_1_dSpacing"], peaks=self.peaks, peak_window=0.03, save_dir=self.fit_dir, do_numeric_integ=False
        )

    def validate(self):
        param_table1 = ADS.retrieve("ENGINX_280625_1.8__Fit_Parameters")
        param_table2 = ADS.retrieve("ENGINX_280625_1.44__Fit_Parameters")
        expected_files = [
            os.path.join(CWDIR, "FitParameters", "1.8", "ENGINX_280625_1.8__Fit_Parameters.nxs"),
            os.path.join(CWDIR, "FitParameters", "1.44", "ENGINX_280625_1.44__Fit_Parameters.nxs"),
        ]
        # ignore the estimated intensity reference data (last column of reference)
        self.validate_table(param_table1, dict(zip(self.reference_columns[:-1], self.peak_1_vals[:-1])))
        self.validate_table(param_table2, dict(zip(self.reference_columns[:-1], self.peak_2_vals[:-1])))
        [self.assertTrue(os.path.exists(ef)) for ef in expected_files]

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["FitParameters"])


class TestFittingPeaksOfFocusedDataWithNumericalIntegration(systemtesting.MantidSystemTest, PeakFitMixin):
    def runTest(self):
        self.setup_fit_peaks_inputs()
        fit_all_peaks(
            wss=["ENGINX_280625_focused_bank_1_dSpacing"], peaks=self.peaks, peak_window=0.03, save_dir=self.fit_dir, do_numeric_integ=True
        )

    def validate(self):
        param_table1 = ADS.retrieve("ENGINX_280625_1.8__Fit_Parameters")
        param_table2 = ADS.retrieve("ENGINX_280625_1.44__Fit_Parameters")
        expected_files = [
            os.path.join(CWDIR, "FitParameters", "1.8", "ENGINX_280625_1.8__Fit_Parameters.nxs"),
            os.path.join(CWDIR, "FitParameters", "1.44", "ENGINX_280625_1.44__Fit_Parameters.nxs"),
        ]
        self.validate_table(param_table1, dict(zip(self.reference_columns, self.peak_1_vals)))
        self.validate_table(param_table2, dict(zip(self.reference_columns, self.peak_2_vals)))
        [self.assertTrue(os.path.exists(ef)) for ef in expected_files]

    def cleanup(self):
        ADS.clear()
        _try_delete_dirs(CWDIR, ["FitParameters"])


class TestFittingPeaksOfFocusedDataWithGroup(systemtesting.MantidSystemTest, PeakFitMixin):
    def runTest(self):
        self.setup_fit_peaks_inputs()
        self.input_ws.getRun().addProperty("Grouping", "TEST", False)

        fit_all_peaks(
            wss=["ENGINX_280625_focused_bank_1_dSpacing"], peaks=self.peaks, peak_window=0.03, save_dir=self.fit_dir, do_numeric_integ=False
        )

    def validate(self):
        param_table1 = ADS.retrieve("ENGINX_280625_1.8_TEST_Fit_Parameters")
        param_table2 = ADS.retrieve("ENGINX_280625_1.44_TEST_Fit_Parameters")
        expected_files = [
            os.path.join(CWDIR, "FitParameters", "TEST", "1.8", "ENGINX_280625_1.8_TEST_Fit_Parameters.nxs"),
            os.path.join(CWDIR, "FitParameters", "TEST", "1.44", "ENGINX_280625_1.44_TEST_Fit_Parameters.nxs"),
        ]
        # ignore the estimated intensity reference data (last column of reference)
        self.validate_table(param_table1, dict(zip(self.reference_columns[:-1], self.peak_1_vals[:-1])))
        self.validate_table(param_table2, dict(zip(self.reference_columns[:-1], self.peak_2_vals[:-1])))
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
