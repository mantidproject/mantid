import unittest
from mantid.simpleapi import logger
import os
import json
import numpy as np
from AbinsModules import AbinsConstants, CalculateS, LoadCASTEP


def old_modules():
    """" Check if there are proper versions of  Python and numpy."""
    is_python_old = AbinsConstants.old_python()
    if is_python_old:
        logger.warning("Skipping ABINSCalculateSPowderTest because Python is too old.")

    is_numpy_old = AbinsConstants.is_numpy_valid(np.__version__)
    if is_numpy_old:
        logger.warning("Skipping ABINSCalculateSPowderTest because numpy is too old.")

    return is_python_old or is_numpy_old


def skip_if(skipping_criteria):
    """
    Skip all tests if the supplied function returns true.
    Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    """

    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and 'test' in attr:
                    delattr(cls, attr)
        return cls

    return decorate


@skip_if(old_modules)
class ABINSCalculateSPowderTest(unittest.TestCase):
    """
    Test of  CalculateS for the Powder scenario.
    """

    _temperature = 10  # 10 K,  temperature for the benchmark
    _sample_form = "Powder"
    _instrument_name = "TOSCA"
    _order_event = AbinsConstants.FUNDAMENTALS

    # data
    core = os.path.normpath("../ExternalData/Testing/Data/UnitTest/")

    squaricn = "squaricn_sum_CalculateSPowder"
    Si2 = "Si2-sc_CalculateSPowder"

    Squaricn_path = os.path.abspath(os.path.join(core, squaricn))
    Si2_path = os.path.abspath(os.path.join(core, Si2))

    def remove_hdf_files(self):
        files = os.listdir(os.getcwd())
        print os.getcwd()
        for filename in files:
            if self.Si2 in filename or self.squaricn in filename:
                os.remove(filename)

    def setUp(self):
        self.remove_hdf_files()

    def tearDown(self):
        self.remove_hdf_files()

    #     test input
    def test_wrong_input(self):
        filename = self.Si2_path + ".phonon"

        _castep_reader = LoadCASTEP(input_dft_filename=filename)
        _good_data = _castep_reader.read_phonon_file()

        # wrong filename
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_s = CalculateS(filename=1, temperature=self._temperature, sample_form=self._sample_form,
                                abins_data=_good_data, instrument_name=self._instrument_name,
                                quantum_order_num=self._order_event)

        # wrong temperature
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_s = CalculateS(filename=filename, temperature=-1, sample_form=self._sample_form, abins_data=_good_data,
                                instrument_name=self._instrument_name, quantum_order_num=self._order_event)

        # wrong sample
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_s = CalculateS(filename=filename, temperature=self._temperature, sample_form="SOLID",
                                abins_data=_good_data, instrument_name=self._instrument_name,
                                quantum_order_num=self._order_event)

        # wrong abins data: content of abins data instead of object abins_data
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_s = CalculateS(filename=filename, temperature=self._temperature, sample_form=self._sample_form,
                                abins_data=_good_data.extract(), instrument_name=self._instrument_name,
                                quantum_order_num=self._order_event)

        # wrong instrument
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_s = CalculateS(filename=filename, temperature=self._temperature, sample_form=self._sample_form,
                                abins_data=_good_data.extract(), instrument_name=self._instrument_name,
                                quantum_order_num=self._order_event)

    #  main test
    def test_good_case(self):
        self._good_case(name=self.Si2_path)
        self._good_case(name=self.Squaricn_path)

    # helper functions
    def _good_case(self, name=None):
        # calculation of powder data
        _good_data = self._get_good_data(filename=name)
        _good_tester = CalculateS(filename=name + ".phonon", temperature=self._temperature,
                                  sample_form=self._sample_form, abins_data=_good_data["DFT"],
                                  instrument_name=self._instrument_name, quantum_order_num=self._order_event)
        calculated_data = _good_tester.get_data()

        self._check_data(good_data=_good_data["S"], data=calculated_data.extract())

        # check if loading powder data is correct
        new_tester = CalculateS(filename=name + ".phonon", temperature=self._temperature, sample_form=self._sample_form,
                                abins_data=_good_data["DFT"], instrument_name=self._instrument_name,
                                quantum_order_num=self._order_event)
        loaded_data = new_tester.load_data()

        self._check_data(good_data=_good_data["S"], data=loaded_data.extract())

    def _get_good_data(self, filename=None):

        _CASTEP_reader = LoadCASTEP(input_dft_filename=filename + ".phonon")
        _S = self._prepare_data(filename=filename + "_S.txt")

        return {"DFT": _CASTEP_reader.read_phonon_file(), "S": _S}

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

        for el in range(len(correct_data)):

            temp = np.asarray(correct_data["atom_%s" % el]["frequencies"]["order_%s" % AbinsConstants.FUNDAMENTALS])
            correct_data["atom_%s" % el]["frequencies"]["order_%s" % AbinsConstants.FUNDAMENTALS] = temp

            temp = np.asarray(correct_data["atom_%s" % el]["s"]["order_%s" % AbinsConstants.FUNDAMENTALS])
            correct_data["atom_%s" % el]["s"]["order_%s" % AbinsConstants.FUNDAMENTALS] = temp

        return correct_data

    def _check_data(self, good_data=None, data=None):

        for el in range(len(good_data)):

            good_temp = good_data["atom_%s" % el]["s"]["order_%s" % AbinsConstants.FUNDAMENTALS]
            data_temp = data["atom_%s" % el]["s"]["order_%s" % AbinsConstants.FUNDAMENTALS]
            self.assertEqual(True, np.allclose(good_temp, data_temp))

            good_temp = good_data["atom_%s" % el]["frequencies"]["order_%s" % AbinsConstants.FUNDAMENTALS]
            data_temp = data["atom_%s" % el]["frequencies"]["order_%s" % AbinsConstants.FUNDAMENTALS]
            self.assertEqual(True, np.allclose(good_temp, data_temp))

            self.assertEqual(good_data["atom_%s" % el]["sort"],
                             data["atom_%s" % el]["sort"])
            self.assertEqual(good_data["atom_%s" % el]["symbol"],
                             data["atom_%s" % el]["symbol"])


if __name__ == '__main__':
    unittest.main()
