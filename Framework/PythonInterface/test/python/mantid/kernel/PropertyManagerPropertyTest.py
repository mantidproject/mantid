# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Test the exposed PropertyManagerProperty"""

import unittest
from mantid.kernel import PropertyManagerProperty, Direction, PropertyManager
from mantid.api import Algorithm


class FakeAlgorithm(Algorithm):
    def PyInit(self):
        self.declareProperty(PropertyManagerProperty("Args"))

    def PyExec(self):
        pass


class PropertyManagerPropertyTest(unittest.TestCase):
    def test_default_constructor_raises_an_exception(self):
        self.assertRaises(Exception, PropertyManagerProperty)

    def test_name_only_constructor_gives_correct_object(self):
        name = "Args"
        pmap = PropertyManagerProperty(name)
        self.assertTrue(isinstance(pmap, PropertyManagerProperty))
        self._check_object_attributes(pmap, name, Direction.Input)

    def test_name_direction_constructor_gives_correct_object(self):
        name = "Args"
        direc = Direction.Output
        arr = PropertyManagerProperty(name, direc)
        self._check_object_attributes(arr, name, direc)

    def test_set_property_on_algorithm_from_dictionary(self):
        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty(
            "Args",
            {
                "A": 1,
                "B": 10.5,
                "C": "String arg",
                "D": [0.0, 11.3],
                "E": {"F": 10.4, "G": [1.0, 2.0, 3.0], "H": {"I": "test", "J": 120.6}},
            },
        )

        pmgr = fake.getProperty("Args").value
        self.assertTrue(isinstance(pmgr, PropertyManager))
        self.assertEqual(5, len(pmgr))
        self.assertTrue("A" in pmgr)
        self.assertEqual(1, pmgr["A"].value)
        self.assertTrue("B" in pmgr)
        self.assertEqual(10.5, pmgr["B"].value)
        self.assertTrue("C" in pmgr)
        self.assertEqual("String arg", pmgr["C"].value)
        self.assertTrue("D" in pmgr)
        array_value = pmgr["D"].value
        self.assertEqual(0.0, array_value[0])
        self.assertEqual(11.3, array_value[1])

        # Check the level-1 nested property manager property
        # Get the level1-nested property manager
        nested_l1_pmgr = pmgr["E"].value
        self.assertEqual(3, len(nested_l1_pmgr))
        self.assertTrue("F" in nested_l1_pmgr)
        self.assertEqual(10.4, nested_l1_pmgr["F"].value)
        self.assertTrue("G" in nested_l1_pmgr)
        self.assertEqual(1.0, nested_l1_pmgr["G"].value[0])
        self.assertEqual(2.0, nested_l1_pmgr["G"].value[1])
        self.assertEqual(3.0, nested_l1_pmgr["G"].value[2])
        self.assertTrue("H" in nested_l1_pmgr)
        self.assertTrue(isinstance(nested_l1_pmgr["H"].value, PropertyManager))

        # Get the level2-nested property manager
        nested_l2_pmgr = nested_l1_pmgr["H"].value
        self.assertTrue("I" in nested_l2_pmgr)
        self.assertEqual("test", nested_l2_pmgr["I"].value)
        self.assertTrue("J" in nested_l2_pmgr)
        self.assertEqual(120.6, nested_l2_pmgr["J"].value)

    def test_set_property_on_algorithm_property_manager(self):
        # set the property directly from a PropertyManager
        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", PropertyManager())

    def test_that_empty_sequence_in_property_manager_raises(self):
        fake = FakeAlgorithm()
        fake.initialize()
        with self.assertRaises(RuntimeError):
            fake.setProperty("Args", {"A": []})

    def test_create_with_dictionary_as_default_value(self):
        default = {"A": {}, "B": 1}
        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", default)
        prop = fake.getProperty("Args").value
        self._check_values(prop, **default)

    def _check_object_attributes(self, prop, name, direction):
        self.assertEqual(prop.name, name)
        self.assertEqual(prop.direction, direction)

    def _check_values(self, prop, **kwargs):
        for key, value in kwargs.items():
            propValue = prop.getProperty(key).value

            if isinstance(propValue, PropertyManager):
                self.assertTrue(isinstance(value, dict))
                self._check_values(propValue, **value)
            else:
                self.assertEqual(propValue, value)


if __name__ == "__main__":
    unittest.main()
