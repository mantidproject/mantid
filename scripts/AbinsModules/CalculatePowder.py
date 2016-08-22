import numpy as np

from IOmodule import  IOmodule
from PowderData import PowderData
from QData import QData
from AbinsData import AbinsData
import AbinsParameters

class CalculatePowder(IOmodule):
    """
    Class for calculating mean square displacements (MSD) for the powder use case.
    Mean square displacements are in Hartree atomic units.
    Working equation is taken from http://atztogo.github.io/phonopy/thermal-displacement.html.
    Additionally Debye-Waller factors are calculated which are  MSD multiplied by coth(omega/2T)^2 for each frequency.
    They have to be multiplied by Q^2 to be equivalent to DW from "Vibrational spectroscopy with neutrons...." .
    Obtained MSD and DW  can be directly used for calculation of S(Q, omega) in case sample has a form of powder.
    """
    def __init__(self, filename=None, abins_data=None, temperature=None):
        """

        @param filename:  name of input DFT filename
        @param abins_data: object of type AbinsData with data from input DFT file
        @param temperature:  temperature in K
        """


        if not isinstance(abins_data, AbinsData):
            raise ValueError("Object of AbinsData was expected.")
        self._abins_data = abins_data

        if not (isinstance(temperature, int) or isinstance(temperature, float)):
            raise ValueError("Invalid value of temperature.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature) # temperature in K

        super(CalculatePowder, self).__init__(input_filename=filename, group_name=AbinsParameters.powder_data_group+ "/" + "%sK"%self._temperature)


    def _calculate_powder(self):
        """
        MSD are calculated according to http://atztogo.github.io/phonopy/thermal-displacement.html.
        MSD are expressed in Hartree atomic units. Additionally DW are calculated. DW are MSD multiplied by
        coth(omega/2T)^2 for every frequency.
        """

        _data = self._abins_data.extract()

        weights = _data["k_points_data"]["weights"]
        mass_hartree_factor  = [1.0 / (2 * atom["mass"]) for atom in _data["atoms_data"]]
        temperature_hartree = self._temperature * AbinsParameters.k_2_hartree
        freq_hartree = _data["k_points_data"]["frequencies"]
        displacements = _data["k_points_data"]["atomic_displacements"]

        num_k = displacements.shape[0]
        num_atoms = displacements.shape[1]
        num_freq = displacements.shape[2]

        exp_factor = 1.0 / temperature_hartree
        _coth_factor = 1.0 / (2.0 * temperature_hartree) # coth( _coth_factor * omega)

        _powder = PowderData(temperature=self._temperature, num_atoms=num_atoms)

        _powder_atom = {"msd": 0.0, "dw":0}

        _coth = np.divide(1.0, np.tanh(np.multiply(_coth_factor, freq_hartree)))
        _coth_square = np.multiply(_coth, _coth)
        expm1 = np.expm1(np.multiply(exp_factor, freq_hartree))

        one_over_freq = np.divide(1.0, freq_hartree)
        two_over_freq_n = np.divide(2.0, np.multiply(freq_hartree, expm1))

        for atom in range(num_atoms):

            temp_msd_k = 0.0
            temp_dw_k = 0.0

            for k in range(num_k):

                temp_msd_freq = 0.0
                temp_dw_freq = 0.0

                # correction for acoustic modes at Gamma point
                if np.linalg.norm(_data["k_points_data"]["k_vectors"][k]) < AbinsParameters.small_k: start = 3
                else: start = 0

                for freq in range(start, num_freq):

                    disp = displacements[k, atom, freq, :]
                    temp = np.vdot(disp, disp).real * (one_over_freq[k, freq] + two_over_freq_n[k, freq])
                    temp_msd_freq += temp
                    temp_dw_freq += temp * _coth_square[k, freq]

                weight_k = weights[k]
                temp_msd_k += temp_msd_freq *  weight_k
                temp_dw_k += temp_dw_freq * weight_k

            mass_factor = mass_hartree_factor[atom]
            _powder_atom["msd"] = temp_msd_k * mass_factor
            _powder_atom["dw"] = temp_dw_k * mass_factor

            _powder._append(num_atom=atom, powder_atom=_powder_atom)

        return _powder


    def calculateData(self):
        """
        Calculates mean square displacements and Debye-Waller factors.  Saves both MSD and DW  to an hdf file.
        @return: object of type PowderData with mean square displacements and Debye-Waller factors.
        """

        data = self._calculate_powder()

        self.addFileAttributes()
        self.addStructuredDataset("powder_data", data.extract())

        self.save()

        return data


    def loadData(self):
        """
        Loads mean square displacements and Debye-Waller factors from an hdf file.
        @return: object of type PowderData with mean square displacements and Debye-Waller factors.
        """
        _data = self.load(list_of_structured_datasets=["powder_data"])
        _powder_data = PowderData(temperature=self._temperature,
                                  num_atoms=_data["structured_datasets"]["powder_data"]["msd"].shape[0])

        _powder_data.set(_data["structured_datasets"]["powder_data"])

        return _powder_data




