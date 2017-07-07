from __future__ import (absolute_import, division, print_function)
import numpy as np

import AbinsModules
from mantid.kernel import logger


# noinspection PyMethodMayBeStatic
class CalculatePowder(object):
    """
    Class for calculating powder data.
    """
    def __init__(self, filename=None, abins_data=None):
        """
        @param filename:  name of input DFT filename
        @param abins_data: object of type AbinsData with data from input DFT file
        """

        if not isinstance(abins_data, AbinsModules.AbinsData):
            raise ValueError("Object of AbinsData was expected.")
        self._abins_data = abins_data

        self._clerk = AbinsModules.IOmodule(input_filename=filename,
                                            group_name=AbinsModules.AbinsParameters.powder_data_group)

    def _get_gamma_data(self, k_data=None):
        """
        Extracts k points data only for Gamma point.
        @param k_data: numpy array with k-points data.
        @return: dictionary with k_data only for Gamma point
        """
        gamma_pkt_index = -1

        # look for index of Gamma point
        num_k = k_data["k_vectors"].shape[0]
        for k in range(num_k):
            if np.linalg.norm(k_data["k_vectors"][k]) < AbinsModules.AbinsConstants.SMALL_K:
                gamma_pkt_index = k
                break
        if gamma_pkt_index == -1:
            raise ValueError("Gamma point not found.")

        k_points = {"weights": k_data["weights"][gamma_pkt_index],
                    "k_vectors": k_data["k_vectors"][gamma_pkt_index],
                    "frequencies": k_data["frequencies"][gamma_pkt_index],
                    "atomic_displacements": k_data["atomic_displacements"][gamma_pkt_index]}

        return k_points

    def _calculate_powder(self):
        """
        Calculates powder data (a_tensors, b_tensors according to aCLIMAX manual).
        """

        # get all necessary data
        data = self._abins_data.extract()
        k_data = self._get_gamma_data(data["k_points_data"])
        displacements = k_data["atomic_displacements"]
        num_atoms = displacements.shape[0]
        atoms_data = data["atoms_data"]

        if k_data["frequencies"].size == 3 * num_atoms:  # use case: crystal
            first_frequency = AbinsModules.AbinsConstants.FIRST_OPTICAL_PHONON
        else:  # use case: molecule
            first_frequency = AbinsModules.AbinsConstants.FIRST_MOLECULAR_VIBRATION
        frequencies = k_data["frequencies"][first_frequency:]

        if min(frequencies) <= AbinsModules.AbinsParameters.acoustic_phonon_threshold:
            raise ValueError("Frequencies which correspond to soft phonons found. Your structure is unstable or you "
                             "need to perform DFT phonon calculation with higher accuracy.")

        powder = AbinsModules.PowderData(num_atoms=num_atoms)

        # Notation for  indices:
        #     num_freq -- number of optical phonons (total number of phonons - acoustic phonons)
        #     num_atoms -- number of atoms
        #     dim -- size of displacement vector for one atom (dim = 3)

        # masses[num_atoms, num_freq]

        masses = np.asarray([([atoms_data["atom_%s" % atom]["mass"]] * frequencies.size) for atom in range(num_atoms)])

        # disp[num_atoms, num_freq, dim]
        disp = displacements[:, AbinsModules.AbinsConstants.FIRST_OPTICAL_PHONON:]

        # factor[num_atoms, num_freq]
        factor = np.einsum('ij,j->ij', 1.0 / masses, AbinsModules.AbinsConstants.ACLIMAX_CONSTANT / frequencies)

        # b_tensors[num_atoms, num_freq, dim, dim]
        b_tensors = np.einsum('ijkl,ij->ijkl', np.einsum('lki, lkj->lkij', disp, disp).real, factor)

        temp = np.fabs(b_tensors)
        indices = temp < AbinsModules.AbinsConstants.NUM_ZERO
        b_tensors[indices] = AbinsModules.AbinsConstants.NUM_ZERO

        # a_tensors[num_atoms, dim, dim]
        a_tensors = np.sum(a=b_tensors, axis=1)

        # fill powder object with powder data
        powder.set(dict(b_tensors=b_tensors, a_tensors=a_tensors))

        return powder

    def get_formatted_data(self):
        """
        Method to obtain data
        @return: obtained data
        """
        try:
            self._clerk.check_previous_data()
            data = self.load_formatted_data()
            logger.notice(str(data) + " has been loaded from the HDF file.")

        except (IOError, ValueError) as err:

            logger.notice("Warning: " + str(err) + " Data has to be calculated.")
            data = self.calculate_data()
            logger.notice(str(data) + " has been calculated.")

        return data

    def calculate_data(self):
        """
        Calculates mean square displacements and Debye-Waller factors.  Saves both MSD and DW  to an hdf file.
        @return: object of type PowderData with mean square displacements and Debye-Waller factors.
        """

        data = self._calculate_powder()

        self._clerk.add_file_attributes()
        self._clerk.add_data("powder_data", data.extract())
        self._clerk.save()

        return data

    def load_formatted_data(self):
        """
        Loads mean square displacements and Debye-Waller factors from an hdf file.
        @return: object of type PowderData with mean square displacements and Debye-Waller factors.
        """
        data = self._clerk.load(list_of_datasets=["powder_data"])
        powder_data = AbinsModules.PowderData(num_atoms=data["datasets"]["powder_data"]["b_tensors"].shape[0])
        powder_data.set(data["datasets"]["powder_data"])

        return powder_data
