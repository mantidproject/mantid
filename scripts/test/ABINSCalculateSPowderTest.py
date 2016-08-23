import unittest
from mantid.simpleapi import *
from os import path
import numpy as np


try:
    import simplejson as json
except ImportError:
    logger.warning("Failure of CalculateSPowderTest because simplejson is unavailable.")
    exit(1)

try:
    import h5py
except ImportError:
    logger.warning("Failure of CalculateSPowderTest because h5py is unavailable.")
    exir(1)


from AbinsModules import CalculateS, LoadCASTEP, AbinsParameters

class ABINSCalculateSPowderTest(unittest.TestCase):
    """
    Test of  CalculateS for the Powder scenario.
    """

    _core = "../ExternalData/Testing/Data/UnitTest/" # path to files
    _temperature = 10 # 10 K,  temperature for the benchmark
    _sample_form = "Powder"
    _instrument_name = "TOSCA"
    # data
    # Use case: one k-point
    C6H6 = path.relpath(_core + "benzene")

    #  Use case: many k-points
    Si2 = path.relpath(_core + "Si2-sc")

    #     test input
    def test_wrong_input(self):
        filename = self.Si2 + ".phonon"

        _castep_reader =  LoadCASTEP(input_DFT_filename=filename)
        _good_data = _castep_reader.readPhononFile()

        # wrong filename
        with self.assertRaises(ValueError):
            poor_S = CalculateS(filename=1, temperature=self._temperature, sample_form=self._sample_form,
                                abins_data=_good_data, instrument_name=self._instrument_name)

        # wrong temperature
        with self.assertRaises(ValueError):
            poor_S = CalculateS(filename=filename, temperature=-1, sample_form=self._sample_form,
                                abins_data=_good_data, instrument_name=self._instrument_name)

        # wrong sample
        with self.assertRaises(ValueError):
            poor_S = CalculateS(filename=filename, temperature=self._temperature, sample_form="SOLID",
                                abins_data=_good_data, instrument_name=self._instrument_name)

        # wrong abins data: content of abins data instead of object abins_data
        with self.assertRaises(ValueError):
            poor_S = CalculateS(filename=filename, temperature=self._temperature, sample_form=self._sample_form,
                                abins_data=_good_data.extract(), instrument_name=self._instrument_name)

        # wrong instrument
        with self.assertRaises(ValueError):
            poor_S = CalculateS(filename=filename, temperature=self._temperature, sample_form=self._sample_form,
                                abins_data=_good_data.extract(), instrument_name=self._instrument_name)


    #       main test
    def test_good_case(self):
        AbinsParameters._pkt_per_peak = 50 # lower internal ABINS parameter for the purpose of this benchmark
        self._good_case(name=self.C6H6)
        self._good_case(name=self.Si2)


    # helper functions
    def _good_case(self, name=None):
        # calculation of powder data
        _good_data = self._get_good_data(filename=name)
        _good_tester = CalculateS(filename=name + ".phonon", temperature=self._temperature, sample_form=self._sample_form,
                                  abins_data=_good_data["DFT"], instrument_name=self._instrument_name)
        calculated_data = _good_tester.getData()

        self._check_data(good_data=_good_data["S"], data=calculated_data.extract())

        # check if loading powder data is correct
        new_tester =  CalculateS(filename=name + ".phonon", temperature=self._temperature, sample_form=self._sample_form,
                                 abins_data=_good_data["DFT"], instrument_name=self._instrument_name)
        loaded_data = new_tester.loadData()

        self._check_data(good_data=_good_data["S"], data=loaded_data.extract())


    def _get_good_data(self, filename=None):

        _CASTEP_reader = LoadCASTEP(input_DFT_filename=filename + ".phonon")
        _S = self._prepare_data(filename=filename + "_S.txt")

        return {"DFT":_CASTEP_reader.readPhononFile(), "S": _S}


    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        correct_data = None
        with open(filename) as data_file:
            correct_data = json.loads(data_file.read().replace("\\n"    , " ").
                                      replace("array" ,   "").
                                      replace("(["    ,  "[").
                                      replace("])"    ,  "]").
                                      replace("'"     ,  '"').
                                      replace("0. "   ,"0.0"))

        for key in correct_data.keys():
            correct_data[key] = np.asarray(correct_data[key])

        return correct_data

    def _check_data(self, good_data=None, data=None):

        self.assertEqual(True, np.allclose(good_data["convoluted_frequencies"], data["convoluted_frequencies"]))

        for el  in range(len(good_data["atoms_data"])):
            self.assertEqual(True, np.allclose(good_data["atoms_data"][el]["value"], data["atoms_data"][el]["value"]))
            self.assertEqual(good_data["atoms_data"][el]["sort"], data["atoms_data"][el]["sort"])
            self.assertEqual(good_data["atoms_data"][el]["symbol"], data["atoms_data"][el]["symbol"])


if __name__ == '__main__':
    unittest.main()