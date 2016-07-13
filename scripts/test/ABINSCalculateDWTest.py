import unittest
from mantid.simpleapi import *
from os import path
import simplejson as json
import numpy as np

# ABINS modules
from AbinsModules import CalculateDW
from AbinsModules import LoadCASTEP


class ABINSCalculateDWTest(unittest.TestCase):
    _core = "../ExternalData/Testing/Data/UnitTest/"
    _dft_file = _core + "Si2-sc.phonon"
    _temperature = 10 # 10 K
    _good_DW = np.array([[[  4.69472745e-02,  -3.41878473e-03,    -3.41878473e-03], # comment: This values are calculated only from 4 k-points
                          [ -3.41878473e-03,   7.99600176e-02,  -9.65204800e-03],
                          [ -3.41878473e-03,  -9.65204800e-03,   4.96517869e+04]],
                         [[  4.69472745e-02,  -3.41878473e-03,  -3.41878473e-03],
                          [ -3.41878473e-03,   7.99600176e-02,  -9.65204800e-03],
                          [ -3.41878473e-03,  -9.65204800e-03,   4.96517869e+04]]])

    # simple tests
    def test_wrong_input(self):

        _good_data = self._get_good_data()

        # wrong filename
        with self.assertRaises(ValueError):
            _poor_tester = CalculateDW(filename=1, temperature=self._temperature, abins_data=_good_data)

        # wrong temperature
        with self.assertRaises(ValueError):
            _poor_tester = CalculateDW(filename=self._dft_file, temperature=-10, abins_data=_good_data)

        # data from object of type AtomsData instead of object of type AbinsData
        bad_data = _good_data.extract()["atoms_data"]
        with self.assertRaises(ValueError):
            _poor_tester = CalculateDW(filename=self._dft_file, temperature=self._temperature, abins_data=bad_data)


    def test_good_case(self):

        # calculation of DW
        _good_data = self._get_good_data()
        _good_tester = CalculateDW(filename=self._dft_file, temperature=self._temperature, abins_data=_good_data)
        calculated_data = _good_tester.getDW()

        # check if evaluated DW are correct
        self.assertEqual(True, np.allclose(self._good_DW, calculated_data.extract()))

        # check loading DW
        new_tester =  CalculateDW(filename=self._dft_file, temperature=self._temperature, abins_data=_good_data)
        loaded_data = new_tester.loadData()
        self.assertEqual(True, np.allclose(calculated_data.extract(), loaded_data.extract()))

    # helper functions
    def _get_good_data(self):
        _CASTEP_reader = LoadCASTEP(input_DFT_filename=self._dft_file)
        return _CASTEP_reader.readPhononFile()



if __name__ == '__main__':
    unittest.main()