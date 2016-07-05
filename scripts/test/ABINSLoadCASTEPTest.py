import unittest
from mantid.simpleapi import *
from os import path
import simplejson as json
import numpy as np

from AbinsModules import IOmodule
from AbinsModules import LoadCASTEP

class ABINSLoadCASTEPTest(unittest.TestCase):

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

        AbinsTypeData = _CASTEP_reader.readPhononFile()
        data["rearranged_data"] = AbinsTypeData.extract()
        data["attributes"] =_CASTEP_reader._attributes
        data["datasets"] = _CASTEP_reader._numpy_datasets
        data["structured_datasets"] = _CASTEP_reader._structured_datasets

        # 2) Test of LoadCASTEP public methods

        # test validData method
        self.assertEqual(True, _CASTEP_reader.validData())

        # test LoadData method
        _loaded_ABINS_data =  _CASTEP_reader.loadData()
        _loaded_data = _loaded_ABINS_data.extract()
        _loaded_k_data = _loaded_data["k_points_data"]
        k_data = data["rearranged_data"]["k_points_data"]
        num_k = len(k_data)

        for el in range(num_k):
            for item in k_data[el]:
                self.assertEqual(True,np.allclose(k_data[el][item],
                                                  _loaded_k_data[el][item])) # test is numpy arrays are equal

        _loaded_atoms_data = _loaded_data["atoms_data"]
        _num_atoms = len(_loaded_atoms_data)
        for num_atom in range(_num_atoms):

            _data_item = data["structured_datasets"]["atoms"][num_atom]
            _loaded_data_item = _loaded_atoms_data[num_atom]

            self.assertEqual(_data_item["sort"], _loaded_data_item["sort"])
            self.assertEqual(_data_item["atom"], _loaded_data_item["atom"])
            self.assertEqual(_data_item["mass"], _loaded_data_item["mass"])
            self.assertEqual(_data_item["symbol"], _loaded_data_item["symbol"])
            self.assertEqual(True, np.allclose(_data_item["fract_coord"], _loaded_data_item["fract_coord"]))

        return data


    def _check_data(self, core=None, name=None):

        # get calculated data
        filename = path.relpath(core + name + ".phonon")
        _data = self._read_DFT(filename=filename)

        # get correct data
        filename = path.relpath(core + name + "_data.txt")
        _correct_data = self._prepare_data(filename=filename)


        # check rearranged_data
        _correct_items = _correct_data["rearranged_data"]["k_points_data"]
        num_k = len(_correct_items)
        _items = _data["rearranged_data"]["k_points_data"]
        for k in range(num_k):
            _correct_item = _correct_items[k]
            _item =  _items[k]

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
        _correct_ions = _correct_data["structured_datasets"]["atoms"]
        _ions = _data["structured_datasets"]["atoms"]

        for item in range(len(_correct_ions)):

            self.assertEqual(_correct_ions[item]["sort"], _ions[item]["sort"])
            self.assertEqual(_correct_ions[item]["mass"], _ions[item]["mass"])
            self.assertEqual(_correct_ions[item]["symbol"], _ions[item]["symbol"])
            self.assertEqual(_correct_ions[item]["atom"], _ions[item]["atom"])
            self.assertEqual(True, np.allclose(np.array(_correct_ions[item]["fract_coord"]), _ions[item]["fract_coord"]))


if __name__ == '__main__':
    unittest.main()