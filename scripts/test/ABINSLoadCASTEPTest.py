import unittest
from mantid.simpleapi import *
import os
import numpy as np
import json
from AbinsModules import LoadCASTEP, AbinsConstants


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsConstants.old_python()
    if is_python_old:
        logger.warning("Skipping ABINSLoadCASTEPTest because Python is too old.")
    return is_python_old


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


@skip_if(old_python)
class ABINSLoadCASTEPTest(unittest.TestCase):

    # simple tests
    def test_non_existing_file(self):
        with self.assertRaises(IOError):
            _bad_castep_reader = LoadCASTEP(input_dft_filename="NonExistingFile.txt")
            _bad_castep_reader.read_phonon_file()

        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_castep_reader = LoadCASTEP(input_dft_filename=1)

    #  *************************** USE CASES ********************************************
    _core = os.path.abspath("../ExternalData/Testing/Data/UnitTest/")  # hardcoded directory with testing files

# ===================================================================================
    # | Use case: Gamma point calculation and sum correction enabled during calculations|
    # ===================================================================================

    _gamma_sum = "squaricn_sum_LoadCASTEP"

    def test_Gamma_sum_correction(self):
        self._check(core=self._core, name=self._gamma_sum)

    # ===================================================================================
    # |     Use case: Gamma point calculation and no sum correction for Gamma point     |
    # ===================================================================================

    _gamma_no_sum = "squaricn_no_sum_LoadCASTEP"

    def test_Gamma_no_sum_correction(self):
        self._check(core=self._core, name=self._gamma_no_sum)
    #
    #
    # ===================================================================================
    # | Use case: more than one k-point and sum correction       |
    # ===================================================================================
    _many_k_sum = "Si2-phonon_LoadCASTEP"

    def test_sum_correction_single_crystal(self):
        self._check(core=self._core, name=self._many_k_sum)

    # ===================================================================================
    # |   Use case: more than one k-point without sum correction                        |
    # ===================================================================================
    #
    _many_k_no_sum = "Si2-sc_LoadCASTEP"

    def test_no_sum_correction_single_crystal(self):
        self._check(core=self._core, name=self._many_k_no_sum)

    # Helper functions

    def _check(self, core=None, name=None):

        cwd = os.getcwd()

        # get calculated data
        filename_unix = os.path.join(core, name + ".phonon")
        input_filename = os.path.abspath(filename_unix)
        input_filename = os.path.relpath(input_filename, cwd)

        data = self._read_DFT(filename=input_filename)

        # get correct data
        filename = os.path.abspath(os.path.join(core, name))
        filename = os.path.relpath(filename, cwd)
        correct_data = self._prepare_data(filename=filename)

        # check read data
        self._check_reader_data(correct_data=correct_data, data=data)

        # check loaded data
        self._check_loader_data(correct_data=correct_data, input_dft_filename=input_filename)

    def _read_DFT(self, filename=None):
        """
        Reads data from .phonon file.
        @param filename:
        @return:
        """
        # 1) Read data
        castep_reader = LoadCASTEP(input_dft_filename=filename)

        data = self._get_reader_data(castep_reader=castep_reader)

        # test validData method
        self.assertEqual(True, castep_reader._valid_hash())

        return data

    # noinspection PyMethodMayBeStatic
    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""

        with open(filename + "_data.txt") as data_file:
            correct_data = json.loads(data_file.read().replace("\n", " "))

        array = np.loadtxt(filename + "_atomic_displacements_data.txt").view(complex).reshape(-1)
        k = len(correct_data["datasets"]["k_points_data"]["weights"])
        atoms = len(correct_data["datasets"]["atoms_data"])
        array = array.reshape(k, atoms, atoms * 3, 3)

        correct_data["datasets"]["k_points_data"]["weights"] = \
            np.asarray(correct_data["datasets"]["k_points_data"]["weights"])

        correct_data["datasets"]["k_points_data"]["frequencies"] = \
            np.asarray(correct_data["datasets"]["k_points_data"]["frequencies"])

        correct_data["datasets"]["k_points_data"].update({"atomic_displacements": array})
        correct_data["datasets"].update({"atomic_displacements": array})

        return correct_data

    def _check_reader_data(self, correct_data=None, data=None):

        # check data
        correct_k_points = correct_data["datasets"]["k_points_data"]
        items = data["datasets"]["k_points_data"]
        self.assertEqual(True, np.allclose(correct_k_points["frequencies"], items["frequencies"]))
        self.assertEqual(True, np.allclose(correct_k_points["atomic_displacements"], items["atomic_displacements"]))
        self.assertEqual(True, np.allclose(correct_k_points["k_vectors"], items["k_vectors"]))
        self.assertEqual(True, np.allclose(correct_k_points["weights"], items["weights"]))

        correct_atoms = correct_data["datasets"]["atoms_data"]
        atoms = data["datasets"]["atoms_data"]
        for item in range(len(correct_atoms)):

            self.assertEqual(correct_atoms[item]["sort"], atoms[item]["sort"])
            self.assertAlmostEqual(correct_atoms[item]["mass"], atoms[item]["mass"], delta=2)  # delta in Hartree units
            self.assertEqual(correct_atoms[item]["symbol"], atoms[item]["symbol"])
            self.assertEqual(correct_atoms[item]["atom"], atoms[item]["atom"])
            self.assertEqual(True, np.allclose(np.array(correct_atoms[item]["fract_coord"]),
                                               atoms[item]["fract_coord"]))

        # check attributes
        self.assertEqual(correct_data["attributes"]["advanced_parameters"], data["attributes"]["advanced_parameters"])
        self.assertEqual(correct_data["attributes"]["hash"], data["attributes"]["hash"])
        self.assertEqual(correct_data["attributes"]["DFT_program"], data["attributes"]["DFT_program"])
        self.assertEqual(correct_data["attributes"]["filename"], data["attributes"]["filename"])

        # check datasets
        self.assertEqual(True, np.allclose(correct_data["datasets"]["unit_cell"], data["datasets"]["unit_cell"]))

        items = ["weights", "frequencies", "k_vectors"]
        for item in items:
            self.assertEqual(True, np.allclose(np.array(correct_data["datasets"]["k_points_data"][item]),
                                               data["datasets"]["k_points_data"][item]))

        correct_atoms = correct_data["datasets"]["atoms_data"]
        atoms = data["datasets"]["atoms_data"]

        for item in range(len(correct_atoms)):

            self.assertEqual(correct_atoms[item]["sort"], atoms[item]["sort"])
            self.assertAlmostEqual(correct_atoms[item]["mass"], atoms[item]["mass"], delta=2)
            self.assertEqual(correct_atoms[item]["symbol"], atoms[item]["symbol"])
            self.assertEqual(correct_atoms[item]["atom"], atoms[item]["atom"])
            self.assertEqual(True, np.allclose(np.array(correct_atoms[item]["fract_coord"]),
                                               atoms[item]["fract_coord"]))

    def _check_loader_data(self, correct_data=None, input_dft_filename=None):

        loader = LoadCASTEP(input_dft_filename=input_dft_filename)
        loaded_data = loader.load_data().extract()

        # k points
        correct_items = correct_data["datasets"]["k_points_data"]
        items = loaded_data["k_points_data"]

        self.assertEqual(True, np.allclose(correct_items["frequencies"], items["frequencies"]))
        self.assertEqual(True, np.allclose(correct_items["atomic_displacements"], items["atomic_displacements"]))
        self.assertEqual(True, np.allclose(correct_items["k_vectors"], items["k_vectors"]))
        self.assertEqual(True, np.allclose(correct_items["weights"], items["weights"]))

        # atoms
        correct_atoms = correct_data["datasets"]["atoms_data"]
        atoms = loaded_data["atoms_data"]

        for item in range(len(correct_atoms)):

            self.assertEqual(correct_atoms[item]["sort"], atoms[item]["sort"])
            self.assertAlmostEqual(correct_atoms[item]["mass"], atoms[item]["mass"], delta=2)
            self.assertEqual(correct_atoms[item]["symbol"], atoms[item]["symbol"])
            self.assertEqual(correct_atoms[item]["atom"], atoms[item]["atom"])
            self.assertEqual(True, np.allclose(np.array(correct_atoms[item]["fract_coord"]),
                                               atoms[item]["fract_coord"]))

    # noinspection PyMethodMayBeStatic
    def _get_reader_data(self, castep_reader=None):
        abins_type_data = castep_reader.read_phonon_file()
        data = {"datasets": abins_type_data.extract(),
                "attributes": castep_reader._attributes,
                }
        data["datasets"].update({"unit_cell": castep_reader._data["unit_cell"]})
        return data

if __name__ == '__main__':
    unittest.main()
