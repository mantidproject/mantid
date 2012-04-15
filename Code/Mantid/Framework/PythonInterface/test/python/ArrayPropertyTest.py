"""Test the exposed ArrayProperty
"""
import unittest
from mantid import FloatArrayProperty, Direction, NullValidator
import numpy as np

class ArrayPropertyTest(unittest.TestCase):
    
    def test_default_constructor_raises_an_exception(self):
        """
            Test that the class cannot be default constructed
        """
        self.assertRaises(Exception, FloatArrayProperty)
  
    def test_name_only_constructor_gives_correct_object(self):
        """
            Tests the simplest constructor that takes 
            only a name
        """
        name = "numbers"
        arr = FloatArrayProperty(name)
        self.assertTrue(isinstance(arr, FloatArrayProperty))
        self._check_object_attributes(arr, name, Direction.Input)

    def test_name_direction_constructor_gives_correct_object(self):
        """
            Tests the constructor that takes 
            only a name & direction
        """
        name = "numbers"
        direc = Direction.Output
        arr = FloatArrayProperty(name, direc)
        self._check_object_attributes(arr, name, direc)
        
    def test_name_validator_direction_constructor_gives_correct_object(self):
        """
            Test the constructor that takes a name, validator & direction
        """
        name = "numbers"
        direc = Direction.Output
        validator = NullValidator()
        arr = FloatArrayProperty(name, validator, direc)
        self._check_object_attributes(arr, name, direc)
        self.assertEquals(arr.isValid(), "")

    def test_name_string_values_validator_direction_constructor_gives_correct_object(self):
        """
            Test the constructor that takes a name, values as string, validator & direction
        """
        name = "numbers"
        direc = Direction.Output
        validator = NullValidator()
        values_str = "1.345,34.2,5345.3,4,5.3948"
        arr = FloatArrayProperty(name, values_str, validator, direc)
        self._check_object_attributes(arr, name, direc, length = 5)
        self.assertEquals(arr.isValid(), "")
        values = arr.value
        self.assertTrue(isinstance(values, np.ndarray))
        
            
    def test_name_values_from_list_validator_direction_constructor_gives_correct_object(self):
        """
            Test the constructor that takes a name, values from python object, 
            validator & direction
        """
        name = "numbers"
        direc = Direction.Output
        validator = NullValidator()
        input_values =[1.1,2.5,5.6,4.6,9.0, 6.0]
        arr = FloatArrayProperty(name, input_values, validator, direc)
        self._check_object_attributes(arr, name, direc, length = len(input_values))

    def test_name_values_from_array_validator_direction_constructor_gives_correct_object(self):
        """
            Test the constructor that takes a name, values from python object, 
            validator & direction
        """
        name = "numbers"
        direc = Direction.Output
        validator = NullValidator()
        input_values = np.array([1.1,2.5,5.6,4.6,9.0, 6.0])
        arr = FloatArrayProperty(name, input_values, validator, direc)
        self._check_object_attributes(arr, name, direc, length = 6)

    def _check_object_attributes(self, arrprop, name, direction, length = 0):
        """
            Do attribute tests
        """
        self.assertEquals(arrprop.name, name)
        self.assertEquals(arrprop.direction, direction)
        self.assertEquals(len(arrprop.value), length)
        
