# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import patch, Mock
import mantid.simpleapi as mantid  # noqa: F401 # runs .pth file to add scripts subdirectories
from isis_powder.pearl import Pearl
from isis_powder.pearl_routines import pearl_advanced_config


class ISISPowderPearlTest(unittest.TestCase):
    @patch("isis_powder.routines.focus.focus")
    def test_long_mode_on(self, mock_focus):
        def generate_long_mode_checker(expected_value):
            if expected_value:
                long_mode_params = pearl_advanced_config.long_mode_on_params["raw_data_tof_cropping"]
            else:
                long_mode_params = pearl_advanced_config.long_mode_off_params["raw_data_tof_cropping"]

            def check_long_mode(*args, **kwargs):
                inst = kwargs.get("instrument")
                self.assertEqual(inst._inst_settings.long_mode, expected_value)
                self.assertEqual(inst._inst_settings.raw_data_crop_vals, long_mode_params)

            return check_long_mode

        mock_focus.side_effect = generate_long_mode_checker(True)
        # firstly set long_mode as default in the Pearl object
        inst_obj = Pearl(
            user_name="PEARL",
            calibration_directory="dummy",
            output_directory="dummy",
            do_absorb_corrections=False,
            perform_attenuation=False,
            vanadium_normalisation=False,
            long_mode=True,
        )
        inst_obj._output_focused_runs = Mock()
        inst_obj.focus(run_number=999)
        mock_focus.assert_called_once()

        # now activate it in the call to focus
        mock_focus.reset_mock()
        inst_obj = Pearl(
            user_name="PEARL",
            calibration_directory="dummy",
            output_directory="dummy",
            do_absorb_corrections=False,
            perform_attenuation=False,
            vanadium_normalisation=False,
            long_mode=False,
        )
        inst_obj._output_focused_runs = Mock()
        inst_obj.focus(run_number=999, long_mode=True)
        mock_focus.assert_called_once()

        # deactivate it in the call to focus
        mock_focus.reset_mock()
        mock_focus.side_effect = generate_long_mode_checker(False)
        inst_obj = Pearl(
            user_name="PEARL",
            calibration_directory="dummy",
            output_directory="dummy",
            do_absorb_corrections=False,
            perform_attenuation=False,
            vanadium_normalisation=False,
            long_mode=True,
        )
        inst_obj._output_focused_runs = Mock()
        inst_obj.focus(run_number=999, long_mode=False)
        mock_focus.assert_called_once()


if __name__ == "__main__":
    unittest.main()
