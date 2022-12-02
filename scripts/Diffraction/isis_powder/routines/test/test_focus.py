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
# from unittest.mock import patch
from isis_powder import Polaris, SampleDetails
from isis_powder.routines import focus
from isis_powder.routines.common_enums import INPUT_BATCHING
from mantid import config
from mantid.simpleapi import Load, CompareWorkspaces, ExtractSpectra, SaveNexus
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


class ISISPowderFocusUnitTest(TestCase):

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
        inst_obj._inst_settings.spline_coeff = 5

        return inst_obj

    def test_focus_per_bank_no_input_batching(self):
        pdf_inst_obj = self.setup_inst_object("PDF")
        pdf_inst_obj._inst_settings.input_mode = "PDF"
        self.assertRaisesRegex(ValueError, "Input batching not passed through. Please contact development team.",
                               focus.focus, run_number_string=98532, instrument=pdf_inst_obj,
                               perform_vanadium_norm=True, absorb=True, sample_details=pdf_inst_obj._sample_details)

    def test_focus_per_bank_individual_input_batching(self):
        pdf_inst_obj = self.setup_inst_object("PDF")
        _ = pdf_inst_obj.create_vanadium(first_cycle_run_no=98532, do_absorb_corrections=True,
                                         multiple_scattering=False)
        ADS.clear()  # clear ADS to work around bug issue #34749
        pdf_inst_obj = self.setup_inst_object("PDF")
        pdf_inst_obj._inst_settings.input_mode = INPUT_BATCHING.Individual
        output = focus.focus(run_number_string=98532, instrument=pdf_inst_obj, perform_vanadium_norm=True, absorb=True,
                             sample_details=pdf_inst_obj._sample_details)
        SaveNexus(output, "/home/danielmurphy/Desktop/focus/focused_per_bank.nxs")
        expected_per_bank = Load("/home/danielmurphy/Desktop/focus/focused_per_bank.nxs")
        match_bool, _ = CompareWorkspaces(expected_per_bank, output)
        self.assertTrue(match_bool)

    def test_focus_per_bank_summed_input_batching(self):
        pdf_inst_obj = self.setup_inst_object("PDF")
        _ = pdf_inst_obj.create_vanadium(first_cycle_run_no=98532, do_absorb_corrections=True,
                                         multiple_scattering=False)
        ADS.clear()  # clear ADS to work around bug issue #34749
        pdf_inst_obj = self.setup_inst_object("PDF")
        pdf_inst_obj._inst_settings.input_mode = INPUT_BATCHING.Summed
        output = focus.focus(run_number_string=98532, instrument=pdf_inst_obj, perform_vanadium_norm=True, absorb=True,
                             sample_details=pdf_inst_obj._sample_details)
        SaveNexus(output, "/home/danielmurphy/Desktop/focus/focused_per_bank.nxs")
        expected_per_bank = Load("/home/danielmurphy/Desktop/focus/focused_per_bank.nxs")
        match_bool, _ = CompareWorkspaces(expected_per_bank, output)
        self.assertTrue(match_bool)


if __name__ == '__main__':
    main()
