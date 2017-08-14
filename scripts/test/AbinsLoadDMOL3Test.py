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
        logger.warning("Skipping AbinsLoadDMOL3Test because Python is too old.")
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
class AbinsLoadDMOL3Test(unittest.TestCase):

    def tearDown(self):
        AbinsModules.AbinsTestHelpers.remove_output_files(list_of_names=["LoadDMOL3"])

        #  *************************** USE CASES ********************************************
    # ===================================================================================
    # | Use cases: Gamma point calculation for DMOL3                                    |
    # ===================================================================================
    _gamma_dmol3 = "LTA_40_O2_LoadDMOL3"
    _gamma_no_h_dmol3 = "Na2SiF6_LoadDMOL3"

    def test_gamma_dmol3(self):
        self._check(name=self._gamma_dmol3)
        self._check(name=self._gamma_no_h_dmol3)

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
        dmol3_reader = AbinsModules.LoadDMOL3(
            input_dft_filename=AbinsModules.AbinsTestHelpers.find_file(filename=filename + ".outmol"))

        data = self._get_reader_data(dmol3_reader=dmol3_reader)

        # test validData method
        self.assertEqual(True, dmol3_reader._clerk._valid_hash())

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
            self.assertEqual(True, np.allclose(np.array(correct_atoms["atom_%s" % item]["coord"]),
                                               atoms["atom_%s" % item]["coord"]))

        # check attributes
        self.assertEqual(correct_data["attributes"]["advanced_parameters"], data["attributes"]["advanced_parameters"])
        self.assertEqual(correct_data["attributes"]["hash"], data["attributes"]["hash"])
        self.assertEqual(correct_data["attributes"]["DFT_program"], data["attributes"]["DFT_program"])
        self.assertEqual(AbinsModules.AbinsTestHelpers.find_file(filename + ".outmol"), data["attributes"]["filename"])

        # check datasets
        self.assertEqual(True, np.allclose(correct_data["datasets"]["unit_cell"], data["datasets"]["unit_cell"]))

    def _check_loader_data(self, correct_data=None, input_dft_filename=None):

        loader = AbinsModules.LoadDMOL3(
            input_dft_filename=AbinsModules.AbinsTestHelpers.find_file(input_dft_filename + ".outmol"))
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
            self.assertEqual(True, np.allclose(np.array(correct_atoms["atom_%s" % item]["coord"]),
                                               atoms["atom_%s" % item]["coord"]))

    # noinspection PyMethodMayBeStatic
    def _get_reader_data(self, dmol3_reader=None):
        abins_type_data = dmol3_reader.read_phonon_file()
        data = {"datasets": abins_type_data.extract(),
                "attributes": dmol3_reader._clerk._attributes
                }
        data["datasets"].update({"unit_cell": dmol3_reader._clerk._data["unit_cell"]})
        return data


if __name__ == '__main__':
    unittest.main()
