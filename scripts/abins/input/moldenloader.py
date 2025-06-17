from io import TextIOBase
from functools import cache
import re

import numpy as np

from abins.abinsdata import AbinsData
from abins.atomsdata import AtomData, AtomsData
from abins.constants import BOHR_TO_ANGSTROM, COMPLEX_TYPE, FLOAT_TYPE
from abins.kpointsdata import KpointsData
from .abinitioloader import AbInitioLoader
from mantid.kernel import Atom


class MoldenLoader(AbInitioLoader):
    @property
    def _ab_initio_program(self) -> str:
        return "Molden"

    @AbInitioLoader.abinsdata_saver
    def read_vibrational_or_phonon_data(self) -> AbinsData:
        """Read AbinsData from a molden .mol file"""
        with open(self._clerk.get_input_filename()) as fd:
            self._check_first_line(fd)

            raw_data = self._read_blocks(fd)

        atoms_data = self.parse_atoms_data(raw_data)
        k_points_data = self.parse_k_points_data(raw_data)
        abins_data = AbinsData(atoms_data=atoms_data, k_points_data=k_points_data)
        return abins_data

    @staticmethod
    def _check_first_line(fd: TextIOBase) -> None:
        if not fd.readline().strip() == "[Molden Format]":
            raise ValueError("File is missing [Molden Format] header. Are you sure this is a Molden file?")

    @staticmethod
    def _read_blocks(fd: TextIOBase) -> dict[str, list[str]]:
        raw_data: dict[str, list[str]] = {}

        header_re = re.compile(r"\[[\w\-]+\]\s*(AU|Angs)?")

        current_header = None
        current_block = None

        for line in map(str.strip, fd):
            if header_re.match(line):
                if current_header:
                    raw_data[current_header] = current_block
                current_header = line
                current_block = []
            else:
                current_block.append(line)

        # Store the last section, as loop is broken at file end
        raw_data[current_header] = current_block

        return raw_data

    @staticmethod
    def parse_atoms_data(raw_data: dict[str, list[str]]) -> AtomsData:
        """Get atomic positions from [FR-COORD] and convert to Angstrom"""
        raw_atom_lines = raw_data["[FR-COORD]"]

        atoms_data = {
            f"atom_{index}": AtomData(
                coord=np.array(coord, dtype=FLOAT_TYPE),
                mass=get_standard_mass(symbol),
                sort=index,
                symbol=symbol,
            )
            for index, (symbol, *coord) in enumerate(map(_parse_atom_line, raw_atom_lines))
        }

        for atom_data in atoms_data.values():
            atom_data["coord"] = atom_data["coord"] * BOHR_TO_ANGSTROM

        return AtomsData(atoms_data)

    @classmethod
    def parse_k_points_data(cls, raw_data: dict[str, list[str]]) -> KpointsData:
        "Read frequencies in recip cm and normalised atomic displacements"
        frequencies = [list(map(float, raw_data["[FREQ]"]))]

        displacements = cls._parse_displacements(raw_data)

        return KpointsData(
            k_vectors=np.array([[0.0, 0.0, 0.0]], dtype=FLOAT_TYPE),
            unit_cell=np.array([[0.0, 0.0, 0.0], [0.0, 0.0, 0.0], [0.0, 0.0, 0.0]], dtype=FLOAT_TYPE),
            weights=np.array([1.0], dtype=FLOAT_TYPE),
            frequencies=np.array(frequencies, dtype=FLOAT_TYPE),
            atomic_displacements=np.array(displacements, dtype=FLOAT_TYPE).view(COMPLEX_TYPE),
        )

    @staticmethod
    def _parse_displacements(raw_data: dict[str, list[str]]) -> list[list[list[list[float]]]]:
        """Get displacements into abins-friendly nested array format

        The array indices are (kpt, atom, mode, axis)

        Note that eigenvectors are _usually_ arranged (mode, atom, axis); the Abins
        ordering is optimised for iterating over incoherent atom contributions...
        """

        modes_array = []
        current_mode_data = []

        for line in raw_data["[FR-NORM-COORD]"]:
            if "vibration" in line:
                if current_mode_data:
                    modes_array.append(current_mode_data)
                    current_mode_data = []
            else:
                # Alternate values with 0. for imaginary component of eigenvector
                x, y, z = line.split()
                current_mode_data.append([float(x), 0.0, float(y), 0.0, float(z), 0.0])

        modes_array.append(current_mode_data)

        # Insert outermost (k-point) axis: we only have one k-point here
        modes_array = np.asarray(modes_array, dtype=float)
        modes_array = np.expand_dims(modes_array, 0)

        # Swap atom and mode indices
        modes_array = modes_array.swapaxes(1, 2)

        return modes_array.tolist()


@cache
def get_standard_mass(symbol: str) -> float:
    """Get standard atomic mass from Mantid reference data"""
    atom = Atom(symbol=symbol)
    return atom.mass


def _parse_atom_line(line: str) -> tuple[str, float, float, float]:
    symbol, x, y, z = line.split()
    return symbol, float(x), float(y), float(z)
