from __future__ import (absolute_import, division, print_function)
import numpy as np

# Abins modules
import AbinsModules


class CalculateDWSingleCrystal(object):
    """
    Class for calculating Debye-Waller factors for single crystals (sample form is SingleCrystal).
    """

    def __init__(self, temperature=None, abins_data=None):
        """
        :param temperature: temperature in K for which Debye-Waller factors should be calculated
        :param abins_data: input Abins data (type: AbinsData)
        """

        if not isinstance(temperature, (float, int)):
            raise ValueError("Invalid value of the temperature. Number was expected.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature)

        if isinstance(abins_data, AbinsModules.AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Improper value of input Abins data.")

        _data = self._abins_data.extract()

        extracted_k_data = _data["k_points_data"]

        gamma = AbinsModules.AbinsConstants.GAMMA_POINT
        self._num_k = len(extracted_k_data["atomic_displacements"])
        self._num_atoms = extracted_k_data["atomic_displacements"][gamma].shape[0]
        super(CalculateDWSingleCrystal, self).__init__()

    def _calculate_dw(self):
        """
        The Debye-Waller coefficients are calculated atom by atom.
        For each atom they consist in a 3X3 matrix. The Debye-Waller factors are given in atomic units.
        Working equation implemented according to:
        https://forge.epn-campus.eu/html/ab2tds/debye_waller.html
        """
        dw = AbinsModules.DWSingleCrystalData(temperature=self._temperature, num_atoms=self._num_atoms)

        data = self._abins_data.extract()
        num_atoms = len(data["atoms_data"])
        mass_hartree_factor = np.asarray([1.0 / (data["atoms_data"]["atom_%s" % atom]["mass"] * 2)
                                          for atom in range(num_atoms)])

        temperature_hartree = self._temperature * AbinsModules.AbinsConstants.K_2_HARTREE
        coth_factor = 1.0 / (2.0 * temperature_hartree)  # coth( coth_factor * omega)

        k_points = [str(k) for k in range(self._num_k)]
        frequencies_hartree = data["k_points_data"]["frequencies"]
        atomic_displacements = data["k_points_data"]["atomic_displacements"]
        weights = data["k_points_data"]["weights"]

        coth_over_omega = {}

        for k in k_points:
            tanh = np.tanh(np.multiply(coth_factor, frequencies_hartree[k]))
            coth_over_omega[k] = np.divide(1.0, np.multiply(tanh, frequencies_hartree[k]))  # coth(...)/omega
            atomic_displacements[k] = atomic_displacements[k] / AbinsModules.AbinsConstants.ATOMIC_LENGTH_2_ANGSTROM

            #     num_freq -- number of phonons
            #     dim -- size of displacement vector for one atom (dim = 3)

        # item_k [dim, dim]
        item_k = np.zeros((3, 3), dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)  # stores DW for one atom

        for num in range(self._num_atoms):
            item_k.fill(0.0)  # erase stored information so that it can be filled with content for the next atom
            for k in k_points:

                disp = atomic_displacements[k][num]

                # temp_1 [freq, dim dim]
                temp_1 = np.einsum('kij,k->kij',
                                   np.einsum('ki, kj->kij', disp, disp.conjugate()).real,
                                   coth_over_omega[k])

                # temp2 [dim, dim]
                temp_2 = np.sum(temp_1, axis=0)

                np.add(item_k, np.multiply(temp_2, weights[k]), item_k)

            np.multiply(item_k, mass_hartree_factor[num], item_k)

            # noinspection PyProtectedMember
            dw._append(item=item_k, num_atom=num)

        return dw

    def calculate_data(self):
        """
        Calculates Debye-Waller factors.
        :return: object of type DwData with Debye-Waller factors.
        """

        data = self._calculate_dw()

        return data
