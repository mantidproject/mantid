import numpy as np

# ABINS modules
from DwData import DwData
from AbinsData import AbinsData
from IOmodule import  IOmodule

import Constants


class CalculateDW(IOmodule):
    """
    Class for calculating Debye-Waller factors.
    """
    
    def __init__(self, filename=None, temperature=None, abins_data=None):
        """
        @param filename:  name of input filename (CASTEP: foo.phonon)
        @param temperature: temperature in K for which Debye-Waller factors should be calculated
        @param abins_data: input Abins data (type: AbinsData)
        """

        super(CalculateDW, self).__init__(input_filename=filename, group_name=Constants.DW_data_group)

        if not (isinstance(temperature, float) or isinstance(temperature, int)):
            raise ValueError("Invalid value of the temperature. Number was expected.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = temperature

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
        _DW = DwData(temperature=self._temperature, num_atoms=self._num_atoms)

        _data = self._abins_data.extract()
        _mass_hartree_factor = np.asarray([1.0 / ( atom["mass"] * 2)  for atom in _data["atoms_data"]])
        _frequencies_hartree = _data["k_points_data"]["frequencies"]
        _temperature_hartree = self._temperature * Constants.k_2_hartree

        _weights = _data["k_points_data"]["weights"]
        _atomic_displacements = _data["k_points_data"]["atomic_displacements"]

        _coth_factor = 1.0 / (2.0 * _temperature_hartree) # coth( _coth_factor * omega)
        _item = np.zeros((3, 3), dtype=Constants.float_type) # stores DW for one atom

        for num in range(self._num_atoms):
            _item.fill(0.0) # erase stored information so that it can be filled with content for the next atom
            for k in range(self._num_k):

                _coth_over_omega = (1.0 / np.tanh(_coth_factor * _frequencies_hartree[k, :])) /  _frequencies_hartree[k, :] # coth(...)/omega

                # correction for acoustic modes at Gamma point
                if np.linalg.norm(_data["k_points_data"]["k_vectors"][k]) < Constants.small_k: start = 2
                else: start = 0

                for n_freq in range(start, self._num_freq):

                    displacement = _atomic_displacements[k, num, n_freq, :]
                    tensor = np.outer(displacement, displacement.conjugate()).real # DW factors are real
                    np.multiply(tensor, _coth_over_omega[n_freq], tensor)
                    np.add(_item, tensor, _item)

                np.add(_item, _item * _weights[k], _item)

            np.multiply(_item, _mass_hartree_factor[num], _item)
            _DW._append(item=_item, num_atom=num)
        return _DW


    def getDW(self):
        """
        Calculates Debye-Waller factors and saves them to an hdf file.
        @return: object of type DwData with Debye-Waller factors.
        """

        data = self._calculate_DW()

        self.addAttribute("temperature", self._temperature)
        self.addAttribute("filename", self._input_filename)
        self.addNumpyDataset("data", data.extract())

        self.save()

        return data


    def loadData(self):
        """
        Loads Debye-Waller factors in the form of DwData from hdf file.
        @return: Debye-Waller factors (DwData)
        """
        _data = self.load(list_of_numpy_datasets=["data"], list_of_attributes=["temperature"])
        _num_atoms = _data["datasets"]["data"].shape[0]
        _dw_data = DwData(num_atoms=_num_atoms, temperature=_data["attributes"]["temperature"])
        _dw_data.set(_data["datasets"]["data"])

        return _dw_data
