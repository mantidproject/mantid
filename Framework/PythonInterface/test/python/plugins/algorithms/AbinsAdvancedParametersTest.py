# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import Abins, DeleteWorkspace, mtd

from AbinsModules import AbinsParameters, AbinsTestHelpers

try:
    from pathos.multiprocessing import ProcessingPool
    PATHOS_FOUND = True
except ImportError:
    PATHOS_FOUND = False


class AbinsAdvancedParametersTest(unittest.TestCase):

    def setUp(self):

        # set up input for Abins
        self._Si2 = "Si2-sc_AbinsAdvancedParameters"
        self._wrk_name = self._Si2 + "_ref"

        # before each test set AbinsParameters to default values
        AbinsParameters.instruments = {
            "fwhm": 3.0,
            "TwoDMap": {"delta_width": 0.0005},
            "TOSCA": {
                "final_neutron_energy": 32.0,
                "cos_scattering_angle": -0.7069,
                "a": 1e-7,
                "b": 5e-3,
                "c": 2.5}}
        AbinsParameters.hdf_groups = {
            "ab_initio_data": "PhononAB",
            "powder_data": "Powder",
            "crystal_data": "Crystal",
            "s_data": "S"}
        AbinsParameters.sampling = {
            "pkt_per_peak": 50,
            "max_wavenumber": 4100.0,
            "min_wavenumber": 0.0,
            "frequencies_threshold": 0.0,
            "s_relative_threshold": 0.001,
            "s_absolute_threshold": 1e-7}
        AbinsParameters.performance = {
            "optimal_size": int(5e6),
            "threads": 1}

    def tearDown(self):
        AbinsTestHelpers.remove_output_files(list_of_names=["AbinsAdvanced"])
        mtd.clear()

    def test_wrong_fwhm(self):
        # fwhm should be positive
        AbinsParameters.instruments["fwhm"] = -1.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # fwhm should be larger than 0
        AbinsParameters.instruments["fwhm"] = 0.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # fwhm should be smaller than 10
        AbinsParameters.instruments["fwhm"] = 10.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_delta_width(self):

        # delta_width should be a number
        AbinsParameters.instruments["TwoDMap"]["delta_width"] = "fd"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # delta_with is positive so it cannot be negative
        AbinsParameters.instruments["TwoDMap"]["delta_width"] = -0.01
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # delta_width should have non-zero value
        AbinsParameters.instruments["TwoDMap"]["delta_width"] = 0.
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # delta_width should be smaller than one
        AbinsParameters.instruments["TwoDMap"]["delta_width"] = 1.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    # Tests for TOSCA parameters
    def test_wrong_tosca_final_energy(self):

        # final energy should be a float not str
        AbinsParameters.instruments["TOSCA"]["final_neutron_energy"] = "0"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # final energy should be of float type not integer
        AbinsParameters.instruments["TOSCA"]["final_neutron_energy"] = 1
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # final energy should be positive
        AbinsParameters.instruments["TOSCA"]["final_neutron_energy"] = -1.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_tosca_cos_scattering_angle(self):

        # cosines of scattering angle is float
        AbinsParameters.instruments["TOSCA"]["cos_scattering_angle"] = "0.0334"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # TOSCA_cos_scattering_angle cannot be integer
        AbinsParameters.instruments["TOSCA"]["cos_scattering_angle"] = 1
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_tosca_resolution_constant_A(self):
        # TOSCA constant should be float
        AbinsParameters.instruments["TOSCA"]["a"] = "wrong"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_tosca_resolution_constant_B(self):
        AbinsParameters.instruments["TOSCA"]["b"] = "wrong"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_tosca_resolution_constant_C(self):
        AbinsParameters.instruments["TOSCA"]["c"] = "wrong"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    # tests for folders
    def test_wrong_dft_group(self):
        # name should be of type str
        AbinsParameters.hdf_groups["ab_initio_data"] = 2
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # name of group cannot be an empty string
        AbinsParameters.hdf_groups["ab_initio_data"] = ""
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_powder_data_group(self):
        # name should be of type str
        AbinsParameters.hdf_groups["powder_data"] = 2
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # name of group cannot be an empty string
        AbinsParameters.hdf_groups["powder_data"] = ""
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_crystal_data_group(self):
        # name should be of type str
        AbinsParameters.hdf_groups["crystal_data"]= 2
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # name of group cannot be an empty string
        AbinsParameters.hdf_groups["crystal_data"]= ""
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_powder_s_data_group(self):
        # name should be of type str
        AbinsParameters.hdf_groups["s_data"] = 2
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # name of group cannot be an empty string
        AbinsParameters.hdf_groups["s_data"] = ""
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_doubled_name(self):
        # Wrong scenario: two groups with the same name
        AbinsParameters.hdf_groups["ab_initio_data"] = "NiceName"
        AbinsParameters.hdf_groups["powder_data"]= "NiceName"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_min_wavenumber(self):

        # minimum wavenumber cannot be negative
        AbinsParameters.sampling["min_wavenumber"] = -0.001
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # minimum wavenumber cannot be int
        AbinsParameters.sampling["min_wavenumber"] = 1
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_max_wavenumber(self):

        # maximum wavenumber cannot be negative
        AbinsParameters.sampling["max_wavenumber"] = -0.01
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # maximum wavenumber cannot be integer
        AbinsParameters.sampling["max_wavenumber"] = 10
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_energy_window(self):

        # min_wavenumber must be smaller than max_wavenumber
        AbinsParameters.sampling["min_wavenumber"] = 1000.0
        AbinsParameters.sampling["max_wavenumber"] = 10.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_s_absolute_threshold(self):

        AbinsParameters.sampling["s_absolute_threshold"] = 1
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        AbinsParameters.sampling["s_absolute_threshold"] = -0.01
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        AbinsParameters.sampling["s_absolute_threshold"] = "Wrong value"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_s_relative_threshold(self):

        AbinsParameters.sampling["s_relative_threshold"] = 1
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        AbinsParameters.sampling["s_relative_threshold"] = -0.01
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        AbinsParameters.sampling["s_relative_threshold"] = "Wrong value"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_optimal_size(self):

        # optimal size cannot be negative
        AbinsParameters.performance["optimal_size"] = -10000
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # optimal size must be of type int
        AbinsParameters.performance["optimal_size"] = 50.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_threads(self):
        if PATHOS_FOUND:
            AbinsParameters.performance["threads"] = -1
            self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                              OutputWorkspace=self._wrk_name)

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
