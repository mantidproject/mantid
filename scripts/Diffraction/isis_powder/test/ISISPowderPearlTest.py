# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import patch
import mantid.simpleapi as mantid   # noqa: F401 # runs .pth file to add scripts subdirectories
from isis_powder.pearl import Pearl
from isis_powder.pearl_routines import pearl_advanced_config


class ISISPowderPearlTest(unittest.TestCase):

    @patch("isis_powder.abstract_inst.AbstractInst._focus")
    def test_long_mode_on(self, mock_focus):
        # firstly set long_mode as default in the Pearl object
        inst_obj = Pearl(user_name="PEARL", calibration_directory="dummy", output_directory="dummy",
                         do_absorb_corrections=False, perform_attenuation=False, vanadium_normalisation=False, long_mode=True)
        inst_obj.focus(run_number=999)
        mock_focus.assert_called_once()
        self.assertEqual(inst_obj._inst_settings.raw_data_crop_vals,
                         pearl_advanced_config.long_mode_on_params["raw_data_tof_cropping"])
        # now activate it in the call to focus
        inst_obj = Pearl(user_name="PEARL", calibration_directory="dummy", output_directory="dummy",
                         do_absorb_corrections=False, perform_attenuation=False, vanadium_normalisation=False, long_mode=False)
        inst_obj.focus(run_number=999, long_mode=True)
        mock_focus.assert_called_once()
        self.assertEqual(inst_obj._inst_settings.raw_data_crop_vals,
                         pearl_advanced_config.long_mode_on_params["raw_data_tof_cropping"])


if __name__ == '__main__':
    unittest.main()
