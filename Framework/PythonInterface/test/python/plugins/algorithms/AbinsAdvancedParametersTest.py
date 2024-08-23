# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import Abins, mtd

from abins import test_helpers
import abins.parameters

try:
    from pathos.multiprocessing import ProcessingPool  # noqa 401

    PATHOS_FOUND = True
except ImportError:
    PATHOS_FOUND = False


class AbinsAdvancedParametersTest(unittest.TestCase):
    def setUp(self):
        # set up input for Abins
        self._Si2 = "Si2-sc_AbinsAdvancedParameters"
        self._wrk_name = self._Si2 + "_ref"

        # before each test set abins.parameters to default values
        abins.parameters.instruments = {
            "fwhm": 3.0,
            "TOSCA": {
                "final_neutron_energy": 32.0,
                "cos_scattering_angle": -0.7069,
                "angles": [135.0],
                "settings": {"forward": {"angles": [135.0]}},
                "settings_default": "forward",
                "a": 1e-7,
                "b": 5e-3,
                "c": 2.5,
            },
        }
        abins.parameters.hdf_groups = {"ab_initio_data": "PhononAB", "powder_data": "Powder", "crystal_data": "Crystal", "s_data": "S"}
        abins.parameters.sampling = {
            "bin_width": None,
            "pkt_per_peak": 50,
            "max_wavenumber": 4100.0,
            "min_wavenumber": 0.0,
            "broadening_scheme": "auto",
            "frequencies_threshold": 0.0,
            "s_relative_threshold": 0.001,
            "s_absolute_threshold": 1e-7,
            "broadening_range": 3.0,
        }
        abins.parameters.performance = {"optimal_size": int(5e6), "threads": 1}

    def tearDown(self):
        test_helpers.remove_output_files(list_of_names=["_AbinsAdvanced"])
        mtd.clear()

    def test_wrong_fwhm(self):
        # fwhm should be positive
        # fwhm should be smaller than 10
        bad_fwhm_values = (-1.0, 0.0, 10.0)

        for fwhm in bad_fwhm_values:
            abins.parameters.instruments["fwhm"] = fwhm
            self.assertRaisesRegex(
                RuntimeError, "Invalid value of fwhm", Abins, VibrationalOrPhononFile=self._Si2 + ".phonon", OutputWorkspace=self._wrk_name
            )

    # Tests for TOSCA parameters
    def test_wrong_tosca_final_energy(self):
        bad_final_neutron_energy_values = (
            "0",  # final energy should be a float not str
            1,  # final energy should be of float type not integer
            -1.0,  # final energy should be positive
        )

        for final_neutron_energy in bad_final_neutron_energy_values:
            abins.parameters.instruments["TOSCA"]["final_neutron_energy"] = final_neutron_energy
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid value of final_neutron_energy for TOSCA",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_wrong_tosca_cos_scattering_angle(self):
        bad_tosca_cos_scattering_angle_values = (
            "0.0334",  # cosines of scattering angle is float
            1,  # TOSCA_cos_scattering_angle cannot be integer
        )

        for tosca_cos_scattering_angle in bad_tosca_cos_scattering_angle_values:
            abins.parameters.instruments["TOSCA"]["cos_scattering_angle"] = tosca_cos_scattering_angle
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid value of cosines scattering angle for TOSCA",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_wrong_tosca_resolution_constant_A(self):
        # TOSCA constant should be float
        abins.parameters.instruments["TOSCA"]["a"] = "wrong"
        self.assertRaisesRegex(
            RuntimeError,
            "Invalid value of constant A for TOSCA",
            Abins,
            VibrationalOrPhononFile=self._Si2 + ".phonon",
            OutputWorkspace=self._wrk_name,
        )

    def test_wrong_tosca_resolution_constant_B(self):
        abins.parameters.instruments["TOSCA"]["b"] = "wrong"
        self.assertRaisesRegex(
            RuntimeError,
            "Invalid value of constant B for TOSCA",
            Abins,
            VibrationalOrPhononFile=self._Si2 + ".phonon",
            OutputWorkspace=self._wrk_name,
        )

    def test_wrong_tosca_resolution_constant_C(self):
        abins.parameters.instruments["TOSCA"]["c"] = "wrong"
        self.assertRaisesRegex(
            RuntimeError,
            "Invalid value of constant C for TOSCA",
            Abins,
            VibrationalOrPhononFile=self._Si2 + ".phonon",
            OutputWorkspace=self._wrk_name,
        )

    # tests for folders
    def test_wrong_dft_group(self):
        # name should be of type str
        # name of group cannot be an empty string
        bad_ab_initio_data_values = (2, "")

        for ab_initio_data in bad_ab_initio_data_values:
            abins.parameters.hdf_groups["ab_initio_data"] = ab_initio_data
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid name for folder in which the ab initio data should be stored.",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_wrong_powder_data_group(self):
        # name should be of type str
        # name of group cannot be an empty string
        bad_powder_data_values = (2, "")

        for powder_data in bad_powder_data_values:
            abins.parameters.hdf_groups["powder_data"] = powder_data
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid value of powder_data_group",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_wrong_crystal_data_group(self):
        # name should be of type str
        # name of group cannot be an empty string
        bad_crystal_data_values = (2, "")

        for crystal_data in bad_crystal_data_values:
            abins.parameters.hdf_groups["crystal_data"] = crystal_data
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid value of crystal_data_group",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_wrong_powder_s_data_group(self):
        # name should be of type str
        # name of group cannot be an empty string
        bad_s_data_values = (2, "")

        for s_data in bad_s_data_values:
            abins.parameters.hdf_groups["s_data"] = s_data
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid value of s_data_group",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_doubled_name(self):
        # Wrong scenario: two groups with the same name
        abins.parameters.hdf_groups["ab_initio_data"] = "NiceName"
        abins.parameters.hdf_groups["powder_data"] = "NiceName"
        self.assertRaisesRegex(
            RuntimeError,
            "Name for powder_data_group already used by as name of another folder.",
            Abins,
            VibrationalOrPhononFile=self._Si2 + ".phonon",
            OutputWorkspace=self._wrk_name,
        )

    def test_wrong_min_wavenumber(self):
        # minimum wavenumber cannot be negative
        # minimum wavenumber cannot be int
        bad_min_wavenumber_values = (-0.001, 1)

        for min_wavenumber in bad_min_wavenumber_values:
            abins.parameters.sampling["min_wavenumber"] = min_wavenumber
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid value of min_wavenumber",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_wrong_max_wavenumber(self):
        # maximum wavenumber cannot be negative
        # maximum wavenumber cannot be integer
        bad_max_wavenumber_values = (-0.01, 10)

        for max_wavenumber in bad_max_wavenumber_values:
            abins.parameters.sampling["max_wavenumber"] = max_wavenumber
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid value of max_wavenumber",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_wrong_energy_window(self):
        # min_wavenumber must be smaller than max_wavenumber
        abins.parameters.sampling["min_wavenumber"] = 1000.0
        abins.parameters.sampling["max_wavenumber"] = 10.0
        self.assertRaisesRegex(
            RuntimeError,
            "Invalid energy window for rebinning.",
            Abins,
            VibrationalOrPhononFile=self._Si2 + ".phonon",
            OutputWorkspace=self._wrk_name,
        )

    def test_wrong_s_absolute_threshold(self):
        bad_s_absolute_threshold_values = (1, -0.01, "Wrong value")

        for s_absolute_threshold in bad_s_absolute_threshold_values:
            abins.parameters.sampling["s_absolute_threshold"] = s_absolute_threshold
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid value of s_absolute_threshold",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_wrong_s_relative_threshold(self):
        bad_s_relative_threshold_values = (1, -0.01, "Wrong value")

        for s_relative_threshold in bad_s_relative_threshold_values:
            abins.parameters.sampling["s_relative_threshold"] = s_relative_threshold
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid value of s_relative_threshold",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_wrong_optimal_size(self):
        # optimal size cannot be negative
        # optimal size must be of type int
        bad_optimal_size_values = (-10000, 50.0)

        for optimal_size in bad_optimal_size_values:
            abins.parameters.performance["optimal_size"] = optimal_size
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid value of optimal_size",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_wrong_threads(self):
        if PATHOS_FOUND:
            abins.parameters.performance["threads"] = -1
            self.assertRaisesRegex(
                RuntimeError,
                "Invalid number of threads for parallelisation over atoms",
                Abins,
                VibrationalOrPhononFile=self._Si2 + ".phonon",
                OutputWorkspace=self._wrk_name,
            )

    def test_good_case(self):
        good_names = [self._wrk_name, self._wrk_name + "_Si", self._wrk_name + "_Si_total"]
        Abins(VibrationalOrPhononFile=self._Si2 + ".phonon", OutputWorkspace=self._wrk_name)
        names = mtd.getObjectNames()

        # Builtin cmp has been removed in Python 3
        def _cmp(a, b):
            return (a > b) - (a < b)

        self.assertAlmostEqual(0, _cmp(good_names, names))


if __name__ == "__main__":
    unittest.main()
