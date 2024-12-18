# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import os
import systemtesting
import shutil

import mantid.simpleapi as mantid
from mantid import config

from isis_powder import Polaris, SampleDetails

DIRS = config["datasearch.directories"].split(";")

# Setup various path details

inst_name = "POLARIS"
# Relative to system data folder
working_folder_name = "ISIS_Powder"

# Relative to working folder
input_folder_name = "input"
output_folder_name = "output"

# Relative to input folder
calibration_folder_name = os.path.join("calibration", inst_name.lower())
calibration_map_rel_path = os.path.join("yaml_files", "polaris_system_test_mapping.yaml")
spline_rel_path = os.path.join("17_1", "VanSplined_98532_cycle_16_3_silicon_all_spectra.cal.nxs")
unsplined_van_rel_path = os.path.join("17_1", "Van_98532_cycle_16_3_silicon_all_spectra.cal.nxs")
summed_empty_rel_path = os.path.join("17_1", "summed_empty_98531.nxs")

# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_dir = os.path.join(DIRS[0], working_folder_name)

input_dir = os.path.join(working_dir, input_folder_name)
output_dir = os.path.join(working_dir, output_folder_name)

calibration_map_path = os.path.join(input_dir, calibration_map_rel_path)
calibration_dir = os.path.join(input_dir, calibration_folder_name)
spline_path = os.path.join(calibration_dir, spline_rel_path)
unsplined_van_path = os.path.join(calibration_dir, unsplined_van_rel_path)
summed_empty_path = os.path.join(calibration_dir, summed_empty_rel_path)

total_scattering_input_file = os.path.join(input_dir, "ISIS_Powder-POLARIS98533_TotalScatteringInput.nxs")
total_scattering_input_file_per_det = os.path.join(input_dir, "ISIS_Powder-POLARIS98533_TotalScatteringInputPerDetector.nxs")


class CreateVanadiumTest(systemtesting.MantidSystemTest):
    calibration_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        self.calibration_results = run_vanadium_calibration(False)

    def validate(self):
        splined_ws, unsplined_ws = self.calibration_results
        for ws in splined_ws + unsplined_ws:
            self.assertEqual(ws.sample().getMaterial().name(), "V")
        return (
            unsplined_ws.name(),
            "ISIS_Powder-POLARIS00098532_unsplined.nxs",
            splined_ws.name(),
            "ISIS_Powder-POLARIS00098532_splined.nxs",
        )

    def cleanup(self):
        try:
            _try_delete(output_dir)
            _try_delete(spline_path)
            _try_delete(unsplined_van_path)
            _try_delete(summed_empty_path)
        finally:
            mantid.mtd.clear()
            config["datasearch.directories"] = self.existing_config


class CreateVanadiumPerDetectorTest(systemtesting.MantidSystemTest):
    calibration_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        self.calibration_results = run_vanadium_calibration(True)

    def validate(self):
        splined_ws, unsplined_ws = self.calibration_results
        ws = splined_ws + unsplined_ws
        self.assertEqual(ws.sample().getMaterial().name(), "V")
        return (
            unsplined_ws.name(),
            "ISIS_Powder-POLARIS00098532_unsplined_per_det.nxs",
            splined_ws.name(),
            "ISIS_Powder-POLARIS00098532_splined_per_det.nxs",
        )

    def cleanup(self):
        try:
            _try_delete(output_dir)
            _try_delete(spline_path)
            _try_delete(unsplined_van_path)
            _try_delete(summed_empty_path)
        finally:
            mantid.mtd.clear()
            config["datasearch.directories"] = self.existing_config


