import numpy as np
from IOmodule import IOmodule
from PowderData import PowderData
from AbinsData import AbinsData
import AbinsParameters
import AbinsConstants


class CalculatePowder(IOmodule):
    """
    Class for calculating powder data.
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

        if not isinstance(temperature, (int, float)):
            raise ValueError("Invalid value of temperature.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature)  # temperature in K

        super(CalculatePowder, self).__init__(input_filename=filename,
                                              group_name=AbinsParameters.powder_data_group + "/" +
                                              "%sK" % self._temperature)

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
            if np.linalg.norm(k_data["k_vectors"][k]) < AbinsConstants.small_k:
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
        freq_hartree = k_data["frequencies"][AbinsConstants.first_optical_phonon:]  # frequencies in Hartree units

        # convert to required units
        frequencies = freq_hartree / AbinsConstants.cm1_2_hartree  # convert frequencies to cm^1

        powder = PowderData(num_atoms=num_atoms)

        # Notation for  indices:
        #     num_freq -- number of optical phonons (total number of phonons - acoustic phonons)
        #     num_atoms -- number of atoms
        #     dim -- size of displacement vector for one atom (dim = 3)

        # masses[num_atoms, num_freq]
        masses = np.asarray([([atom["mass"]] * frequencies.size) for atom in atoms_data])
        masses = masses / AbinsConstants.m_2_hartree  # convert  mass of atoms to amu units

        # disp[num_atoms, num_freq, dim]
        disp = displacements[:, AbinsConstants.first_optical_phonon:]

        # factor[num_atoms, num_freq]
        factor = np.einsum('ij,j->ij', masses, AbinsConstants.aCLIMAX_constant / frequencies)

        # b_tensors[num_atoms, num_freq, dim, dim]
        b_tensors = np.einsum('ijkl,ij->ijkl', np.einsum('lki, lkj->lkij', disp, disp).real, factor)

        # a_tensors[num_atoms, dim, dim]
        a_tensors = np.sum(a=b_tensors, axis=1)

        # fill powder object with powder data
        powder.set(dict(b_tensors=b_tensors, a_tensors=a_tensors))

        return powder

    def calculateData(self):
        """
        Calculates mean square displacements and Debye-Waller factors.  Saves both MSD and DW  to an hdf file.
        @return: object of type PowderData with mean square displacements and Debye-Waller factors.
        """

        data = self._calculate_powder()
        self.addFileAttributes()
        self.addData("powder_data", data.extract())
        self.save()

        return data

    def loadData(self):
        """
        Loads mean square displacements and Debye-Waller factors from an hdf file.
        @return: object of type PowderData with mean square displacements and Debye-Waller factors.
        """

        data = self.load(list_of_datasets=["powder_data"])
        powder_data = PowderData(num_atoms=data["datasets"]["powder_data"]["b_tensors"].shape[0])
        powder_data.set(data["datasets"]["powder_data"])

        return powder_data
