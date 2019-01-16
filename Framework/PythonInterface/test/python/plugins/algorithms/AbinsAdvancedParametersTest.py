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
        AbinsParameters.fwhm = 3.0
        AbinsParameters.delta_width = 0.0005
        AbinsParameters.tosca_final_neutron_energy = 32.0
        AbinsParameters.tosca_cos_scattering_angle = -0.7069
        AbinsParameters.tosca_a = 0.0000001
        AbinsParameters.tosca_b = 0.005
        AbinsParameters.tosca_c = 2.5
        AbinsParameters.ab_initio_group = "PhononAB"
        AbinsParameters.powder_data_group = "Powder"
        AbinsParameters.crystal_data_group = "Crystal"
        AbinsParameters.s_data_group = "S"
        AbinsParameters.pkt_per_peak = 50
        AbinsParameters.bin_width = 1.0
        AbinsParameters.max_wavenumber = 4100.0
        AbinsParameters.min_wavenumber = 0.0
        AbinsParameters.s_relative_threshold = 0.001
        AbinsParameters.s_absolute_threshold = 10e-8
        AbinsParameters.optimal_size = 5000000
        AbinsParameters.threads = 1

    def tearDown(self):
        AbinsTestHelpers.remove_output_files(list_of_names=["AbinsAdvanced"])
        mtd.clear()

    def test_wrong_fwhm(self):
        # fwhm should be positive
        AbinsParameters.fwhm = -1.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # fwhm should be larger than 0
        AbinsParameters.fwhm = 0.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # fwhm should be smaller than 10
        AbinsParameters.fwhm = 10.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_delta_width(self):

        # delta_width should be a number
        AbinsParameters.delta_width = "fd"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # delta_with is positive so it cannot be negative
        AbinsParameters.delta_width = -0.01
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # delta_width should have non-zero value
        AbinsParameters.delta_width = 0.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # delta_width should be smaller than one
        AbinsParameters.delta_width = 1.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    # Tests for TOSCA parameters
    def test_wrong_tosca_final_energy(self):

        # final energy should be a float not str
        AbinsParameters.tosca_final_neutron_energy = "0"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # final energy should be of float type not integer
        AbinsParameters.tosca_final_neutron_energy = 1
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # final energy should be positive
        AbinsParameters.tosca_final_neutron_energy = -1.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_tosca_cos_scattering_angle(self):

        # cosines of scattering angle is float
        AbinsParameters.tosca_cos_scattering_angle = "0.0334"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # TOSCA_cos_scattering_angle cannot be integer
        AbinsParameters.tosca_cos_scattering_angle = 1
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_tosca_resolution_constant_A(self):
        # TOSCA constant should be float
        AbinsParameters.tosca_a = "wrong"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_tosca_resolution_constant_B(self):
        AbinsParameters.tosca_b = "wrong"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_tosca_resolution_constant_C(self):
        AbinsParameters.tosca_c = "wrong"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    # tests for folders
    def test_wrong_dft_group(self):
        # name should be of type str
        AbinsParameters.ab_initio_group = 2
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # name of group cannot be an empty string
        AbinsParameters.ab_initio_group = ""
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_powder_data_group(self):
        # name should be of type str
        AbinsParameters.powder_data_group = 2
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # name of group cannot be an empty string
        AbinsParameters.powder_data_group = ""
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_crystal_data_group(self):
        # name should be of type str
        AbinsParameters.crystal_data_group = 2
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # name of group cannot be an empty string
        AbinsParameters.crystal_data_group = ""
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_powder_s_data_group(self):
        # name should be of type str
        AbinsParameters.s_data_group = 2
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # name of group cannot be an empty string
        AbinsParameters.s_data_group = ""
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_doubled_name(self):
        # Wrong scenario: two groups with the same name
        AbinsParameters.ab_initio_group = "NiceName"
        AbinsParameters.powder_data_group = "NiceName"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_min_wavenumber(self):

        # minimum wavenumber cannot be negative
        AbinsParameters.min_wavenumber = -0.001
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # minimum wavenumber cannot be int
        AbinsParameters.min_wavenumber = 1
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_max_wavenumber(self):

        # maximum wavenumber cannot be negative
        AbinsParameters.max_wavenumber = -0.01
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # maximum wavenumber cannot be integer
        AbinsParameters.max_wavenumber = 10
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_energy_window(self):

        # min_wavenumber must be smaller than max_wavenumber
        AbinsParameters.min_wavenumber = 1000.0
        AbinsParameters.max_wavenumber = 10.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_s_absolute_threshold(self):

        AbinsParameters.s_absolute_threshold = 1
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        AbinsParameters.s_absolute_threshold = -0.01
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        AbinsParameters.s_absolute_threshold = "Wrong value"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_s_relative_threshold(self):

        AbinsParameters.s_relative_threshold = 1
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        AbinsParameters.s_relative_threshold = -0.01
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        AbinsParameters.s_relative_threshold = "Wrong value"
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_optimal_size(self):

        # optimal size cannot be negative
        AbinsParameters.optimal_size = -10000
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

        # optimal size must be of type int
        AbinsParameters.optimal_size = 50.0
        self.assertRaises(RuntimeError, Abins, VibrationalOrPhononFile=self._Si2 + ".phonon",
                          OutputWorkspace=self._wrk_name)

    def test_wrong_threads(self):
        if PATHOS_FOUND:
            AbinsParameters.threads = -1
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
