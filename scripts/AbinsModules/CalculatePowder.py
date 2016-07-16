import numpy as np

from IOmodule import  IOmodule
from PowderData import PowderData
from AbinsData import AbinsData
import Constants

class CalculatePowder(IOmodule):
    """
    Class for calculating mean square displacements (MSD) for the powder use case.
    Mean square displacements are in Hartree atomic units.
    Working equation is taken from http://atztogo.github.io/phonopy/thermal-displacement.html.
    Additionally Debye-Waller factors are calculated which are  MSD multiplied by coth(omega/2T)^2 for each frequency.
    Obtained MSD and DW  can be directly used for calculation of S(Q, omega) in case sample has a form of powder.
    """
    def __init__(self, filename=None, abins_data=None, temperature=None):
        super(CalculatePowder, self).__init__(input_filename=filename, group_name=Constants.powder_data_group)

        if not isinstance(abins_data, AbinsData):
            raise ValueError("Object of AbinsData was expected.")
        self._abins_data = abins_data

        if not (isinstance(temperature, int) or isinstance(temperature, float)):
            raise ValueError("Invalid value of temperature.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = temperature # temperature in K


    def _calculate_msd(self):
        """
        MSD are calculated according to http://atztogo.github.io/phonopy/thermal-displacement.html.
        MSD are expressed in Hartree atomic units. Additionally DW are calculated. DW are MSD for every frequency
        multiplied by coth(omega/2T)^2.
        """

        _data = self._abins_data.extract()

        weights = _data["k_points_data"]["weights"]
        mass_hartree_factor  = [1.0 / (2 * atom["mass"]) for atom in _data["atoms_data"]]
        temperature_hartree = self._temperature * Constants.k_2_hartree
        freq_hartree = _data["k_points_data"]["frequencies"]
        displacements = _data["k_points_data"]["atomic_displacements"]

        num_k = displacements.shape[0]
        num_atoms = displacements.shape[1]
        num_freq = displacements.shape[2]

        exp_factor = 1.0 / temperature_hartree
        _coth_factor = 1.0 / (2.0 * temperature_hartree) # coth( _coth_factor * omega)

        _msd = PowderData(temperature=self._temperature, num_atoms=num_atoms)

        for atom in range(num_atoms):

            _powder_atom = {"msd": 0.0, "dw":0}
            _coth = 1.0 / np.tanh(_coth_factor * freq_hartree)
            _coth_square = np.multiply(_coth, _coth)
            expm1 = np.expm1(exp_factor * freq_hartree)
            one_over_freq = 1.0 / freq_hartree
            two_over_freq_n = 2.0 / (np.multiply(freq_hartree, expm1))

            for k in range(num_k):

                # correction for acoustic modes at Gamma point
                if np.linalg.norm(_data["k_points_data"]["k_vectors"][k]) < Constants.small_k: start = 2
                else: start = 0

                for freq in range(start, num_freq):

                    disp = displacements[k, atom, freq, :]
                    _powder_atom["msd"] += np.vdot(disp, disp).real * (one_over_freq[k, freq] + two_over_freq_n[k, freq])
                    _powder_atom["dw"] = _powder_atom["msd"] * _coth_square[k, freq]
                _powder_atom["msd"] += _powder_atom["msd"] * weights[k]
                _powder_atom["dw"] += _powder_atom["dw"] * weights[k]

            _powder_atom["msd"] += _powder_atom["msd"] * mass_hartree_factor[atom]
            _powder_atom["dw"] += _powder_atom["dw"] * mass_hartree_factor[atom]
            _msd._append(num_atom=atom, powder_atom=_powder_atom)

        return _msd


    def getPowder(self):
        """
        Calculates mean square displacements and Debye-Waller factors.  Saves both MSD and DW  to an hdf file.
        @return: object of type PowderData with mean square displacements and Debye-Waller factors.
        """

        data = self._calculate_msd()

        self.addAttribute("temperature", self._temperature)
        self.addAttribute("filename", self._input_filename)
        self.addStructuredDataset("data", data.extract())

        self.save()

        return data


    def loadData(self):
        """
        Loads mean square displacements and Debye-Waller factors from an hdf file.
        @return: object of type PowderData with mean square displacements and Debye-Waller factors.
        """
        _data = self.load(list_of_structured_datasets=["data"], list_of_attributes=["temperature"])
        _num_atoms = _data["structured_datasets"]["data"].shape[0]
        _msd_data = PowderData(temperature=_data["attributes"]["temperature"], num_atoms=_num_atoms)
        _msd_data.set(_data["structured_datasets"]["data"])

        return _msd_data




