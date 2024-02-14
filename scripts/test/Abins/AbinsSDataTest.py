# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from copy import deepcopy
import logging
import unittest

import numpy as np
from numpy.testing import assert_almost_equal
from pydantic import ValidationError

import abins
from abins import SData


class AbinsSpectraTest(unittest.TestCase):
    """Test operations on euphonic-derived Spectrum classes"""

    def test_autoconvolution(self):
        from abins.sdata import AbinsSpectrum1DCollection, add_autoconvolution_spectra
        from euphonic import ureg

        # Check a trivial case: starting with a single peak,
        # expect evenly-spaced sequence of same intensity
        #
        # _|____ .... -> _|_|_|_|_ ...
        #
        y_data = np.zeros((1, 50)) * ureg("1 / meV")
        y_data[0, 2] = 1.0 * ureg("1 / meV")
        expected_y_data = np.zeros((10, 50)) * ureg("1 / meV")
        for i in range(10):
            expected_y_data[i][2 * (i + 1)] = 1.0 * ureg("1 / meV")

        spectra = AbinsSpectrum1DCollection(
            x_data=(np.linspace(0, 10, 50) * ureg("meV")),
            y_data=y_data,
            metadata={"test_key": "test_value", "line_data": [{"atom_index": 0, "quantum_order": 1}]},
        )

        spectra = add_autoconvolution_spectra(spectra)

        assert_almost_equal(spectra.y_data.magnitude, expected_y_data.magnitude)
        for i, row in enumerate(spectra):
            self.assertEqual(row.metadata, {"atom_index": 0, "quantum_order": i + 1, "test_key": "test_value"})

        # Check range restriction works, and beginning with more orders
        #
        # O1 _|____ ... + O2 __|___ ... -> O3 ___|___ ... + O4 ____|__ ...

        y_data = np.zeros((2, 50)) * ureg("1 / meV")
        y_data[0, 2] = 1.0 * ureg("1 / meV")
        y_data[1, 3] = 1.0 * ureg("1 / meV")
        spectra = AbinsSpectrum1DCollection(
            x_data=(np.linspace(0, 10, 50) * ureg("meV")),
            y_data=y_data,
            metadata={
                "test_key": "test_value",
                "line_data": [{"atom_index": 0, "quantum_order": 1}, {"atom_index": 0, "quantum_order": 2}],
            },
        )
        output_spectra = add_autoconvolution_spectra(spectra, max_order=4)

        # Check only the approriate orders were included
        assert set(spectrum.metadata["quantum_order"] for spectrum in output_spectra) == set(range(1, 5))
        for order in 1, 2:
            selection = output_spectra.select(quantum_order=order)
            self.assertEqual(len(selection), 1)
            assert_almost_equal(spectra.select(quantum_order=order).y_data.magnitude, selection.y_data.magnitude)

        for order in 3, 4:
            expected = np.zeros((1, 50))
            expected[0, (order - 2) * 2 + 3] = 1.0
            assert_almost_equal(output_spectra.select(quantum_order=order).y_data.magnitude, expected)


