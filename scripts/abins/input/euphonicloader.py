# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from pathlib import Path

from .abinitioloader import AbInitioLoader
from abins.parameters import sampling as sampling_parameters

from dos.load_euphonic import euphonic_available, euphonic_calculate_modes


class EuphonicLoader(AbInitioLoader):
    """Get frequencies/eigenvalues from force constants using Euphonic"""

    def __init__(self, input_ab_initio_filename):
        """

        :param input_ab_initio_filename: name of file with phonon data (foo.phonon)
        """
        if not isinstance(input_ab_initio_filename, str):
            raise TypeError('Filename must be a string')
        elif not Path(input_ab_initio_filename).is_file():
            raise IOError(f'Ab initio file {input_ab_initio_filename} not found.')

        super().__init__(input_ab_initio_filename=input_ab_initio_filename)
        self._ab_initio_program = "FORCECONSTANTS"

    def read_vibrational_or_phonon_data(self):
        """Get AbinsData (structure and modes) from force constants data.

        Frequencies/displacements are interpolated using the Euphonic library
        over a regular q-point mesh. The mesh is determined by a Moreno-Soler
        realspace cutoff, related to the size of an equivalent
        supercell. Meshes are rounded up so a very small cutoff will yield
        gamma-point-only sampling.

        """
        if not euphonic_available():
            raise ImportError("Could not import Euphonic library; this is "
                              "required to import force constants from Phonopy or .castep_bin.")

        cutoff = sampling_parameters['force_constants']['qpt_cutoff']
        modes = euphonic_calculate_modes(filename=self._clerk.get_input_filename(), cutoff=cutoff)

        unit_cell = modes.crystal.cell_vectors.to('angstrom').magnitude
        atoms = {f'atom_{atom_index}': {'symbol': str(atom_type),
                                        'sort': atom_index,
                                        'coord': unit_cell.T.dot(atom_r),
                                        'mass': mass}
                 for atom_index, atom_type, atom_r, mass in zip(range(modes.crystal.n_atoms),
                                                                modes.crystal.atom_type,
                                                                modes.crystal.atom_r,
                                                                modes.crystal.atom_mass.to('amu').magnitude)}

        file_data = {'frequencies': modes.frequencies.to('1/cm').magnitude,
                     'weights': modes.weights,
                     'k_vectors': modes.qpts,
                     'atomic_displacements': np.swapaxes(modes.eigenvectors, 1, 2),
                     'unit_cell': unit_cell,
                     'atoms': atoms}

        # save stuff to hdf file
        save_keys = ["frequencies", "weights", "k_vectors", "atomic_displacements", "unit_cell", "atoms"]
        data_to_save = {key: file_data[key] for key in save_keys}
        self.save_ab_initio_data(data=data_to_save)

        return self._rearrange_data(data=file_data)
