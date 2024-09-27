# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import copy
import unittest

from sans_core.common.enums import RowState
from sans_core.common.RowEntries import RowEntries, _UserEntries


class RowEntriesTest(unittest.TestCase):
    def test_settings_attr_resets_state(self):
        observed_attrs = vars(_UserEntries())

        for observed_attr in observed_attrs.keys():
            obj = RowEntries()
            obj.state = RowState.PROCESSED
            setattr(obj, observed_attr, "")

            self.assertEqual(RowState.UNPROCESSED, obj.state, "Row state did not reset for attr: {0}".format(observed_attr))

    def test_non_user_keys_keep_state(self):
        observed_attrs = ["tool_tip", "state"]

        for attr in observed_attrs:
            obj = RowEntries()
            obj.state = RowState.ERROR
            setattr(obj, attr, RowState.ERROR)

            self.assertEqual(RowState.ERROR, obj.state)  # This will likely stack-overflow instead of failing

    def test_is_multi_period(self):
        multi_period_keys = [
            "can_direct_period",
            "can_scatter_period",
            "can_transmission_period",
            "sample_direct_period",
            "sample_scatter_period",
            "sample_transmission_period",
        ]

        for key in multi_period_keys:
            obj = RowEntries()
            setattr(obj, key, 1.0)
            self.assertTrue(obj.is_multi_period())

    def test_is_empty(self):
        blank_obj = RowEntries()
        self.assertTrue(blank_obj.is_empty(), "Default Row Entries is not blank")

        for attr in vars(_UserEntries()).keys():
            obj = RowEntries()
            setattr(obj, attr, 1.0)
            self.assertFalse(obj.is_empty())

    def test_that_state_starts_unprocessed(self):
        obj = RowEntries()
        self.assertEqual(RowState.UNPROCESSED, obj.state)

    def test_deep_copy_eq(self):
        obj = RowEntries()
        # Randomly selected subset of fields
        for attr in ["sample_transmission", "can_direct_period", "user_file", "output_name", "tool_tip", "state", "background_ws"]:
            obj2 = copy.deepcopy(obj)
            self.assertEqual(obj, obj2)
            setattr(obj2, attr, 1)
            self.assertNotEqual(obj, obj2)


if __name__ == "__main__":
    unittest.main()
