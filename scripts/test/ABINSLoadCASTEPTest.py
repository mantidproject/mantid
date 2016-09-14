import unittest
from mantid.simpleapi import *
from os import path
import numpy as np


try:
    import simplejson as json
except ImportError:
    logger.warning("Failure of LoadCASTEPTest because simplejson is unavailable.")
    exit(1)

try:
    import scipy
except ImportError:
    logger.warning("Failure of LoadCASTEPTest because scipy is unavailable.")
    exit(1)

try:
    import h5py
except ImportError:
    logger.warning("Failure of LoadCASTEPTest because h5py is unavailable.")
    exit(1)

from AbinsModules import LoadCASTEP, AbinsParameters



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
         self._check(core=self._core, name=self._gamma_sum)

    # ===================================================================================
    # |     Use case: Gamma point calculation and no sum correction for Gamma point     |
    # ===================================================================================

    _gamma_no_sum = "squaricn_no_sum"
    def test_Gamma_no_sum_correction(self):
        self._check(core=self._core, name=self._gamma_no_sum)


    # ===================================================================================
    # |          Use case: more than one k-point and sum correction enabled             |
    # ===================================================================================
    _many_k_sum = "Si2-phonon"
    def test_sum_correction(self):
        self._check(core=self._core, name=self._many_k_sum)

    # ===================================================================================
    # |              Use case: more than one k-point without sum correction             |
    # ===================================================================================

    _many_k_no_sum = "Si2-sc"
    def test_no_sum_correction(self):
        self._check(core=self._core, name=self._many_k_no_sum)


    # # Helper functions

    def _check(self, core=None, name=None):

        # get calculated data
        input_filename = path.relpath(core + name + ".phonon")
        _data = self._read_DFT(filename=input_filename)

        # get correct data
        filename = path.relpath(core + name)
        _correct_data = self._prepare_data(filename=filename)

        # check read data
        self._check_reader_data(correct_data=_correct_data, data=_data)

        # check loaded data
        self._check_loader_data(correct_data=_correct_data, input_DFT_filename=input_filename)


    def _read_DFT(self, filename=None):
        """
        Reads data from .phonon file.
        @param filename:
        @return:
        """
        # 1) Read data
        data={}

        _CASTEP_reader = LoadCASTEP(input_DFT_filename=filename)

        data = self._get_reader_data(castep_reader=_CASTEP_reader)

        # test validData method
        self.assertEqual(True, _CASTEP_reader._valid_hash())

        return data


    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""
        correct_data = None
        with open(filename + "_data.txt") as data_file:
            correct_data = json.loads(data_file.read().replace("\n"," "))

        array = np.loadtxt(filename + "_atomic_displacements_data.txt").view(complex).reshape(-1)
        k = len(correct_data["rearranged_data"]["k_points_data"]["weights"])
        atoms = len(correct_data["rearranged_data"]["atoms_data"])

        array = array.reshape(k,atoms,atoms * 3,3)

        correct_data["rearranged_data"]["k_points_data"]["weights"] = np.asarray(correct_data["rearranged_data"]["k_points_data"]["weights"])
        correct_data["rearranged_data"]["k_points_data"]["frequencies"] = np.asarray(correct_data["rearranged_data"]["k_points_data"]["frequencies"]) * AbinsParameters.cm1_2_hartree
        correct_data["rearranged_data"]["k_points_data"].update({"atomic_displacements": array})
        correct_data["datasets"].update({"atomic_displacements": array})

        for atom in correct_data["rearranged_data"]["atoms_data"]:
            atom["mass"] = atom["mass"] * AbinsParameters.m_2_hartree
        for atom in correct_data["datasets"]["atoms"]:
            atom["mass"] = atom["mass"] * AbinsParameters.m_2_hartree



        return correct_data


    def _check_reader_data(self, correct_data=None, data=None):

        # check rearranged_data
        _correct_k_points = correct_data["rearranged_data"]["k_points_data"]
        _items = data["rearranged_data"]["k_points_data"]

        self.assertEqual(True, np.allclose(_correct_k_points["frequencies"], _items["frequencies"]))
        self.assertEqual(True, np.allclose(_correct_k_points["atomic_displacements"], _items["atomic_displacements"]))
        self.assertEqual(True, np.allclose(_correct_k_points["k_vectors"], _items["k_vectors"]))
        self.assertEqual(True, np.allclose(_correct_k_points["weights"], _items["weights"]))

        _correct_atoms = correct_data["rearranged_data"]["atoms_data"]
        _atoms = data["rearranged_data"]["atoms_data"]
        for item in range(len(_correct_atoms)):

            self.assertEqual(_correct_atoms[item]["sort"], _atoms[item]["sort"])
            self.assertEqual(_correct_atoms[item]["mass"], _atoms[item]["mass"])
            self.assertEqual(_correct_atoms[item]["symbol"], _atoms[item]["symbol"])
            self.assertEqual(_correct_atoms[item]["atom"], _atoms[item]["atom"])
            self.assertEqual(True, np.allclose(np.array(_correct_atoms[item]["fract_coord"]), _atoms[item]["fract_coord"]))

        # check attributes
        self.assertEqual(correct_data["attributes"]["hash"], data["attributes"]["hash"])
        self.assertEqual(correct_data["attributes"]["DFT_program"], data["attributes"]["DFT_program"])
        self.assertEqual(correct_data["attributes"]["filename"], data["attributes"]["filename"])


        # check datasets
        items = ["weights","k_vectors", "frequencies", "unit_cell" ]
        for item in items:
            self.assertEqual(True, np.allclose(np.array(correct_data["datasets"][item]), data["datasets"][item]))

        # check structured_data
        _correct_atoms = correct_data["datasets"]["atoms"]
        _atoms = data["datasets"]["atoms"]

        for item in range(len(_correct_atoms)):

            self.assertEqual(_correct_atoms[item]["sort"], _atoms[item]["sort"])
            self.assertEqual(_correct_atoms[item]["mass"], _atoms[item]["mass"])
            self.assertEqual(_correct_atoms[item]["symbol"], _atoms[item]["symbol"])
            self.assertEqual(_correct_atoms[item]["atom"], _atoms[item]["atom"])
            self.assertEqual(True, np.allclose(np.array(_correct_atoms[item]["fract_coord"]), _atoms[item]["fract_coord"]))


    def _check_loader_data(self, correct_data=None, input_DFT_filename=None):

        loader = LoadCASTEP(input_DFT_filename=input_DFT_filename)
        _loaded_data = loader.loadData().extract()


        # k points
        _correct_items = correct_data["rearranged_data"]["k_points_data"]
        num_k = len(_correct_items)
        _items = _loaded_data["k_points_data"]

        self.assertEqual(True, np.allclose(_correct_items["frequencies"], _items["frequencies"]))
        self.assertEqual(True, np.allclose(_correct_items["atomic_displacements"], _items["atomic_displacements"]))
        self.assertEqual(True, np.allclose(_correct_items["k_vectors"], _items["k_vectors"]))
        self.assertEqual(True, np.allclose(_correct_items["weights"], _items["weights"]))

        # atoms
        _correct_atoms = correct_data["datasets"]["atoms"]
        _atoms = _loaded_data["atoms_data"]

        for item in range(len(_correct_atoms)):

            self.assertEqual(_correct_atoms[item]["sort"], _atoms[item]["sort"])
            self.assertEqual(_correct_atoms[item]["mass"], _atoms[item]["mass"])
            self.assertEqual(_correct_atoms[item]["symbol"], _atoms[item]["symbol"])
            self.assertEqual(_correct_atoms[item]["atom"], _atoms[item]["atom"])
            self.assertEqual(True, np.allclose(np.array(_correct_atoms[item]["fract_coord"]), _atoms[item]["fract_coord"]))


    def _get_reader_data(self, castep_reader=None):
        abins_type_data = castep_reader.readPhononFile()
        data = {"rearranged_data": abins_type_data.extract(),
                "datasets": castep_reader._data,
                "attributes": castep_reader._attributes,
                }
        return data

if __name__ == '__main__':
    unittest.main()