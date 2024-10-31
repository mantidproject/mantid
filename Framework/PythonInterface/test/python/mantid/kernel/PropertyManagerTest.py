# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import PropertyManager


class PropertyManagerTest(unittest.TestCase):
    def test_static_methods(self):
        test_log_name = "testLog"
        self.assertFalse(PropertyManager.isAnInvalidValuesFilterLog(test_log_name))
        self.assertEqual(PropertyManager.getInvalidValuesFilterLogName(test_log_name), test_log_name + "_invalid_values")

        self.assertTrue(PropertyManager.isAnInvalidValuesFilterLog(PropertyManager.getInvalidValuesFilterLogName(test_log_name)))

        # not a valid invalid values log
        self.assertEqual(PropertyManager.getLogNameFromInvalidValuesFilter(test_log_name), "")
        # A valid invalid values log
        self.assertEqual(
            PropertyManager.getLogNameFromInvalidValuesFilter(PropertyManager.getInvalidValuesFilterLogName(test_log_name)), test_log_name
        )

    def test_propertymanager_population(self):
        manager = PropertyManager()

        # check that it is empty
        self.assertEqual(manager.__len__(), 0)
        self.assertEqual(len(manager), 0)

        # add some values
        manager["f"] = 1.0
        manager["i"] = 2
        manager["s"] = "3"

        self.assertEqual(len(manager), 3)
        self.assertEqual(manager.propertyCount(), 3)

        # confirm they are in there
        self.assertTrue("f" in manager)
        self.assertTrue("i" in manager)
        self.assertTrue("s" in manager)
        self.assertFalse("nonsense" in manager)

        # check string return values
        self.assertEqual(manager.getPropertyValue("f"), "1")
        self.assertEqual(manager.getPropertyValue("i"), "2")
        self.assertEqual(manager.getPropertyValue("s"), "3")

        # check actual values
        self.assertEqual(manager.getProperty("f").value, 1.0)
        self.assertEqual(manager.getProperty("i").value, 2)
        self.assertEqual(manager.getProperty("s").value, "3")

        # ...and accessing them through dict interface
        self.assertEqual(manager["f"].value, 1.0)
        self.assertEqual(manager["i"].value, 2)
        self.assertEqual(manager["s"].value, "3")

        # see that you can get keys and values
        self.assertTrue(len(manager.values()), 3)
        keys = manager.keys()
        self.assertEqual(len(keys), 3)
        self.assertTrue("f" in keys)
        self.assertTrue("i" in keys)
        self.assertTrue("s" in keys)

        # check for members
        self.assertTrue(manager.has_key("f"))
        self.assertTrue(manager.has_key("i"))
        self.assertTrue(manager.has_key("s"))
        self.assertFalse(manager.has_key("q"))

        self.assertTrue("f" in manager)
        self.assertTrue("i" in manager)
        self.assertTrue("s" in manager)
        self.assertFalse("q" in manager)

        # check for delete
        self.assertTrue(len(manager), 3)
        del manager["f"]
        self.assertTrue(len(manager), 2)

    def test_propertymanager_can_be_created_from_dict(self):
        values = {"int": 5, "float": 20.0, "str": "a string"}
        pmgr = PropertyManager(values)
        self.assertEqual(len(pmgr), 3)
        self.assertEqual(5, pmgr["int"].value)
        self.assertEqual(20.0, pmgr["float"].value)
        self.assertEqual("a string", pmgr["str"].value)

    def test_propertymanager_cannot_be_created_from_arbitrary_sequence(self):
        with self.assertRaises(Exception):
            PropertyManager((1, 2, 3, 4, 5))


if __name__ == "__main__":
    unittest.main()
