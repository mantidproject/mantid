# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Test the exposed PropertyManagerProperty
"""
import unittest
from mantid.kernel import Direction, PythonObjectProperty
from mantid.api import Algorithm
import numpy as np


class FakeAlgorithm(Algorithm):
    def PyInit(self):
        self.declareProperty(PythonObjectProperty("PyObject"))

    def PyExec(self):
        pass


def my_serializer():
    pass


class PythonObjectPropertyTest(unittest.TestCase):
    def test_construction_with_no_value_defaults_to_input(self):
        prop = PythonObjectProperty("MyInputProperty")

        self.assertEqual(prop.direction, Direction.Input)

    def test_construction_with_direction_is_respected(self):
        prop = PythonObjectProperty("MyOutputProperty", direction=Direction.Output)

        self.assertEqual(prop.direction, Direction.Output)

    def test_construction_with_no_value_defaults_to_None(self):
        prop = PythonObjectProperty("MyProperty")

        self.assertEqual(
            "MyProperty",
            prop.name,
        )
        self.assertEqual(Direction.Input, prop.direction)
        self.assertIs(prop.value, None)

    def test_construction_with_default_value_returns_same_value(self):
        default_value = 10
        prop = PythonObjectProperty("MyProperty", default_value)

        self.assertEqual(prop.name, "MyProperty")
        self.assertEqual(prop.direction, Direction.Input)
        self.assertEqual(prop.value, default_value)

    def test_set_property_on_algorithm_PyObject(self):
        fake = FakeAlgorithm()
        fake.initialize()

        value = 5
        fake.setProperty("PyObject", value)

        # It is important that both the value and address match to prove it
        # is just the same object that we got back
        self.assertEqual(value, fake.getProperty("PyObject").value)
        self.assertEqual(id(value), id(fake.getProperty("PyObject").value))


if __name__ == "__main__":
    unittest.main()
