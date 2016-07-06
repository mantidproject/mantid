import numpy as np

# ABINS modules
from QData import QData
from DwData import DwData
from AbinsData import AbinsData
from IOmodule import  IOmodule
import Constants


class CalculateDW(IOmodule):
    
    def __init__(self, filename=None, temperature=None, abins_data=None):
        """

        @param filename:  name of input filename (CASTEP: foo.phonon)
        @param temperature: temperature in K for which Debye-Waller factors should be calculated
        @param abins_data: input Abins data (type: AbinsData)
        """
        super(CalculateDW, self).__init__( input_filename=filename, group_name=Constants.DW_data_group)

        if not (isinstance(temperature, float) or isinstance(temperature, int)):
            raise ValueError("Invalid value of temperature. Number was expected.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = temperature

        if isinstance(abins_data, AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Improper value of input Abins data.")

        k_data = self._abins_data["k_points_data"]
        extracted_k_data = k_data.extract()
        self._num_atoms = int(extracted_k_data[0]["atomic_displacements"].shape[0] / float(extracted_k_data[0]["frequencies"].shape[0]))
        self._num_k = len(extracted_k_data)
        self._num_freq = extracted_k_data[0]["frequencies"].shape[0]


    def _rearrange_atoms_displacements(self, k_point=None):
        """
        @return: rearranged atoms displacements in the 2D numpy array for one k-point. Each entry corresponds to atom displacements for one atom 
        """
        
        _atoms_displacements = []
        for ion in range(self._num_atoms):
            _atom_displacement = []
            for freq in range(self._num_freq):
                _atom_displacement.append(self._abins_data[k_point]["atomic_displacements"][ion + freq * self._num_atoms])
            _atoms_displacements.append(_atom_displacement)

            # _atoms_displacements[atom, coord_1, coord_2, coord_3]
            # atom - index for atom
            # coord1 - x coordinate of atom
            # coord2 - y coordinate of atom
            # coord3 - z coordinate of atom

        return np.asarray(_atoms_displacements)


    def _calculate_coth(self, freq=None):
        """

        @param freq: numpy array with frequencies
        @return:
        """
        return 1.0 / np.tanh(Constants.h_bar_inv_cm* freq / (Constants.K * self._temperature) )

    def _calculate_DW(self):
        self._DW = DwData(temperature=self._temperature, num_atoms=self._num_atoms)

        for k in range(self._num_k):
            _atoms_displacements = self._rearrange_atoms_displacements(k_point=k)
            _freq = self._abins_data[k]["frequencies"]
            _bose_factor = self._calculate_Bose_factor(freq=_freq)


        else:
            raise  ValueError("Only Powder scenario implemented at the moment.")


    def getDW(self):


            data = self._calculate_DW()

            self.addAttribute("temperature", self._temperature)
            self.addAttribute("filename", self._input_filename)
            self.addNumpyDataset("data", data)

            self.save()


    def loadData(self):
        """
        Loads Debye-Waller factors in the form of DwData from hdf file.
        @return: Debye-Waller factors (DwData)
        """
        _data = self.load(list_of_numpy_datasets=["data"], list_of_attributes=["temperature"])
        _num_atoms = len(_data.shape[0])
        _dw_data = DwData(num_atoms=_num_atoms, temperature=_data["attributes"]["temperature"])
        _dw_data.set(_data["datasets"]["data"])
        return _dw_data
