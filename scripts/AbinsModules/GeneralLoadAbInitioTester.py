# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import AbinsModules
import json
import numpy as np


class GeneralLoadAbInitioTester(object):

    _loaders_extensions = {"LoadCASTEP": "phonon", "LoadCRYSTAL": "out", "LoadDMOL3": "outmol", "LoadGAUSSIAN": "log"}

    # noinspection PyMethodMayBeStatic
    def _prepare_data(self, filename=None):
        """Reads reference values from ASCII file."""

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

    def _check_reader_data(self, correct_data=None, data=None, filename=None, extension=None):

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
        self.assertEqual(correct_data["attributes"]["hash"], data["attributes"]["hash"])
        self.assertEqual(correct_data["attributes"]["ab_initio_program"], data["attributes"]["ab_initio_program"])
        # advanced_parameters is stored as a str but we unpack/compare to dict
        # so comparison will tolerate unimportant formatting changes
        self.assertEqual(correct_data["attributes"]["advanced_parameters"],
                         json.loads(data["attributes"]["advanced_parameters"]))

        try:
            self.assertEqual(AbinsModules.AbinsTestHelpers.find_file(filename + "." + extension),
                             data["attributes"]["filename"])
        except AssertionError:
            self.assertEqual(AbinsModules.AbinsTestHelpers.find_file(filename + "." + extension.upper()),
                             data["attributes"]["filename"])

        # check datasets
        self.assertEqual(True, np.allclose(correct_data["datasets"]["unit_cell"], data["datasets"]["unit_cell"]))

    def _check_loader_data(self, correct_data=None, input_ab_initio_filename=None, extension=None, loader=None):

        try:
            read_filename = AbinsModules.AbinsTestHelpers.find_file(input_ab_initio_filename + "." + extension)
            ab_initio_loader = loader(input_ab_initio_filename=read_filename)
        except ValueError:
            read_filename = AbinsModules.AbinsTestHelpers.find_file(input_ab_initio_filename + "." + extension.upper())
            ab_initio_loader = loader(input_ab_initio_filename=read_filename)

        loaded_data = ab_initio_loader.load_formatted_data().extract()

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

    def check(self, name=None, loader=None):

        extension = self._loaders_extensions[str(loader)]

        # get calculated data
        data = self._read_ab_initio(loader=loader, filename=name, extension=extension)

        # get correct data
        correct_data = self._prepare_data(filename=name)

        # check read data
        self._check_reader_data(correct_data=correct_data, data=data, filename=name, extension=extension)

        # check loaded data
        self._check_loader_data(correct_data=correct_data, input_ab_initio_filename=name, extension=extension, loader=loader)

    def _read_ab_initio(self, loader=None, filename=None, extension=None):
        """
        Reads data from .{extension} file.
        :param loader: ab initio loader
        :param filename: name of file with vibrational or phonon data (name + extension)
        :returns: vibrational or phonon data
        """
        # 1) Read data
        try:
            read_filename = AbinsModules.AbinsTestHelpers.find_file(filename=filename + "." + extension)
            ab_initio_reader = loader(input_ab_initio_filename=read_filename)
        except ValueError:
            read_filename = AbinsModules.AbinsTestHelpers.find_file(filename=filename + "." + extension.upper())
            ab_initio_reader = loader(input_ab_initio_filename=read_filename)

        data = self._get_reader_data(ab_initio_reader=ab_initio_reader)

        # test validData method
        self.assertEqual(True, ab_initio_reader._clerk._valid_hash())

        return data

    # noinspection PyMethodMayBeStatic
    def _get_reader_data(self, ab_initio_reader=None):
        """
        :param ab_initio_reader: object of type  GeneralAbInitioProgram
        :returns: read data
        """
        abins_type_data = ab_initio_reader.read_vibrational_or_phonon_data()
        data = {"datasets": abins_type_data.extract(),
                "attributes": ab_initio_reader._clerk._attributes
                }
        data["datasets"].update({"unit_cell": ab_initio_reader._clerk._data["unit_cell"]})
        return data
