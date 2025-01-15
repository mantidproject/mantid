# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from copy import deepcopy
import functools
import json
from pathlib import Path
import unittest
from tempfile import TemporaryDirectory

import numpy as np
from numpy.testing import assert_almost_equal
from pydantic import ValidationError

import abins
from abins import AbinsData
import abins.input
from abins.constants import FUNDAMENTALS
import abins.instruments


class SCalculatorFactoryPowderTest(unittest.TestCase):
    """
    Test of SCalculatorFactory for the Powder scenario.
    """

    _temperature = 10  # 10 K,  temperature for the benchmark
    _sample_form = "Powder"
    _instrument = abins.instruments.get_instrument("TOSCA")
    _order_event = FUNDAMENTALS
    _si2 = "Si2-sc_CalculateSPowder"
    _instruments_defaults = {}

    def setUp(self):
        self.default_threads = abins.parameters.performance["threads"]
        abins.parameters.performance["threads"] = 1
        self._instruments_defaults = deepcopy(abins.parameters.instruments)
        self._temporary_directory = TemporaryDirectory()
        self._cache_directory = Path(self._temporary_directory.name)

        self.default_calculator_kwargs = dict(
            temperature=self._temperature,
            instrument=self._instrument,
            sample_form=self._sample_form,
            quantum_order_num=self._order_event,
            autoconvolution_max=0,
            cache_directory=self._cache_directory,
        )

    def tearDown(self):
        from mantid.kernel import ConfigService

        abins.test_helpers.remove_output_files(
            list_of_names=["_CalculateSPowder"], directory=ConfigService.getString("defaultsave.directory")
        )
        abins.parameters.performance["threads"] = self.default_threads
        abins.parameters.instruments.update(self._instruments_defaults)

    #     test input
    def test_invalid_input(self):
        full_path_filename = abins.test_helpers.find_file(filename=self._si2 + ".json")
        with open(full_path_filename, "r") as fp:
            good_data = AbinsData.from_dict(json.load(fp))

        # invalid filename
        with self.assertRaisesRegex(ValidationError, "Input should be a valid string"):
            abins.SCalculatorFactory.init(
                filename=1,
                temperature=self._temperature,
                sample_form=self._sample_form,
                abins_data=good_data,
                instrument=self._instrument,
                quantum_order_num=self._order_event,
                cache_directory=self._cache_directory,
            )

        # invalid temperature
        with self.assertRaisesRegex(ValidationError, "Input should be greater than 0"):
            abins.SCalculatorFactory.init(
                filename=full_path_filename,
                temperature=-1,
                sample_form=self._sample_form,
                abins_data=good_data,
                instrument=self._instrument,
                quantum_order_num=self._order_event,
                cache_directory=self._cache_directory,
            )

        # invalid sample
        with self.assertRaises(ValueError):
            abins.SCalculatorFactory.init(
                filename=full_path_filename,
                temperature=self._temperature,
                sample_form="SOLID",
                abins_data=good_data,
                instrument=self._instrument,
                quantum_order_num=self._order_event,
                cache_directory=self._cache_directory,
            )

        # invalid abins data: content of abins data instead of object abins_data
        with self.assertRaisesRegex(ValidationError, "Input should be an instance of AbinsData"):
            abins.SCalculatorFactory.init(
                filename=full_path_filename,
                temperature=self._temperature,
                sample_form=self._sample_form,
                abins_data=good_data.extract(),
                instrument=self._instrument,
                quantum_order_num=self._order_event,
                cache_directory=self._cache_directory,
            )

        # invalid instrument
        with self.assertRaisesRegex(ValidationError, "Input should be an instance of Instrument"):
            abins.SCalculatorFactory.init(
                filename=full_path_filename,
                temperature=self._temperature,
                sample_form=self._sample_form,
                abins_data=good_data,
                instrument="INSTRUMENT",
                quantum_order_num=self._order_event,
                cache_directory=self._cache_directory,
            )

    def test_1d_order1(self):
        self._good_case()

    def test_1d_order2(self):
        self._good_case(test_name=self._si2 + "_1d_o2", abinsdata_name=self._si2, quantum_order_num=2)

    def test_1d_order10(self):
        self._good_case(test_name=self._si2 + "_1d_o10", abinsdata_name=self._si2, quantum_order_num=2, autoconvolution_max=10)

    def test_2d_order1_mari(self):
        abins.parameters.instruments["MARI"].update({"q_size": 10, "n_energy_bins": 100})
        mari = abins.instruments.get_instrument("MARI")
        mari.set_incident_energy(1000.0)

        self._good_case(test_name=self._si2 + "_2d_o1_mari", abinsdata_name=self._si2, instrument=mari)

    def test_2d_order2_panther(self):
        abins.parameters.instruments["PANTHER"].update({"q_size": 8, "n_energy_bins": 60})
        panther = abins.instruments.get_instrument("PANTHER")
        panther.set_incident_energy(1000.0)

        self._good_case(test_name=self._si2 + "_2d_o2_panther", quantum_order_num=2, abinsdata_name=self._si2, instrument=panther)

    # helper functions
    def _good_case(self, test_name=_si2, abinsdata_name=_si2, **calculator_kwargs):
        from abins.constants import ONE_DIMENSIONAL_INSTRUMENTS

        abinsdata_file = abins.test_helpers.find_file(filename=abinsdata_name + ".json")
        abins_data = self._get_abins_data(abinsdata_file)

        calc_kwargs = self.default_calculator_kwargs.copy()
        calc_kwargs.update(calculator_kwargs)

        # calculation of powder data
        good_tester = abins.SCalculatorFactory.init(
            filename=abinsdata_file,
            abins_data=abins_data,
            **calc_kwargs,
        )
        calculated_data = good_tester.get_formatted_data()

        ### Uncomment to generate new data ###
        # from pathlib import Path
        # data_path = Path(abins.test_helpers.find_file(filename=abinsdata_name + ".json")).parent
        # self._write_data(data=calculated_data.extract(),
        #                  filename=(data_path / f"{name}_S.txt"))

        good_data = self._get_good_data(test_name=test_name, abinsdata_filename=abinsdata_name)
        self._check_data(good_data=good_data, data=calculated_data.extract())

        # This time the data should be loaded from cache. Check this is consistent with calculation
        if calc_kwargs["instrument"].get_name() in ONE_DIMENSIONAL_INSTRUMENTS:
            new_tester = abins.SCalculatorFactory.init(filename=abinsdata_file, abins_data=abins_data, **calc_kwargs)
            loaded_data = new_tester.load_formatted_data()

            self._check_data(good_data=good_data, data=loaded_data.extract())

    @staticmethod
    @functools.lru_cache(maxsize=4)
    def _get_abins_data(abinsdata_file):
        with open(abinsdata_file, "r") as fp:
            return AbinsData.from_dict(json.load(fp))

    def _get_good_data(self, *, test_name, abinsdata_filename):
        s_data = self._prepare_data(filename=abins.test_helpers.find_file(filename=(test_name + "_S.txt")))
        return s_data

    @staticmethod
    def _write_data(*, data, filename):
        from abins.test_helpers import dict_arrays_to_lists

        with open(filename, "w") as fd:
            json.dump(dict_arrays_to_lists(data), fd, indent=4, sort_keys=True)

    @staticmethod
    def _prepare_data(filename=None):
        """Reads a correct values from ASCII file."""
        with open(filename) as data_file:
            # noinspection PyPep8
            correct_data = json.loads(
                data_file.read()
                .replace("\\n", " ")
                .replace("array", "")
                .replace("([", "[")
                .replace("])", "]")
                .replace("'", '"')
                .replace("0. ", "0.0")
            )

        for array_key in ("frequencies", "q_bins"):
            if correct_data.get(array_key) is not None:
                correct_data[array_key] = np.asarray(correct_data[array_key])

        n_atoms = len([True for key in correct_data if "atom" in key])
        for i in range(n_atoms):
            for order_key, order_data in correct_data[f"atom_{i}"]["s"].items():
                correct_data[f"atom_{i}"]["s"][order_key] = np.asarray(order_data)

        return correct_data

    def _check_data(self, good_data=None, data=None):
        for array_key in ("frequencies", "q_bins"):
            if good_data.get(array_key) is not None:
                assert_almost_equal(good_data[array_key], data[array_key])

        n_atoms = len([True for key in good_data if "atom" in key])
        for i in range(n_atoms):
            for order_key in good_data[f"atom_{i}"]["s"]:
                ref = good_data[f"atom_{i}"]["s"][order_key]
                calculated = data[f"atom_{i}"]["s"].get(order_key)
                assert_almost_equal(ref, calculated)


if __name__ == "__main__":
    unittest.main()
