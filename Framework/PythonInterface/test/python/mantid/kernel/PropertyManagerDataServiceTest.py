# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.kernel import PropertyManager, PropertyManagerDataService


class PropertyManagerDataServiceTest(unittest.TestCase):
    def test_add_existing_mgr_object(self):
        name = "PropertyManagerDataServiceTest_test_add_existing_mgr_object"
        values = {"key": 100.5}
        mgr = PropertyManager(values)
        self._do_add_test(name, mgr)

    def test_add_straight_from_dict(self):
        name = "PropertyManagerDataServiceTest_test_add_straight_from_dict"
        values = {"key": 100.5}
        self._do_add_test(name, values)

    def test_addOrReplace_straight_from_dict(self):
        name = "PropertyManagerDataServiceTest_addOrReplace_straight_from_dict"
        values = {"key": 100.5}
        values2 = {"key2": 50}
        self._do_addOrReplace_test(name, values, values2)

    def _do_add_test(self, name, value):
        pmds = PropertyManagerDataService.Instance()
        pmds.add(name, value)
        self.assertTrue(name in pmds)
        pmds.remove(name)

    def _do_addOrReplace_test(self, name, value, value2):
        pmds = PropertyManagerDataService.Instance()
        pmds.add(name, value)
        pmds.addOrReplace(name, value2)
        pmgr = pmds[name]
        self.assertEqual(value2["key2"], pmgr["key2"].value)
        pmds.remove(name)

    def test_contains(self):
        # verify check against None
        self.assertFalse(None in PropertyManagerDataService)
        # verify check against things that bool to False
        self.assertFalse("" in PropertyManagerDataService)
        self.assertFalse(0 in PropertyManagerDataService)
        # verify check for converting checked value to string
        self.assertFalse(1 in PropertyManagerDataService)


if __name__ == "__main__":
    unittest.main()
