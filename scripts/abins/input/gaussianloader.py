# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import io
from io import BufferedReader
import re
from typing import Iterable, List

import numpy as np

from .abinitioloader import AbInitioLoader
from .textparser import TextParser
from abins.constants import COMPLEX_TYPE, FLOAT_TYPE, ROTATIONS_AND_TRANSLATIONS
from mantid.kernel import Atom


class GAUSSIANLoader(AbInitioLoader):
    """
    Class for loading GAUSSIAN ab initio vibrational data.
    """

    def __init__(self, input_ab_initio_filename) -> None:
        """
        :param input_ab_initio_filename: name of file with vibrational data (foo.log or foo.LOG)
        """
        super().__init__(input_ab_initio_filename=input_ab_initio_filename)
        self._active_atoms = None
        self._num_atoms = None
        self._num_read_freq = 0

    @property
    def _ab_initio_program(self) -> str:
        return "GAUSSIAN"

    def read_vibrational_or_phonon_data(self):
        """
        Reads vibrational data from GAUSSIAN output files. Saves frequencies and atomic displacements (only molecular
        calculations), hash of file with vibrational data to <>.hdf5.
        :returns: structure, frequency and displacement data from file
        """

        data = {}  # container to store read data

        with io.open(
            self._clerk.get_input_filename(),
            "rb",
        ) as gaussian_file:
            # create dummy lattice vectors
            self._generate_lattice_vectors(data=data)

            masses = self._read_masses_from_file(file_obj=gaussian_file)
            # move file pointer to the last optimized atomic positions, in standard orientation
            # (i.e. the reference structure for reported atomic displacements)
            TextParser.find_last(file_obj=gaussian_file, regex=r"^\s+(Standard orientation|Input orientation):\s+$")
            self._read_atomic_coordinates(file_obj=gaussian_file, data=data, masses_from_file=masses)

            # read frequencies, corresponding atomic displacements for a molecule
            TextParser.find_first(file_obj=gaussian_file, msg="Harmonic frequencies (cm**-1), IR intensities (KM/Mole), Raman scattering")
            self._read_modes(file_obj=gaussian_file, data=data)

            # Check if atoms were frozen and remove from structure if so
            self._remove_frozen_atoms(active_atoms=self._active_atoms, data=data)

            # save data to hdf file
            self.save_ab_initio_data(data=data)

            # return AbinsData object
            return self._rearrange_data(data=data)

    def _read_atomic_coordinates(self, *, file_obj: BufferedReader, data: dict, masses_from_file: List[float]) -> None:
        """
        Read atomic coordinates from .log GAUSSIAN file and update data dict

        :param file_obj: file object from which we read
        :param data: Python dictionary to which atoms data should be added
        :param masses_from_file:  masses read from an ab initio output file
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

        while not TextParser.block_end(file_obj=file_obj, msg=end_msgs):
            line = file_obj.readline()
            entries = line.split()
            z_number = int(entries[1])
            atom = Atom(z_number=z_number)
            coord = np.asarray([float(i) for i in entries[3:6]])
            atoms["atom_{}".format(atom_indx)] = {"symbol": atom.symbol, "mass": atom.mass, "sort": atom_indx, "coord": coord}

            atom_indx += 1
        self.check_isotopes_substitution(atoms=atoms, masses=masses_from_file, approximate=True)

        self._num_atoms = len(atoms)
        data["atoms"] = atoms

    @staticmethod
    def _remove_frozen_atoms(*, active_atoms: List[int], data: dict) -> None:
        """Modify data in-place, removing atoms that are missing from active_atoms and re-indexing"""

        # Do nothing if there are no frozen atoms
        if len(active_atoms) == len(data["atoms"]):
            return None

        new_atoms = {}

        for data_index, atom_index in enumerate(active_atoms):
            new_atoms[f"atom_{data_index}"] = data["atoms"][f"atom_{atom_index - 1}"]
            new_atoms[f"atom_{data_index}"]["sort"] = data_index

        data["atoms"] = new_atoms

    @staticmethod
    def _generate_lattice_vectors(*, data) -> None:
        """
        Generate dummy lattice vectors. Gaussian is only for molecular calculations.
        :param obj_file: file object from which we read
        :param data: Python dictionary to which found lattice vectors should be added
        """
        data["unit_cell"] = np.zeros(shape=(3, 3), dtype=FLOAT_TYPE)

    @staticmethod
    def _read_active_atoms(*, file_obj: BufferedReader) -> List[int]:
        """Get indices of rows from atomic displacement data block

        If the calculation uses frozen atoms, these will be a subset of the structure
        which was previously read.
        """
        HEADER_RE = r"\s+Atom\s+AN\s+X      Y      Z        X      Y      Z        X      Y      Z"
        INT_RE = r"\d+"
        FLOAT_RE = r"-?\d+\.\d+"
        row_re = re.compile(bytes(rf"\s+({INT_RE})\s+{INT_RE}" + 9 * rf"\s+{FLOAT_RE}", "utf8"))

        active_atoms = []

        with TextParser.save_excursion(file_obj):
            TextParser.find_first(file_obj=file_obj, regex=HEADER_RE)

            while re_match := row_re.match(file_obj.readline()):
                active_atoms.append(int(re_match.groups()[0]))

        return active_atoms

    def _read_modes(self, *, file_obj: BufferedReader, data: dict):
        """
        Reads vibrational modes (frequencies and atomic displacements).
        :param file_obj: file object from which we read
        :param data: Python dictionary to which k-point data should be added

        This includes a check for frozen atoms, which sets self._active_atoms
        and updates self._num_atoms to reflect the number of non-frozen atoms
        """
        num_all_atoms = self._num_atoms
        self._active_atoms = self._read_active_atoms(file_obj=file_obj)
        self._num_atoms = len(self._active_atoms)

        # Usually there will be 3N-6 modes, as rotations and translations are
        # removed. However, if some atoms are frozen then we will get the full
        # set of 3N modes as translation is blocked. (Gaussian doesn't seem to
        # account for the case that one atom is frozen and rotation is free.)

        if self._num_atoms == num_all_atoms:
            num_freq = 3 * self._num_atoms - ROTATIONS_AND_TRANSLATIONS
        else:
            num_freq = 3 * len(self._active_atoms)

        dim = 3

        freq = []
        atomic_disp = np.zeros(shape=(num_freq, self._num_atoms, dim), dtype=COMPLEX_TYPE)

        # Next block is:
        # -------------------
        # - Thermochemistry -
        # -------------------

        # parse block with frequencies and atomic displacements
        with TextParser.save_excursion(file_obj):
            try:
                TextParser.find_first(file_obj=file_obj, msg="Frequencies --- ")
                high_precision = True
                end_msg = ["activities (A**4/AMU)"]
            except EOFError:
                high_precision = False
                end_msg = ["-------------------"]

        file_obj.readline()  # Advance past false-positive end marker

        while not (TextParser.block_end(file_obj=file_obj, msg=end_msg) or TextParser.file_end(file_obj=file_obj)):
            self._read_freq_block(file_obj=file_obj, freq=freq, high_precision=high_precision)
            self._read_atomic_disp_block(file_obj=file_obj, disp=atomic_disp, high_precision=high_precision)

        data["frequencies"] = np.asarray([freq]).astype(dtype=FLOAT_TYPE, casting="safe")

        # we mimic that we have one Gamma k-point
        data["k_vectors"] = np.asarray([[0.0, 0.0, 0.0]]).astype(dtype=FLOAT_TYPE, casting="safe")
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

    @staticmethod
    def _read_freq_block(*, file_obj: BufferedReader, freq: List[float], high_precision: bool = False) -> None:
        """
        Parses block with frequencies.
        :param file_obj: file object from which we read
        :param freq: list with frequencies which we update
        :param high_precision: If true, look for block beginning 'Frequencies ---'
        """
        msg = "Frequencies --- " if high_precision else "Frequencies -- "
        line = TextParser.find_first(file_obj=file_obj, msg=msg)

        line = line.split()
        freq.extend([float(i) for i in line[2:]])

    def _read_atomic_disp_block(self, *, file_obj: BufferedReader, disp: np.ndarray, high_precision: bool = False) -> None:
        """
        Parses block with atomic displacements.
        :param file_obj: file object from which we read
        :param disp: Complex-valued array [num_freq, num_atoms, dim] to be updated with displacement data
        :param high_precision: Read block in alternate format (direction-per-row instead of direction-per-column)
        """
        if high_precision:
            self._read_atomic_disp_block_high_precision(file_obj=file_obj, disp=disp)
        else:
            self._read_atomic_disp_block_low_precision(file_obj=file_obj, disp=disp)

    def _read_atomic_disp_block_low_precision(self, *, file_obj: BufferedReader, disp: np.ndarray) -> None:
        """
        Parses block with atomic displacements.
        :param file_obj: file object from which we read
        :param disp: Complex-valued array [num_freq, num_atoms, dim] to be updated with displacement data
        """
        sub_block_start = "X      Y      Z        X      Y      Z        X      Y      Z"
        TextParser.find_first(file_obj=file_obj, msg=sub_block_start)

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

    def _read_atomic_disp_block_high_precision(self, *, file_obj: BufferedReader, disp: np.ndarray) -> None:
        """
        Parses block with atomic displacements.
        :param file_obj: file object from which we read
        :param disp: Complex-valued array [num_freq, num_atoms, dim] to be updated with displacement data
        """
        # Get the expected number of columns from reduced mass row of header
        TextParser.find_first(file_obj=file_obj, msg="Reduced masses --- ")
        n_columns = len(file_obj.readline().split()) - 3

        sub_block_start = " Coord Atom Element:"
        TextParser.find_first(file_obj=file_obj, msg=sub_block_start)

        while re.match(r"\s+\d+\s+\d+\s+\d+\s+(-?\d+\.\d+)", (line := file_obj.readline().decode())):
            # e.g. "  1  72 1  -0.0001  -0.0001 -0.0000"
            line = line.split()
            coord, atom, _ = map(int, line[:3])
            displacements = map(float, line[3:])
            disp[self._num_read_freq : self._num_read_freq + n_columns, atom - 1, coord - 1] = self._real_to_complex(displacements)

        self._num_read_freq += n_columns

    @staticmethod
    def _real_to_complex(floats: Iterable[float]) -> list[complex]:
        return [complex(x, 0) for x in floats]

    @staticmethod
    def _read_masses_from_file(file_obj: BufferedReader) -> List[float]:
        masses = []
        with TextParser.save_excursion(file_obj):
            TextParser.find_first(file_obj=file_obj, msg="Thermochemistry")

            end_msg = "Molecular mass:"
            key = "Atom"
            end_msg = bytes(end_msg, "utf8")
            key = bytes(key, "utf8")

            while not TextParser.file_end(file_obj=file_obj):
                line = file_obj.readline()
                if end_msg in line:
                    break
                if key in line:
                    masses.append(float(line.split()[-1]))

        return masses
