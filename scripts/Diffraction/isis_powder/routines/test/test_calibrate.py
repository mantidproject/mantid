# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import os
from unittest import TestCase, main
from unittest.mock import patch
from isis_powder import Polaris, SampleDetails
from isis_powder.routines import calibrate
from mantid import config
from mantid.simpleapi import Load, CompareWorkspaces, ExtractSpectra
from mantid.api import AnalysisDataService as ADS

DIRS = config['datasearch.directories'].split(';')

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

# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_dir = os.path.join(DIRS[0], working_folder_name)

input_dir = os.path.join(working_dir, input_folder_name)
output_dir = os.path.join(working_dir, output_folder_name)

calibration_map_path = os.path.join(input_dir, calibration_map_rel_path)
calibration_dir = os.path.join(input_dir, calibration_folder_name)
spline_path = os.path.join(calibration_dir, spline_rel_path)
unsplined_van_path = os.path.join(calibration_dir, unsplined_van_rel_path)

total_scattering_input_file = os.path.join(input_dir, "ISIS_Powder-POLARIS98533_TotalScatteringInput.nxs")


def crop_to_small_ws_for_test(input_workspace):
    return ExtractSpectra(InputWorkspace=input_workspace, OutputWorkspace=input_workspace, XMin=2000, XMax=2020,
                          DetectorList='672, 101001-101002,201001-201002,301001-301002,'
                                       '401001-401002,501001-501002,601001-601002')


class ISISPowderCalibrateUnitTest(TestCase):

    def tearDown(self) -> None:
        ADS.clear()

    def setup_inst_object(self, mode, with_container=False):
        user_name = "Test"
        if mode:
            inst_obj = Polaris(user_name=user_name, calibration_mapping_file=calibration_map_path,
                               calibration_directory=calibration_dir, output_directory=output_dir, mode=mode)
        else:
            inst_obj = Polaris(user_name=user_name, calibration_mapping_file=calibration_map_path,
                               calibration_directory=calibration_dir, output_directory=output_dir)

        sample_details = SampleDetails(height=4.0, radius=0.2985, center=[0, 0, 0], shape='cylinder')
        sample_details.set_material(chemical_formula='Si')
        if with_container:
            sample_details.set_container(radius=0.3175, chemical_formula='V')
        inst_obj.set_sample_details(sample=sample_details)

        return inst_obj

    def test_create_vanadium_per_bank(self):
        pdf_inst_obj = self.setup_inst_object("PDF")
        d_spacing_group = pdf_inst_obj.create_vanadium(first_cycle_run_no=98532, do_absorb_corrections=True,
                                                       multiple_scattering=False)
        expected_per_bank = Load("/home/danielmurphy/Desktop/calibrate/vanadium_group_per_bank.nxs")
        match_bool, _ = CompareWorkspaces(expected_per_bank, d_spacing_group)
        self.assertTrue(match_bool)

    @patch("isis_powder.routines.calibrate.crop_to_small_ws_for_test", side_effect=crop_to_small_ws_for_test)
    @patch("isis_powder.polaris_routines.polaris_algs.mantid.MaskDetectors")
    def test_create_vanadium_per_detector(self, mock_mask_spectra, mock_override_crop_for_test):
        pdf_inst_obj = self.setup_inst_object("PDF")
        run_details = pdf_inst_obj._get_run_details(run_number_string="98532")
        unsplined_workspace = calibrate.create_van_per_detector(instrument=pdf_inst_obj, run_details=run_details,
                                                                absorb=True, test=True)
        expected_per_bank = Load("/home/danielmurphy/Desktop/calibrate/unsplined_workspace.nxs")
        match_bool, _ = CompareWorkspaces(expected_per_bank, unsplined_workspace)
        self.assertTrue(match_bool)


if __name__ == '__main__':
    main()
