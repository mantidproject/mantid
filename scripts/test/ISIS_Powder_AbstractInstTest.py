import unittest

import mantid as mantid  # For next import to work
from isis_powder.mock_instrument import MockInstrument
import os


class isis_powder_AbstractInstTest(unittest.TestCase):

    def test_init_sets_params(self):
        inst = self._get_abstract_inst_all_specified()

        # Default Params

        self.assertEquals(inst.calibration_dir, self.calibration_dir)
        self.assertEquals(inst.output_dir, self.output_dir)

        # Optional Params
        self.assertEquals(inst.default_input_ext, self.default_ext)


    def test_generate_full_input_path(self):
        inst = self._get_abstract_inst_all_specified()
        run_number = 12345
        inst_file_name = inst._generate_inst_file_name(run_number)
        input_path = "test"
        reference_output = os.path.join(input_path, (inst_file_name + self.default_ext))

        output = inst._generate_input_full_path(run_number, input_path)
        self.assertEquals(output, reference_output)

    # Test empty hooks work correctly
    def test_attenuate_workspace_hook(self):
        inst = self._get_abstract_inst_defaults()
        input_ws = "ws_in"

        output_ws = inst._attenuate_workspace(input_ws)
        self.assertEquals(input_ws, output_ws)

    def test_create_calibration_si(self):
        inst = self._get_abstract_inst_defaults()
        self.assertRaises(NotImplementedError, lambda: inst._create_calibration_silicon("", "", ""))

    def test_create_calibration(self):
        inst = self._get_abstract_inst_defaults()
        self.assertRaises(NotImplementedError, lambda: inst.create_calibration("", "", ""))

    def test_get_monitor_spectra_hook(self):
        inst = self._get_abstract_inst_defaults()
        unused_param = None
        output = inst._get_monitor_spectra(unused_param)

        # Should return empty string
        self.assertEquals(output, str(""))

    def test_spline_background_hook(self):
        inst = self._get_abstract_inst_defaults()
        # Use type this isn't None to make sure its not returning to us
        unused_param = "unused"
        output = inst._spline_background(unused_param, unused_param, unused_param)

        self.assertEquals(isinstance(output, type(None)), True)

    # Helper methods to create fresh instrument objects

    def _get_abstract_inst_defaults(self):
        return MockInstrument(user_name=self.user_name, calibration_dir=self.calibration_dir,
                              output_dir=self.output_dir)
    
    def _get_abstract_inst_all_specified(self):
        return MockInstrument(user_name=self.user_name, calibration_dir=self.calibration_dir,
                              output_dir=self.output_dir, default_ext=self.default_ext)

    # Test params
    user_name = "unit_test_abstract_inst"
    calibration_dir = "calDir"
    output_dir = "outDir"

    # Optional Params
    default_ext = ".ext"


if __name__ == '__main__':
    unittest.main()





