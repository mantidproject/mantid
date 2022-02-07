# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import json
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

    def setUp(self):
        self.default_threads = abins.parameters.performance['threads']
        abins.parameters.performance['threads'] = 1

    def tearDown(self):
        abins.test_helpers.remove_output_files(list_of_names=["CalculateSPowder"])
        abins.parameters.performance['threads'] = self.default_threads

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

    #  main test
    def test_good_case(self):
        self._good_case(name=self._si2)

    # helper functions
    def _good_case(self, name=None):
        # calculation of powder data
        good_data = self._get_good_data(filename=name)
        good_tester = abins.SCalculatorFactory.init(
            filename=abins.test_helpers.find_file(filename=name + ".phonon"), temperature=self._temperature,
            sample_form=self._sample_form, abins_data=good_data["DFT"], instrument=self._instrument,
            quantum_order_num=self._order_event)
        calculated_data = good_tester.get_formatted_data()

        self._check_data(good_data=good_data["S"], data=calculated_data.extract())

        # check if loading powder data is correct
        new_tester = abins.SCalculatorFactory.init(
            filename=abins.test_helpers.find_file(filename=name + ".phonon"), temperature=self._temperature,
            sample_form=self._sample_form, abins_data=good_data["DFT"], instrument=self._instrument,
            quantum_order_num=self._order_event)
        loaded_data = new_tester.load_formatted_data()

        self._check_data(good_data=good_data["S"], data=loaded_data.extract())

    def _get_good_data(self, filename=None):

        castep_reader = abins.input.CASTEPLoader(
            input_ab_initio_filename=abins.test_helpers.find_file(filename=filename + ".phonon"))
        s_data = self._prepare_data(filename=abins.test_helpers.find_file(filename=filename + "_S.txt"))

        return {"DFT": castep_reader.read_vibrational_or_phonon_data(), "S": s_data}

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

        temp = np.asarray(correct_data["frequencies"])
        correct_data["frequencies"] = temp

        # we need to - 1 because one entry is "frequencies"
        for el in range(len(correct_data) - 1):

            temp = np.asarray(correct_data["atom_%s" % el]["s"]["order_%s" % FUNDAMENTALS])
            correct_data["atom_%s" % el]["s"]["order_%s" % FUNDAMENTALS] = temp

        return correct_data

    def _check_data(self, good_data=None, data=None):

        good_temp = good_data["frequencies"]
        data_temp = data["frequencies"]
        assert_almost_equal(good_temp, data_temp)

        # we need to - 1 because one entry is "frequencies"
        for el in range(len(good_data) - 1):

            good_temp = good_data["atom_%s" % el]["s"]["order_%s" % FUNDAMENTALS]
            data_temp = data["atom_%s" % el]["s"]["order_%s" % FUNDAMENTALS]

            assert_almost_equal(good_temp, data_temp)


if __name__ == '__main__':
    unittest.main()
