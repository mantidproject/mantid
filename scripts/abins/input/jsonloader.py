# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from enum import auto, Enum
import json
from pathlib import Path

from euphonic import QpointPhononModes

from .abinitioloader import AbInitioLoader
from .euphonicloader import EuphonicLoader
from abins.abinsdata import AbinsData
from abins.parameters import sampling as sampling_parameters
from dos.load_euphonic import euphonic_calculate_modes

# json-stream implementation converts data lazily so we can quickly check the
# top-level keys even if a data file is huge.
try:
    from json_stream import load as json_load
except ImportError:
    from json import load as json_load


class PhononJSON(Enum):
    EUPHONIC_FREQUENCIES = auto()
    EUPHONIC_MODES = auto()
    EUPHONIC_FORCE_CONSTANTS = auto()
    ABINS_DATA = auto()
    UNKNOWN = auto()


abins_supported_json_formats = {PhononJSON.EUPHONIC_MODES, PhononJSON.EUPHONIC_FORCE_CONSTANTS, PhononJSON.ABINS_DATA}


class JSONLoader(AbInitioLoader):
    """Get frequencies/eigenvalues from a JSON file using Euphonic"""

    @property
    def _ab_initio_program(self) -> str:
        return "JSON"

    @staticmethod
    def check_json_format(json_file: str | Path) -> PhononJSON:
        """Check if JSON file is a known phonon data format"""

        for class_key in "__euphonic_class__", "__abins_class__":
            with open(json_file, "r") as fd:
                data = json_load(fd)
                data_class = data.get(class_key)

            if data_class is not None:
                break
        else:
            class_key = ""

        match class_key, data_class:
            case ("__euphonic_class__", "QpointPhononModes"):
                return PhononJSON.EUPHONIC_MODES
            case ("__euphonic_class__", "QpointFrequencies"):
                return PhononJSON.EUPHONIC_FREQUENCIES
            case ("__euphonic_class__", "ForceConstants"):
                return PhononJSON.EUPHONIC_FORCE_CONSTANTS
            case ("__abins_class__", "AbinsData"):
                return PhononJSON.ABINS_DATA
            case _:
                return PhononJSON.UNKNOWN

    def save_from_abins_data(self, abins_data: AbinsData) -> None:
        """Save data to hdf5 cache from AbinsData format

        Usually we construct a data dict for the cache and then use it to
        construct AbinsData. Sometimes it makes sense to do it the other way
        around, so this method provides the reverse operation.
        """
        data = abins_data.get_atoms_data().extract()
        data.update(abins_data.get_kpoints_data().extract())
        self.save_ab_initio_data(data=data)

    def read_vibrational_or_phonon_data(self) -> AbinsData:
        """Get AbinsData (structure and modes) from force constants data.

        Frequencies/displacements are interpolated using the Euphonic library
        over a regular q-point mesh. The mesh is determined by a Moreno-Soler
        realspace cutoff, related to the size of an equivalent
        supercell. Meshes are rounded up so a very small cutoff will yield
        gamma-point-only sampling.

        """
        json_file = self._clerk.get_input_filename()
        json_format = self.check_json_format(json_file)

        match json_format:
            case PhononJSON.ABINS_DATA:
                with open(json_file, "r") as fd:
                    data = json.load(fd)
                abins_data = AbinsData.from_dict(data)
                self.save_from_abins_data(abins_data)
                return abins_data

            case PhononJSON.EUPHONIC_MODES:
                modes = QpointPhononModes.from_json_file(json_file)

            case PhononJSON.EUPHONIC_FORCE_CONSTANTS:
                cutoff = sampling_parameters["force_constants"]["qpt_cutoff"]
                modes = euphonic_calculate_modes(filename=json_file, cutoff=cutoff)

            case _:
                raise ValueError(f"Cannot use JSON data of type {json_format.name}")

        file_data = EuphonicLoader.data_dict_from_modes(modes)
        self.save_ab_initio_data(data=file_data)

        return self._rearrange_data(data=file_data)
