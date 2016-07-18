import unittest
from mantid.simpleapi import *
from os import path
import simplejson as json
import numpy as np

from AbinsModules import CalculateCrystal, LoadCASTEP


class ABINSCalculateCrystalTest(unittest.TestCase):

    _core = "../ExternalData/Testing/Data/UnitTest/" # path to files
    _temperature = 10 # 10 K,  temperature for the benchmark

    # data
    # Use case: one k-point
    C6H6 = _core + "benzene"

    #  Use case: many k-points
    Si2 = _core + "Si2-sc"

    #     test input
    def test_wrong_input(self):

        filename = self.Si2 + ".phonon"

        _castep_reader =  LoadCASTEP(input_DFT_filename=filename)
        _good_data = _castep_reader.readPhononFile()

        # wrong filename
        with self.assertRaises(ValueError):
            _poor_tester = CalculatePowder(filename=1, temperature=self._temperature, abins_data=_good_data)

        # wrong temperature
        with self.assertRaises(ValueError):
            _poor_tester = CalculatePowder(filename=filename, temperature=-10, abins_data=_good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = _good_data.extract()["atoms_data"]
        with self.assertRaises(ValueError):
            _poor_tester = CalculatePowder(filename=filename, temperature=self._temperature, abins_data=bad_data)