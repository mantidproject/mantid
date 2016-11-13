import unittest
from mantid.simpleapi import *
from os import path
import numpy as np
from AbinsModules import AbinsConstants

try:
    import simplejson as json
except ImportError:
    logger.warning("Failure of CalculateSPowderTest because simplejson is unavailable.")
    exit(1)

try:
    import h5py
except ImportError:
    logger.warning("Failure of CalculateSPowderTest because h5py is unavailable.")
    exit(1)


from AbinsModules import CalculateS, LoadCASTEP


# noinspection PyPep8Naming
class ABINSCalculateSPowderTest(unittest.TestCase):
    """
    Test of  CalculateS for the Powder scenario.
    """

    _temperature = 10  # 10 K,  temperature for the benchmark
    _sample_form = "Powder"
    _instrument_name = "TOSCA"
    _order_event = AbinsConstants.fundamentals

    # data
    core = "../ExternalData/Testing/Data/UnitTest/"

    squaricn = "squaricn_sum_CalculateSPowder"
    Si2 = "Si2-sc_CalculateSPowder"

    Squaricn_path = path.relpath(core + squaricn)
    Si2_path = path.relpath(core + Si2)

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

        _castep_reader = LoadCASTEP(input_DFT_filename=filename)
        _good_data = _castep_reader.readPhononFile()

        # wrong filename
        with self.assertRaises(ValueError):
            poor_S = CalculateS(filename=1, temperature=self._temperature, sample_form=self._sample_form,
                                abins_data=_good_data, instrument_name=self._instrument_name,
                                quantum_order_num=self._order_event)

        # wrong temperature
        with self.assertRaises(ValueError):
            poor_S = CalculateS(filename=filename, temperature=-1, sample_form=self._sample_form, abins_data=_good_data,
                                instrument_name=self._instrument_name, quantum_order_num=self._order_event)

        # wrong sample
        with self.assertRaises(ValueError):
            poor_S = CalculateS(filename=filename, temperature=self._temperature, sample_form="SOLID",
                                abins_data=_good_data, instrument_name=self._instrument_name,
                                quantum_order_num=self._order_event)

        # wrong abins data: content of abins data instead of object abins_data
        with self.assertRaises(ValueError):
            poor_S = CalculateS(filename=filename, temperature=self._temperature, sample_form=self._sample_form,
                                abins_data=_good_data.extract(), instrument_name=self._instrument_name,
                                quantum_order_num=self._order_event)

        # wrong instrument
        with self.assertRaises(ValueError):
            poor_S = CalculateS(filename=filename, temperature=self._temperature, sample_form=self._sample_form,
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
        calculated_data = _good_tester.getData()

        self._check_data(good_data=_good_data["S"], data=calculated_data.extract())

        # check if loading powder data is correct
        new_tester = CalculateS(filename=name + ".phonon", temperature=self._temperature, sample_form=self._sample_form,
                                abins_data=_good_data["DFT"], instrument_name=self._instrument_name,
                                quantum_order_num=self._order_event)
        loaded_data = new_tester.loadData()

        self._check_data(good_data=_good_data["S"], data=loaded_data.extract())

    def _get_good_data(self, filename=None):

        _CASTEP_reader = LoadCASTEP(input_DFT_filename=filename + ".phonon")
        _S = self._prepare_data(filename=filename + "_S.txt")

        return {"DFT": _CASTEP_reader.readPhononFile(), "S": _S}

    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        correct_data = None
        with open(filename) as data_file:
            # noinspection PyPep8
            correct_data = json.loads(data_file.read().replace("\\n", " ").
                                      replace("array",    "").
                                      replace("([",  "[").
                                      replace("])",  "]").
                                      replace("'",  '"').
                                      replace("0. ", "0.0"))

        for el in range(len(correct_data)):

            temp = np.asarray(correct_data["atom_%s" % el]["frequencies"]["order_%s" % AbinsConstants.fundamentals])
            correct_data["atom_%s" % el]["frequencies"]["order_%s" % AbinsConstants.fundamentals] = temp

            temp = np.asarray(correct_data["atom_%s" % el]["s"]["order_%s" % AbinsConstants.fundamentals])
            correct_data["atom_%s" % el]["s"]["order_%s" % AbinsConstants.fundamentals] = temp

        return correct_data

    def _check_data(self, good_data=None, data=None):

        for el in range(len(good_data)):

            good_temp = good_data["atom_%s" % el]["s"]["order_%s" % AbinsConstants.fundamentals]
            data_temp = data["atom_%s" % el]["s"]["order_%s" % AbinsConstants.fundamentals]
            self.assertEqual(True, np.allclose(good_temp, data_temp))

            good_temp = good_data["atom_%s" % el]["frequencies"]["order_%s" % AbinsConstants.fundamentals]
            data_temp = data["atom_%s" % el]["frequencies"]["order_%s" % AbinsConstants.fundamentals]
            self.assertEqual(True, np.allclose(good_temp, data_temp))

            self.assertEqual(good_data["atom_%s" % el]["sort"],
                             data["atom_%s" % el]["sort"])
            self.assertEqual(good_data["atom_%s" % el]["symbol"],
                             data["atom_%s" % el]["symbol"])


if __name__ == '__main__':
    unittest.main()
