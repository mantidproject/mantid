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
from abins.sdata import SDataByAngle


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

    def test_s_data_apply_dw(self):
        dw = np.random.RandomState(42).rand(2, 5)

        for min_order, max_order, expected in [
            (
                1,
                1,
                {
                    "atom_0": {"order_1": np.linspace(0, 2, 5) * dw[0, :], "order_2": np.linspace(2, 4, 5)},
                    "atom_1": {"order_1": np.linspace(3, 1, 5) * dw[1, :], "order_2": np.linspace(2, 1, 5)},
                },
            ),
            (
                2,
                2,
                {
                    "atom_0": {"order_1": np.linspace(0, 2, 5), "order_2": np.linspace(2, 4, 5) * dw[0, :]},
                    "atom_1": {"order_1": np.linspace(3, 1, 5), "order_2": np.linspace(2, 1, 5) * dw[1, :]},
                },
            ),
            (
                1,
                2,
                {
                    "atom_0": {"order_1": np.linspace(0, 2, 5) * dw[0, :], "order_2": np.linspace(2, 4, 5) * dw[0, :]},
                    "atom_1": {"order_1": np.linspace(3, 1, 5) * dw[1, :], "order_2": np.linspace(2, 1, 5) * dw[1, :]},
                },
            ),
        ]:

            sdata = SData(data=deepcopy(self.sample_data_two_orders), frequencies=self.frequencies)
            sdata.apply_dw(dw, min_order=min_order, max_order=max_order)
            for atom_key, atom_data in sdata.extract().items():
                if atom_key == "frequencies":
                    continue
                for order_key in atom_data["s"]:
                    assert_almost_equal(atom_data["s"][order_key], expected[atom_key][order_key])

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

    def test_s_data_autoconvolution(self):
        # Check a trivial case: starting with a single peak,
        # expect evenly-spaced sequence of same intensity
        #
        # _|____ .... -> _|_|_|_|_ ...
        #
        frequencies = np.linspace(0, 10, 50)
        data_o1 = {"atom_0": {"s": {"order_1": np.zeros(50)}}}
        data_o1["atom_0"]["s"]["order_1"][2] = 1.0
        expected_o1 = {"atom_0": {"s": {f"order_{i}": np.zeros(50) for i in range(1, 11)}}}
        for i in range(1, 11):
            expected_o1["atom_0"]["s"][f"order_{i}"][(2 * i)] = 1.0

        s_data_o1 = SData(data=data_o1, frequencies=frequencies)
        s_data_o1.add_autoconvolution_spectra()

        expected_s_data = SData(data=expected_o1, frequencies=frequencies)

        for i in range(1, 11):
            assert_almost_equal(s_data_o1[0][f"order_{i}"], expected_s_data[0][f"order_{i}"])

        # Check range restriction works, and beginning with more orders
        #
        # O1 _|____ ... + O2 __|___ ... -> O3 ___|___ ... + O4 ____|__ ...

        data_o2 = {"atom_0": {"s": {"order_1": np.zeros(50), "order_2": np.zeros(50)}}}
        data_o2["atom_0"]["s"]["order_1"][2] = 1.0
        data_o2["atom_0"]["s"]["order_2"][3] = 1.0
        s_data_o2 = SData(data=data_o2, frequencies=frequencies)
        s_data_o2.add_autoconvolution_spectra(max_order=4)

        # Check only the approriate orders were included
        assert set(s_data_o2[0].keys()) == set([f"order_{i}" for i in range(1, 5)])
        for order_key in ("order_1", "order_2"):
            assert_almost_equal(s_data_o2[0][order_key], data_o2["atom_0"]["s"][order_key])
        for order in range(3, 5):
            expected = np.zeros(50)
            #         ac steps    o1  o2
            expected[(order - 2) * 2 + 3] = 1.0
            assert_almost_equal(s_data_o2[0][f"order_{order}"], expected)

    def test_s_data_temperature(self):
        # Good temperature should pass without issue
        good_temperature = 10.5
        s_data_good_temperature = SData(frequencies=self.frequencies, data=self.sample_data, temperature=good_temperature)
        self.assertAlmostEqual(good_temperature, s_data_good_temperature.get_temperature())

        for bad_temperature, regex in (
            (-20.0, "Input should be greater than 0"),
            (0, "Input should be greater than 0"),
            ("10", "Input should be a valid number"),
        ):
            with self.assertRaisesRegex(ValidationError, regex):
                _ = SData(frequencies=self.frequencies, data=self.sample_data, temperature=bad_temperature)


