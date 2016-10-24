import unittest
import mantid

from PearlPowder_Mock import PearlPowder_Mock


class PearlPowder_AbstractInstTest(unittest.TestCase):

    def test_init_sets_params(self):
        inst = self.get_abstract_inst_all_specified()

        # Default Params

        self.assertEquals(inst.calibration_dir, self.calibration_dir)
        self.assertEquals(inst.raw_data_dir, self.raw_data_dir)
        self.assertEquals(inst.output_dir, self.output_dir)

        # Optional Params
        self.assertEquals(inst.tt_mode, self.tt_mode)
        self.assertEquals(inst.default_input_ext, self.default_ext)

    def test_generate_out_file_paths(self):
        inst = self.get_abstract_inst_defaults()

        test_out_dir = "testDir\\"
        reference_name = inst.generate_inst_file_name(run_number="123")
        file_name_without_ext = test_out_dir + reference_name

        output = inst.generate_out_file_paths(123, test_out_dir)

        self.assertEquals(output["nxs_filename"], file_name_without_ext+".nxs")
        self.assertEquals(output["gss_filename"], file_name_without_ext+".gss")
        self.assertEquals(output["tof_xye_filename"], file_name_without_ext+"_tof_xye.dat")
        self.assertEquals(output["dspacing_xye_filename"], file_name_without_ext+"_d_xye.dat")

        self.assertEquals(output["output_name"], reference_name)

    def test_generate_cycle_dir(self):
        inst = self.get_abstract_inst_defaults()
        # Have to manually set it for Windows path separator
        input_dir = self.raw_data_dir + '\\'
        inst.test_set_raw_data_dir(input_dir)
        inst_cycle = "test_1"
        reference_output = input_dir + inst_cycle + '\\'

        output = inst.generate_raw_data_cycle_dir(inst_cycle)
        self.assertEquals(output, reference_output)

        # Next test for UNIX paths
        unix_input_dir = self.raw_data_dir + '/'
        inst.test_set_raw_data_dir(unix_input_dir)
        reference_output = unix_input_dir + inst_cycle + '/'

        output = inst.generate_raw_data_cycle_dir(inst_cycle)
        self.assertEquals(output, reference_output)

    def test_generate_cycle_dir_throws(self):
        # Test generate cycle throws if there is no path seperator at the end
        inst = self.get_abstract_inst_defaults()
        inst_cycle = "test"

        self.assertRaises(ValueError, lambda: inst.generate_raw_data_cycle_dir(inst_cycle))

    def test_generate_cycle_dir_respects_skip_flag(self):
        # This test is for a function which supports the old API - it should
        # not change the path which was set for calibration dir
        inst = self.get_abstract_inst_defaults()
        inst.generate_cycle_dir_flag = True

        output = inst.generate_raw_data_cycle_dir("test_not_appended")
        self.assertEquals(output, self.raw_data_dir)

    def test_generate_full_input_path(self):
        inst = self.get_abstract_inst_all_specified()
        run_number = 12345
        inst_file_name = inst.generate_inst_file_name(run_number)
        input_path = "test\\"
        reference_output = input_path + inst_file_name + self.default_ext

        output = inst.generate_input_full_path(run_number, input_path)
        self.assertEquals(output, reference_output)

    # Test empty hooks work correctly
    def test_attenuate_workspace_hook(self):
        inst = self.get_abstract_inst_defaults()
        input_ws = "ws_in"

        output_ws = inst.attenuate_workspace(input_ws)
        self.assertEquals(input_ws, output_ws)

    def test_create_calibration_si(self):
        inst = self.get_abstract_inst_defaults()
        self.assertRaises(NotImplementedError, lambda: inst.create_calibration_si("", ""))

    def test_get_monitor_hook(self):
        inst = self.get_abstract_inst_defaults()
        # Use type this isn't None to make sure its not returning to us
        unused_param = "unused"
        output = inst.get_monitor(unused_param, unused_param, unused_param)

        self.assertEquals(isinstance(output, type(None)), True)

    def test_get_monitor_spectra_hook(self):
        inst = self.get_abstract_inst_defaults()
        unused_param = None
        output = inst.get_monitor_spectra(unused_param)

        # Should return empty string
        self.assertEquals(output, str(""))

    def test_spline_background_hook(self):
        inst = self.get_abstract_inst_defaults()
        # Use type this isn't None to make sure its not returning to us
        unused_param = "unused"
        output = inst.spline_background(unused_param, unused_param, unused_param)

        self.assertEquals(isinstance(output, type(None)), True)

    # Helper methods to create fresh instrument objects

    def get_abstract_inst_defaults(self):
        return PearlPowder_Mock(calibration_dir=self.calibration_dir, raw_data_dir=self.raw_data_dir, 
                                output_dir=self.output_dir)
    
    def get_abstract_inst_all_specified(self):
        return PearlPowder_Mock(calibration_dir=self.calibration_dir, raw_data_dir=self.raw_data_dir, 
                                output_dir=self.output_dir, default_ext=self.default_ext, tt_mode=self.tt_mode)

    # Test params
    calibration_dir = "calDir"
    raw_data_dir = "rawDir"
    output_dir = "outDir"

    # Optional Params
    default_ext = ".ext"
    tt_mode = "tt_test"


if __name__ == '__main__':
    unittest.main()





