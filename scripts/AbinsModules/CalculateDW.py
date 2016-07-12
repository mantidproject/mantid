import numpy as np

# ABINS modules
from QData import QData
from DwData import DwData
from AbinsData import AbinsData
from IOmodule import  IOmodule
from RearrangeData import  RearrangeData

import Constants


class CalculateDW(IOmodule, RearrangeData):
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
        Internally the routine takes temperature in Kelvin and converts it to Hartree, while the eigenvectors are
        dimensionless, the frequencies are given in units of 1/cm and are converted also to Hartree.
        Working equation implemented according to:
        https://forge.epn-campus.eu/html/ab2tds/debye_waller.html
        """
        _DW = DwData(temperature=self._temperature, num_atoms=self._num_atoms)

        _data = self._abins_data.extract()

        _rearranged_data = self._convert_to_hartree(abins_data=_data)
        _mass_hartree = _rearranged_data["mass_hartree"]
        _frequencies_hartree = _rearranged_data["frequencies_hartree"]
        _temperature_hartree = _rearranged_data["temperature_hartree"]

        _weights = _data["k_points_data"]["weights"]
        _atomic_displacements = _data["k_points_data"]["atomic_displacements"]

        _coth_factor = 1.0 / (2.0 * _temperature_hartree) # coth( _coth_factor * omega)
        _item = np.zeros((3, 3), dtype=Constants.floats_type) # stores DW for one atom

        for num in range(self._num_atoms):
            _item.fill(0.0) # erase stored information so that it can be filled with content for the next atom
            for k in range(self._num_k):
                _coth_over_omega = (1.0 / np.tanh(_coth_factor * _frequencies_hartree[k, :])) /  _frequencies_hartree[k, :] # coth(...)/omega
                for n_freq in range(self._num_freq):

                    displacement = _atomic_displacements[k, num, n_freq, :]
                    tensor =  np.outer(displacement, displacement.conjugate())
                    _item[:, :] += tensor * _coth_over_omega[n_freq]

                _item[:, :] += _item[:, :] * _weights[k]
            _item[:, :] /= 2.0 * _mass_hartree[num]
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
