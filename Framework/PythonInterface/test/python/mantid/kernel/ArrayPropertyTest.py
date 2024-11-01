# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Test the exposed ArrayProperty"""

import unittest
from mantid.kernel import FloatArrayProperty, StringArrayProperty, IntArrayProperty, Direction, NullValidator
from mantid.api import PythonAlgorithm
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
        self.assertEqual(arr.isValid, "")

    def test_name_string_values_validator_direction_constructor_gives_correct_object(self):
        """
        Test the constructor that takes a name, values as string, validator & direction
        """
        name = "numbers"
        direc = Direction.Output
        validator = NullValidator()
        values_str = "1.345,34.2,5345.3,4,5.3948"
        arr = FloatArrayProperty(name, values_str, validator, direc)
        self._check_object_attributes(arr, name, direc, length=5)
        self.assertEqual(arr.isValid, "")
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
        input_values = [1.1, 2.5, 5.6, 4.6, 9.0, 6.0]
        arr = FloatArrayProperty(name, input_values, validator, direc)
        self._check_object_attributes(arr, name, direc, length=len(input_values))

    def test_name_values_from_array_validator_direction_constructor_gives_correct_object(self):
        """
        Test the constructor that takes a name, values from python object,
        validator & direction
        """
        name = "numbers"
        direc = Direction.Output
        validator = NullValidator()
        input_values = np.array([1.1, 2.5, 5.6, 4.6, 9.0, 6.0])
        arr = FloatArrayProperty(name, input_values, validator, direc)
        self._check_object_attributes(arr, name, direc, length=6)

    def _check_object_attributes(self, arrprop, name, direction, length=0):
        """
        Do attribute tests
        """
        self.assertEqual(arrprop.name, name)
        self.assertEqual(arrprop.direction, direction)
        self.assertEqual(len(arrprop.value), length)

    def test_setProperty_with_FloatArrayProperty(self):
        """
        Test ArrayProperty within a python algorithm
        """

        class AlgWithFloatArrayProperty(PythonAlgorithm):
            _input_values = None

            def PyInit(self):
                self.declareProperty(FloatArrayProperty("Input", Direction.Input), "Float array")

            def PyExec(self):
                self._input_values = self.getProperty("Input").value

        input_values = [1.1, 2.5, 5.6, 4.6, 9.0, 6.0]
        self._do_algorithm_test(AlgWithFloatArrayProperty, input_values)

    def test_setProperty_With_FloatArrayProperty_And_Py3_Range_Object(self):
        """
        Python 3 range() returns a range object that behaves like a sequence
        whereas it just returned a list in Python 2
        """

        class AlgWithFloatArrayProperty(PythonAlgorithm):
            _input_values = None

            def PyInit(self):
                self.declareProperty(FloatArrayProperty("Input", Direction.Input), "Float array")

            def PyExec(self):
                self._input_values = self.getProperty("Input").value

        input_values = range(1, 5)
        self._do_algorithm_test(AlgWithFloatArrayProperty, input_values)

    def test_dtype_function_calls(self):
        """
        Tests the dtype() function call for the data types stored in the array.
        """
        # Set up
        direc = Direction.Output
        validator = NullValidator()

        # Create float array
        float_input_values = [1.1, 2.5, 5.6, 4.6, 9.0, 6.0]
        float_arr = FloatArrayProperty("floats", float_input_values, validator, direc)

        # Create int array
        int_input_values = [1, 2, 5, 4, 9, 6]
        int_arr = IntArrayProperty("integers", int_input_values, validator, direc)

        # Create string array
        str_input_values = ["a", "b", "c", "d", "e"]
        str_arr = StringArrayProperty("letters", str_input_values, validator, direc)

        # Test
        self.assertEqual(float_arr.dtype(), "f")
        self.assertEqual(int_arr.dtype(), "i")
        self.assertEqual(str_arr.dtype(), "S1")

    def test_construct_numpy_array_with_given_dtype_float(self):
        # Set up
        direc = Direction.Output
        validator = NullValidator()

        # Create float array
        float_input_values = [1.1, 2.5, 5.6, 4.6, 9.0, 6.0]
        float_arr = FloatArrayProperty("floats", float_input_values, validator, direc)

        # Use the returned dtype() to check it works with numpy arrays
        x = np.arange(1, 10, dtype=float_arr.dtype())
        self.assertIsInstance(x, np.ndarray)
        self.assertEqual(x.dtype, float_arr.dtype())

    def test_construct_numpy_array_with_given_dtype_int(self):
        # Set up
        direc = Direction.Output
        validator = NullValidator()

        # Create int array
        int_input_values = [1, 2, 5, 4, 9, 6]
        int_arr = IntArrayProperty("integers", int_input_values, validator, direc)

        # Use the returned dtype() to check it works with numpy arrays
        x = np.arange(1, 10, dtype=int_arr.dtype())
        self.assertIsInstance(x, np.ndarray)
        self.assertEqual(x.dtype, int_arr.dtype())

    def test_construct_numpy_array_with_given_dtype_string(self):
        # Set up
        direc = Direction.Output
        validator = NullValidator()

        # Create string array
        str_input_values = ["hello", "testing", "word", "another word", "word"]
        str_arr = StringArrayProperty("letters", str_input_values, validator, direc)

        # Use the returned dtype() to check it works with numpy arrays
        x = np.array(str_input_values, dtype=str_arr.dtype())
        self.assertIsInstance(x, np.ndarray)
        # Expect longest string to be returned
        self.assertEqual(x.dtype, "S12")

    def test_PythonAlgorithm_setProperty_With_Ranges_String(self):
        """
        Test ArrayProperty within a python algorithm can
        be set with a string range
        """

        class AlgWithIntArrayProperty(PythonAlgorithm):
            _input_values = None

            def PyInit(self):
                self.declareProperty(IntArrayProperty("Input", Direction.Input), "Run numbers")

            def PyExec(self):
                self._input_values = self.getProperty("Input").value

        alg = AlgWithIntArrayProperty()
        alg.initialize()
        alg.setProperty("Input", "10:15")
        alg.execute()

        self.assertEqual(6, len(alg._input_values))

    def test_PythonAlgorithm_setProperty_with_StringArrayProperty(self):
        """
        Test StringArrayProperty within a python algorithm
        """

        class AlgWithStringArrayProperty(PythonAlgorithm):
            _input_values = None

            def PyInit(self):
                self.declareProperty(StringArrayProperty("Input", Direction.Input), "string array")

            def PyExec(self):
                self._input_values = self.getProperty("Input").value

        input_values = ["val1", "val2", "val3"]
        self._do_algorithm_test(AlgWithStringArrayProperty, input_values)

    def _do_algorithm_test(self, class_, input_values):
        """
        Run the algorithm and test the values are passed correctly
        """
        alg = class_()
        alg.initialize()
        alg.setProperty("Input", input_values)
        alg.execute()

        self.assertTrue(alg._input_values is not None)
        self.assertEqual(len(alg._input_values), len(input_values))
        for index, val in enumerate(input_values):
            self.assertEqual(val, input_values[index])


if __name__ == "__main__":
    unittest.main()
