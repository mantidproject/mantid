import unittest
from mantid.simpleapi import *
from os import path
import simplejson as json
import numpy as np

from AbinsModules import IOmodule
from AbinsModules import LoadCASTEP

class LoadCASTEPTest(unittest.TestCase):

    # simple tests
    def test_non_existing_file(self):
        with self.assertRaises(IOError):
            _bad_CASTEP_reader = LoadCASTEP("NonExistingFile.txt")
            _bad_CASTEP_reader.readPhononFile()

        with self.assertRaises(ValueError):
            poor_CASTEP_reader = LoadCASTEP(input_DFT_filename=1)

    #  *************************** USE CASES ********************************************
    _core = "../ExternalData/Testing/Data/UnitTest/"
    # ===================================================================================
    # | Use case: Gamma point calculation and sum correction enabled during calculations|
    # ===================================================================================

    _gamma_sum = "squaricn_sum"
    def test_Gamma_sum_correction(self):
         self._check_data(core=self._core, name=self._gamma_sum)

    # ===================================================================================
    # |     Use case: Gamma point calculation and no sum correction for Gamma point     |
    # ===================================================================================

    _gamma_no_sum = "squaricn_no_sum"
    def test_Gamma_no_sum_correction(self):
        self._check_data(core=self._core, name=self._gamma_no_sum)


    # ===================================================================================
    # |          Use case: more than one k-point and sum correction enabled             |
    # ===================================================================================
    _many_k_sum = "Si2-phonon"
    def test_sum_correction(self):
        self._check_data(core=self._core, name=self._many_k_sum)

    # ===================================================================================
    # |              Use case: more than one k-point without sum correction             |
    # ===================================================================================

    _many_k_no_sum = "Si2-sc"
    def test_no_sum_correction(self):
        self._check_data(core=self._core, name=self._many_k_no_sum)


    # # Helper functions
    def _prepare_data(self, filename=None):
        """Reads a corrects values from ASCII file."""
        correct_data = None
        with open(filename) as data_file:
            correct_data = json.loads(data_file.read().replace("\n"," ").
                                      replace("array","").
                                      replace("([","[").
                                      replace("])","]").
                                      replace(".,",".0,").
                                      replace(".]",".0]").
                                      replace(". ",".0").
                                      replace("'",'"'))

        return correct_data


    def _read_DFT(self, filename=None):
        """
        Reads data from .phonon file and test some public methods
        @param filename:
        @return:
        """
        # 1) Read data
        data={}

        _CASTEP_reader = LoadCASTEP(input_DFT_filename=filename)

        data["rearranged_data"] = _CASTEP_reader.readPhononFile() 
        data["attributes"] =_CASTEP_reader._attributes
        data["datasets"] = _CASTEP_reader._numpy_datasets
        data["structured_datasets"] = _CASTEP_reader._structured_datasets

        # 2) Test of LoadCASTEP public methods

        # test validData method
        self.assertEqual(True, _CASTEP_reader.validData())

        # test LoadData method
        _loaded_data =  _CASTEP_reader.loadData()
        num_k = len(data["rearranged_data"])
        for el in range(num_k):
            for item in data["rearranged_data"][el]:
                self.assertEqual(True,np.allclose(data["rearranged_data"][el][item],
                                                  _loaded_data[el][item])) # test is numpy arrays are equal

        return data


    def _check_data(self, core=None, name=None):

        # get calculated data
        filename = path.relpath(core + name + ".phonon")
        _data = self._read_DFT(filename=filename)

        # get correct data
        filename = path.relpath(core + name + "_data.txt")
        _correct_data = self._prepare_data(filename=filename)


        # check rearranged_data
        num_k = len(_correct_data["rearranged_data"])
        for k in range(num_k):
            _correct_item = _correct_data["rearranged_data"][k]
            _item =  _data["rearranged_data"][k]

            self.assertEqual(True, np.allclose(np.array(_correct_item["frequencies"]), _item["frequencies"]))
            self.assertEqual(True, np.allclose(np.array(_correct_item["atomic_displacements"]), _item["atomic_displacements"]))
            self.assertEqual(True, np.allclose(np.array(_correct_item["value"]), _item["value"]))
            self.assertEqual(True, np.allclose(np.array(_correct_item["weight"]), _item["weight"]))


        # check attributes
        self.assertEqual(_correct_data["attributes"]["hash"], _data["attributes"]["hash"])
        self.assertEqual(_correct_data["attributes"]["DFT_program"], _data["attributes"]["DFT_program"] )


        # check datasets
        for item in _correct_data["datasets"]:
            self.assertEqual(True, np.allclose(np.array(_correct_data["datasets"][item]),_data["datasets"][item]))

        # check structured_data
        _correct_ions = _correct_data["structured_datasets"]["ions"]
        _ions = _data["structured_datasets"]["ions"]

        for item in range(len(_correct_ions)):

            self.assertEqual(_correct_ions[item]["sort"], _ions[item]["sort"])
            self.assertEqual(_correct_ions[item]["symbol"], _ions[item]["symbol"])
            self.assertEqual(_correct_ions[item]["atom"], _ions[item]["atom"])
            self.assertEqual(True, np.allclose(np.array(_correct_ions[item]["fract_coord"]), _ions[item]["fract_coord"]))


if __name__ == '__main__':
    unittest.main()