class FocusTestNoAbsorption(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        self.focus_results = run_focus_no_absorption()

    def validate(self):
        # check output files as expected
        def generate_error_message(expected_file, output_dir):
            return "Unable to find {} in {}.\nContents={}".format(expected_file, output_dir, os.listdir(output_dir))

        def assert_output_file_exists(directory, filename):
            self.assertTrue(os.path.isfile(os.path.join(directory, filename)), msg=generate_error_message(filename, directory))

        user_output = os.path.join(output_dir, "17_1", "Test")
        assert_output_file_exists(user_output, "POLARIS98533.nxs")
        assert_output_file_exists(user_output, "POLARIS98533.gsas")
        output_dat_dir = os.path.join(user_output, "dat_files")
        for bankno in range(1, 6):
            assert_output_file_exists(output_dat_dir, "POL98533-b_{}-TOF.dat".format(bankno))
            assert_output_file_exists(output_dat_dir, "POL98533-b_{}-d.dat".format(bankno))

        for ws in self.focus_results:
            self.assertEqual(ws.sample().getMaterial().name(), "Si Si")
        self.tolerance_is_rel_err = True
        self.tolerance = 1e-6
        return self.focus_results.name(), "ISIS_Powder-POLARIS98533_FocusSempty.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            mantid.mtd.clear()
            config["datasearch.directories"] = self.existing_config


class FocusTestAbsorptionPaalmanPings(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        self.focus_results = run_focus_absorption("98533", paalman_pings=True)

    def validate(self):
        self.disableChecking.append("Uncertainty")

        # check output files as expected
        def generate_error_message(expected_file, output_dir):
            return "Unable to find {} in {}.\nContents={}".format(expected_file, output_dir, os.listdir(output_dir))

        def assert_output_file_exists(directory, filename):
            self.assertTrue(os.path.isfile(os.path.join(directory, filename)), msg=generate_error_message(filename, directory))

        user_output = os.path.join(output_dir, "17_1", "Test")
        assert_output_file_exists(user_output, "POLARIS98533.nxs")
        assert_output_file_exists(user_output, "POLARIS98533.gsas")
        output_dat_dir = os.path.join(user_output, "dat_files")
        for bankno in range(1, 6):
            assert_output_file_exists(output_dat_dir, "POL98533-b_{}-TOF.dat".format(bankno))
            assert_output_file_exists(output_dat_dir, "POL98533-b_{}-d.dat".format(bankno))

        for ws in self.focus_results:
            self.assertEqual(ws.sample().getMaterial().name(), "Si Si")
        self.tolerance_is_rel_err = True
        self.tolerance = 1e-6
        return self.focus_results.name(), "ISIS_Powder-POLARIS98533_FocusPaalmanPings.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            mantid.mtd.clear()
            config["datasearch.directories"] = self.existing_config


class FocusTestAbsorptionMayers(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        self.focus_results = run_focus_absorption("98533", paalman_pings=False)

    def validate(self):
        # check output files as expected
        def generate_error_message(expected_file, output_dir):
            return "Unable to find {} in {}.\nContents={}".format(expected_file, output_dir, os.listdir(output_dir))

        def assert_output_file_exists(directory, filename):
            self.assertTrue(os.path.isfile(os.path.join(directory, filename)), msg=generate_error_message(filename, directory))

        user_output = os.path.join(output_dir, "17_1", "Test")
        assert_output_file_exists(user_output, "POLARIS98533.nxs")
        assert_output_file_exists(user_output, "POLARIS98533.gsas")
        output_dat_dir = os.path.join(user_output, "dat_files")
        for bankno in range(1, 6):
            assert_output_file_exists(output_dat_dir, "POL98533-b_{}-TOF.dat".format(bankno))
            assert_output_file_exists(output_dat_dir, "POL98533-b_{}-d.dat".format(bankno))

        for ws in self.focus_results:
            self.assertEqual(ws.sample().getMaterial().name(), "Si Si")
        self.tolerance_is_rel_err = True
        self.tolerance = 1e-5
        # MayersSampleCorrection involves a fit that may give slightly different results on different OS
        return self.focus_results.name(), "ISIS_Powder-POLARIS98533_FocusMayers.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            mantid.mtd.clear()
            config["datasearch.directories"] = self.existing_config


class FocusTestChopperMode(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        self.focus_results = run_focus_no_chopper("98533")

    def validate(self):
        # This will only pass if instead of failing or deafaulting to PDF it correctly picks Rietveld
        for ws in self.focus_results:
            self.assertEqual(ws.sample().getMaterial().name(), "Si Si")
        # this needs to be put in due to rounding errors between OS' for the proton_charge_by_period log
        self.disableChecking.append("Sample")
        self.tolerance = 1e-7
        return self.focus_results.name(), "ISIS_Powder-POLARIS98533_Auto_chopper.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            mantid.mtd.clear()
            config["datasearch.directories"] = self.existing_config


class FocusTestRunTwice(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        self.focus_results = run_focus_absorption("98533", paalman_pings=False)
        self.focus_results = run_focus_absorption("98533", paalman_pings=False)

    def validate(self):
        self.tolerance_is_rel_err = True
        self.tolerance = 1e-5  # same tolused in FocusTestAbsorptionMayers
        return self.focus_results.name(), "ISIS_Powder-POLARIS98533_FocusMayers.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            mantid.mtd.clear()
            config["datasearch.directories"] = self.existing_config


class FocusTestPerDetector(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        self.focus_results = run_focus_no_absorption(per_detector=True)

    def validate(self):
        # check output files as expected
        def generate_error_message(expected_file, output_dir):
            return "Unable to find {} in {}.\nContents={}".format(expected_file, output_dir, os.listdir(output_dir))

        def assert_output_file_exists(directory, filename):
            self.assertTrue(os.path.isfile(os.path.join(directory, filename)), msg=generate_error_message(filename, directory))

        user_output = os.path.join(output_dir, "17_1", "Test")
        assert_output_file_exists(user_output, "POLARIS98533.nxs")
        assert_output_file_exists(user_output, "POLARIS98533.gsas")
        output_dat_dir = os.path.join(user_output, "dat_files")
        for bankno in range(1, 6):
            assert_output_file_exists(output_dat_dir, "POL98533-b_{}-TOF.dat".format(bankno))
            assert_output_file_exists(output_dat_dir, "POL98533-b_{}-d.dat".format(bankno))

        for ws in self.focus_results:
            self.assertEqual(ws.sample().getMaterial().name(), "Si Si")
        self.tolerance_is_rel_err = True
        self.tolerance = 1e-5
        return self.focus_results.name(), "ISIS_Powder-POLARIS98533_FocusPerDet.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            mantid.mtd.clear()
            config["datasearch.directories"] = self.existing_config


class TotalScatteringTest(systemtesting.MantidSystemTest):
    pdf_output = None

    def runTest(self):
        setup_mantid_paths()
        # Load Focused ws
        mantid.LoadNexus(Filename=total_scattering_input_file, OutputWorkspace="98533-ResultTOF")
        self.pdf_output = run_total_scattering("98533", False)

    def validate(self):
        # Whilst total scattering is in development, the validation will avoid using reference files as they will have
        # to be updated very frequently. In the meantime, the expected peak in the PDF at ~3.9 Angstrom will be checked
        expected_peak_values = [0.8808, 1.1001, 2.9530, 4.6593, 4.3521]
        for index, ws in enumerate(self.pdf_output):
            idx = get_bin_number_at_given_r(ws.dataX(0), 3.9)
            self.assertAlmostEqual(ws.dataY(0)[idx], expected_peak_values[index], places=3)


class TotalScatteringMergedTest(systemtesting.MantidSystemTest):
    pdf_output = None

    def runTest(self):
        setup_mantid_paths()
        # Load Focused ws
        mantid.LoadNexus(Filename=total_scattering_input_file, OutputWorkspace="98533-ResultTOF")
        q_lims = np.array([2.5, 3, 4, 6, 7, 3.5, 5, 7, 11, 40]).reshape((2, 5))
        self.pdf_output = run_total_scattering("98533", True, q_lims=q_lims)

    def validate(self):
        # Whilst total scattering is in development, the validation will avoid using reference files as they will have
        # to be updated very frequently. In the meantime, the expected peak in the PDF at ~3.9 Angstrom will be checked.
        idx = get_bin_number_at_given_r(self.pdf_output.dataX(0), 3.9)
        self.assertAlmostEqual(self.pdf_output.dataY(0)[idx], 4.5799, places=3)


class TotalScatteringMergedPerDetTest(systemtesting.MantidSystemTest):
    pdf_output = None

    def runTest(self):
        setup_mantid_paths()
        # Load Focused ws
        mantid.LoadNexus(Filename=total_scattering_input_file_per_det, OutputWorkspace="98533-ResultTOF")
        q_lims = np.array([2.5, 3, 4, 6, 7, 3.5, 5, 7, 11, 40]).reshape((2, 5))
        self.pdf_output = run_total_scattering("98533", True, q_lims=q_lims, per_detector=True)

    def validate(self):
        # Whilst total scattering is in development, the validation will avoid using reference files as they will have
        # to be updated very frequently. In the meantime, the expected peak in the PDF at ~3.9 Angstrom will be checked.
        idx = get_bin_number_at_given_r(self.pdf_output.dataX(0), 3.9)
        self.assertAlmostEqual(self.pdf_output.dataY(0)[idx], 4.5305, places=3)


class TotalScatteringPDFRebinTest(systemtesting.MantidSystemTest):
    pdf_output = None

    def runTest(self):
        setup_mantid_paths()
        # Load Focused ws
        mantid.LoadNexus(Filename=total_scattering_input_file, OutputWorkspace="98533-ResultTOF")
        q_lims = np.array([2.5, 3, 4, 6, 7, 3.5, 5, 7, 11, 40]).reshape((2, 5))
        self.pdf_output = run_total_scattering("98533", True, q_lims=q_lims, delta_r=0.1)

    def validate(self):
        # Whilst total scattering is in development, the validation will avoid using reference files as they will have
        # to be updated very frequently. In the meantime, the rebin test will be done by testing the histogram size in
        # a truncated WS
        self.assertAlmostEqual(self.pdf_output.dataX(0).size, 201, places=3)


class TotalScatteringMergedRebinTest(systemtesting.MantidSystemTest):
    pdf_output = None

    def runTest(self):
        setup_mantid_paths()
        # Load Focused ws
        mantid.LoadNexus(Filename=total_scattering_input_file, OutputWorkspace="98533-ResultTOF")
        q_lims = np.array([2.5, 3, 4, 6, 7, 3.5, 5, 7, 11, 40]).reshape((2, 5))
        self.pdf_output = run_total_scattering("98533", True, q_lims=q_lims, delta_q=0.1)

    def validate(self):
        # Whilst total scattering is in development, the validation will avoid using reference files as they will have
        # to be updated very frequently. In the meantime, the rebin test will be done by testing the histogram size in
        # a truncated WS
        self.assertAlmostEqual(self.pdf_output.dataX(0).size, 255, places=3)


class TotalScatteringPdfTypeTest(systemtesting.MantidSystemTest):
    pdf_output = None

    def runTest(self):
        setup_mantid_paths()
        # Load Focused ws
        mantid.LoadNexus(Filename=total_scattering_input_file, OutputWorkspace="98533-ResultTOF")
        q_lims = np.array([2.5, 3, 4, 6, 7, 3.5, 5, 7, 11, 40]).reshape((2, 5))
        self.pdf_output = run_total_scattering("98533", True, q_lims=q_lims, pdf_type="g(r)")

    def validate(self):
        # Whilst total scattering is in development, the validation will avoid using reference files as they will have
        # to be updated very frequently. In the meantime, the expected peak in the PDF at ~3.9 Angstrom will be checked.
        idx = get_bin_number_at_given_r(self.pdf_output.dataX(0), 3.9)
        self.assertAlmostEqual(self.pdf_output.dataY(0)[idx], 2.8955, places=3)
        self.assertEqual(self.pdf_output.name(), "98533_pdf_g(r)")


class TotalScatteringPdfNameTest(systemtesting.MantidSystemTest):
    pdf_output = None

    def runTest(self):
        setup_mantid_paths()
        # Load Focused ws
        mantid.LoadNexus(Filename=total_scattering_input_file, OutputWorkspace="98533-ResultTOF")
        q_lims = np.array([2.5, 3, 4, 6, 7, 3.5, 5, 7, 11, 40]).reshape((2, 5))
        self.pdf_output = run_total_scattering("98533", True, q_lims=q_lims, pdf_type="g(r)", pdf_output_name="test_pdf_output")

    def validate(self):
        self.assertEqual(self.pdf_output.name(), "test_pdf_output")


class TotalScatteringFourierFilterTest(systemtesting.MantidSystemTest):
    pdf_output = None
    r_min = 1.0

    def runTest(self):
        setup_mantid_paths()
        # Load Focused ws
        mantid.LoadNexus(Filename=total_scattering_input_file, OutputWorkspace="98533-ResultTOF")
        q_lims = np.array([2.5, 3, 4, 6, 7, 3.5, 5, 7, 11, 40]).reshape((2, 5))
        self.pdf_output = run_total_scattering("98533", True, q_lims=q_lims, pdf_type="g(r)", freq_params=[self.r_min])

    def validate(self):
        # Whilst total scattering is in development, the validation will avoid using reference files as they will have
        # to be updated very frequently. In the meantime, the expected peak in the PDF at ~3.9 Angstrom will be checked.
        x_data = self.pdf_output.dataX(0)
        y_data = self.pdf_output.dataY(0)
        idx_to_check_filter = 5
        self.assertTrue(x_data[idx_to_check_filter] < self.r_min)
        self.assertAlmostEqual(y_data[idx_to_check_filter], 0.0, places=1)
        idx = get_bin_number_at_given_r(self.pdf_output.dataX(0), 3.9)
        self.assertAlmostEqual(y_data[idx], 2.6732, places=3)


class TotalScatteringLorchFilterTest(systemtesting.MantidSystemTest):
    pdf_output = None

    def runTest(self):
        setup_mantid_paths()
        # Load Focused ws
        mantid.LoadNexus(Filename=total_scattering_input_file, OutputWorkspace="98533-ResultTOF")
        q_lims = np.array([2.5, 3, 4, 6, 7, 3.5, 5, 7, 11, 40]).reshape((2, 5))
        self.pdf_output = run_total_scattering("98533", True, q_lims=q_lims, lorch_filter=False)

    def validate(self):
        # Whilst total scattering is in development, the validation will avoid using reference files as they will have
        # to be updated very frequently. In the meantime, the expected peak in the PDF at ~3.9 Angstrom will be checked.
        idx = get_bin_number_at_given_r(self.pdf_output.dataX(0), 3.9)
        self.assertAlmostEqual(self.pdf_output.dataY(0)[idx], 8.8473, places=3)


def run_total_scattering(
    run_number,
    merge_banks,
    q_lims=None,
    delta_q=None,
    delta_r=None,
    pdf_type="G(r)",
    freq_params=None,
    lorch_filter=True,
    per_detector=False,
    pdf_output_name=None,
):
    pdf_inst_obj = setup_inst_object(mode="PDF")
    return pdf_inst_obj.create_total_scattering_pdf(
        run_number=run_number,
        merge_banks=merge_banks,
        q_lims=q_lims,
        delta_q=delta_q,
        delta_r=delta_r,
        pdf_type=pdf_type,
        lorch_filter=lorch_filter,
        freq_params=freq_params,
        per_detector_vanadium=per_detector,
        pdf_output_name=pdf_output_name,
    )


def _gen_required_files():
    required_run_numbers = ["98531", "98532", "98533"]  # create_van : PDF mode  # File to focus (Si)

    # Generate file names of form "INSTxxxxx.nxs"
    input_files = [os.path.join(input_dir, (inst_name + "000" + number + ".nxs")) for number in required_run_numbers]
    input_files.append(calibration_map_path)
    input_files.append(total_scattering_input_file)
    return input_files


def run_vanadium_calibration(per_detector):
    vanadium_run = 98532  # Choose arbitrary run in the cycle 17_1

    pdf_inst_obj = setup_inst_object(mode="PDF")

    # Run create vanadium twice to ensure we get two different output splines / files
    pdf_inst_obj.create_vanadium(
        first_cycle_run_no=vanadium_run,
        do_absorb_corrections=True,
        multiple_scattering=True,
        per_detector_vanadium=per_detector,
        mayers_mult_scat_events=100,
    )

    # Check the spline looks good and was saved
    if not os.path.exists(spline_path):
        raise RuntimeError("Could not find output spline at the following path: " + spline_path)
    splined_ws = mantid.Load(Filename=spline_path)
    unsplined_ws = mantid.Load(Filename=unsplined_van_path)

    return splined_ws, unsplined_ws


def run_focus_no_absorption(per_detector=False):
    run_number = 98533
    sample_empty = 98532  # Use the vanadium empty again to make it obvious
    sample_empty_scale = 0.5  # Set it to 50% scale

    # Copy the required splined file into place first (instead of relying on generated one)
    if per_detector:
        splined_file_name = "POLARIS00098532_splined_per_det.nxs"
    else:
        splined_file_name = "POLARIS00098532_splined.nxs"

    original_splined_path = os.path.join(input_dir, splined_file_name)
    shutil.copy(original_splined_path, spline_path)

    inst_object = setup_inst_object(mode="PDF")
    return inst_object.focus(
        run_number=run_number,
        input_mode="Individual",
        do_van_normalisation=True,
        do_absorb_corrections=False,
        sample_empty=sample_empty,
        sample_empty_scale=sample_empty_scale,
        per_detector_vanadium=per_detector,
    )


def run_focus_no_chopper(run_number):
    sample_empty = 98532  # Use the vanadium empty again to make it obvious
    sample_empty_scale = 0.5  # Set it to 50% scale

    # Copy the required splined file into place first (instead of relying on generated one)
    splined_file_name = "POLARIS00098532_splined.nxs"

    original_splined_path = os.path.join(input_dir, splined_file_name)
    shutil.copy(original_splined_path, spline_path)

    inst_object = setup_inst_object(None)
    return inst_object.focus(
        run_number=run_number,
        input_mode="Individual",
        do_van_normalisation=True,
        do_absorb_corrections=False,
        sample_empty=sample_empty,
        sample_empty_scale=sample_empty_scale,
    )


def run_focus_absorption(run_number, paalman_pings=False):
    sample_empty = 98532  # Use the vanadium empty again to make it obvious
    sample_empty_scale = 0.5  # ignored if paalman_pings True

    # Copy the required splined file into place first (instead of relying on generated one)
    splined_file_name = "POLARIS00098532_splined.nxs"

    original_splined_path = os.path.join(input_dir, splined_file_name)
    shutil.copy(original_splined_path, spline_path)

    inst_object = setup_inst_object("PDF", with_container=True)
    if paalman_pings:
        inst_object._inst_settings.empty_can_subtraction_method = "PaalmanPings"  # the default is Simple
        inst_object._inst_settings.paalman_pings_events_per_point = 1

        return inst_object.focus(
            run_number=run_number,
            input_mode="Summed",
            do_van_normalisation=True,
            do_absorb_corrections=True,
            sample_empty=sample_empty,
            multiple_scattering=False,
        )
    else:
        return inst_object.focus(
            run_number=run_number,
            input_mode="Summed",
            do_van_normalisation=True,
            do_absorb_corrections=True,
            sample_empty=sample_empty,
            sample_empty_scale=sample_empty_scale,
            multiple_scattering=False,
        )


def setup_mantid_paths():
    config["datasearch.directories"] += ";" + input_dir


def setup_inst_object(mode, with_container=False):
    user_name = "Test"
    if mode:
        inst_obj = Polaris(
            user_name=user_name,
            calibration_mapping_file=calibration_map_path,
            calibration_directory=calibration_dir,
            output_directory=output_dir,
            mode=mode,
        )
    else:
        inst_obj = Polaris(
            user_name=user_name,
            calibration_mapping_file=calibration_map_path,
            calibration_directory=calibration_dir,
            output_directory=output_dir,
        )

    sample_details = SampleDetails(height=4.0, radius=0.2985, center=[0, 0, 0], shape="cylinder")
    # define multi-atom cell to stop regression of bug calling MaterialBuilder.build() without setting number density
    # which works for one atom cells as number density is automatically calculated
    sample_details.set_material(chemical_formula="Si Si", number_density=0.04996)
    if with_container:
        sample_details.set_container(radius=0.3175, chemical_formula="V")
    inst_obj.set_sample_details(sample=sample_details)

    return inst_obj


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print("Could not delete output file at: ", path)


def get_bin_number_at_given_r(r_data, r):
    r_edges = r_data
    r_centres = (r_edges[:-1] + r_edges[1:]) / 2
    diffs = [abs(i - r) for i in r_centres]
    idx = diffs.index(min(diffs))
    return idx
