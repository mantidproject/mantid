# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import json
from numbers import Real
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Any, Dict

import numpy as np
from numpy.testing import assert_allclose
from typing import Union

import abins
from abins.input import AbInitioLoader
from abins.test_helpers import dict_arrays_to_lists


class Tester(object):
    """Base class for testing Abins input loaders"""

    _loaders_extensions = {
        "CASTEPLoader": "phonon",
        "CRYSTALLoader": "out",
        "DMOL3Loader": "outmol",
        "EuphonicLoader": "castep_bin",
        "GAUSSIANLoader": "log",
        "JSONLoader": "json",
        "VASPLoader": "xml",
        "VASPOUTCARLoader": "OUTCAR",
    }

    MASS_DELTA = 1e-5
    FLOAT_EPS = np.finfo(np.float64).eps

    @staticmethod
    def _to_array_inplace(data: dict, key: str) -> None:
        """Replace a dict item with equivalent numpy array"""
        data[key] = np.asarray(data[key])

    @classmethod
    def _prepare_data(cls, seedname: str, max_displacement_kpt: Real = float("Inf")):
        """Reads reference values from ASCII files

        Args:

            seedname:
                Reference data will read from the file {seedname}_data.txt,
                except for the atomic displacements which are read from files
                {seedname}_atomic_displacements_data_{I}.txt, where {I} are
                k-point indices.

            max_displacement_kpt:
                Highest kpt index for which displacements data is read
        """

        with open(abins.test_helpers.find_file(seedname + "_data.txt")) as data_file:
            correct_data = json.loads(data_file.read().replace("\n", " "))

        num_k = len(correct_data["datasets"]["k_points_data"]["weights"])

        atoms = len(correct_data["datasets"]["atoms_data"])
        array = {}

        for k in range(num_k):
            cls._to_array_inplace(correct_data["datasets"]["k_points_data"]["frequencies"], str(k))

            if k <= max_displacement_kpt:
                try:
                    disp_file = f"{seedname}_atomic_displacements_data_{k}.txt"
                    temp = np.loadtxt(abins.test_helpers.find_file(disp_file)).view(complex).reshape(-1)
                except FileNotFoundError:
                    raise FileNotFoundError(disp_file)

                total_size = temp.size
                num_freq = int(total_size / (atoms * 3))
                array[str(k)] = temp.reshape(atoms, num_freq, 3)

        correct_data["datasets"]["k_points_data"].update({"atomic_displacements": array})

        return correct_data

    @staticmethod
    def _cull_imaginary_modes(frequencies: np.ndarray, displacements: Union[np.ndarray, None]):
        from abins.constants import ACOUSTIC_PHONON_THRESHOLD

        finite_mode_indices = frequencies > ACOUSTIC_PHONON_THRESHOLD

        if displacements is None:
            return (frequencies[finite_mode_indices], None)
        else:
            return (frequencies[finite_mode_indices], displacements[:, finite_mode_indices])

    def _check_reader_data(self, correct_data=None, data=None, filename=None, extension=None, max_displacement_kpt=float("Inf")):
        # check data
        correct_k_points = correct_data["datasets"]["k_points_data"]
        items = data["datasets"]["k_points_data"]

        for k in correct_k_points["frequencies"]:
            correct_frequencies, correct_displacements = self._cull_imaginary_modes(
                correct_k_points["frequencies"][k], correct_k_points["atomic_displacements"].get(k, None)
            )
            calc_frequencies, calc_displacements = self._cull_imaginary_modes(
                items["frequencies"][k], items["atomic_displacements"].get(k, None)
            )

            assert_allclose(correct_frequencies, calc_frequencies)
            assert_allclose(correct_k_points["k_vectors"][k], items["k_vectors"][k])
            self.assertEqual(correct_k_points["weights"][k], items["weights"][k])

            if int(k) <= max_displacement_kpt:
                assert_allclose(
                    correct_displacements, calc_displacements, err_msg="Atomic displacements do not match values from reference file"
                )

        correct_atoms = correct_data["datasets"]["atoms_data"]
        atoms = data["datasets"]["atoms_data"]
        for item in range(len(correct_atoms)):
            self.assertEqual(correct_atoms["atom_%s" % item]["sort"], atoms["atom_%s" % item]["sort"])
            self.assertAlmostEqual(
                correct_atoms["atom_%s" % item]["mass"], atoms["atom_%s" % item]["mass"], delta=self.MASS_DELTA
            )  # delta in amu units

            self.assertEqual(correct_atoms["atom_%s" % item]["symbol"], atoms["atom_%s" % item]["symbol"])
            assert_allclose(correct_atoms["atom_%s" % item]["coord"], atoms["atom_%s" % item]["coord"], rtol=1e-7, atol=self.FLOAT_EPS)

        # check attributes
        self.assertEqual(correct_data["attributes"]["hash"], data["attributes"]["hash"])
        self.assertEqual(correct_data["attributes"]["ab_initio_program"], data["attributes"]["ab_initio_program"])
        self.assertEqual(abins.test_helpers.find_file(f"{filename}.{extension}"), data["attributes"]["filename"])

        # check datasets
        assert_allclose(correct_data["datasets"]["unit_cell"], data["datasets"]["unit_cell"])

    def _check_loader_data(
        self,
        correct_data=None,
        input_ab_initio_filename=None,
        extension=None,
        loader=None,
        max_displacement_kpt: Real = float("Inf"),
        cache_directory: Path | None = None,
    ):
        read_filename = abins.test_helpers.find_file(f"{input_ab_initio_filename}.{extension}")
        ab_initio_loader = loader(input_ab_initio_filename=read_filename, cache_directory=cache_directory)

        abins_data = ab_initio_loader.load_formatted_data()
        self.assertTrue(abins_data.get_kpoints_data().is_normalised())

        loaded_data = abins_data.extract()

        # k points
        correct_items = correct_data["datasets"]["k_points_data"]
        items = loaded_data["k_points_data"]

        for k in correct_items["frequencies"]:
            correct_frequencies, correct_displacements = self._cull_imaginary_modes(
                correct_items["frequencies"][k], correct_items["atomic_displacements"].get(k, None)
            )
            calc_frequencies, calc_displacements = self._cull_imaginary_modes(
                items["frequencies"][k], items["atomic_displacements"].get(k, None)
            )

            assert_allclose(correct_frequencies, calc_frequencies)
            assert_allclose(correct_items["k_vectors"][k], items["k_vectors"][k])
            assert_allclose(correct_items["weights"][k], items["weights"][k])

            if int(k) <= max_displacement_kpt:
                assert_allclose(correct_displacements, calc_displacements)

        # atoms
        correct_atoms = correct_data["datasets"]["atoms_data"]
        atoms = loaded_data["atoms_data"]

        for item in range(len(correct_atoms)):
            self.assertEqual(correct_atoms["atom_%s" % item]["sort"], atoms["atom_%s" % item]["sort"])
            self.assertAlmostEqual(correct_atoms["atom_%s" % item]["mass"], atoms["atom_%s" % item]["mass"], delta=self.MASS_DELTA)
            self.assertEqual(correct_atoms["atom_%s" % item]["symbol"], atoms["atom_%s" % item]["symbol"])
            assert_allclose(np.array(correct_atoms["atom_%s" % item]["coord"]), atoms["atom_%s" % item]["coord"], atol=self.FLOAT_EPS)

    def check(
        self, *, name: str, loader: AbInitioLoader, extension: str = None, max_displacement_kpt: Real = float("Inf"), **loader_kwargs
    ):
        """Run loader and compare output with reference files

        Args:
            name: prefix for test files (e.g. 'ethane_LoadVASP')

            loader: loader class under test
            extension: file extension if not the default for given loader
            max_displacement_kpt: highest kpt index for which displacement data is checked
            **loader_kwargs: passed to loader.__init__()
        """
        if extension is None:
            extension = self._loaders_extensions[str(loader)]

        # get correct data
        correct_data = self._prepare_data(name, max_displacement_kpt=max_displacement_kpt)

        with TemporaryDirectory() as tmpdir:
            # get calculated data
            data = self._read_ab_initio(loader=loader, filename=name, extension=extension, cache_directory=Path(tmpdir), **loader_kwargs)

            # check read data
            self._check_reader_data(
                correct_data=correct_data, data=data, filename=name, extension=extension, max_displacement_kpt=max_displacement_kpt
            )

            # check loaded data
            self._check_loader_data(
                correct_data=correct_data,
                input_ab_initio_filename=name,
                extension=extension,
                loader=loader,
                max_displacement_kpt=max_displacement_kpt,
                cache_directory=Path(tmpdir),
            )

    def _read_ab_initio(self, loader=None, filename=None, extension=None, **loader_kwargs) -> Dict[str, Any]:
        """
        Reads data from .{extension} file.

        Args:
            loader: ab initio loader class (to instantiate and test)
            filename: prefix of file with vibrational or phonon data
            extension: extension of data file (e.g. ".phonon")
            **loader_kwargs: passed to loader.__init__()

        returns: vibrational or phonon data

        """
        # 1) Read data
        read_filename = abins.test_helpers.find_file(filename=f"{filename}.{extension}")
        ab_initio_reader = loader(input_ab_initio_filename=read_filename, **loader_kwargs)
        data = self._get_reader_data(ab_initio_reader)

        # test validData method
        self.assertTrue(ab_initio_reader._clerk._valid_hash())

        return data

    @staticmethod
    def _get_reader_data(ab_initio_reader):
        """
        :param ab_initio_reader: object of type AbInitioLoader
        :returns: read data
        """
        abins_type_data = ab_initio_reader.read_vibrational_or_phonon_data()
        data = {"datasets": abins_type_data.extract(), "attributes": ab_initio_reader._clerk._attributes.copy()}
        data["datasets"].update({"unit_cell": ab_initio_reader._clerk._data["unit_cell"]})
        return data

    @classmethod
    def save_ab_initio_test_data(cls, ab_initio_reader, seedname):
        """
        Write ab initio calculation data to JSON file for use in test cases

        :param ab_initio_reader: Reader after import of external calculation
        :type ab_initio_reader: abins.AbInitioProgram
        :param filename: Seed for text files for JSON output. Data will be written to the file {seedname}_data.txt,
            except for the atomic displacements which are written to files {seedname}_atomic_displacements_data_{I}.txt,
            where {I} are k-point indices.
        :type filename: str

        """

        data = cls._get_reader_data(ab_initio_reader)

        # Discard advanced_parameters cache as this is not relevant to loader tests
        del data["attributes"]["advanced_parameters"]

        displacements = data["datasets"]["k_points_data"].pop("atomic_displacements")
        for i, eigenvector in displacements.items():
            with open("{seedname}_atomic_displacements_data_{i}.txt".format(seedname=seedname, i=i), "wt") as f:
                eigenvector.flatten().view(float).tofile(f, sep=" ")

        with open("{seedname}_data.txt".format(seedname=seedname), "wt") as f:
            json.dump(dict_arrays_to_lists(data), f, indent=4, sort_keys=True)
