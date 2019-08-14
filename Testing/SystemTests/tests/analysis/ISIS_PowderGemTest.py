# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import os
import systemtesting
import shutil
import platform

import mantid.simpleapi as mantid
from mantid import config

from isis_powder import Gem, SampleDetails

DIRS = config['datasearch.directories'].split(';')

# Setup various path details

inst_name = "GEM"
# Relative to system data folder
working_folder_name = "ISIS_Powder"

# Relative to working folder
input_folder_name = "input"
output_folder_name = "output"

# Relative to input folder
calibration_folder_name = os.path.join("calibration", inst_name.lower())
calibration_map_rel_path = os.path.join("yaml_files", "gem_system_test_mapping.yaml")
spline_rel_path = os.path.join("17_1", "VanSplined_83608_offsets_2011_cycle111b.cal.nxs")

# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_dir = os.path.join(DIRS[0], working_folder_name)

input_dir = os.path.join(working_dir, input_folder_name)
output_dir = os.path.join(working_dir, output_folder_name)

calibration_map_path = os.path.join(input_dir, calibration_map_rel_path)
calibration_dir = os.path.join(input_dir, calibration_folder_name)
spline_path = os.path.join(calibration_dir, spline_rel_path)
generated_offset = os.path.join(calibration_dir, "19_1")


class CreateVanadiumTest(systemtesting.MantidSystemTest):
    calibration_results = None
    existing_config = config['datasearch.directories']

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        setup_mantid_paths()
        self.calibration_results = run_vanadium_calibration()

    def validate(self):
        return self.calibration_results.name(), \
               "ISIS_Powder-GEM-VanSplined_83608_offsets_2011_cycle111b.cal.nxs"

    def cleanup(self):
        try:
            _try_delete(output_dir)
            _try_delete(spline_path)
        finally:
            mantid.mtd.clear()
            config['datasearch.directories'] = self.existing_config


class FocusTestMixin(object):
    focus_results = None
    existing_config = config['datasearch.directories']

    def requiredFiles(self):
        return _gen_required_files()

    def doTest(self, absorb_corrections):
        # Gen vanadium calibration first
        setup_mantid_paths()
        self.focus_results = run_focus(absorb_corrections)

    def cleanup(self):
        try:
            _try_delete(spline_path)
            _try_delete(output_dir)
        finally:
            config['datasearch.directories'] = self.existing_config
            mantid.mtd.clear()


class FocusTestNoAbsCorr(FocusTestMixin, systemtesting.MantidSystemTest):

    def runTest(self):
        self.doTest(absorb_corrections=False)

    def validate(self):
        return self.focus_results.name(), "ISIS_Powder-GEM83605_FocusSempty.nxs"


class FocusTestWithAbsCorr(FocusTestMixin, systemtesting.MantidSystemTest):

    def runTest(self):
        self.doTest(absorb_corrections=True)

    def validate(self):
        return self.focus_results.name(), "ISIS_Powder-GEM83605_FocusSempty_abscorr.nxs"


class CreateCalTest(systemtesting.MantidSystemTest):
    focus_results = None
    existing_config = config['datasearch.directories']

    def requiredFiles(self):
        return _gen_required_files()

    def runTest(self):
        # Gen vanadium calibration first
        setup_mantid_paths()

        self.focus_results = run_calibration()

    def validate(self):
        self.tolerance = 1e-5
        if _current_os_has_gsl_lvl2():
            return self.focus_results.name(), "ISIS_Powder-GEM87618_grouped.nxs"
        else:
            return self.focus_results.name(), "ISIS_Powder-GEM87618_groupedGSAS1.nxs"

    def cleanup(self):
        try:
            _try_delete(generated_offset)
        finally:
            config['datasearch.directories'] = self.existing_config
            mantid.mtd.clear()


def _gen_required_files():
    required_run_numbers = ["83607", "83608",  # create_van : PDF mode
                            "83605", "83608_splined"]  # File to focus (Si)

    # Generate file names of form "INSTxxxxx.nxs"
    input_files = [os.path.join(input_dir, (inst_name + number + ".nxs")) for number in required_run_numbers]
    input_files.append(calibration_map_path)
    return input_files


def run_vanadium_calibration():
    vanadium_run = 83605  # Choose arbitrary run in the cycle 17_1

    pdf_inst_obj = setup_inst_object(mode="PDF")

    # Run create vanadium twice to ensure we get two different output splines / files
    pdf_inst_obj.create_vanadium(first_cycle_run_no=vanadium_run,
                                 do_absorb_corrections=True, multiple_scattering=False)

    # Check the spline looks good and was saved
    if not os.path.exists(spline_path):
        raise RuntimeError("Could not find output spline at the following path: " + spline_path)
    splined_ws = mantid.Load(Filename=spline_path)

    return splined_ws


def run_focus(absorb_corrections):
    run_number = 83605
    sample_empty = 83608  # Use the vanadium empty again to make it obvious
    sample_empty_scale = 0.5  # Set it to 50% scale

    # Copy the required splined file into place first (instead of relying on generated one)
    splined_file_name = "GEM83608_splined.nxs"

    original_splined_path = os.path.join(input_dir, splined_file_name)
    shutil.copy(original_splined_path, spline_path)

    inst_object = setup_inst_object(mode="PDF")
    if absorb_corrections:

        sample = SampleDetails(height=5.0, radius=0.3, center=[0,0,0], shape='cylinder')
        sample.set_material(chemical_formula='(Li7)14 Mg1.05 Si2 S12.05',
                            number_density=0.001641)
        inst_object.set_sample_details(sample=sample, mode="Rietveld")

    return inst_object.focus(run_number=run_number, input_mode="Individual", vanadium_normalisation=True,
                             do_absorb_corrections=absorb_corrections, multiple_scattering=False, sample_empty=sample_empty,
                             sample_empty_scale=sample_empty_scale)


def run_calibration():
    iron_run = 87618
    inst_object = setup_inst_object(mode="PDF")
    return inst_object.create_cal(run_number=iron_run)


def setup_mantid_paths():
    config['datasearch.directories'] += ";" + input_dir


def setup_inst_object(mode):
    user_name = "Test"

    inst_obj = Gem(user_name=user_name, calibration_mapping_file=calibration_map_path,
                   calibration_directory=calibration_dir, output_directory=output_dir, mode=mode)
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


def _current_os_has_gsl_lvl2():
    """ Check whether the current OS should be running GSLv2 """
    return platform.linux_distribution()[0].lower() == "ubuntu" or platform.mac_ver()[0] != ''
