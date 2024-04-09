# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import systemtesting
import shutil

import mantid.simpleapi as mantid
from mantid import config

from isis_powder.osiris import Osiris
from isis_powder.routines.sample_details import SampleDetails

DIRS = config["datasearch.directories"].split(";")

# Setup various path details

inst_name = "OSIRIS"
# Relative to system data folder
working_folder_name = "ISIS_Powder"

# Relative to working folder
input_folder_name = "input"

# Relative to input folder
calibration_folder_name = os.path.join("calibration", inst_name.lower())
calibration_map_rel_path = os.path.join("yaml_files", "osiris_system_test_mapping.yaml")

# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_dir = os.path.join(DIRS[0], working_folder_name)

input_dir = os.path.join(working_dir, input_folder_name)

calibration_map_path = os.path.join(input_dir, calibration_map_rel_path)
calibration_dir = os.path.join(input_dir, calibration_folder_name)


def setup_mantid_paths():
    config["datasearch.directories"] += ";" + input_dir


def setup_inst_object(user_name, output_dir):
    inst_obj = Osiris(
        user_name=user_name,
        calibration_mapping_file=calibration_map_path,
        calibration_directory=calibration_dir,
        output_directory=output_dir,
    )

    return inst_obj


def setup_sample_details():
    sample_details = SampleDetails(radius=1.1, height=8, center=[0, 0, 0], shape="cylinder")
    sample_details.set_material(chemical_formula="Cr2-Ga-N", number_density=0.02)
    sample_details.set_container(chemical_formula="Al", mass_density=2.71, radius=1.2)

    return sample_details


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print("Could not delete output file at: ", path)


class _OSIRISDiffractionFocusingTest(systemtesting.MantidSystemTest):
    existing_config = config["datasearch.directories"]
    refrence_ws_name = None
    required_run_files = []
    output_dir = None

    def skipTests(self):
        # Don't actually run this test, as it is a common interface for other tests tests
        return True

    def runPreTest(self):
        setup_mantid_paths()

    def requiredFiles(self):
        return self._gen_required_files()

    def validate(self):
        foccussed_ws = self.results
        return (foccussed_ws.name(), self.refrence_ws_name)

    def cleanup(self):
        try:
            _try_delete(self.output_dir)
        finally:
            mantid.mtd.clear()
            config["datasearch.directories"] = self.existing_config

    def _gen_required_files(self):
        input_files = [os.path.join(input_dir, file) for file in self.required_run_files]
        input_files.append(calibration_map_path)
        return input_files

    def run_diffraction_focusing(
        self,
        sample_runs,
        user_name,
        output_file,
        merge_drange=False,
        subtract_empty_can=False,
        vanadium_normalisation=False,
        sample_empty_scale=None,
        absorb_corrections=False,
        empty_can_subtraction_method="Simple",
        sample_details=None,
        paalman_pings_events_per_point=None,
        simple_events_per_point=None,
        neutron_paths_multiple=None,
        neutron_paths_single=None,
        multiple_scattering=False,
    ):
        sample_runs = sample_runs  # Choose full drange set in the cycle 1_1

        osiris_inst_obj = setup_inst_object(user_name, self.output_dir)

        osiris_inst_obj.create_vanadium(
            run_number=sample_runs,
            subtract_empty_can=subtract_empty_can,
        )

        if sample_details:
            osiris_inst_obj.set_sample_details(sample=sample_details)

        # Run diffraction focusing
        osiris_inst_obj.focus(
            run_number=sample_runs,
            merge_drange=merge_drange,
            subtract_empty_can=subtract_empty_can,
            vanadium_normalisation=vanadium_normalisation,
            sample_empty_scale=sample_empty_scale,
            absorb_corrections=absorb_corrections,
            empty_can_subtraction_method=empty_can_subtraction_method,
            paalman_pings_events_per_point=paalman_pings_events_per_point,
            simple_events_per_point=simple_events_per_point,
            neutron_paths_multiple=neutron_paths_multiple,
            neutron_paths_single=neutron_paths_single,
            multiple_scattering=multiple_scattering,
        )

        focussed_rel_path = os.path.join("1_1", user_name, output_file)
        focused_path = os.path.join(self.output_dir, focussed_rel_path)

        foccussed_ws = mantid.Load(Filename=focused_path)

        return foccussed_ws


