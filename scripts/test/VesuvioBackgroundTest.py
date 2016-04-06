import unittest

from mantid.simpleapi import *
from vesuvio.backgrounds import (create_from_str, PolynomialBackground)

# --------------------------------------------------------------------------------
# Polynomial
# --------------------------------------------------------------------------------

class PolynomialBackgroundTest(unittest.TestCase):

    def test_create_object_from_str(self):
        background_str = "function=Polynomial,order=2"

        background = create_from_str(background_str)
        self.assertTrue(isinstance(background, PolynomialBackground))
        self.assertEqual(2, background.order)

    def test_create_function_str_for_nth_order_with_no_values(self):
        background = PolynomialBackground(order=2)

        expected = "name=Polynomial,n=2"
        self.assertEqual(expected, background.create_fit_function_str())

    def test_create_function_str_for_nth_order_given_fixed_values(self):
        background = PolynomialBackground(order=2)
        param_values = {"f1.A0": 2.0, "f1.A1": 3.0, "f1.A2": 4.0}

        expected = "name=Polynomial,n=2,A0=2.000000,A1=3.000000,A2=4.000000"
        self.assertEqual(expected, background.create_fit_function_str(param_values, param_prefix="f1."))

if __name__ == '__main__':
    unittest.main()
