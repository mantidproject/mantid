from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import numpy as np
import json
import AbinsModules


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsModules.AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsLoadCASTEPTest because Python is too old.")
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
class AbinsLoadCASTEPTest(unittest.TestCase):

    # simple tests
    def test_non_existing_file(self):
        with self.assertRaises(IOError):
            bad_castep_reader = AbinsModules.LoadCASTEP(input_dft_filename="NonExistingFile.txt")
            bad_castep_reader.read_phonon_file()

        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_castep_reader = AbinsModules.LoadCASTEP(input_dft_filename=1)

    def tearDown(self):
        AbinsModules.AbinsTestHelpers.remove_output_files(list_of_names=["LoadCASTEP"])


#  *************************** USE CASES ********************************************
# ===================================================================================
    # | Use case: Gamma point calculation and sum correction enabled during calculations|
    # ===================================================================================
    _gamma_sum = "squaricn_sum_LoadCASTEP"

    def test_gamma_sum_correction(self):
        self._check(name=self._gamma_sum)

    # ===================================================================================
    # |     Use case: Gamma point calculation and no sum correction for Gamma point     |
    # ===================================================================================
    _gamma_no_sum = "squaricn_no_sum_LoadCASTEP"

    def test_gamma_no_sum_correction(self):
        self._check(name=self._gamma_no_sum)

    # ===================================================================================
    # | Use case: more than one k-point and sum correction       |
    # ===================================================================================
    _many_k_sum = "Si2-phonon_LoadCASTEP"

    def test_sum_correction_single_crystal(self):
        self._check(name=self._many_k_sum)

    # ===================================================================================
    # |   Use case: more than one k-point without sum correction                        |
    # ===================================================================================
    #
    _many_k_no_sum = "Si2-sc_LoadCASTEP"

    def test_no_sum_correction_single_crystal(self):
        self._check(name=self._many_k_no_sum)

    # Helper functions
    def _check(self, name=None):
        # get calculated data
        data = self._read_dft(filename=name)

        # get correct data
        correct_data = self._prepare_data(filename=name)

        # check read data
        self._check_reader_data(correct_data=correct_data, data=data, filename=name)

        # check loaded data
        self._check_loader_data(correct_data=correct_data, input_dft_filename=name)

    def _read_dft(self, filename=None):
        """
        Reads data from .phonon file.
        :param filename: name of file with phonon data (name + phonon)
        :return: phonon data
        """
        # 1) Read data
        castep_reader = AbinsModules.LoadCASTEP(
            input_dft_filename=AbinsModules.AbinsTestHelpers.find_file(filename=filename + ".phonon"))

        data = self._get_reader_data(castep_reader=castep_reader)

        # test validData method
        self.assertEqual(True, castep_reader._clerk._valid_hash())

        return data

    # noinspection PyMethodMayBeStatic
    def _prepare_data(self, filename=None):
        """Reads a correct values from ASCII file."""

        with open(AbinsModules.AbinsTestHelpers.find_file(filename + "_data.txt")) as data_file:
            correct_data = json.loads(data_file.read().replace("\n", " "))

        num_k = len(correct_data["datasets"]["k_points_data"]["weights"])
        atoms = len(correct_data["datasets"]["atoms_data"])
        array = {}
        for k in range(num_k):
            temp = np.loadtxt(
                AbinsModules.AbinsTestHelpers.find_file(
                    filename + "_atomic_displacements_data_%s.txt" % k)).view(complex).reshape(-1)
            total_size = temp.size
            num_freq = int(total_size / (atoms * 3))
            array[str(k)] = temp.reshape(atoms, num_freq, 3)

            freq = correct_data["datasets"]["k_points_data"]["frequencies"][str(k)]
            correct_data["datasets"]["k_points_data"]["frequencies"][str(k)] = np.asarray(freq)

        correct_data["datasets"]["k_points_data"].update({"atomic_displacements": array})

        return correct_data

    def _check_reader_data(self, correct_data=None, data=None, filename=None):

        # check data
        correct_k_points = correct_data["datasets"]["k_points_data"]
        items = data["datasets"]["k_points_data"]

        for k in correct_k_points["frequencies"]:
            self.assertEqual(True, np.allclose(correct_k_points["frequencies"][k], items["frequencies"][k]))
            self.assertEqual(True, np.allclose(correct_k_points["atomic_displacements"][k],
                                               items["atomic_displacements"][k]))
            self.assertEqual(True, np.allclose(correct_k_points["k_vectors"][k], items["k_vectors"][k]))
            self.assertEqual(correct_k_points["weights"][k], items["weights"][k])

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
        self.assertEqual(AbinsModules.AbinsTestHelpers.find_file(filename + ".phonon"), data["attributes"]["filename"])

        # check datasets
        self.assertEqual(True, np.allclose(correct_data["datasets"]["unit_cell"], data["datasets"]["unit_cell"]))

    def _check_loader_data(self, correct_data=None, input_dft_filename=None):

        loader = AbinsModules.LoadCASTEP(
            input_dft_filename=AbinsModules.AbinsTestHelpers.find_file(input_dft_filename + ".phonon"))
        loaded_data = loader.load_formatted_data().extract()

        # k points
        correct_items = correct_data["datasets"]["k_points_data"]
        items = loaded_data["k_points_data"]

        for k in correct_items["frequencies"]:
            self.assertEqual(True, np.allclose(correct_items["frequencies"][k], items["frequencies"][k]))
            self.assertEqual(True, np.allclose(correct_items["atomic_displacements"][k],
                                               items["atomic_displacements"][k]))
            self.assertEqual(True, np.allclose(correct_items["k_vectors"][k], items["k_vectors"][k]))
            self.assertEqual(correct_items["weights"][k], items["weights"][k])

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
    def _get_reader_data(self, castep_reader=None):
        abins_type_data = castep_reader.read_phonon_file()
        data = {"datasets": abins_type_data.extract(),
                "attributes": castep_reader._clerk._attributes
                }
        data["datasets"].update({"unit_cell": castep_reader._clerk._data["unit_cell"]})
        return data

if __name__ == '__main__':
    unittest.main()
