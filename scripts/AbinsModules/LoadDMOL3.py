# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import AbinsModules
import io
import six
import numpy as np
from math import sqrt
from mantid.kernel import Atom


class LoadDMOL3(AbinsModules.GeneralAbInitioProgram):
    """
    Class for loading DMOL3 ab initio vibrational data.
    """
    def __init__(self, input_ab_initio_filename):
        """
        :param input_ab_initio_filename: name of file with vibrational data (foo.outmol)
        """
        super(LoadDMOL3, self).__init__(input_ab_initio_filename=input_ab_initio_filename)
        self._ab_initio_program = "DMOL3"
        self._norm = 0
        self._parser = AbinsModules.GeneralAbInitioParser()

    def read_vibrational_or_phonon_data(self):
        """
        Reads vibrational data from DMOL3 output files. Saves frequencies, weights of k-point vectors,
        k-point vectors, amplitudes of atomic displacements, hash of file  with vibrational data to <>.hdf5
        :returns: object of type AbinsData.
        """
        data = {}  # container to store read data

        with io.open(self._clerk.get_input_filename(), "rb", ) as dmol3_file:

            # Move read file pointer to the last calculation recorded in the .outmol file. First calculation could be
            # geometry optimization. The last calculation in the file is expected to be calculation of vibrational data.
            # There may be some intermediate resume calculations.
            self._parser.find_last(file_obj=dmol3_file, msg="$cell vectors")

            # read lattice vectors
            self._read_lattice_vectors(obj_file=dmol3_file, data=data)

            # read info about atoms and construct atom data
            masses = self._read_masses_from_file(obj_file=dmol3_file)
            self._read_atomic_coordinates(file_obj=dmol3_file, data=data, masses_from_file=masses)

            # read frequencies, corresponding atomic displacements and construct k-points data
            self._parser.find_first(file_obj=dmol3_file, msg="Frequencies (cm-1) and normal modes ")
            self._read_modes(file_obj=dmol3_file, data=data)

            # save data to hdf file
            self.save_ab_initio_data(data=data)

            # return AbinsData object
            return self._rearrange_data(data=data)

    def _read_lattice_vectors(self, obj_file=None, data=None):
        """
        Reads lattice vectors from .outmol DMOL3 file.
        :param obj_file: file object from which we read
        :param data: Python dictionary to which found lattice vectors should be added
        """
        self._parser.find_first(file_obj=obj_file, msg="$cell vectors")

        dim = 3
        vectors = []
        for i in range(dim):
            line = obj_file.readline().split()
            vector = [self._convert_to_angstroms(string=s) for s in line]
            vectors.append(vector)

        data["unit_cell"] = np.asarray(vectors).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

    def _convert_to_angstroms(self, string=None):
        """
        :param string: string with number
        :returns: converted coordinate of lattice vector to Angstroms
        """
        au2ang = AbinsModules.AbinsConstants.ATOMIC_LENGTH_2_ANGSTROM
        return float(string) * au2ang

    def _read_atomic_coordinates(self, file_obj=None, data=None, masses_from_file=None):
        """
        Reads atomic coordinates from .outmol DMOL3 file.

        :param file_obj: file object from which we read
        :param data: Python dictionary to which atoms data should be added
        :param masses_from_file: masses read from an ab initio output file
        """
        atoms = {}
        atom_indx = 0
        self._parser.find_first(file_obj=file_obj, msg="$coordinates")
        end_msgs = ["$end", "----------------------------------------------------------------------"]

        while not self._parser.block_end(file_obj=file_obj, msg=end_msgs):

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

        self.check_isotopes_substitution(atoms=atoms, masses=masses_from_file)

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
        while not (self._parser.block_end(file_obj=file_obj, msg=end_msgs) or
                   self._parser.file_end(file_obj=file_obj)):

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
        displacements /= self._norm
        norm = np.sum(np.trace(np.einsum('lki, lkj->lkij', displacements, displacements.conjugate()),
                               axis1=2, axis2=3), axis=1)
        displacements = np.einsum('ijk, i-> ijk', displacements, 1.0 / np.sqrt(norm))

        # displacements[num_atoms, num_freq, dim]
        displacements = np.transpose(a=displacements, axes=(1, 0, 2))

        # displacements[num_k, num_atoms, num_freq, dim]
        # with num_k = 1
        data["atomic_displacements"] = np.asarray([displacements])

    def _read_freq_block(self, file_obj=None, freq=None):
        """
        Parses block with frequencies.
        :param file_obj: file object from which we read
        :param freq: list with frequencies which we update
        """
        self._parser.move_to(file_obj=file_obj, msg=":")
        items = file_obj.readline().replace(b"\n", b" ").split()
        length = len(items)
        freq.extend([float(items[i]) for i in range(1, length, 2)])

    def _read_coord_block(self, file_obj=None, xdisp=None, ydisp=None, zdisp=None, part="real"):
        """
        Parses block with coordinates.
        :param file_obj: file object from which we read
        :param xdisp: list with x coordinates which we update
        :param ydisp: list with y coordinates which we update
        :param zdisp: list with z coordinates which we update
        """
        self._parser.move_to(file_obj=file_obj, msg=" x ")

        atom_mass = None

        while not self._parser.file_end(file_obj=file_obj):

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
        floated_item = float(item)
        self._norm += floated_item * floated_item
        if part == "real":
            container.append(complex(float(floated_item), 0.0) * sqrt(mass))
        elif part == "imaginary":
            container.append(complex(0.0, float(floated_item)) * sqrt(mass))
        else:
            raise ValueError("Real or imaginary part of complex number was expected.")

    def _read_masses_from_file(self, obj_file):
        masses = []
        pos = obj_file.tell()
        self._parser.find_first(file_obj=obj_file, msg="Zero point vibrational energy:      ")

        end_msg = "Molecular Mass:"
        key = "Atom"
        if not six.PY2:
            end_msg = bytes(end_msg, "utf8")
            key = bytes(key, "utf8")

        while not self._parser.file_end(file_obj=obj_file):
            line = obj_file.readline()
            if end_msg in line:
                break
            if key in line:
                masses.append(float(line.split()[-1]))
        obj_file.seek(pos)
        return masses
