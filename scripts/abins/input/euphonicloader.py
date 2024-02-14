# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from typing import Any, Dict

from euphonic import QpointPhononModes
import numpy as np

from .abinitioloader import AbInitioLoader
from abins.parameters import sampling as sampling_parameters
from dos.load_euphonic import euphonic_calculate_modes


class EuphonicLoader(AbInitioLoader):
    """Get frequencies/eigenvalues from force constants using Euphonic"""

    @property
    def _ab_initio_program(self) -> str:
        return "FORCECONSTANTS"

    @staticmethod
    def data_dict_from_modes(modes: QpointPhononModes) -> Dict[str, Any]:
        """Convert from Euphonic phonon modes to data for KpointsData

        Drop any zero-weighted modes at this point as they will not be used
        """

        unit_cell = modes.crystal.cell_vectors.to("angstrom").magnitude
        atoms = {
            f"atom_{atom_index}": {"symbol": str(atom_type), "sort": atom_index, "coord": unit_cell.T.dot(atom_r), "mass": mass}
            for atom_index, atom_type, atom_r, mass in zip(
                range(modes.crystal.n_atoms), modes.crystal.atom_type, modes.crystal.atom_r, modes.crystal.atom_mass.to("amu").magnitude
            )
        }

        non_zero = np.nonzero(modes.weights)

        file_data = {
            "frequencies": modes.frequencies[non_zero].to("1/cm").magnitude,
            "weights": modes.weights[non_zero],
            "k_vectors": modes.qpts[non_zero],
            "atomic_displacements": np.swapaxes(modes.eigenvectors[non_zero], 1, 2),
            "unit_cell": unit_cell,
            "atoms": atoms,
        }
        return file_data

    def read_vibrational_or_phonon_data(self):
        """Get AbinsData (structure and modes) from force constants data.

        Frequencies/displacements are interpolated using the Euphonic library
        over a regular q-point mesh. The mesh is determined by a Moreno-Soler
        realspace cutoff, related to the size of an equivalent
        supercell. Meshes are rounded up so a very small cutoff will yield
        gamma-point-only sampling.

        """
        cutoff = sampling_parameters["force_constants"]["qpt_cutoff"]
        modes = euphonic_calculate_modes(filename=self._clerk.get_input_filename(), cutoff=cutoff)
        file_data = self.data_dict_from_modes(modes)
        self.save_ab_initio_data(data=file_data)

        return self._rearrange_data(data=file_data)