class AbinsSDataByAngleTest(unittest.TestCase):
    def setUp(self):
        self.list_data = [
            {
                "atom_0": {
                    "s": {
                        "order_1": np.array(
                            [
                                0.0,
                                0.001,
                                1.0,
                                1.0,
                                0.0,
                                0.0,
                            ]
                        )
                    }
                },
                "atom_1": {
                    "s": {
                        "order_1": np.array(
                            [
                                0.0,
                                1.001,
                                2.0,
                                0.0,
                                3.0,
                                0.0,
                            ]
                        )
                    }
                },
            },
            {
                "atom_0": {
                    "s": {
                        "order_1": np.array(
                            [
                                2.0,
                                2.001,
                                3.0,
                                2.0,
                                0.0,
                                0.0,
                            ]
                        )
                    }
                },
                "atom_1": {
                    "s": {
                        "order_1": np.array(
                            [
                                2.0,
                                3.001,
                                2.0,
                                0.0,
                                2.0,
                                0.0,
                            ]
                        )
                    }
                },
            },
        ]

        self.init_data = {
            "atom_0": {"s": {"order_1": np.array([[0.0, 0.001, 1.0, 1.0, 0.0, 0.0], [2.0, 2.001, 3.0, 2.0, 0.0, 0.0]])}},
            "atom_1": {
                "s": {
                    "order_1": np.array(
                        [
                            [
                                0.0,
                                1.001,
                                2.0,
                                0.0,
                                3.0,
                                0.0,
                            ],
                            [
                                2.0,
                                3.001,
                                2.0,
                                0.0,
                                2.0,
                                0.0,
                            ],
                        ]
                    )
                }
            },
        }

        self.angles = [27.0, 45.0]
        self.frequencies = np.linspace(105, 145, 6)
        self.sum_data = {
            "atom_0": {"s": {"order_1": np.array([2.0, 2.002, 4.0, 3.0, 0, 0.0])}},
            "atom_1": {"s": {"order_1": np.array([2.0, 4.002, 4.0, 0.0, 5.0, 0.0])}},
        }
        self.avg_data = {
            "atom_0": {"s": {"order_1": np.array([1.0, 1.001, 2.0, 1.5, 0, 0.0])}},
            "atom_1": {"s": {"order_1": np.array([1.0, 2.001, 2.0, 0.0, 2.5, 0.0])}},
        }
        self.weighted_data = {
            "atom_0": {"s": {"order_1": np.array([2.0, 2.003, 5.0, 4.0, 0, 0.0])}},
            "atom_1": {"s": {"order_1": np.array([2.0, 5.003, 6.0, 0.0, 8.0, 0.0])}},
        }

    def test_s_data_by_angle_set_angle_data(self):
        sdba = self.get_s_data_by_angle()
        sdba.set_angle_data(0, SData(data=self.list_data[1], frequencies=self.frequencies))
        self.assertTrue(np.allclose(sdba[0].extract()["atom_1"]["s"]["order_1"], self.list_data[1]["atom_1"]["s"]["order_1"]))

        sdba.set_angle_data(0, SData(data=self.list_data[0], frequencies=self.frequencies), add_to_existing=True)
        self.assertTrue(np.allclose(sdba[0].extract()["atom_1"]["s"]["order_1"], self.sum_data["atom_1"]["s"]["order_1"]))

    def get_s_data_by_angle(self, **kwargs):
        return SDataByAngle(data=self.init_data, angles=self.angles, frequencies=self.frequencies, **kwargs)

    def test_s_data_by_angle_indexing(self):
        temperature, sample_form = (100.0, "Powder")

        sdba = self.get_s_data_by_angle(temperature=temperature, sample_form=sample_form)

        for i in range(2):
            self.assertIsInstance(sdba[i], SData)
            self.assertTrue(np.allclose(sdba[i].get_frequencies(), self.frequencies))
            self.assertTrue(np.allclose(sdba[i].extract()["atom_1"]["s"]["order_1"], self.list_data[i]["atom_1"]["s"]["order_1"]))

        with self.assertRaises(IndexError):
            sdba[2]

    def test_s_data_by_angle_slicing(self):
        temperature, sample_form = (101.0, "Powder")

        sdba = self.get_s_data_by_angle(temperature=temperature, sample_form=sample_form)
        sliced_data = sdba[-1:]

        self.assertIsInstance(sliced_data, SDataByAngle)
        self.assertTrue(np.allclose(self.frequencies, sliced_data.frequencies))
        self.assertEqual(sample_form, sliced_data.sample_form)
        self.assertEqual(temperature, sliced_data.temperature)

        self.assertTrue(np.allclose(self.list_data[1]["atom_0"]["s"]["order_1"], sliced_data[0].extract()["atom_0"]["s"]["order_1"]))

        self.assertEqual(len(sdba), len(self.list_data))
        self.assertEqual(len(sdba[-1:]), 1)

    def test_sdba_from_sdata_series(self):
        temperature, sample_form = (102.0, "Powder")
        angle_0_sdata = SData(data=self.list_data[0], frequencies=self.frequencies, temperature=temperature, sample_form=sample_form)
        angle_1_sdata = SData(data=self.list_data[1], frequencies=self.frequencies, temperature=temperature, sample_form=sample_form)

        sdba = SDataByAngle.from_sdata_series([angle_0_sdata, angle_1_sdata], angles=self.angles)

        for input_data, output_data in zip([angle_0_sdata, angle_1_sdata], sdba):
            self.assertEqual(input_data.get_sample_form(), output_data.get_sample_form())
            self.assertEqual(input_data.get_temperature(), output_data.get_temperature())
        self.assertTrue(np.allclose(input_data.get_frequencies(), output_data.get_frequencies()))
        self.assertTrue(np.allclose(input_data.extract()["atom_0"]["s"]["order_1"], output_data.extract()["atom_0"]["s"]["order_1"]))

    def test_sdba_from_sdata_series_bad_inputs(self):
        bad_data = [  # Inconsistent frequencies
            {
                "data": [
                    SData(data=self.list_data[0], frequencies=np.array([1, 2, 3, 4, 5, 6])),
                    SData(data=self.list_data[1], frequencies=np.array([2, 4, 6, 8, 10, 12])),
                ],
                "angles": self.angles,
                "error": ValueError,
            },
            # Inconsistent temperature
            {
                "data": [
                    SData(data=self.list_data[0], frequencies=self.frequencies, temperature=100.0),
                    SData(data=self.list_data[1], frequencies=self.frequencies, temperature=200.0),
                ],
                "angles": self.angles,
                "error": ValueError,
            },
            # Inconsistent number of angles
            {
                "data": [SData(data=self.list_data[0], frequencies=self.frequencies, temperature=100.0)],
                "angles": self.angles,
                "error": IndexError,
            },
            # Bad typing
            {
                "data": [SData(data=self.list_data[0], frequencies=self.frequencies, temperature=100.0), "SData (actually just a string)"],
                "angles": self.angles,
                "error": ValidationError,
            },
        ]

        for dataset in bad_data:
            with self.assertRaises(dataset["error"]):
                SDataByAngle.from_sdata_series(dataset["data"], angles=dataset["angles"])

    def test_s_data_sum_over_angles(self):
        temperature, sample_form = (100.0, "Powder")

        sdba = self.get_s_data_by_angle(temperature=temperature, sample_form=sample_form)

        for kwargs, ref_data in [({}, self.sum_data), ({"average": True}, self.avg_data), ({"weights": [2, 1]}, self.weighted_data)]:

            summed_data = sdba.sum_over_angles(**kwargs)
            for atom_key in ref_data:
                self.assertTrue(np.allclose(summed_data.extract()[atom_key]["s"]["order_1"], ref_data[atom_key]["s"]["order_1"]))
