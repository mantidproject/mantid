import unittest

from mantid.simpleapi import *
from vesuvio.profiles import (create_from_str, GaussianMassProfile,
                              GramCharlierMassProfile)

# --------------------------------------------------------------------------------
# Gaussian
# --------------------------------------------------------------------------------

class GaussianMassProfileTest(unittest.TestCase):

    # ---------------- Success cases ---------------------------

    def test_string_with_fixed_width_produces_valid_object(self):
        function_str = "function=Gaussian,width=10"
        mass = 16.0

        profile = create_from_str(function_str, mass)
        self.assertTrue(isinstance(profile, GaussianMassProfile))
        self.assertAlmostEqual(mass, profile.mass)
        self.assertAlmostEqual(10.0, profile.width)

    def test_string_with_constrained_width_produces_valid_object(self):
        function_str = "function=Gaussian,width=[2, 5, 7]"
        mass = 16.0

        profile = create_from_str(function_str, mass)
        self.assertTrue(isinstance(profile, GaussianMassProfile))
        self.assertAlmostEqual(mass, profile.mass)
        self.assertEqual([2, 5, 7], profile.width)

    def test_function_string_has_expected_form_with_no_defaults(self):
        test_profiles = GaussianMassProfile(10, 16)

        expected = "name=GaussianComptonProfile,Mass=16.000000,Width=10.000000;"
        self.assertEqual(expected, test_profiles.create_fit_function_str())

    def test_function_string_has_expected_form_with_defaults_given(self):
        test_profiles = GaussianMassProfile(10, 16)
        param_prefix = "f1."
        param_vals = {"f1.Width": 11.0, "f1.Intensity": 4.5}

        expected = "name=GaussianComptonProfile,Mass=16.000000,Width=11.000000,Intensity=4.500000;"
        self.assertEqual(expected, test_profiles.create_fit_function_str(param_vals, param_prefix))

    def test_constraint_str_is_only_intensity_for_fixed_width(self):
        test_profile = GaussianMassProfile(10, 16)

        self.assertEqual("Intensity > 0.0", test_profile.create_constraint_str())

    def test_constraint_str_for_constrained_width(self):
        test_profile = GaussianMassProfile([2,5,7], 16)

        self.assertEqual("2.000000 < Width < 7.000000,Intensity > 0.0", test_profile.create_constraint_str())
        # and with prefix
        self.assertEqual("2.000000 < f0.Width < 7.000000,f0.Intensity > 0.0", test_profile.create_constraint_str("f0."))

    def test_ties_str_is_empty_for_fixed_width(self):
        test_profile = GaussianMassProfile(10, 16)

        self.assertEqual("Width=10.000000", test_profile.create_ties_str())

    def test_ties_str_for_constrained_width_is_empty(self):
        test_profile = GaussianMassProfile([2,5,7], 16)

        self.assertEqual("", test_profile.create_ties_str())
        # and with prefix
        self.assertEqual("", test_profile.create_ties_str("f0."))


    # ---------------- Failure cases ---------------------------

    def test_string_not_starting_with_function_equals_name_gives_error(self):
        function_str = "function=Gaussia,width=[2, 5, 7]"
        mass = 16.0

        self.assertRaises(TypeError, GaussianMassProfile.from_str,
                          function_str, mass)

    def test_string_not_starting_with_function_gives_error(self):
        function_str = "Gaussian,width=[2, 5, 7]"
        mass = 16.0

        self.assertRaises(TypeError, GaussianMassProfile.from_str,
                          function_str, mass)

    def test_string_with_wrong_function_gives_error(self):
        function_str = "function=GramCharlier,width=[2, 5, 7]"
        mass = 16.0

        self.assertRaises(TypeError, GaussianMassProfile.from_str,
                          function_str, mass)

# --------------------------------------------------------------------------------
# GramCharlier
# --------------------------------------------------------------------------------

