# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from copy import deepcopy
import functools
import json
import unittest

import numpy as np
from numpy.testing import assert_almost_equal

import abins
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

    default_calculator_kwargs = dict(temperature=_temperature,
                                     instrument=_instrument,
                                     sample_form=_sample_form,
                                     quantum_order_num=_order_event,
                                     autoconvolution=False)

    def setUp(self):
        self.default_threads = abins.parameters.performance['threads']
        abins.parameters.performance['threads'] = 1
        self._instruments_defaults = deepcopy(abins.parameters.instruments)

    def tearDown(self):
        abins.test_helpers.remove_output_files(list_of_names=["CalculateSPowder"])
        abins.parameters.performance['threads'] = self.default_threads
        abins.parameters.instruments.update(self._instruments_defaults)

    #     test input
    def test_wrong_input(self):
        full_path_filename = abins.test_helpers.find_file(filename=self._si2 + ".phonon")

        castep_reader = abins.input.CASTEPLoader(input_ab_initio_filename=full_path_filename)
        good_data = castep_reader.read_vibrational_or_phonon_data()

        # wrong filename
        with self.assertRaises(ValueError):
            abins.SCalculatorFactory.init(filename=1, temperature=self._temperature, sample_form=self._sample_form,
                                          abins_data=good_data, instrument=self._instrument,
                                          quantum_order_num=self._order_event)

        # wrong temperature
        with self.assertRaises(ValueError):
            abins.SCalculatorFactory.init(filename=full_path_filename, temperature=-1, sample_form=self._sample_form,
                                          abins_data=good_data, instrument=self._instrument,
                                          quantum_order_num=self._order_event)

        # wrong sample
        with self.assertRaises(ValueError):
            abins.SCalculatorFactory.init(filename=full_path_filename, temperature=self._temperature,
                                          sample_form="SOLID", abins_data=good_data, instrument=self._instrument,
                                          quantum_order_num=self._order_event)

        # wrong abins data: content of abins data instead of object abins_data
        with self.assertRaises(ValueError):
            abins.SCalculatorFactory.init(filename=full_path_filename, temperature=self._temperature,
                                          sample_form=self._sample_form, abins_data=good_data.extract(),
                                          instrument=self._instrument, quantum_order_num=self._order_event)

        # wrong instrument
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            abins.SCalculatorFactory.init(filename=full_path_filename, temperature=self._temperature,
                                          sample_form=self._sample_form, abins_data=good_data.extract(),
                                          instrument=self._instrument, quantum_order_num=self._order_event)

    def test_1d_order1(self):
        self._good_case()

    def test_1d_order2(self):
        self._good_case(name=self._si2 + '_1d_o2',
                        castep_name=self._si2,
                        quantum_order_num=2)

    def test_1d_order10(self):
        self._good_case(name=self._si2 + '_1d_o10',
                        castep_name=self._si2,
                        quantum_order_num=2,
                        autoconvolution=True)

    def test_2d_order1_mari(self):
        abins.parameters.instruments['MARI'].update({'q_size': 10,
                                                     'n_energy_bins': 100})
        mari = abins.instruments.get_instrument("MARI")
        mari.set_incident_energy(1000.)

        self._good_case(name=self._si2 + '_2d_o1_mari',
                        castep_name=self._si2,
                        instrument=mari)

    def test_2d_order2_panther(self):
        abins.parameters.instruments['PANTHER'].update({'q_size': 8,
                                                        'n_energy_bins': 60})
        panther = abins.instruments.get_instrument("PANTHER")
        panther.set_incident_energy(1000.)

        self._good_case(name=self._si2 + '_2d_o2_panther',
                        quantum_order_num=2,
                        castep_name=self._si2,
                        instrument=panther)

    # helper functions
    def _good_case(self, name=_si2, castep_name=None, **calculator_kwargs):
        from abins.constants import ONE_DIMENSIONAL_INSTRUMENTS

        if castep_name is None:
            castep_name = name

        calc_kwargs = self.default_calculator_kwargs.copy()
        calc_kwargs.update(calculator_kwargs)

        # calculation of powder data
        good_tester = abins.SCalculatorFactory.init(
            filename=abins.test_helpers.find_file(filename=castep_name + ".phonon"),
            abins_data = self._get_abins_data(castep_name),
            **calc_kwargs)
        calculated_data = good_tester.get_formatted_data()

        ### Uncomment to generate new data ###
        # from pathlib import Path
        # data_path = Path(abins.test_helpers.find_file(filename=castep_name + ".phonon")).parent
        # self._write_data(data=calculated_data.extract(),
        #                  filename=(data_path / f"{name}_S.txt"))

        good_data = self._get_good_data(filename=name, castep_filename=castep_name)
        self._check_data(good_data=good_data["S"], data=calculated_data.extract())

        # check if loading powder data is correct
        if calc_kwargs['instrument'].get_name() in ONE_DIMENSIONAL_INSTRUMENTS:
            new_tester = abins.SCalculatorFactory.init(
                filename=abins.test_helpers.find_file(filename=castep_name + ".phonon"),
                abins_data=good_data["DFT"],
                **calc_kwargs)
            loaded_data = new_tester.load_formatted_data()

            self._check_data(good_data=good_data["S"], data=loaded_data.extract())

    @staticmethod
    @functools.lru_cache(maxsize=4)
    def _get_abins_data(castep_filename):
        castep_reader = abins.input.CASTEPLoader(
            input_ab_initio_filename=abins.test_helpers.find_file(filename=castep_filename + ".phonon"))
        return castep_reader.read_vibrational_or_phonon_data()

    def _get_good_data(self, filename=None, castep_filename=None):
        if castep_filename is None:
            castep_filename = filename

        s_data = self._prepare_data(filename=abins.test_helpers.find_file(filename=(filename + "_S.txt")))

        return {"DFT": self._get_abins_data(castep_filename), "S": s_data}

    @staticmethod
    def _write_data(*, data, filename):
        from abins.input.tester import Tester
        with open(filename, 'w') as fd:
            json.dump(Tester._arrays_to_lists(data), fd, indent=4, sort_keys=True)

    # noinspection PyMethodMayBeStatic
    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        with open(filename) as data_file:
            # noinspection PyPep8
            correct_data = json.loads(data_file.read().replace("\\n", " ").
                                      replace("array",    "").
                                      replace("([",  "[").
                                      replace("])",  "]").
                                      replace("'",  '"').
                                      replace("0. ", "0.0"))

        for array_key in ("frequencies", "q_bins"):
            if correct_data.get(array_key) is not None:
                correct_data[array_key] = np.asarray(correct_data[array_key])

        n_atoms = len([True for key in correct_data if 'atom' in key])
        for i in range(n_atoms):
            for order_key, order_data in correct_data[f"atom_{i}"]["s"].items():
                correct_data[f"atom_{i}"]["s"][order_key] = np.asarray(order_data)

            # correct_data[f"atom_{i}"]["s"]["order_1"] = np.asarray(
            #     correct_data[f"atom_{i}"]["s"]["order_1"])

        return correct_data

    def _check_data(self, good_data=None, data=None):

        for array_key in ("frequencies", "q_bins"):
            if good_data.get(array_key) is not None:
                assert_almost_equal(good_data[array_key],
                                    data[array_key])

        n_atoms = len([True for key in good_data if 'atom' in key])
        for i in range(n_atoms):
            for order_key in good_data[f"atom_{i}"]["s"]:
                ref = good_data[f"atom_{i}"]["s"][order_key]
                calculated = data[f"atom_{i}"]["s"].get(order_key)
                assert_almost_equal(ref, calculated)


if __name__ == '__main__':
    unittest.main()
