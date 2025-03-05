# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from pathlib import Path
from typing import Dict, Tuple

import numpy as np

import abins
from abins.constants import ACOUSTIC_PHONON_THRESHOLD, CONSTANT, CM1_2_HARTREE, K_2_HARTREE, NUM_ZERO


# noinspection PyMethodMayBeStatic
class PowderCalculator:
    """
    Class for calculating powder data.
    """

    def __init__(self, *, filename: str, abins_data: abins.AbinsData, temperature: float, cache_directory: Path) -> None:
        """
        :param filename:  name of input DFT filename
        :param abins_data: object of type AbinsData with data from input DFT file
        :param temperature: temperature in Kelvin (for Bose-Einstein occupation)
        """
        if not isinstance(abins_data, abins.AbinsData):
            raise ValueError("Object of AbinsData was expected.")

        k_data: abins.KpointsData = abins_data.get_kpoints_data()
        self._frequencies: Dict[str, np.ndarray] = {}
        self._displacements: Dict[str, np.ndarray] = {}
        self._temperature = temperature

        atoms_data = abins_data.get_atoms_data()

        # Populate data, removing imaginary modes
        for k, k_point_data in enumerate(k_data):
            mask = k_point_data.frequencies > ACOUSTIC_PHONON_THRESHOLD
            self._frequencies[k] = k_point_data.frequencies[mask]
            self._displacements[k] = k_point_data.atomic_displacements[:, mask]

        self._masses = np.asarray([atoms_data[atom]["mass"] for atom in range(len(atoms_data))])
        self._clerk = abins.IO(
            input_filename=filename,
            group_name=abins.parameters.hdf_groups["powder_data"],
            temperature=self._temperature,
            cache_directory=cache_directory,
        )

    def _calculate_powder(self) -> abins.PowderData:
        """
        Calculates powder data (a_tensors, b_tensors according to aCLIMAX manual).
        """

        k_indices = sorted(self._frequencies.keys())  # make sure dictionary keys are in the same order on each machine
        b_tensors = {}
        a_tensors = {}
        n_plus_1 = {}

        tensors = [self._calculate_powder_k(k=k) for k in k_indices]

        for i, k_index in enumerate(k_indices):
            a_tensors[k_index] = tensors[i][0]
            b_tensors[k_index] = tensors[i][1]
            n_plus_1[k_index] = tensors[i][2]

        powder = abins.PowderData(
            a_tensors=a_tensors, b_tensors=b_tensors, frequencies=self._frequencies, n_plus_1=n_plus_1, num_atoms=len(self._masses)
        )
        return powder

    def _calculate_powder_k(self, *, k: str) -> Tuple[np.ndarray, np.ndarray]:
        """
        :param k: k index
        """

        # Notation for  indices:
        #     num_freq -- number of phonons
        #     num_atoms -- number of atoms
        #     num_k  -- number of k-points
        #     dim -- size of displacement vector for one atom (dim = 3)

        # masses[num_atoms, num_freq]
        num_freq = self._frequencies[k].size
        masses = np.asarray([np.full(num_freq, mass) for mass in self._masses])

        # disp[num_atoms, num_freq, dim]
        disp: np.array = self._displacements[k]

        # factor[num_atoms, num_freq]
        factor = np.einsum("ij,j->ij", 1.0 / masses, CONSTANT / self._frequencies[k])

        # b_tensors[num_atoms, num_freq, dim, dim]
        b_tensors = np.einsum("ijkl,ij->ijkl", np.einsum("lki, lkj->lkij", disp, disp.conjugate()).real, factor)

        # Replace tensor values close to zero with a small finite value.
        # Not clear why this is done; we never divide by these values?
        # Presumably this stabilises the division by b_trace in first order
        # intensity calculation; but it could be handled more efficiently
        # and elegantly at that stage.
        temp = np.fabs(b_tensors)
        indices = temp < NUM_ZERO
        b_tensors[indices] = NUM_ZERO

        if self._temperature < np.finfo(type(self._temperature)).eps:
            two_n_plus_1 = 1.0
            n_plus_1 = 1.0
            b_tensors_2n_plus_1 = b_tensors_n_plus_1 = b_tensors

        else:
            two_n_plus_1 = 1.0 / np.tanh(self._frequencies[k] * CM1_2_HARTREE / (2.0 * self._temperature * K_2_HARTREE))
            n_plus_1 = two_n_plus_1 * 0.5 + 0.5
            b_tensors_2n_plus_1 = b_tensors * two_n_plus_1[None, :, None, None]
            b_tensors_n_plus_1 = b_tensors * n_plus_1[None, :, None, None]

        # a_tensors[num_atoms, dim, dim]
        a_tensors = np.sum(a=b_tensors_2n_plus_1, axis=1)

        return a_tensors, b_tensors_n_plus_1, n_plus_1

    def get_formatted_data(self) -> abins.PowderData:
        """
        Method to obtain data.
        :returns: obtained data
        """
        try:
            self._clerk.check_previous_data()
            data = self.load_formatted_data()
            self._report_progress(str(data) + " has been loaded from the HDF file.")

        except (IOError, ValueError) as err:
            self._report_progress("Warning: " + str(err) + " Data has to be calculated.")
            data = self.calculate_data()
            self._report_progress(str(data) + " has been calculated.")

        return data

    def calculate_data(self) -> abins.PowderData:
        """
        Calculates mean square displacements.
        :returns: object of type PowderData with mean square displacements.
        """

        data = self._calculate_powder()

        self._clerk.add_file_attributes()
        self._clerk.add_data("powder_data", data.extract())
        self._clerk.save()

        return data

    def load_formatted_data(self) -> abins.PowderData:
        """
        Loads mean square displacements.
        :returns: object of type PowderData with mean square displacements.
        """
        data = self._clerk.load(list_of_datasets=["powder_data"])
        powder_data = abins.PowderData.from_extracted(data["datasets"]["powder_data"])
        return powder_data

    def _report_progress(self, msg: str) -> None:
        """
        :param msg:  message to print out
        """
        # In order to avoid
        #
        # RuntimeError: Pickling of "mantid.kernel._kernel.Logger"
        # instances is not enabled (http://www.boost.org/libs/python/doc/v2/pickle.html)
        #
        # logger has to be imported locally

        from mantid.kernel import logger

        logger.notice(msg)