class OSIRISDiffractionFocusingWithSubtractionTest(_OSIRISDiffractionFocusingTest):
    refrence_ws_name = "OSI119977_d_spacing.nxs"
    required_run_files = [
        "OSI82717.nxs",  # empty can
        "OSIRIS00119963.nxs",  # van
        "OSIRIS00119977.nxs",
    ]
    output_dir = os.path.join(working_dir, "output_with_subtraction")

    def runTest(self):
        super().runPreTest()
        self.results = self.run_diffraction_focusing(
            "119977",
            "Test",
            "OSI119977_d_spacing.nxs",
            subtract_empty_can=True,
        )

    def skipTests(self):
        return False


class OSIRISDiffractionFocusingWithMergingTest(_OSIRISDiffractionFocusingTest):
    refrence_ws_name = "OSI119977-119978_d_spacing.nxs"
    required_run_files = [
        "OSI82717.nxs",
        "OSI82718.nxs",  # empty can
        "OSIRIS00119963.nxs",
        "OSIRIS00119964.nxs",  # van
        "OSIRIS00119977.nxs",
        "OSIRIS00119978.nxs",
    ]
    output_dir = os.path.join(working_dir, "output_with_merging")

    def runTest(self):
        super().runPreTest()
        self.results = self.run_diffraction_focusing(
            "119977-119978",
            "Test_Merge",
            "OSI119977-119978_d_spacing.nxs",
            merge_drange=True,
        )

    def skipTests(self):
        return False


class OSIRISDiffractionFocusingWithSimpleAbsorptionCorrection(_OSIRISDiffractionFocusingTest):
    refrence_ws_name = "OSI120032_d_spacing_simple_corrected.nxs"
    required_run_files = [
        "OSI82717.nxs",  # empty can
        "OSIRIS00119963.nxs",
        "OSIRIS00120032.nxs",  # van
    ]
    output_dir = os.path.join(working_dir, "output_with_simple")

    def runTest(self):
        super().runPreTest()
        sample_details = setup_sample_details()

        self.results = self.run_diffraction_focusing(
            "120032",
            "Test_Simple_Absorb",
            "OSI120032_d_spacing.nxs",
            absorb_corrections=True,
            empty_can_subtraction_method="Simple",
            sample_details=sample_details,
            simple_events_per_point=100,
        )

    def skipTests(self):
        return False


class OSIRISDiffractionFocusingWithPaalmanPingsAbsorptionCorrection(_OSIRISDiffractionFocusingTest):
    refrence_ws_name = "OSI120032_d_spacing_paalman_corrected.nxs"
    required_run_files = [
        "OSI82718.nxs",  # empty can
        "OSIRIS00119964.nxs",  # van
        "OSIRIS00120032.nxs",
    ]
    output_dir = os.path.join(working_dir, "output_with_paalman")

    def runTest(self):
        self.tolerance = 1e-8
        self.tolerance_is_rel_err = True
        super().runPreTest()
        sample_details = setup_sample_details()

        self.results = self.run_diffraction_focusing(
            "120032",
            "Test_PaalmanPings_Absorb",
            "OSI120032_d_spacing.nxs",
            absorb_corrections=True,
            empty_can_subtraction_method="PaalmanPings",
            sample_details=sample_details,
            paalman_pings_events_per_point=100,
        )

    def skipTests(self):
        return False


class OSIRISDiffractionFocusingWithMultipleScattering(_OSIRISDiffractionFocusingTest):
    refrence_ws_name = "OSI120032_d_multiple_scattering_corrected.nxs"
    required_run_files = [
        "OSI82717.nxs",
        "OSI82718.nxs",  # empty can
        "OSIRIS00119963.nxs",
        "OSIRIS00119964.nxs",  # van
        "OSIRIS00120032.nxs",
    ]
    output_dir = os.path.join(working_dir, "output_with_multiple")

    def runTest(self):
        super().runPreTest()
        sample_details = setup_sample_details()

        self.results = self.run_diffraction_focusing(
            "120032",
            "Test_Multiple_Scattering",
            "OSI120032_d_spacing.nxs",
            absorb_corrections=True,
            empty_can_subtraction_method="Simple",
            sample_details=sample_details,
            multiple_scattering=True,
            simple_events_per_point=100,
            neutron_paths_single=5,
            neutron_paths_multiple=5,
        )

    def skipTests(self):
        return False
