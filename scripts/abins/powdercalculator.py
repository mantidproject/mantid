# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from typing import Dict, Tuple

import abins
from abins.constants import CONSTANT, GAMMA_POINT, NUM_ZERO

try:
    # noinspection PyUnresolvedReferences
    from pathos.multiprocessing import ProcessPool
    PATHOS_FOUND = True
except ImportError:
    PATHOS_FOUND = False


# noinspection PyMethodMayBeStatic
class PowderCalculator:
    """
    Class for calculating powder data.
    """
    def __init__(self, *, filename: str, abins_data: abins.AbinsData) -> None:
        """
        :param filename:  name of input DFT filename
        :param abins_data: object of type AbinsData with data from input DFT file
        """
        if not isinstance(abins_data, abins.AbinsData):
            raise ValueError("Object of AbinsData was expected.")

        k_data = abins_data.get_kpoints_data().extract()  # type: Dict[str, Dict[str, np.ndarray]]
        self._frequencies = k_data["frequencies"]  # type: Dict[str, np.ndarray]
        self._displacements = k_data["atomic_displacements"]  # type: Dict[str, np.ndarray]

        self._num_atoms = self._displacements[GAMMA_POINT].shape[0]
        self._atoms_data = abins_data.get_atoms_data().extract()

        self._clerk = abins.IO(input_filename=filename,
                               group_name=abins.parameters.hdf_groups['powder_data'])

    def _calculate_powder(self) -> abins.PowderData:
        """
        Calculates powder data (a_tensors, b_tensors according to aCLIMAX manual).
        """

        k_indices = sorted(self._frequencies.keys())  # make sure dictionary keys are in the same order on each machine
        b_tensors = {}
        a_tensors = {}

        if PATHOS_FOUND:
            threads = abins.parameters.performance['threads']
            p_local = ProcessPool(nodes=threads)
            tensors = p_local.map(self._calculate_powder_k, k_indices)
        else:
            tensors = [self._calculate_powder_k(k=k) for k in k_indices]

        for indx, k in enumerate(k_indices):
            a_tensors[k] = tensors[indx][0]
            b_tensors[k] = tensors[indx][1]

        powder = abins.PowderData(dict(b_tensors=b_tensors,
                                       a_tensors=a_tensors),
                                  num_atoms=self._num_atoms)
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
        masses = np.asarray([([self._atoms_data["atom_%s" % atom]["mass"]] * self._frequencies[k].size)
                             for atom in range(self._num_atoms)])

        # disp[num_atoms, num_freq, dim]
        disp = self._displacements[k]  # type np.ndarray

        # factor[num_atoms, num_freq]
        factor = np.einsum('ij,j->ij', 1.0 / masses, CONSTANT / self._frequencies[k])

        # b_tensors[num_atoms, num_freq, dim, dim]
        b_tensors = np.einsum('ijkl,ij->ijkl',
                              np.einsum('lki, lkj->lkij', disp, disp.conjugate()).real, factor)

        # Replace tensor values close to zero with a small finite value.
        # Not clear why this is done; we never divide by these values?
        # Presumably this stabilises the division by b_trace in first order
        # intensity calculation; but it could be handled more efficiently
        # and elegantly at that stage.
        temp = np.fabs(b_tensors)
        indices = temp < NUM_ZERO
        b_tensors[indices] = NUM_ZERO

        # a_tensors[num_atoms, dim, dim]
        a_tensors = np.sum(a=b_tensors, axis=1)

        return a_tensors, b_tensors

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
        powder_data = abins.PowderData(data["datasets"]["powder_data"],
                                       num_atoms=data["datasets"]["powder_data"]["b_tensors"][GAMMA_POINT].shape[0])

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