class GramCharlierMassProfileTest(unittest.TestCase):

    def test_string_with_fixed_width_produces_valid_object(self):
        function_str = "function=GramCharlier,width=[2, 5,7],k_free=1,hermite_coeffs=[1,0,1],sears_flag=0,"
        mass = 16.0

        profile = create_from_str(function_str, mass)
        self.assertTrue(isinstance(profile, GramCharlierMassProfile))
        self.assertAlmostEqual(mass, profile.mass)
        self.assertEqual([2, 5, 7], profile.width)
        self.assertEqual([1, 0, 1], profile.hermite_co)
        self.assertEqual(0, profile.sears_flag)
        self.assertEqual(1, profile.k_free)

    def test_function_string_has_expected_form_with_no_defaults(self):
        test_profile = GramCharlierMassProfile(10, 16,[1,0,1],1,1)

        expected = "name=GramCharlierComptonProfile,Mass=16.000000,HermiteCoeffs=1 0 1,Width=10.000000;"
        self.assertEqual(expected, test_profile.create_fit_function_str())

    def test_function_string_has_expected_form_with_given_values(self):
        test_profile = GramCharlierMassProfile(10, 16,[1,0,1],1,1)
        param_prefix = "f1."
        param_vals = {"f1.Width": 11.0, "f1.FSECoeff": 0.1, "f1.C_0": 0.25,
                      "f1.C_2": 0.5, "f1.C_4": 0.75}

        expected = "name=GramCharlierComptonProfile,Mass=16.000000,HermiteCoeffs=1 0 1,"\
                   "Width=11.000000,FSECoeff=0.100000,C_0=0.250000,C_4=0.750000;"
        self.assertEqual(expected, test_profile.create_fit_function_str(param_vals, param_prefix))

    def test_constraint_str_for_fixed_width(self):
        test_profile = GramCharlierMassProfile(10, 16, [1, 0, 1], k_free=1, sears_flag=1)

        expected = "C_0 > 0.0,C_4 > 0.0"
        self.assertEqual(expected, test_profile.create_constraint_str())

    def test_constraint_str_for_constrained_width(self):
        test_profile = GramCharlierMassProfile([2,5,7], 16, [1,0,1], k_free=1, sears_flag=1)

        expected = "2.000000 < Width < 7.000000,C_0 > 0.0,C_4 > 0.0"
        self.assertEqual(expected, test_profile.create_constraint_str())
        prefix = "f0."
        expected = "2.000000 < f0.Width < 7.000000,f0.C_0 > 0.0,f0.C_4 > 0.0"
        self.assertEqual(expected, test_profile.create_constraint_str(prefix))

    def test_ties_str_for_constrained_width_and_k_is_free_is_empty(self):
        test_profile = GramCharlierMassProfile([2,5,7], 16, [1,0,1], k_free=1, sears_flag=1)

        expected = ""
        self.assertEqual(expected, test_profile.create_ties_str())

    def test_ties_str_for_constrained_width(self):
        # k is free
        test_profile = GramCharlierMassProfile([2,5,7], 16, [1,0,1], k_free=1, sears_flag=1)

        expected = ""
        self.assertEqual(expected, test_profile.create_ties_str())

        # k is tied, sears=0
        test_profile = GramCharlierMassProfile([2,5,7], 16, [1,0,1], k_free=0, sears_flag=0)

        expected = "f0.FSECoeff=0"
        self.assertEqual(expected, test_profile.create_ties_str("f0."))

        # k is tied, sears=1
        test_profile = GramCharlierMassProfile([2,5,7], 16, [1,0,1], k_free=0, sears_flag=1)

        expected = "f0.FSECoeff=f0.Width*sqrt(2)/12"
        self.assertEqual(expected, test_profile.create_ties_str("f0."))

    def test_ties_str_for_fixed_width(self):
        test_profile = GramCharlierMassProfile(5, 16, [1,0,1], k_free=1, sears_flag=1)

        expected = "Width=5.000000"
        self.assertEqual(expected, test_profile.create_ties_str())

        # k is tied, sears=0
        test_profile = GramCharlierMassProfile(5, 16, [1,0,1], k_free=0, sears_flag=0)

        expected = "f0.Width=5.000000,f0.FSECoeff=0"
        self.assertEqual(expected, test_profile.create_ties_str("f0."))

        # k is tied, sears=1
        test_profile = GramCharlierMassProfile(5, 16, [1,0,1], k_free=0, sears_flag=1)

        expected = "f0.Width=5.000000,f0.FSECoeff=f0.Width*sqrt(2)/12"
        self.assertEqual(expected, test_profile.create_ties_str("f0."))

    # ---------------- Failure cases ---------------------------

    def test_string_not_starting_with_function_equals_name_gives_error(self):
        function_str = "function=GramCharlie,width=[2, 5, 7]"
        mass = 16.0

        self.assertRaises(TypeError, GramCharlierMassProfile.from_str,
                          function_str, mass)

    def test_string_not_starting_with_function_gives_error(self):
        function_str = "GramCharlie,width=[2, 5, 7]"
        mass = 16.0

        self.assertRaises(TypeError, GramCharlierMassProfile.from_str,
                          function_str, mass)

    def test_string_with_wrong_function_gives_error(self):
        function_str = "function=Gaussian,width=[2, 5, 7]"
        mass = 16.0

        self.assertRaises(TypeError, GramCharlierMassProfile.from_str,
                          function_str, mass)


if __name__ == '__main__':
    unittest.main()
