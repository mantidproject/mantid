# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.Containers.MonitorID import MonitorID
from sans.user_file.settings_tags import TransId
from user_file.txt_parsers.ParserAdapterTestCommon import ParserAdapterTestCommon


class ParserAdapterTransmissionDetailsTest(ParserAdapterTestCommon, unittest.TestCase):
    def test_shift_spectrum(self):
        attr_map = {TransId.SPEC_4_SHIFT: 4, TransId.SPEC_5_SHIFT: 5}
        for key, spec_num in attr_map.items():
            expected_value = [(MonitorID(monitor_spec_num=spec_num), 1.2)]
            self.set_return_val({key: 1.2})

            returned = self.instance.get_transmission_details()
            self.assertIsNotNone(returned)
            self.assertEqual(expected_value, returned.spectrum_shifts)

    def test_all_other_attrs_transposed(self):
        attr_map = {TransId.SPEC: "selected_spectrum",
                    TransId.RADIUS: "selected_radius",
                    TransId.MASK: "selected_mask_filenames",
                    TransId.ROI: "selected_roi_filenames",
                    TransId.CAN_WORKSPACE: "selected_can_workspace",
                    TransId.SAMPLE_WORKSPACE: "selected_sample_workspace"}

        for old_key, new_attr in attr_map.items():
            expected_value = 123
            self.set_return_val({old_key: expected_value})

            returned = self.instance.get_transmission_details()

            self.assertIsNotNone(returned)
            self.assertEqual(expected_value, getattr(returned, new_attr),
                             msg="{0} did not translate correctly".format(new_attr))


if __name__ == '__main__':
    unittest.main()
