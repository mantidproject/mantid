from __future__ import (absolute_import, division, print_function)
import AbinsModules
import io
import six
import numpy as np
from math import sqrt
from mantid.kernel import Atom


class LoadDMOL3(AbinsModules.GeneralDFTProgram):
    """
    Class for loading DMOL3 DFT phonon data.
    """
    def __init__(self, input_dft_filename):
        """
        @param input_dft_filename: name of file with phonon data (foo.outmol)
        """
        super(LoadDMOL3, self).__init__(input_dft_filename=input_dft_filename)
        self._dft_program = "DMOL3"

    def read_phonon_file(self):
        """
        Reads phonon data from DMOL3 output files. Saves frequencies, weights of k-point vectors, k-point vectors,
        amplitudes of atomic displacements, hash of the phonon file (hash) to <>.hdf5
        :return: object of type AbinsData.
        """
        data = {}  # container to store read data

        with io.open(self._clerk.get_input_filename(), "rb", ) as dmol3_file:

            # Move read file pointer to the last calculation recorded in the .outmol file. First calculation could be
            # geometry optimization. The last calculation in the file is expected to be calculation of vibrational
            # data. There may be some intermediate resume calculations.
            self._find_last(file_obj=dmol3_file, msg=b"$cell vectors")

            # read lattice vectors
            self._read_lattice_vectors(obj_file=dmol3_file, data=data)

            # read info about atoms and construct atom data
            self._read_atomic_coordinates(file_obj=dmol3_file, data=data)

            # read frequencies, corresponding atomic displacements and construct k-points data
            self._find_first(file_obj=dmol3_file, msg="Frequencies (cm-1) and normal modes ")
            self._read_modes(file_obj=dmol3_file, data=data)

            # save data to hdf file
            self.save_dft_data(data=data)

            # return AbinsData object
            return self._rearrange_data(data=data)

    def _find_first(self, file_obj=None, msg=None):
        """
        Finds the first line with msg. Moves file current position to the next line.
        :param file_obj: file object from which we read
        :param msg: keyword to find
        """
        if six.PY3:
            msg = bytes(msg, "utf8")
        while not self._file_end(file_obj=file_obj):
            line = file_obj.readline()
            if line.strip() and msg in line:
                break

    def _find_last(self, file_obj=None, msg=None):
        """
        Moves file current position to the last occurrence of msg.
        :param file_obj: file object from which we read
        :param msg: keyword to find
        """
        if six.PY3:
            msg = bytes(msg, "utf8")

        found = False
        last_entry = None

        while not self._file_end(file_obj=file_obj):
            pos = file_obj.tell()
            line = file_obj.readline()
            if line.strip() and msg in line:
                last_entry = pos
                found = True

        if not found:
            raise ValueError("No entry " + msg + " has been found.")
        else:
            file_obj.seek(last_entry)

    def _file_end(self, file_obj=None):
        """
        Checks end of the text file.
        :param file_obj: file object which was open in "r" mode
        :return: True if end of file, otherwise False
        """
        n = AbinsModules.AbinsConstants.ONE_CHARACTER
        pos = file_obj.tell()
        potential_end = file_obj.read(n)
        if potential_end == AbinsModules.AbinsConstants.EOF:
            return True
        else:
            file_obj.seek(pos)
            return False

    def _read_lattice_vectors(self, obj_file=None, data=None):
        """
        Reads lattice vectors from .outmol DMOL3 file.
        :param obj_file: file object from which we read
        :param data: Python dictionary to which found lattice vectors should be added
        """
        self._find_first(file_obj=obj_file, msg=b"$cell vectors")

        dim = 3
        vectors = []
        for i in range(dim):
            line = obj_file.readline().split()
            vector = map(self._convert_to_angstroms, line)
            vectors.append(vector)

        data["unit_cell"] = np.asarray(vectors).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

    def _convert_to_angstroms(self, string=None):
        """
        :param string: string with number
        :return: converted coordinate of lattice vector to Angstroms
        """
        au2ang = AbinsModules.AbinsConstants.ATOMIC_LENGTH_2_ANGSTROM
        return float(string) * au2ang

    def _read_atomic_coordinates(self, file_obj=None, data=None):
        """
        Reads atomic coordinates from .outmol DMOL3 file.
        :param file_obj: file object from which we read
        :param data: Python dictionary to which atoms data should be added
        """
        atoms = {}
        atom_indx = 0
        self._find_first(file_obj=file_obj, msg="$coordinates")
        end_msgs = ["$end", "----------------------------------------------------------------------"]

        while not self._block_end(file_obj=file_obj, msg=end_msgs):

            line = file_obj.readline()
            entries = line.split()

            symbol = str(entries[0].decode("utf-8").capitalize())
            atom = Atom(symbol=symbol)
            # We change unit of atomic displacements from atomic length units to Angstroms
            au2ang = AbinsModules.AbinsConstants.ATOMIC_LENGTH_2_ANGSTROM
            float_type = AbinsModules.AbinsConstants.FLOAT_TYPE

            atoms["atom_{}".format(atom_indx)] = {"symbol": symbol, "mass": atom.mass, "sort": atom_indx,
                                                  "coord": np.asarray(entries[1:]).astype(dtype=float_type) * au2ang}

            atom_indx += 1

        data["atoms"] = atoms

    def _read_modes(self, file_obj=None, data=None):
        """
        Reads vibrational modes (frequencies and atomic displacements).
        :param file_obj: file object from which we read
        :param data: Python dictionary to which k-point data should be added
        """
        end_msgs = ["STANDARD"]
        freq = []
        xdisp = []
        ydisp = []
        zdisp = []

        # parse block with frequencies and atomic displacements
        while not (self._block_end(file_obj=file_obj, msg=end_msgs) or self._file_end(file_obj=file_obj)):

            self._read_freq_block(file_obj=file_obj, freq=freq)
            self._read_coord_block(file_obj=file_obj, xdisp=xdisp, ydisp=ydisp, zdisp=zdisp)

        freq = [freq]
        weights = [1.0]
        k_coordinates = [[0.0, 0.0, 0.0]]
        displacements = [[xdisp, ydisp, zdisp]]

        self._num_modes = len(freq[0])
        if self._num_modes % 3 == 0:
            self._num_atoms = int(self._num_modes / 3)
        else:
            raise ValueError("Invalid number of modes.")

        self._num_k = 1  # Only Gamma point

        data["frequencies"] = np.asarray(freq).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE, casting="safe")
        data["k_vectors"] = np.asarray(k_coordinates).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE,
                                                             casting="safe")
        data["weights"] = np.asarray(weights).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE, casting="safe")

        num_freq = len(freq[0])
        num_atoms = len(data["atoms"])

        # Reshape displacements so that Abins can use it to create its internal data objects
        # num_atoms: number of atoms in the system
        # num_freq: number of modes
        # dim: dimension for each atomic displacement (atoms vibrate in 3D space)
        #
        # The following conversion is necessary:
        # (num_freq * num_atom * dim) -> (num_freq, num_atom, dim) -> (num_atom, num_freq, dim)
        dim = 3

        # displacements[num_freq, num_atom, dim]
        displacements = np.reshape(a=np.asarray(a=displacements, order="C"), newshape=(num_freq, num_atoms, dim))

        # Normalise atomic displacements so that the sum of all atomic
        # displacements in any normal mode is normalised (equal 1).
        # num_freq, num_atom, dim -> num_freq, num_atom, dim, dim -> num_freq, num_atom -> num_freq
        norm = np.sum(np.trace(np.einsum('lki, lkj->lkij', displacements, displacements.conjugate()),
                               axis1=2, axis2=3), axis=1)
        displacements = np.einsum('ijk, i-> ijk', displacements, 1.0 / np.sqrt(norm))

        # displacements[num_atoms, num_freq, dim]
        displacements = np.transpose(a=displacements, axes=(1, 0, 2))

        # displacements[num_k, num_atoms, num_freq, dim]
        # with num_k = 1
        data["atomic_displacements"] = np.asarray([displacements])

    def _block_end(self, file_obj=None, msg=None):
        """
        Checks for msg which terminates block.
        :param file_obj: file object from which we read
        :param msg: list with messages which end kpoint block.
        :return: True if end of block otherwise False
        """
        for item in msg:
            pos = file_obj.tell()
            line = file_obj.readline()
            file_obj.seek(pos)
            if six.PY3:
                item = bytes(item, "utf8")
            if item in line:
                return True
        return False

    def _read_freq_block(self, file_obj=None, freq=None):
        """
        Parses block with frequencies.
        :param file_obj: file object from which we read
        :param freq: list with frequencies which we update
        """
        self._move_to(file_obj=file_obj, msg=":")
        items = file_obj.readline().replace(b"\n", b" ").split()
        length = len(items)
        for i in range(1, length, 2):
            freq.append(float(items[i]))

    def _read_coord_block(self, file_obj=None, xdisp=None, ydisp=None, zdisp=None, part="real"):
        """
        Parses block with coordinates.
        :param file_obj: file object from which we read
        :param xdisp: list with x coordinates which we update
        :param ydisp: list with y coordinates which we update
        :param zdisp: list with z coordinates which we update
        """
        self._move_to(file_obj=file_obj, msg=" x ")

        atom_mass = None

        while not self._file_end(file_obj=file_obj):

            pos = file_obj.tell()
            line = file_obj.readline()

            if line.strip():
                line = line.strip(b"\n").split()
                if b"x" in line:
                    symbol = str(line[0].decode("utf-8").capitalize())
                    atom_mass = Atom(symbol=symbol).mass
                    for item in line[2:]:
                        self._parse_item(item=item, container=xdisp, part=part, mass=atom_mass)
                elif b"y" in line:
                    for item in line[1:]:
                        self._parse_item(item=item, container=ydisp, part=part, mass=atom_mass)
                elif b"z" in line:
                    for item in line[1:]:
                        self._parse_item(item=item, container=zdisp, part=part, mass=atom_mass)
                    atom_mass = None
                else:
                    file_obj.seek(pos)
                    break

    def _move_to(self, file_obj=None, msg=None):
        """
        Finds the first line with msg and moves read file pointer to that line.
        :param file_obj: file object from which we read
        :param msg: keyword to find
        """
        if six.PY3:
            msg = bytes(msg, "utf8")
        while not self._file_end(file_obj=file_obj):
            pos = file_obj.tell()
            line = file_obj.readline()
            if line.strip() and msg in line:
                file_obj.seek(pos)
                break

    def _parse_item(self, item=None, container=None, part=None, mass=None):
        """
        Creates atomic displacement from item.
        :param item: string with atomic displacement for the given atom and frequency
        :param container: list to which atomic displacement should be added
        :param part: if real than real part of atomic displacement is created if imaginary then imaginary part is
                     created
        :param mass: mass of atom
        """
        # We multiply by square root of atomic mass so that no modifications are needed in CalculatePowder module
        if part == "real":
            container.append(complex(float(item), 0.0) * sqrt(mass))
        elif part == "imaginary":
            container.append(complex(0.0, float(item)) * sqrt(mass))
        else:
            raise ValueError("Real or imaginary part of complex number was expected.")
