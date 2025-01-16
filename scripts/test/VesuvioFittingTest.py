# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from vesuvio.backgrounds import PolynomialBackground
from vesuvio.fitting import FittingOptions, parse_fit_options
from vesuvio.profiles import GaussianMassProfile, GramCharlierMassProfile


class VesuvioFittingTest(unittest.TestCase):
    def test_function_str_with_no_given_params_looks_as_expected(self):
        fit_opts = self._create_test_fitting_opts()

        expected = (
            'composite=ComptonScatteringCountRate,NumDeriv=1,IntensityConstraints="Matrix(1|2)1.000000|-4.000000";'
            "name=GramCharlierComptonProfile,Mass=1.007900,HermiteCoeffs=1 0 0,Width=5.000000;"
            "name=GaussianComptonProfile,Mass=16.000000,Width=10.000000"
        )
        self.assertEqual(expected, fit_opts.create_function_str())

        # add background
        fit_opts.background = PolynomialBackground(order=2)
        expected += ";name=Polynomial,n=2"
        self.assertEqual(expected, fit_opts.create_function_str())

    def test_function_str_with_given_params_looks_as_expected(self):
        fit_opts = self._create_test_fitting_opts()

        param_vals = {"f0.Width": 7.5, "f0.FSECoeff": 0.1, "f0.C_0": 0.25, "f0.C_2": 0.5, "f0.C_4": 0.75}
        param_vals.update({"f1.Width": 11.0, "f1.Intensity": 4.5})

        expected = (
            "composite=CompositeFunction,NumDeriv=1;"
            "name=GramCharlierComptonProfile,Mass=1.007900,HermiteCoeffs=1 0 0,Width=7.500000,FSECoeff=0.100000,"
            "C_0=0.250000;"
            "name=GaussianComptonProfile,Mass=16.000000,Width=11.000000,Intensity=4.500000"
        )
        self.assertEqual(expected, fit_opts.create_function_str(param_vals))

        fit_opts.background = PolynomialBackground(order=2)
        param_vals.update({"f2.A0": 2.0, "f2.A1": 3.0, "f2.A2": 4.0})

        expected += ";name=Polynomial,n=2,A0=2.000000,A1=3.000000,A2=4.000000"
        self.assertEqual(expected, fit_opts.create_function_str(param_vals))

    def test_constraint_str_gives_expected_value_when_width_has_constraint(self):
        fit_opts = self._create_test_fitting_opts()

        expected = "2.000000 < f0.Width < 7.000000,f0.C_0 > 0.0,f1.Intensity > 0.0"
        self.assertEqual(expected, fit_opts.create_constraints_str())

        # Fix the width and the constraint should be just the intensity
        fit_opts.mass_profiles[0].width = 5.0
        expected = "f0.C_0 > 0.0,f1.Intensity > 0.0"
        self.assertEqual(expected, fit_opts.create_constraints_str())

    def test_ties_str_gives_expected_value(self):
        fit_opts = self._create_test_fitting_opts()

        expected = "f0.Mass=1.007900,f1.Mass=16.000000,f1.Width=10.000000"
        self.assertEqual(expected, fit_opts.create_ties_str())
        # Fix the width and FSECoeff
        fit_opts.mass_profiles[0].width = 5.0
        fit_opts.mass_profiles[0].k_free = 0
        expected = "f0.Mass=1.007900,f0.Width=5.000000,f0.FSECoeff=f0.Width*sqrt(2)/12,f1.Mass=16.000000,f1.Width=10.000000"
        self.assertEqual(expected, fit_opts.create_ties_str())

    def test_parse_fit_options(self):
        mass_values = [1.0079, 16]
        profile_strs = "function=Gaussian,width=[2,5,7];function=Gaussian,width=10.000000"
        background_str = "function=Polynomial,order=2"
        constraints_str = "[1,-4]"

        fit_opts = parse_fit_options(mass_values, profile_strs, background_str, constraints_str)
        self.assertEqual(2, len(fit_opts.mass_profiles))
        self.assertTrue(isinstance(fit_opts.background, PolynomialBackground))
        self.assertEqual(1, len(fit_opts.intensity_constraints))
        self.assertEqual(2, len(fit_opts.intensity_constraints[0]))

    def _create_test_fitting_opts(self):
        gramc = GramCharlierMassProfile([2, 5, 7], 1.0079, [1, 0, 0], 1, 1)
        gauss = GaussianMassProfile(10, 16)
        constraints = list([1, -4])

        return FittingOptions([gramc, gauss], intensity_constraints=constraints)


if __name__ == "__main__":
    unittest.main()
