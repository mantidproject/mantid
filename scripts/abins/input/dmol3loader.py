# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import io
import numpy as np
import re
from typing import List, Tuple

from .textparser import TextParser
from .abinitioloader import AbInitioLoader
from abins.constants import FLOAT_TYPE, COMPLEX_TYPE
from abins.constants import ATOMIC_LENGTH_2_ANGSTROM as BOHR_TO_ANGSTROM
from mantid.kernel import Atom

# This regular expression matches a row of displacement data, formatted
#
#  El x   disp_1x  disp_2x ...
#     y   disp_1y  disp_2y ...
#     z   disp_1z  disp_2z ...
#  El x   disp_1x  disp_2x ...
# ...
#
_disp_row_regex = re.compile(
    r" [A-Z]?[a-z]?"  # Optional element symbol
    + r"\s+[x-z]\s+"  # Cartesian direction
    + r"(\s+-?\d+.\d+)"  # At least one column
    + r"(\s+-?\d+.\d+)?" * 8  # up to max nine
    + r"\s*$"
)  # Followed by end-of-line


class DMOL3Loader(AbInitioLoader):
    """
    Class for loading DMOL3 ab initio vibrational data.
    """

    def __init__(self, input_ab_initio_filename):
        """
        :param input_ab_initio_filename: name of file with vibrational data (foo.outmol)
        """
        super().__init__(input_ab_initio_filename=input_ab_initio_filename)
        self._norm = 0

    @property
    def _ab_initio_program(self) -> str:
        return "DMOL3"

    def read_vibrational_or_phonon_data(self):
        """
        Reads vibrational data from DMOL3 output files. Saves frequencies, weights of k-point vectors,
        k-point vectors, amplitudes of atomic displacements, hash of file  with vibrational data to <>.hdf5
        :returns: object of type AbinsData.
        """
        data = {}  # container to store read data

        with io.open(
            self._clerk.get_input_filename(),
            "rb",
        ) as dmol3_file:

            # Move read file pointer to the last calculation recorded in the .outmol file. First calculation could be
            # geometry optimization. The last calculation in the file is expected to be calculation of vibrational data.
            # There may be some intermediate resume calculations.
            try:
                TextParser.find_last(file_obj=dmol3_file, msg="$cell vectors")
            except EOFError:  # for non-period calc, go to final coordinates instead
                dmol3_file.seek(0)
                TextParser.find_last(file_obj=dmol3_file, msg="$coordinates")

            # read lattice vectors
            data["unit_cell"] = self._read_lattice_vectors(dmol3_file)

            # read info about atoms and construct atom data
            masses = self._read_masses_from_file(obj_file=dmol3_file)
            atoms = self._read_atomic_coordinates(file_obj=dmol3_file)
            self.check_isotopes_substitution(atoms=atoms, masses=masses)
            self._num_atoms = len(atoms)
            data["atoms"] = atoms

            # Update masses based on behaviour of check_isotopes_substitution
            masses = [atom["mass"] for _, atom in atoms.items()]

            # read frequencies, corresponding atomic displacements and construct k-points data
            TextParser.find_first(file_obj=dmol3_file, msg="Frequencies (cm-1) and normal modes ")
            frequencies, raw_modes = self._read_modes(dmol3_file)
            data.update(self._assemble_mode_data(frequencies, raw_modes, masses))

            # save data to hdf file
            self.save_ab_initio_data(data=data)

            # return AbinsData object
            return self._rearrange_data(data=data)

    @classmethod
    def _read_lattice_vectors(cls, file_obj):
        """
        Reads lattice vectors from .outmol DMOL3 file.
        :param obj_file: file object from which we read
        :param data: Python dictionary to which found lattice vectors should be added
        """
        with TextParser.save_excursion(file_obj):
            try:
                TextParser.find_first(file_obj=file_obj, msg="$cell vectors")

            except EOFError:
                # No unit cell for non-periodic calculations; set to zero
                unit_cell = np.zeros(shape=(3, 3), dtype=FLOAT_TYPE)

            else:
                vectors = [file_obj.readline().split() for _ in range(3)]
                unit_cell = np.asarray(vectors, dtype=FLOAT_TYPE)
                unit_cell *= BOHR_TO_ANGSTROM

        return unit_cell

    @staticmethod
    def _read_atomic_coordinates(file_obj: io.BufferedReader) -> dict:
        """
        Reads atomic coordinates from .outmol DMOL3 file.

        :param file_obj: open file handle to DMOL3 output data
        """
        atoms = {}
        atom_index = 0
        TextParser.find_first(file_obj=file_obj, msg="$coordinates")
        end_msgs = ["$end", "------------------------------------------------"]

        while not TextParser.block_end(file_obj, msg=end_msgs):
            symbol, *coord = file_obj.readline().split()

            symbol = symbol.decode("utf-8").capitalize()
            atom = Atom(symbol=symbol)

            # Convert coordinate units from Bohr to Angstrom
            coord = np.asarray(coord, dtype=FLOAT_TYPE) * BOHR_TO_ANGSTROM

            atoms[f"atom_{atom_index}"] = {"symbol": symbol, "mass": atom.mass, "sort": atom_index, "coord": coord}
            atom_index += 1

        return atoms

    @staticmethod
    def _get_to_next_nonempty(file_obj: io.BufferedReader) -> None:
        # Scroll to next non-empty line
        last_line = file_obj.tell()
        for line in file_obj:
            if line.decode("utf8").strip():
                file_obj.seek(last_line)
                return
            else:
                last_line = file_obj.tell()

    @classmethod
    def _read_modes(cls, file_obj) -> Tuple[np.ndarray, np.ndarray]:
        """Read vibrational frequencies and atomic displacements

        Modes are in their raw format: a 2-D array with modes along index 1
        (i.e. columns, corresponding to frequencies) and atomic degrees of
        freedom along index 0 (i.e. rows of x y z x y z x y ... for successive
        atoms).
        """
        frequency_blocks, displacement_blocks = [], []

        while not (TextParser.block_end(file_obj=file_obj, msg=["STANDARD"]) or TextParser.file_end(file_obj=file_obj)):
            frequencies, displacements = cls._read_mode_block(file_obj)

            frequency_blocks.append(frequencies)
            displacement_blocks.append(displacements)

            cls._get_to_next_nonempty(file_obj)

        # Eigenvector regex includes optional matches for columns 2-9; these
        # lead to None groups, converted to NaN by numpy. No such cleanup is
        # needed for frequencies as these were obtained by simple line.split().

        for column in range(1, 9):
            # Check if the last displacement block has column(s) of NaN;
            # if so number of modes was not multiple of 9 and extra columns
            # should be removed before assembling blocks to full array
            if np.all(np.isnan(displacement_blocks[-1][:, column])):
                displacement_blocks[-1] = displacement_blocks[-1][:, :column]
                break

        frequencies = np.concatenate(frequency_blocks)
        displacements = np.concatenate(displacement_blocks, axis=1)

        return frequencies, displacements

    @classmethod
    def _read_mode_block(cls, file_obj) -> Tuple[np.ndarray, np.ndarray]:
        """Get a row of frequencies and corresponding array of displacements"""

        # Frequency row: "  1: freq1  2: freq2 ..."
        TextParser.move_to(file_obj=file_obj, msg=":")
        frequencies = np.asarray(file_obj.readline().split()[1::2], dtype=FLOAT_TYPE)

        # Displacements first row: " El x   disp_1x  disp_2x ..."
        TextParser.move_to(file_obj=file_obj, msg=" x ")

        displacement_rows = []
        line = file_obj.readline()
        match = _disp_row_regex.match(line.decode("utf8"))
        while match:
            displacement_rows.append(np.asarray(match.groups(), dtype=FLOAT_TYPE))
            match = _disp_row_regex.match(file_obj.readline().decode("utf8"))

        if not displacement_rows:
            raise ValueError("Displacement data not found in this block")
        displacements = np.stack(displacement_rows)

        return frequencies, displacements

    @classmethod
    def _assemble_mode_data(cls, frequencies: np.ndarray, raw_displacements: np.ndarray, masses: np.ndarray) -> dict:
        """Process DMOL3 mode data to relevant keys and values for AbinsData

        k-point information is set to single Gamma-point as this is all DMOL3
        provides.

        Displacements are also normalised here. Note that owing to a different
        scaling/reporting convention a sqrt(mass) factor is _not_ applied as
        with codes that output displacements in length units (e.g. CRYSTAL17).
        """
        data = {
            "frequencies": frequencies[None, :],  # Add first (kpt) index
            "k_vectors": np.array([[0.0, 0.0, 0.0]], dtype=FLOAT_TYPE),
            "weights": np.array([1.0], dtype=FLOAT_TYPE),
        }

        displacements = cls._reshape_displacements(raw_displacements)
        displacements = cls._normalise_displacements(displacements)
        data["atomic_displacements"] = displacements.astype(COMPLEX_TYPE)

        return data

    def _read_freq_block(self, file_obj=None, freq=None):
        """
        Parses block with frequencies.
        :param file_obj: file object from which we read
        :param freq: list with frequencies which we update
        """
        TextParser.move_to(file_obj=file_obj, msg=":")
        items = file_obj.readline().replace(b"\n", b" ").split()
        length = len(items)
        freq.extend([float(items[i]) for i in range(1, length, 2)])

    @staticmethod
    def _normalise_displacements(displacements) -> np.ndarray:
        """ "Normalise displacements of each mode over all atoms"""
        from numpy.linalg import norm

        mode_weights = norm(norm(displacements, axis=1), axis=2)
        return displacements / mode_weights[:, None, :, None]

    @staticmethod
    def _reshape_displacements(raw_displacements) -> np.ndarray:
        """Convert displacements array from DMOL3 format to AbinsData format

        In raw format rows are ordered atom1x, atom1y, atom1z, atom2x, ...
        [i.e. indices are (3*atom + dir, mode)]
        For AbinsData indices are (kpt, atom, mode, direction). There is only
        one k-point, so the outer index is trivial.
        """
        n_modes = raw_displacements.shape[1]
        displacements = raw_displacements.reshape((1, -1, 3, n_modes)).swapaxes(2, 3)
        return displacements

    @staticmethod
    def _read_masses_from_file(obj_file) -> List[float]:
        masses = []

        with TextParser.save_excursion(obj_file):
            TextParser.find_first(file_obj=obj_file, msg="Zero point vibrational energy:      ")

            end_msg = "Molecular Mass:"
            key = "Atom"
            end_msg = bytes(end_msg, "utf8")
            key = bytes(key, "utf8")

            while not TextParser.file_end(file_obj=obj_file):
                line = obj_file.readline()
                if end_msg in line:
                    break
                if key in line:
                    masses.append(float(line.split()[-1]))

        return masses
