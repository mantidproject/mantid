from __future__ import (absolute_import, division, print_function)
import numpy as np
import AbinsModules
try:
    # noinspection PyUnresolvedReferences
    from pathos.multiprocessing import ProcessPool
    PATHOS_FOUND = True
except ImportError:
    PATHOS_FOUND = False


# noinspection PyMethodMayBeStatic
class CalculatePowder(object):
    """
    Class for calculating powder data.
    """
    def __init__(self, filename=None, abins_data=None):
        """
        :param filename:  name of input DFT filename
        :param abins_data: object of type AbinsData with data from input DFT file
        """
        if not isinstance(abins_data, AbinsModules.AbinsData):
            raise ValueError("Object of AbinsData was expected.")

        k_data = abins_data.get_kpoints_data().extract()
        gamma_pkt = AbinsModules.AbinsConstants.GAMMA_POINT
        self._frequencies = k_data["frequencies"]
        self._displacements = k_data["atomic_displacements"]
        self._num_atoms = self._displacements[gamma_pkt].shape[0]
        self._atoms_data = abins_data.get_atoms_data().extract()

        self._clerk = AbinsModules.IOmodule(input_filename=filename,
                                            group_name=AbinsModules.AbinsParameters.powder_data_group)

    def _calculate_powder(self):
        """
        Calculates powder data (a_tensors, b_tensors according to aCLIMAX manual).
        """
        # define container for powder data
        powder = AbinsModules.PowderData(num_atoms=self._num_atoms)

        k_indices = sorted(self._frequencies.keys())  # make sure dictionary keys are in the same order on each machine
        b_tensors = {}
        a_tensors = {}

        if PATHOS_FOUND:
            threads = AbinsModules.AbinsParameters.threads
            p_local = ProcessPool(nodes=threads)
            tensors = p_local.map(self._calculate_powder_k, k_indices)
        else:
            tensors = [self._calculate_powder_k(k=k) for k in k_indices]

        for indx, k in enumerate(k_indices):
            a_tensors[k] = tensors[indx][0]
            b_tensors[k] = tensors[indx][1]

        # fill powder object with powder data
        powder.set(dict(b_tensors=b_tensors, a_tensors=a_tensors))

        return powder

    def _calculate_powder_k(self, k=None):
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
        disp = self._displacements[k]

        # factor[num_atoms, num_freq]
        factor = np.einsum('ij,j->ij', 1.0 / masses, AbinsModules.AbinsConstants.CONSTANT / self._frequencies[k])

        # b_tensors[num_atoms, num_freq, dim, dim]
        b_tensors = np.einsum('ijkl,ij->ijkl',
                              np.einsum('lki, lkj->lkij', disp, disp.conjugate()).real, factor)

        temp = np.fabs(b_tensors)
        indices = temp < AbinsModules.AbinsConstants.NUM_ZERO
        b_tensors[indices] = AbinsModules.AbinsConstants.NUM_ZERO

        # a_tensors[num_atoms, dim, dim]
        a_tensors = np.sum(a=b_tensors, axis=1)

        return a_tensors, b_tensors

    def get_formatted_data(self):
        """
        Method to obtain data.
        :return: obtained data
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

    def calculate_data(self):
        """
        Calculates mean square displacements.
        :return: object of type PowderData with mean square displacements.
        """

        data = self._calculate_powder()

        self._clerk.add_file_attributes()
        self._clerk.add_data("powder_data", data.extract())
        self._clerk.save()

        return data

    def load_formatted_data(self):
        """
        Loads mean square displacements.
        :return: object of type PowderData with mean square displacements.
        """
        data = self._clerk.load(list_of_datasets=["powder_data"])
        k_pkt = AbinsModules.AbinsConstants.GAMMA_POINT
        powder_data = AbinsModules.PowderData(num_atoms=data["datasets"]["powder_data"]["b_tensors"][k_pkt].shape[0])
        powder_data.set(data["datasets"]["powder_data"])

        return powder_data

    def _report_progress(self, msg):
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
