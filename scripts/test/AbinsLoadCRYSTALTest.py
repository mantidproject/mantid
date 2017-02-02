from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import numpy as np
import json
from AbinsModules import LoadCRYSTAL, AbinsTestHelpers


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsLoadCRYSTALTest because Python is too old.")
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
class AbinsLoadCRYSTALTest(unittest.TestCase):

    # simple tests
    def test_non_existing_file(self):
        with self.assertRaises(IOError):
            bad_crystal_reader = LoadCRYSTAL(input_dft_filename="NonExistingFile.txt")
            bad_crystal_reader.read_phonon_file()

        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_crystal_reader = LoadCRYSTAL(input_dft_filename=1)

    def tearDown(self):
        AbinsTestHelpers.remove_output_files(list_of_names=["LoadCRYSTAL"])

        #  *************************** USE CASES ********************************************
    # ===================================================================================
    # | Use case: Gamma point calculation for CRYSTAL                                    |
    # ===================================================================================
    _gamma_crystal = "crystalB3LYP_LoadCRYSTAL"
    _set_crystal = "crystal_set_key_LoadCRYSTAL"

    def test_gamma_crystal(self):
        self._check(name=self._gamma_crystal)
        self._check(name=self._set_crystal)

    # TODO: prepare test for molecule

    def _check(self, name=None):
        # get calculated data
        data = self._read_dft(filename=name)
        shape = data["datasets"]["k_points_data"]["atomic_displacements"].shape

        # get correct data
        correct_data = self._prepare_data(filename=name, shape=shape)

        # check read data
        self._check_reader_data(correct_data=correct_data, data=data, filename=name)

        # check loaded data
        self._check_loader_data(correct_data=correct_data, input_dft_filename=name)

    def _read_dft(self, filename=None):
        """
        Reads data from .phonon file.
        @param filename: name of file with phonon data (name + phonon)
        @return: phonon data
        """
        # 1) Read data
        crystal_reader = LoadCRYSTAL(input_dft_filename=AbinsTestHelpers.find_file(filename=filename + ".out"))

        data = self._get_reader_data(crystal_reader=crystal_reader)

        # test validData method
        self.assertEqual(True, crystal_reader._clerk._valid_hash())

        return data

    # noinspection PyMethodMayBeStatic
    def _prepare_data(self, filename=None, shape=None):
        """Reads a correct values from ASCII file."""

        with open(AbinsTestHelpers.find_file(filename + "_data.txt")) as data_file:
            correct_data = json.loads(data_file.read().replace("\n", " "))

        array = np.genfromtxt(AbinsTestHelpers.find_file(filename + "_atomic_displacements_data.txt"), dtype=complex)
        array.reshape(shape)
        k = len(correct_data["datasets"]["k_points_data"]["weights"])
        atoms = len(correct_data["datasets"]["atoms_data"])
        array = array.reshape((k, atoms, atoms * 3, 3))
        correct_data["datasets"]["k_points_data"]["weights"] = \
            np.asarray(correct_data["datasets"]["k_points_data"]["weights"])

        correct_data["datasets"]["k_points_data"]["frequencies"] = \
            np.asarray(correct_data["datasets"]["k_points_data"]["frequencies"])

        correct_data["datasets"]["k_points_data"].update({"atomic_displacements": array})
        correct_data["datasets"].update({"atomic_displacements": array})

        return correct_data

    def _check_reader_data(self, correct_data=None, data=None, filename=None):

        # check data
        correct_k_points = correct_data["datasets"]["k_points_data"]
        items = data["datasets"]["k_points_data"]
        # atol = 0.2 work around for accuracy bug on Windows.
        self.assertEqual(True, np.allclose(correct_k_points["frequencies"], items["frequencies"], atol=0.2))
        self.assertEqual(True, np.allclose(correct_k_points["atomic_displacements"], items["atomic_displacements"]))
        self.assertEqual(True, np.allclose(correct_k_points["k_vectors"], items["k_vectors"]))
        self.assertEqual(True, np.allclose(correct_k_points["weights"], items["weights"]))

        correct_atoms = correct_data["datasets"]["atoms_data"]
        atoms = data["datasets"]["atoms_data"]
        for item in range(len(correct_atoms)):

            self.assertEqual(correct_atoms["atom_%s" % item]["sort"], atoms["atom_%s" % item]["sort"])
            self.assertAlmostEqual(correct_atoms["atom_%s" % item]["mass"], atoms["atom_%s" % item]["mass"],
                                   delta=0.00001)  # delta in amu units
            self.assertEqual(correct_atoms["atom_%s" % item]["symbol"], atoms["atom_%s" % item]["symbol"])
            self.assertEqual(True, np.allclose(np.array(correct_atoms["atom_%s" % item]["fract_coord"]),
                                               atoms["atom_%s" % item]["fract_coord"]))

        # check attributes
        self.assertEqual(correct_data["attributes"]["advanced_parameters"], data["attributes"]["advanced_parameters"])
        self.assertEqual(correct_data["attributes"]["hash"], data["attributes"]["hash"])
        self.assertEqual(correct_data["attributes"]["DFT_program"], data["attributes"]["DFT_program"])
        self.assertEqual(AbinsTestHelpers.find_file(filename + ".out"), data["attributes"]["filename"])

        # check datasets
        self.assertEqual(True, np.allclose(correct_data["datasets"]["unit_cell"], data["datasets"]["unit_cell"]))

        # atol = 0.2 work around for accuracy bug on Windows.
        self.assertEqual(True, np.allclose(np.array(correct_data["datasets"]["k_points_data"]["frequencies"]),
                                           data["datasets"]["k_points_data"]["frequencies"], atol=0.2))
        items = ["weights", "k_vectors"]
        for item in items:
            self.assertEqual(True, np.allclose(np.array(correct_data["datasets"]["k_points_data"][item]),
                                               data["datasets"]["k_points_data"][item]))

        correct_atoms = correct_data["datasets"]["atoms_data"]
        atoms = data["datasets"]["atoms_data"]

        for item in range(len(correct_atoms)):

            self.assertEqual(correct_atoms["atom_%s" % item]["sort"], atoms["atom_%s" % item]["sort"])
            self.assertAlmostEqual(correct_atoms["atom_%s" % item]["mass"], atoms["atom_%s" % item]["mass"],
                                   delta=0.00001)
            self.assertEqual(correct_atoms["atom_%s" % item]["symbol"], atoms["atom_%s" % item]["symbol"])
            self.assertEqual(True, np.allclose(np.array(correct_atoms["atom_%s" % item]["fract_coord"]),
                                               atoms["atom_%s" % item]["fract_coord"]))

    def _check_loader_data(self, correct_data=None, input_dft_filename=None):

        loader = LoadCRYSTAL(input_dft_filename=AbinsTestHelpers.find_file(input_dft_filename + ".out"))
        loaded_data = loader.load_formatted_data().extract()

        # k points
        correct_items = correct_data["datasets"]["k_points_data"]
        items = loaded_data["k_points_data"]

        # atol = 0.2 work around for accuracy bug on Windows.
        self.assertEqual(True, np.allclose(correct_items["frequencies"], items["frequencies"], atol=0.2))
        self.assertEqual(True, np.allclose(correct_items["atomic_displacements"], items["atomic_displacements"]))
        self.assertEqual(True, np.allclose(correct_items["k_vectors"], items["k_vectors"]))
        self.assertEqual(True, np.allclose(correct_items["weights"], items["weights"]))

        # atoms
        correct_atoms = correct_data["datasets"]["atoms_data"]
        atoms = loaded_data["atoms_data"]

        for item in range(len(correct_atoms)):

            self.assertEqual(correct_atoms["atom_%s" % item]["sort"], atoms["atom_%s" % item]["sort"])
            self.assertAlmostEqual(correct_atoms["atom_%s" % item]["mass"], atoms["atom_%s" % item]["mass"],
                                   delta=0.00001)
            self.assertEqual(correct_atoms["atom_%s" % item]["symbol"], atoms["atom_%s" % item]["symbol"])
            self.assertEqual(True, np.allclose(np.array(correct_atoms["atom_%s" % item]["fract_coord"]),
                                               atoms["atom_%s" % item]["fract_coord"]))

    # noinspection PyMethodMayBeStatic
    def _get_reader_data(self, crystal_reader=None):
        abins_type_data = crystal_reader.read_phonon_file()
        data = {"datasets": abins_type_data.extract(),
                "attributes": crystal_reader._clerk._attributes
                }
        data["datasets"].update({"unit_cell": crystal_reader._clerk._data["unit_cell"]})
        return data


if __name__ == '__main__':
    unittest.main()