class AbinsSDataTest(unittest.TestCase):
    def setUp(self):
        self.default_threshold_value = abins.parameters.sampling["s_absolute_threshold"]
        self.default_min_wavenumber = abins.parameters.sampling["min_wavenumber"]
        self.default_max_wavenumber = abins.parameters.sampling["max_wavenumber"]
        self.logger = logging.getLogger("abins-sdata-test")
        self.sample_data = {
            "atom_0": {"s": {"order_1": np.array([0.0, 0.001, 1.0, 1.0, 0.0])}},
            "atom_1": {"s": {"order_1": np.array([0.0, 1.001, 2.0, 0.0, 3.0])}},
        }

        self.sample_data_two_orders = {
            "atom_0": {"s": {"order_1": np.linspace(0, 2, 5), "order_2": np.linspace(2, 4, 5)}},
            "atom_1": {"s": {"order_1": np.linspace(3, 1, 5), "order_2": np.linspace(2, 1, 5)}},
        }

        self.frequencies = np.linspace(105, 145, 5)
        self.bin_width = 10

    def tearDown(self):
        abins.parameters.sampling["s_absolute_threshold"] = self.default_threshold_value
        abins.parameters.sampling["min_wavenumber"] = self.default_min_wavenumber
        abins.parameters.sampling["max_wavenumber"] = self.default_max_wavenumber

    def test_s_data(self):
        abins.parameters.sampling["min_wavenumber"] = 100
        abins.parameters.sampling["max_wavenumber"] = 150

        s_data = SData(temperature=10, sample_form="Powder", data=self.sample_data, frequencies=self.frequencies)

        self.assertTrue(np.allclose(s_data.extract()["frequencies"], self.frequencies))

        self.assertTrue(np.allclose(s_data.extract()["atom_0"]["s"]["order_1"], self.sample_data["atom_0"]["s"]["order_1"]))
        self.assertTrue(np.allclose(s_data.extract()["atom_1"]["s"]["order_1"], self.sample_data["atom_1"]["s"]["order_1"]))

        with self.assertRaises(AssertionError):
            with self.assertLogs(logger=self.logger, level="WARNING"):
                s_data.check_thresholds(logger=self.logger)

        abins.parameters.sampling["s_absolute_threshold"] = 0.5
        with self.assertLogs(logger=self.logger, level="WARNING"):
            s_data.check_thresholds(logger=self.logger)

    def test_s_data_get_empty(self):
        from itertools import product

        sdata = SData.get_empty(
            frequencies=np.linspace(1.0, 5.0, 10),
            atom_keys=["atom_2", "atom_3"],
            order_keys=["order_2", "order_3"],
            temperature=101.0,
            sample_form="Powder",
        )
        with self.assertRaises(IndexError):
            sdata[1]
        with self.assertRaises(KeyError):
            sdata[2]["order_1"]

        for atom, order in product([2, 3], ["order_2", "order_3"]):
            assert_almost_equal(sdata[atom][order], np.zeros(10))

        assert_almost_equal(sdata.get_temperature(), 101.0)
        self.assertEqual(sdata.get_sample_form(), "Powder")

    def test_s_data_update(self):
        # Case 1: add new atom
        sdata = SData(data=self.sample_data, frequencies=self.frequencies)
        sdata_new = SData(data={"atom_2": {"s": {"order_1": np.linspace(0, 2, 5)}}}, frequencies=self.frequencies)
        sdata.update(sdata_new)
        assert_almost_equal(sdata[0]["order_1"], self.sample_data["atom_0"]["s"]["order_1"])
        assert_almost_equal(sdata[2]["order_1"], np.linspace(0, 2, 5))

        # Case 2: add new order
        sdata = SData(data=self.sample_data, frequencies=self.frequencies)
        sdata_new = SData(data={"atom_1": {"s": {"order_2": np.linspace(0, 2, 5)}}}, frequencies=self.frequencies)
        sdata.update(sdata_new)
        assert_almost_equal(sdata[1]["order_1"], self.sample_data["atom_1"]["s"]["order_1"])
        assert_almost_equal(sdata[1]["order_2"], np.linspace(0, 2, 5))

        # Case 3: update in-place
        sdata = SData(data=self.sample_data, frequencies=self.frequencies)
        sdata_new = SData(
            data={"atom_1": {"s": {"order_1": np.linspace(0, 2, 5), "order_2": np.linspace(2, 4, 5)}}}, frequencies=self.frequencies
        )
        sdata.update(sdata_new)
        assert_almost_equal(sdata[1]["order_1"], np.linspace(0, 2, 5))
        assert_almost_equal(sdata[1]["order_2"], np.linspace(2, 4, 5))

        # Case 4: incompatible frequencies
        sdata = SData(data=self.sample_data, frequencies=self.frequencies)
        sdata_new = SData(data={"atom_2": {"s": {"order_1": np.linspace(0, 2, 4)}}}, frequencies=self.frequencies[:-1])
        with self.assertRaises(ValueError):
            sdata.update(sdata_new)

    def test_s_data_add_dict(self):
        from copy import deepcopy

        s_data = SData(data=deepcopy(self.sample_data), frequencies=self.frequencies)
        s_data.add_dict({"atom_1": {"s": {"order_1": np.ones(5)}}})

        assert_almost_equal(s_data[1]["order_1"], self.sample_data["atom_1"]["s"]["order_1"] + 1)

    def test_s_data_indexing(self):
        s_data = SData(data=self.sample_data, frequencies=self.frequencies)
        self.assertTrue(np.allclose(s_data[1]["order_1"], self.sample_data["atom_1"]["s"]["order_1"]))

        sliced_items = s_data[:]
        self.assertIsInstance(sliced_items, list)
        self.assertTrue(np.allclose(sliced_items[0]["order_1"], self.sample_data["atom_0"]["s"]["order_1"]))
        self.assertTrue(np.allclose(sliced_items[1]["order_1"], self.sample_data["atom_1"]["s"]["order_1"]))

    def test_s_data_multiply(self):
        s_data = SData(data=deepcopy(self.sample_data_two_orders), frequencies=self.frequencies)

        factors = np.array([[1, 2, 3, 4, 5], [2, 3, 4, 5, 6]])

        s_data_multiplied = s_data * factors

        assert_almost_equal(s_data_multiplied[0]["order_1"], np.linspace(0, 2, 5) * factors[0])
        assert_almost_equal(s_data_multiplied[0]["order_2"], np.linspace(2, 4, 5) * factors[1])
        assert_almost_equal(s_data_multiplied[1]["order_1"], np.linspace(3, 1, 5) * factors[0])
        assert_almost_equal(s_data_multiplied[1]["order_2"], np.linspace(2, 1, 5) * factors[1])

        # Check there was no side-effect on initial sdata
        assert_almost_equal(s_data[0]["order_2"], self.sample_data_two_orders["atom_0"]["s"]["order_2"])

        # Now check that in-place mult gives the same results
        s_data *= factors
        for atom1, atom2 in zip(s_data, s_data_multiplied):
            assert_almost_equal(atom1["order_1"], atom2["order_1"])
            assert_almost_equal(atom1["order_2"], atom2["order_2"])

    def test_sample_form(self):
        sample_form = "Polycrystalline"

        with self.assertRaisesRegex(ValidationError, "Input should be 'Powder'"):
            _ = SData(sample_form=sample_form, data=self.sample_data, frequencies=self.frequencies)

    def test_bin_width(self):
        s_data = SData(data=self.sample_data, frequencies=self.frequencies)
        self.assertAlmostEqual(self.bin_width, s_data.get_bin_width())

        # Nonlinear frequency sampling has no bin width
        irregular_frequencies = np.exp(self.frequencies)
        s_data_irregular_freq = SData(data=self.sample_data, frequencies=irregular_frequencies)
        self.assertIsNone(s_data_irregular_freq.get_bin_width())

        # Unordered frequencies are rejected at init
        shuffled_frequencies = np.concatenate([self.frequencies[3:], self.frequencies[:3]])
        with self.assertRaises(ValueError):
            SData(data=self.sample_data, frequencies=shuffled_frequencies)

    def test_s_data_temperature(self):
        # Good temperature should pass without issue
        good_temperature = 10.5
        s_data_good_temperature = SData(frequencies=self.frequencies, data=self.sample_data, temperature=good_temperature)
        self.assertAlmostEqual(good_temperature, s_data_good_temperature.get_temperature())
        # Check some validation cases
        with self.assertRaises(ValidationError):
            SData(frequencies=self.frequencies, data=self.sample_data, temperature="10")

        for bad_temperature in (-20.0, 0):
            with self.assertRaises(ValidationError):
                s_data_bad_temperature = SData(frequencies=self.frequencies, data=self.sample_data, temperature=bad_temperature)
