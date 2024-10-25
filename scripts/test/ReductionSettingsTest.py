# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import PropertyManagerDataService
from reduction_settings import get_settings_object


class BasicSettingsObjectUsageTest(unittest.TestCase):
    def setUp(self):
        self.settings = get_settings_object("BasicSettingsObjectUsageTest")

    def tearDown(self):
        for prop_man_name in PropertyManagerDataService.getObjectNames():
            PropertyManagerDataService.remove(prop_man_name)

    def test_string_roundtrip(self):
        self.settings["name"] = "value"
        self.assertEqual(self.settings["name"], "value")

    def test_float_roundtrip(self):
        self.settings["name"] = 0.1
        self.assertEqual(self.settings["name"], 0.1)

    def test_int_roundtrip(self):
        self.settings["name"] = 1
        self.assertEqual(self.settings["name"], 1)

    def test_keys(self):
        self.settings["A"] = 1
        self.settings["B"] = 2
        self.assertEqual(self.settings.keys(), ["A", "B"])

    def test_values(self):
        self.settings["A"] = 1
        self.settings["B"] = 2
        self.assertEqual(self.settings.values(), [1, 2])

    def test_items(self):
        self.settings["A"] = 1
        self.settings["B"] = 2
        self.assertEqual(self.settings.items(), [("A", 1), ("B", 2)])

    def test_size(self):
        self.settings["A"] = 1
        self.settings["B"] = 2
        self.assertEqual(len(self.settings), 2)

    def test_contains(self):
        self.settings["name"] = 1
        self.assertTrue("name" in self.settings)

    def test_clear(self):
        settings = get_settings_object("test_clear")
        settings["name"] = "value"
        self.assertEqual(len(settings), 1)
        settings.clear()
        self.assertEqual(len(settings), 0)

    def test_clone(self):
        self.settings["A"] = 1
        self.settings["B"] = 2
        cloned = self.settings.clone("ClonedManager")
        self.assertTrue("A" in cloned)
        self.assertTrue("B" in cloned)

    def test_clone_same_name_throws(self):
        self.assertRaises(RuntimeError, self.settings.clone, "BasicSettingsObjectUsageTest")

    def test_clone_name_already_exists_is_cleared(self):
        a = get_settings_object("A")
        a["a"] = 1
        b = get_settings_object("B")
        b["b"] = 2
        c = b.clone("A")
        c["c"] = 3

        self.assertFalse("a" in c)


if __name__ == "__main__":
    unittest.main()
