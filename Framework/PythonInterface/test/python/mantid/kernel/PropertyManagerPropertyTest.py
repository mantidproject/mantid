"""Test the exposed PropertyManagerProperty
"""
import unittest
from mantid.kernel import PropertyManagerProperty, Direction
import numpy as np

class PropertyManagerPropertyTest(unittest.TestCase):

    def test_default_constructor_raises_an_exception(self):
        """
            Test that the class cannot be default constructed
        """
        self.assertRaises(Exception, PropertyManagerProperty)

    def test_name_only_constructor_gives_correct_object(self):
        """
            Tests the simplest constructor that takes
            only a name
        """
        name = "Args"
        pmap = PropertyManagerProperty(name)
        self.assertTrue(isinstance(pmap, PropertyManagerProperty))
        self._check_object_attributes(pmap, name, Direction.Input)

    def test_name_direction_constructor_gives_correct_object(self):
        """
            Tests the constructor that takes
            only a name & direction
        """
        name = "Args"
        direc = Direction.Output
        arr = PropertyManagerProperty(name, direc)
        self._check_object_attributes(arr, name, direc)

    def _check_object_attributes(self, prop, name, direction):
        """
            Do attribute tests
        """
        self.assertEquals(prop.name, name)
        self.assertEquals(prop.direction, direction)

if __name__ == "__main__":
    unittest.main()
