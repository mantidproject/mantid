import numpy as np

# ABINS modules
from DwCrystalData import DwCrystalData
from AbinsData import AbinsData
from IOmodule import  IOmodule

import AbinsParameters


class CalculateDWCrystal(IOmodule):
    """
    Class for calculating Debye-Waller factors for single crystals (sample form is SingleCrystal).
    """
    
    def __init__(self, temperature=None, abins_data=None):
        """
        @param temperature: temperature in K for which Debye-Waller factors should be calculated
        @param abins_data: input Abins data (type: AbinsData)
        """

        if not (isinstance(temperature, float) or isinstance(temperature, int)):
            raise ValueError("Invalid value of the temperature. Number was expected.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature)

        if isinstance(abins_data, AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Improper value of input Abins data.")

        _data = self._abins_data.extract()

        extracted_k_data = _data["k_points_data"]

        self._num_k = extracted_k_data["atomic_displacements"].shape[0]
        self._num_atoms = extracted_k_data["atomic_displacements"].shape[1]
        self._num_freq = extracted_k_data["atomic_displacements"].shape[2]


    def _calculate_DW(self):
        """
        The Debye-Waller coefficients are calculated atom by atom.
        For each atom they consist in a 3X3 matrix. The Debye-Waller factors are given in atomic units.
        Working equation implemented according to:
        https://forge.epn-campus.eu/html/ab2tds/debye_waller.html
        """
        _DW = DwCrystalData(temperature=self._temperature, num_atoms=self._num_atoms)

        _data = self._abins_data.extract()
        _mass_hartree_factor = np.asarray([1.0 / ( atom["mass"] * 2)  for atom in _data["atoms_data"]])
        _frequencies_hartree = _data["k_points_data"]["frequencies"]
        _temperature_hartree = self._temperature * AbinsParameters.k_2_hartree

        _weights = _data["k_points_data"]["weights"]
        _atomic_displacements = _data["k_points_data"]["atomic_displacements"]

        _coth_factor = 1.0 / (2.0 * _temperature_hartree) # coth( _coth_factor * omega)

        _tanh =  np.tanh(np.multiply(_coth_factor,  _frequencies_hartree))
        _coth_over_omega = np.divide(1.0, np.multiply(_tanh ,_frequencies_hartree)) # coth(...)/omega

        _item_k = np.zeros((3, 3), dtype=AbinsParameters.float_type) # stores DW for one atom
        _item_freq = np.zeros((3, 3), dtype=AbinsParameters.float_type)

        for num in range(self._num_atoms):
            _item_k.fill(0.0) # erase stored information so that it can be filled with content for the next atom

            for k in range(self._num_k):

                # correction for acoustic modes at Gamma point
                if np.linalg.norm(_data["k_points_data"]["k_vectors"][k]) < AbinsParameters.small_k: start = 3
                else: start = 0

                _item_freq.fill(0.0)

                for n_freq in range(start, self._num_freq):

                    displacement = _atomic_displacements[k, num, n_freq, :]
                    tensor = np.outer(displacement, displacement.conjugate()).real # DW factors are real
                    np.multiply(tensor, _coth_over_omega[k, n_freq], tensor)
                    np.add(_item_freq , tensor, _item_freq)

                np.add(_item_k, np.multiply(_item_freq, _weights[k]), _item_k)

            np.multiply(_item_k, _mass_hartree_factor[num], _item_k)
            _DW._append(item=_item_k, num_atom=num)
        return _DW


    def calculateData(self):
        """
        Calculates Debye-Waller factors.
        @return: object of type DwData with Debye-Waller factors.
        """

        data = self._calculate_DW()

        return data

