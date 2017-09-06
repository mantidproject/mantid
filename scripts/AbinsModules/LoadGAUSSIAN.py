from __future__ import (absolute_import, division, print_function)
import AbinsModules
import io
import numpy as np
from mantid.kernel import Atom


class LoadGAUSSIAN(AbinsModules.GeneralDFTProgram):
    """
    Class for loading GAUSSIAN DFT vibrational data.
    """
    def __init__(self, input_dft_filename):
        """
        :param input_dft_filename: name of file with phonon data (foo.log)
        """
        super(LoadGAUSSIAN, self).__init__(input_dft_filename=input_dft_filename)
        self._dft_program = "GAUSSIAN"
        self._parser = AbinsModules.GeneralDFTParser()

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
                                                  "coord":coord}

            atom_indx += 1

        data["atoms"] = atoms

    def _generates_lattice_vectors(self, data=None):
        """
        Generates dummy lattice vectors. Gaussian is only for molecular calculations.
        :param obj_file: file object from which we read
        :param data: Python dictionary to which found lattice vectors should be added
        """
        data["unit_cell"] = np.zeros(shape=(3,3), dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

    def _read_modes(self, file_obj=None, data=None):
        """
        Reads vibrational modes (frequencies and atomic displacements).
        :param file_obj: file object from which we read
        :param data: Python dictionary to which k-point data should be added
        """
        freq = []
        atomic_disp = []
        end_msg = ["-------------------"]
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

        # Reshape displacements so that Abins can use it to create its internal data objects
        # num_atoms: number of atoms in the system
        # num_freq: number of modes
        # dim: dimension for each atomic displacement (atoms vibrate in 3D space)

        dim = 3
        num_freq = len(freq)
        self._num_k = 1
        self._num_atoms = len(data["atoms"])

        # displacements[num_freq, dim, num_atoms]
        displacements = np.reshape(a=np.asarray(a=atomic_disp, order="C"), newshape=(num_freq, dim, self._num_atoms))

        # [num_freq, dim, num_atoms] -> [num_atoms, num_freq, dim]
        displacements = np.transpose(a=displacements, axes=(2, 0, 1))

        data["atomic_displacements"] = np.asarray([displacements])

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
        :param disp: list with x coordinates which we update
        """
        sub_block_start = "Atom AN      X      Y      Z        X      Y      Z        X      Y      Z"
        self._parser.find_first(file_obj=file_obj, msg=sub_block_start)
        l = file_obj.readline().split()
        while len(l) == len(sub_block_start.split()):
            disp.extend([complex(float(i), 0) for i in l[2:]])
            l = file_obj.readline().split()
