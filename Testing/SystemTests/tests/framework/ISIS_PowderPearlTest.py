# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import systemtesting
import shutil

import mantid.simpleapi as mantid
from mantid import config

from isis_powder import Pearl

DIRS = config["datasearch.directories"].split(";")

# Setup various path details

inst_name = "PEARL"
user_name = "Test"
# Relative to system data folder
working_folder_name = "ISIS_Powder"

# Relative to working folder
input_folder_name = "input"
output_folder_name = "output"

# Relative to input folder
calibration_folder_name = os.path.join("calibration", inst_name.lower())
calibration_map_rel_path = os.path.join("yaml_files", "pearl_system_test_mapping.yaml")
cycle = "17_1"
spline_rel_path = os.path.join(cycle, "VanSplined_98472_tt70_pearl_offset_16_4.cal.nxs")
summed_empty_rel_path = os.path.join(cycle, "summed_empty_98485.nxs")

# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_dir = os.path.join(DIRS[0], working_folder_name)

input_dir = os.path.join(working_dir, input_folder_name)
output_dir = os.path.join(working_dir, output_folder_name)
user_dir = os.path.join(output_dir, cycle, user_name)
xye_tof_outdir = os.path.join(user_dir, "ToF")
xye_dSpac_outdir = os.path.join(user_dir, "dSpacing")
gss_outdir = os.path.join(user_dir, "GSAS")

calibration_map_path = os.path.join(input_dir, calibration_map_rel_path)
calibration_dir = os.path.join(input_dir, calibration_folder_name)
spline_path = os.path.join(calibration_dir, spline_rel_path)
summed_empty_path = os.path.join(calibration_dir, summed_empty_rel_path)


class _CreateVanadiumTest(systemtesting.MantidSystemTest):
    existing_config = config["datasearch.directories"]
    focus_mode = None

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        inst_obj = setup_inst_object(tt_mode="tt70", focus_mode="trans")
        run_vanadium_calibration(inst_obj, focus_mode=self.focus_mode)

        # Make sure that inst settings reverted to the default after create_vanadium
        self.assertEqual(inst_obj._inst_settings.focus_mode, "trans")

    def skipTests(self):
        # Don't actually run this test, as it is a dummy for the focus-mode-specific tests
        return True

    def validate(self):
        self.tolerance = 0.05  # Required for difference in spline data between operating systems
        return (
            "PRL98472_tt70-d",
            "ISIS_Powder_PRL98472_tt70_{}.nxs".format(self.focus_mode),
            "Van_spline_data_tt70",
            "ISIS_Powder-PEARL00098472_splined.nxs",
        )

    def cleanup(self):
        try:
            _try_delete(output_dir)
            _try_delete(spline_path)
            _try_delete(summed_empty_path)
        finally:
            mantid.mtd.clear()
            config["datasearch.directories"] = self.existing_config


class CreateVanadiumAllTest(_CreateVanadiumTest):
    focus_mode = "all"

    def skipTests(self):
        return False


class CreateVanadiumTransTest(_CreateVanadiumTest):
    focus_mode = "trans"

    def skipTests(self):
        return False


class CreateVanadiumGroupsTest(_CreateVanadiumTest):
    focus_mode = "groups"

    def skipTests(self):
        return False


class CreateVanadiumModsTest(_CreateVanadiumTest):
    focus_mode = "mods"

    def skipTests(self):
        return False


