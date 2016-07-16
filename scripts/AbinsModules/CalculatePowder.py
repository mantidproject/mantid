import numpy as np

from IOmodule import  IOmodule
from PowderData import PowderData
from AbinsData import AbinsData
import Constants

class CalculatePowder(IOmodule):
    """
    Class for calculating means square displacements (MSD).
    Means square displacements are in Hartree atomic units.
    Working equation is taken from http://atztogo.github.io/phonopy/thermal-displacement.html.
    Additionally MSD are multiplied by coth(omega/2T)^2 for each frequency.  MSD can be directly used for calculation of
    S(Q, omega) in case sample has form of powder.
    """
    def __init__(self, filename=None, abins_data=None, temperature=None):
        super(CalculatePowder, self).__init__(input_filename=filename, group_name=Constants.MSD_data_group)

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
        MSD are expressed in Hartree atomic units. Additionally for every frequency they are multiplied by
        coth(omega/2T)^2.
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

            _msd_atom = {"msd": 0.0, "dw":0}
            _coth = 1.0 / np.tanh(_coth_factor * freq_hartree)
            _coth_square = np.multiply(_coth, _coth)
            expm1 = np.expm1(freq_hartree * exp_factor)
            one_over_freq = 1.0 / freq_hartree
            two_over_freq = 2.0 / (np.multiply(freq_hartree, expm1))

            for k in range(num_k):

                # correction for acoustic modes at Gamma point
                if np.linalg.norm(_data["k_points_data"]["k_vectors"][k]) < Constants.small_k: start = 2
                else: start = 0

                for freq in range(start, num_freq):
                    disp = displacements[k, atom, freq, :]
                    _msd_atom["msd"] += np.vdot(disp, disp).real * (one_over_freq[k, freq] + two_over_freq[k, freq])
                    _msd_atom["dw"] = _msd_atom["dw"] * _coth_square[k, freq]
                _msd_atom["msd"] += _msd_atom["msd"] * weights[k]
                _msd_atom["dw"] += _msd_atom["dw"] * weights[k]

            _msd._append(num_atom=atom, msd_atom=_msd_atom * mass_hartree_factor[atom])

        return _msd


    def getMSD(self):

        data = self._calculate_msd()

        self.addAttribute("temperature", self._temperature)
        self.addAttribute("filename", self._input_filename)
        self.addNumpyDataset("data", data.extract())

        self.save()

        return data


    def loadData(self):

        _data = self.load(list_of_numpy_datasets=["data"], list_of_attributes=["temperature"])
        _num_atoms = _data["datasets"]["data"].shape[0]
        _msd_data = PowderData(temperature=_data["attributes"]["temperature"], num_atoms=_num_atoms)
        _msd_data.set(_data["datasets"]["data"])

        return _msd_data




