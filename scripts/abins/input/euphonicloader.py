# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

from .abinitioloader import AbInitioLoader
from abins.parameters import sampling as sampling_parameters

from dos.load_euphonic import euphonic_available, euphonic_calculate_modes


class EuphonicLoader(AbInitioLoader):
    """Get frequencies/eigenvalues from force constants using Euphonic"""

    def __init__(self, input_ab_initio_filename):
        """

        :param input_ab_initio_filename: name of file with phonon data (foo.phonon)
        """
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
        data_to_save = ["frequencies", "weights", "k_vectors", "atomic_displacements", "unit_cell", "atoms"]
        data = {}
        for key in data_to_save:
            data[key] = file_data[key]

        self.save_ab_initio_data(data=data)
        return self._rearrange_data(data=file_data)

        # @abstractmethod
        # def read_vibrational_or_phonon_data(self):
        """
        This method is different for different ab initio programs. It has to be overridden by inheriting class.
        This method reads vibrational or phonon data produced by an ab initio program.
        This method should do the following:

          1) Open file with vibrational or phonon data (CASTEP: foo.phonon). Name of a file is
             accessed via self._clerk.get_input_filename(). There must be no spaces in the name of a
             file. Extension of a file (part of a name after '.') is arbitrary.

          2) Method should read from an ab initio file information about frequencies, atomic displacements,
          k-point vectors, weights of k-points and ions.

          3) Method should reconstruct data for symmetry equivalent k-points
             (protected method _recover_symmetry_points).

             **Notice: this step is not implemented now. At the moment only Gamma point calculations are supported.**

          4) Method should determine symmetry equivalent atoms

              **Notice: this step is not implemented now.**

          5) Method should calculate hash of a file with vibrational or phonon data (protected method _calculateHash).

          6) Method should store vibrational or phonon data in an hdf file (using inherited method
             save_ab_initio_data()). The name of an hdf file is foo.hdf5 (CASTEP: foo.phonon -> foo.hdf5). In order to
             save the data to hdf file the following fields should be set:

                    self._hdf_filename
                    self._group_name
                    self._attributes
                    self._datasets

              The datasets should be a dictionary with the following entries:

                        "frequencies"  - frequencies for all k-points grouped in one numpy.array in cm^-1

                        "weights"      - weights of all k-points in one numpy.array

                        "k_vectors"    - all k-points in one numpy array

                                         **Notice: both symmetry equivalent and inequivalent points should be stored; at
                                          the moment only Gamma point calculations are supported**

                        "atomic_displacements" - atomic displacements for all atoms and all k-points in one numpy array
                                                 indexed: (kpt, atom, mode, direction)

                        "unit_cell"      -   numpy array with unit cell vectors in Angstroms

              The following structured datasets should be also defined:

                        "atoms"          - Python dictionary with the information about ions. Each entry in the
                                           dictionary has the following format 'atom_n'. Here n means number of
                                           atom in the unit cell.

                                           Each entry 'atom_n' in the  dictionary is a dictionary with the following
                                           entries:

                                               "symbol" - chemical symbol of the element (for example hydrogen -> H)

                                               "sort"   - defines symmetry equivalent atoms, e.g, atoms with the same
                                                          sort are symmetry equivalent

                                                          **Notice at the moment this parameter is not functional
                                                            in LoadCastep**

                                               "coord" - equilibrium position of atom in Angstroms;
                                                         it has a form of numpy array with three floats

                                               "mass" - mass of atom

              The attributes should be a dictionary with the following entries:

                        "hash"  - hash of a file with the vibrational or phonon data. It should be a string
                                  representation of hash.

                        "ab_initio_program" - name of the ab initio program which was used to obtain vibrational or
                                              phonon data (for CASTEP -> CASTEP).

                        "filename" - name of input ab initio file

          For more details about these fields please look at the documentation of abins.IO class.

        :returns: Method should return an object of type AbinsData.

        """