class FocusTest(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        inst_object = setup_inst_object(tt_mode="tt88", focus_mode="Trans")
        self.focus_results = run_focus(inst_object, tt_mode="tt70", subtract_empty=True, spline_path=spline_path)

        # Make sure that inst settings reverted to the default after focus
        self.assertEqual(inst_object._inst_settings.tt_mode, "tt88")

    def validate(self):
        # check output files as expected
        def assert_output_file_exists(directory, filename):
            self.assertTrue(os.path.isfile(os.path.join(directory, filename)), msg=generate_error_message(filename, directory))

        assert_output_file_exists(user_dir, "PRL98507_tt70.nxs")
        assert_output_file_exists(gss_outdir, "PRL98507_tt70.gsas")
        assert_output_file_exists(xye_tof_outdir, "PRL98507_tt70_tof.xye")
        assert_output_file_exists(xye_dSpac_outdir, "PRL98507_tt70_d.xye")

        self.tolerance = 1e-8  # Required for difference in spline data between operating systems
        return "PRL98507_tt70-d", "ISIS_Powder-PEARL00098507_tt70Atten.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config["datasearch.directories"] = self.existing_config
            mantid.mtd.clear()


class FocusTestFocusModeTransSubset(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        inst_object = setup_inst_object(focus_mode="trans_subset")
        self.focus_results = run_focus(inst_object, tt_mode="tt70", subtract_empty=True, spline_path=spline_path, trans_mod_nums="1-9")

    def validate(self):
        def assert_output_file_exists(directory, filename):
            self.assertTrue(os.path.isfile(os.path.join(directory, filename)), msg=generate_error_message(filename, directory))

        assert_output_file_exists(user_dir, "PRL98507_tt70.nxs")
        assert_output_file_exists(gss_outdir, "PRL98507_tt70.gsas")
        assert_output_file_exists(xye_tof_outdir, "PRL98507_tt70_tof.xye")
        assert_output_file_exists(xye_dSpac_outdir, "PRL98507_tt70_d.xye")

        self.tolerance = 1e-8  # Required for difference in spline data between operating systems
        return "PRL98507_tt70-d", "ISIS_Powder-PEARL00098507_tt70Atten.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config["datasearch.directories"] = self.existing_config
            mantid.mtd.clear()


class FocusTestIncludingFileExtWorkspaceName(systemtesting.MantidSystemTest):
    # same as FocusTest but with added argument incl_file_ext_in_wsname=True
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        inst_object = setup_inst_object(tt_mode="tt88", focus_mode="Trans")
        self.focus_results = run_focus(
            inst_object, tt_mode="tt70", file_ext=".nxs", incl_file_ext_in_wsname=True, subtract_empty=True, spline_path=spline_path
        )

    def validate(self):
        # just check file extension (nxs) in workspace name
        return "PRL98507_tt70_nxs-d", "ISIS_Powder-PEARL00098507_tt70Atten.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config["datasearch.directories"] = self.existing_config
            mantid.mtd.clear()


class FocusLongThenShortTest(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()
        inst_object = setup_inst_object(tt_mode="tt88", focus_mode="Trans")
        inst_object.focus(
            run_number=98507,
            vanadium_normalisation=False,
            do_absorb_corrections=False,
            long_mode=True,
            perform_attenuation=False,
            tt_mode="tt70",
        )
        self.focus_results = run_focus(inst_object, tt_mode="tt70", subtract_empty=True, spline_path=spline_path)

        # Make sure that inst settings reverted to the default after focus
        self.assertEqual(inst_object._inst_settings.tt_mode, "tt88")
        self.assertFalse(inst_object._inst_settings.long_mode)

    def validate(self):
        # check output files as expected
        def assert_output_file_exists(directory, filename):
            self.assertTrue(os.path.isfile(os.path.join(directory, filename)), msg=generate_error_message(filename, directory))

        assert_output_file_exists(user_dir, "PRL98507_tt70.nxs")
        assert_output_file_exists(user_dir, "PRL98507_tt70_long.nxs")
        assert_output_file_exists(gss_outdir, "PRL98507_tt70.gsas")
        assert_output_file_exists(gss_outdir, "PRL98507_tt70_long.gsas")
        assert_output_file_exists(xye_tof_outdir, "PRL98507_tt70_tof.xye")
        assert_output_file_exists(xye_dSpac_outdir, "PRL98507_tt70_d.xye")
        assert_output_file_exists(xye_tof_outdir, "PRL98507_tt70_long_tof.xye")
        assert_output_file_exists(xye_dSpac_outdir, "PRL98507_tt70_long_d.xye")

        self.tolerance = 1e-8  # Required for difference in spline data between operating systems
        return (
            "PRL98507_tt70-d",
            "ISIS_Powder-PEARL00098507_tt70Atten.nxs",
            "PRL98507_tt70_long-d",
            "ISIS_Powder-PEARL00098507_tt70Long.nxs",
        )

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config["datasearch.directories"] = self.existing_config
            mantid.mtd.clear()


class FocusWithAbsorbCorrectionsTest(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        self.focus_results = run_focus_with_absorb_corrections()

    def validate(self):
        return "PRL98507_tt70-d", "ISIS_Powder-PEARL00098507_tt70_absorb.nxs"

    def cleanup(self):
        try:
            _try_delete(output_dir)
        finally:
            config["datasearch.directories"] = self.existing_config
            mantid.mtd.clear()


class FocusWithoutEmptySubtractionTest(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config["datasearch.directories"]

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        inst_object = setup_inst_object(tt_mode="tt70", focus_mode="Trans")
        self.focus_results = run_focus(inst_object, tt_mode="tt70", subtract_empty=False, spline_path=spline_path)

    def validate(self):
        return "PRL98507_tt70-d", "ISIS_Powder-PEARL00098507_tt70NoEmptySub.nxs"

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config["datasearch.directories"] = self.existing_config
            mantid.mtd.clear()


class CreateCalTest(systemtesting.MantidSystemTest):
    calibration_results = None
    existing_config = config["datasearch.directories"]
    run_number = 98494
    run_details = None

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        inst_object = setup_inst_object(tt_mode="tt88", focus_mode="trans")
        self.calibration_results = run_create_cal(inst_object, focus_mode="all", ceria_run=self.run_number)

        # Make sure that inst_settings reverted to the default after create_cal
        self.assertEqual(inst_object._inst_settings.focus_mode, "trans")

        self.run_details = inst_object._get_run_details(self.run_number)

    def validate(self):
        self.tolerance = 1e-5
        return "PRL98494_tt88_grouped", "ISIS_Powder-PEARL98494_grouped.nxs"

    def cleanup(self):
        try:
            _try_delete(self.run_details.offset_file_path)
        finally:
            config["datasearch.directories"] = self.existing_config
            mantid.mtd.clear()


class CreateVanadiumAndFocusCustomMode(systemtesting.MantidSystemTest):
    existing_config = config["datasearch.directories"]

    def runTest(self):
        setup_mantid_paths()
        inst_object = setup_inst_object(tt_mode="tt70", focus_mode="Trans")
        # for tt_mode=custom you can specify a different grouping file on each run. So need
        # to make sure the V file is tracked per grouping file and focus actions use the correct one
        run_vanadium_calibration(
            inst_object, tt_mode="custom", focus_mode="Trans", custom_grouping_filename=os.path.join(calibration_dir, "DAC_group.cal")
        )
        run_vanadium_calibration(
            inst_object,
            tt_mode="custom",
            focus_mode="Trans",
            custom_grouping_filename=os.path.join(calibration_dir, "pearl_group_12_1_TT70.cal"),
        )
        focus_results_grp = run_focus(
            inst_object, tt_mode="custom", subtract_empty=False, custom_grouping_filename=os.path.join(calibration_dir, "DAC_group.cal")
        )
        self.assertEqual(focus_results_grp.size(), 1)
        focus_results_grp = run_focus(
            inst_object,
            tt_mode="custom",
            subtract_empty=False,
            custom_grouping_filename=os.path.join(calibration_dir, "pearl_group_12_1_TT70.cal"),
        )
        self.assertEqual(focus_results_grp.size(), 10)

    def cleanup(self):
        spline_rel_path1 = os.path.join(cycle, "VanSplined_98472_custom_DAC_group_pearl_offset_16_4.cal.nxs")
        spline_path1 = os.path.join(calibration_dir, spline_rel_path1)
        spline_rel_path2 = os.path.join(cycle, "VanSplined_98472_custom_pearl_group_12_1_TT70_pearl_offset_16_4.cal.nxs")
        spline_path2 = os.path.join(calibration_dir, spline_rel_path2)
        try:
            _try_delete(spline_path1)
            _try_delete(spline_path2)
            _try_delete(output_dir)
            _try_delete(summed_empty_path)
        finally:
            config["datasearch.directories"] = self.existing_config
            mantid.mtd.clear()


def _gen_required_files():
    required_run_numbers = ["98472", "98485", "98507", "98472_splined", "98494"]  # create_van  # Focus (Si)  # create_cal (Ce)

    # Generate file names of form "INSTxxxxx.nxs" - PEARL requires 000 padding
    input_files = [os.path.join(input_dir, (inst_name + "000" + number + ".nxs")) for number in required_run_numbers]
    input_files.append(calibration_map_path)
    return input_files


def run_create_cal(inst_object, focus_mode, ceria_run):
    return inst_object.create_cal(run_number=ceria_run, focus_mode=focus_mode)


def run_vanadium_calibration(inst_object, **kwargs):
    vanadium_run = 98507  # Choose arbitrary run in the cycle 17_1

    # Run create vanadium twice to ensure we get two different output splines / files
    inst_object.create_vanadium(run_in_cycle=vanadium_run, do_absorb_corrections=True, **kwargs)


def run_focus(inst_object, tt_mode, subtract_empty, spline_path=None, custom_grouping_filename=None, **kwargs):
    run_number = 98507
    attenuation_file_name = "PRL112_DC25_10MM_FF.OUT"

    # Copy the required splined file into place first (instead of relying on generated one)
    splined_file_name = "PEARL00098472_splined.nxs"

    attenuation_path = os.path.join(calibration_dir, attenuation_file_name)
    if spline_path:
        original_splined_path = os.path.join(input_dir, splined_file_name)
        shutil.copy(original_splined_path, spline_path)

    default_kwargs = {
        "vanadium_normalisation": True,
        "do_absorb_corrections": False,
        "perform_attenuation": True,
        "attenuation_file": "ZTA",
        "attenuation_files": [{"name": "ZTA", "path": attenuation_path}],
    }
    kwargs = {**default_kwargs, **kwargs}  # overwrite defaults with kwargs passed

    return inst_object.focus(
        run_number=run_number,
        tt_mode=tt_mode,
        subtract_empty_instrument=subtract_empty,
        custom_grouping_filename=custom_grouping_filename,
        **kwargs,
    )


def run_focus_with_absorb_corrections():
    run_number = 98507
    inst_object = setup_inst_object(tt_mode="tt70", focus_mode="Trans")
    return inst_object.focus(
        run_number=run_number, vanadium_normalisation=False, perform_attenuation=False, do_absorb_corrections=True, long_mode=False
    )


def setup_mantid_paths():
    config["datasearch.directories"] += ";" + input_dir


def setup_inst_object(**kwargs):
    inst_obj = Pearl(
        user_name=user_name,
        calibration_mapping_file=calibration_map_path,
        long_mode=False,
        calibration_directory=calibration_dir,
        output_directory=output_dir,
        **kwargs,
    )
    return inst_obj


def generate_error_message(expected_file, output_dir):
    return "Unable to find {} in {}.\nContents={}".format(expected_file, output_dir, os.listdir(output_dir))


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print("Could not delete output file at: ", path)
