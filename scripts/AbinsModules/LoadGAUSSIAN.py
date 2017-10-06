from __future__ import (absolute_import, division, print_function)
import AbinsModules
import io
import numpy as np
from mantid.kernel import Atom


class LoadGAUSSIAN(AbinsModules.GeneralDFTProgram):
    """
    Class for loading GAUSSIAN ab initio vibrational data.
    """
    def __init__(self, input_dft_filename):
        """
        :param input_dft_filename: name of file with phonon data (foo.log)
        """
        super(LoadGAUSSIAN, self).__init__(input_dft_filename=input_dft_filename)
        self._dft_program = "GAUSSIAN"
        self._parser = AbinsModules.GeneralDFTParser()
        self._num_atoms = None
        self._num_read_freq = 0

    def read_phonon_file(self):
        """
        Reads phonon data from GAUSSIAN output files. Saves frequencies and atomic displacements (only molecular
        calculations), hash of the phonon  file (hash) to <>.hdf5.
        :returns: object of type AbinsData.
        """

        data = {}  # container to store read data

        with io.open(self._clerk.get_input_filename(), "rb", ) as gaussian_file:

            # create dummy lattice vectors
            self._generates_lattice_vectors(data=data)

            # move file pointer to the last optimized atomic positions
            self._parser.find_last(file_obj=gaussian_file, msg="Input orientation:")
            self._read_atomic_coordinates(file_obj=gaussian_file, data=data)

            # read frequencies, corresponding atomic displacements for a molecule
            self._parser.find_first(file_obj=gaussian_file,
                                    msg="Harmonic frequencies (cm**-1), IR intensities (KM/Mole), Raman scattering")
            self._read_modes(file_obj=gaussian_file, data=data)

            # save data to hdf file
            self.save_dft_data(data=data)

            # return AbinsData object
            return self._rearrange_data(data=data)

    def _read_atomic_coordinates(self, file_obj=None, data=None):
        """
        Reads atomic coordinates from .log GAUSSIAN file.
        :param file_obj: file object from which we read
        :param data: Python dictionary to which atoms data should be added
        """
        atoms = {}
        atom_indx = 0
        end_msgs = ["---------------------------------------------------------------------"]

        header_lines = 5
        #                         Input orientation:
        # ---------------------------------------------------------------------
        # Center     Atomic     Atomic              Coordinates (Angstroms)
        # Number     Number      Type              X           Y           Z
        # ---------------------------------------------------------------------
        for i in range(header_lines):
            file_obj.readline()

        while not self._parser.block_end(file_obj=file_obj, msg=end_msgs):

            line = file_obj.readline()
            entries = line.split()
            z_number = int(entries[1])
            atom = Atom(z_number=z_number)
            coord = np.asarray([float(i) for i in entries[3:6]])
            atoms["atom_{}".format(atom_indx)] = {"symbol": atom.symbol, "mass": atom.mass, "sort": atom_indx,
                                                  "coord": coord}

            atom_indx += 1
        self._num_atoms = len(atoms)
        data["atoms"] = atoms

    def _generates_lattice_vectors(self, data=None):
        """
        Generates dummy lattice vectors. Gaussian is only for molecular calculations.
        :param obj_file: file object from which we read
        :param data: Python dictionary to which found lattice vectors should be added
        """
        data["unit_cell"] = np.zeros(shape=(3, 3), dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

    def _read_modes(self, file_obj=None, data=None):
        """
        Reads vibrational modes (frequencies and atomic displacements).
        :param file_obj: file object from which we read
        :param data: Python dictionary to which k-point data should be added
        """
        freq = []
        # it is a molecule so we subtract 3 translations and 3 rotations
        num_freq = 3 * self._num_atoms - AbinsModules.AbinsConstants.ROTATIONS_AND_TRANSLATIONS
        dim = 3
        atomic_disp = np.zeros(shape=(num_freq, self._num_atoms, dim), dtype=AbinsModules.AbinsConstants.COMPLEX_TYPE)
        end_msg = ["-------------------"]
        # Next block is:
        # -------------------
        # - Thermochemistry -
        # -------------------

        # parse block with frequencies and atomic displacements
        while not (self._parser.block_end(file_obj=file_obj, msg=end_msg) or self._parser.file_end(file_obj=file_obj)):

            self._read_freq_block(file_obj=file_obj, freq=freq)
            self._read_atomic_disp_block(file_obj=file_obj, disp=atomic_disp)

        data["frequencies"] = np.asarray([freq]).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE, casting="safe")

        # we mimic that we have one Gamma k-point
        data["k_vectors"] = np.asarray([[0.0, 0.0, 0.0]]).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE,
                                                                 casting="safe")
        data["weights"] = np.asarray([1.0])

        # Normalize displacements so that Abins can use it to create its internal data objects
        # num_atoms: number of atoms in the system
        # num_freq: number of modes
        # dim: dimension for each atomic displacement (atoms vibrate in 3D space)
        self._num_k = 1

        # normalisation
        # atomic_disp [num_freq, num_atoms, dim]
        # masses [num_atoms]

        masses = np.asarray([data["atoms"]["atom_%s" % atom]["mass"] for atom in range(self._num_atoms)])

        # [num_freq, num_atoms, dim] -> [num_freq, num_atoms, dim, dim] -> [num_freq, num_atoms]
        temp1 = np.trace(np.einsum("lki, lkj->lkij", atomic_disp, atomic_disp), axis1=2, axis2=3)
        temp2 = np.einsum("ij, j->ij", temp1, masses)

        # [num_freq, num_atoms] -> [num_freq]
        norm = np.sum(temp2, axis=1)
        # noinspection PyTypeChecker
        atomic_disp = np.einsum("ijk,i->ijk", atomic_disp, 1.0 / np.sqrt(norm))
        atomic_disp = np.einsum("ijk,j->ijk", atomic_disp, np.sqrt(masses))

        # [num_freq, num_atoms, dim] ->  [num_k, num_atoms, num_freq, dim]
        data["atomic_displacements"] = np.asarray([np.transpose(a=atomic_disp, axes=(1, 0, 2))])

    def _read_freq_block(self, file_obj=None, freq=None):
        """
        Parses block with frequencies.
        :param file_obj: file object from which we read
        :param freq: list with frequencies which we update
        """
        line = self._parser.find_first(file_obj=file_obj, msg="Frequencies --")
        line = line.split()
        freq.extend([float(i) for i in line[2:]])

    def _read_atomic_disp_block(self, file_obj=None, disp=None):
        """
        Parses block with atomic displacements.
        :param file_obj: file object from which we read
        :param disp: list with x coordinates which we update [num_freq, num_atoms, dim]
        """
        sub_block_start = "X      Y      Z        X      Y      Z        X      Y      Z"
        self._parser.find_first(file_obj=file_obj, msg=sub_block_start)

        num_atom = 0
        #   Atom  AN      X      Y      Z        X      Y      Z        X      Y      Z
        line_size = len(sub_block_start.split()) + 2
        freq_per_line = sub_block_start.count("X")

        l = file_obj.readline().split()
        while len(l) == line_size:
            for f in range(freq_per_line):
                disp[self._num_read_freq + f, num_atom, 0] = complex(float(l[2 + 3 * f]), 0)
                disp[self._num_read_freq + f, num_atom, 1] = complex(float(l[3 + 3 * f]), 0)
                disp[self._num_read_freq + f, num_atom, 2] = complex(float(l[4 + 3 * f]), 0)
            l = file_obj.readline().split()
            num_atom += 1
        self._num_read_freq += freq_per_line
