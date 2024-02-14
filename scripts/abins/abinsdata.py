# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Any, Dict, Type, TypedDict, TypeVar

from pydantic import validate_call

import mantid
from abins.kpointsdata import KpointsData
from abins.atomsdata import AtomsData


AD = TypeVar("AD", bound="AbinsData")


class AbinsData:
    """
    Class for storing input DFT data.

    :param k_points_data: object of type KpointsData
    :param atoms_data: object of type AtomsData

    """

    @validate_call(config=dict(arbitrary_types_allowed=True, strict=True))
    def __init__(self, *, k_points_data: KpointsData, atoms_data: AtomsData) -> None:
        self._k_points_data = k_points_data
        self._atoms_data = atoms_data
        self._check_consistent_dimensions()

    @staticmethod
    def from_calculation_data(filename: str, ab_initio_program: str) -> "AbinsData":
        """
        Get AbinsData from ab initio calculation output file.

        :param filename: Path to vibration/phonon data file
        :param ab_initio_program: Program which generated data file; this should be a key in AbinsData.ab_initio_loaders
        """
        from abins.input import all_loaders  # Defer import to avoid loops when abins.__init__ imports AbinsData

        if ab_initio_program.upper() not in all_loaders:
            raise ValueError(
                "No loader available for {}: unknown program. "
                "supported loaders: {}".format(ab_initio_program.upper(), " ".join(all_loaders.keys()))
            )
        loader = all_loaders[ab_initio_program.upper()](input_ab_initio_filename=filename)
        data = loader.get_formatted_data()
        return data

    def get_kpoints_data(self) -> KpointsData:
        """Get vibration data mapped over k-points"""
        return self._k_points_data

    def get_atoms_data(self) -> AtomsData:
        """Get atomic structure data"""
        return self._atoms_data

    def _check_consistent_dimensions(self) -> None:
        """Raise an error if atoms_data and k_points_data have different numbers of atoms"""
        data = self.extract()
        for k in data["k_points_data"]["atomic_displacements"]:
            if data["k_points_data"]["atomic_displacements"][k].shape[0] != len(data["atoms_data"]):
                raise ValueError(
                    "KpointsData and AtomsData are not consistent: number of atoms in structure " "does not match displacement data."
                )

    def extract(self) -> Dict[str, Any]:
        """Get a dict with k-points data and atoms data"""
        return {"k_points_data": self.get_kpoints_data().extract(), "atoms_data": self.get_atoms_data().extract()}

    def __str__(self) -> str:
        return "Abins data"

    class JSONableData(TypedDict):
        """JSON-friendly representation of AbinsData"""

        __abins__class__: str
        __mantid_version__: str
        atoms_data: AtomsData.JSONableData
        k_points_data: KpointsData.JSONableData

    def to_dict(self) -> "AbinsData.JSONableData":
        """Get a JSON-compatible representation of the data"""
        return self.JSONableData(
            atoms_data=self.get_atoms_data().to_dict(),
            k_points_data=self.get_kpoints_data().to_dict(),
            __abins_class__="AbinsData",
            __mantid_version__=mantid.__version__,
        )

    @classmethod
    def from_dict(cls: Type[AD], data: "AbinsData.JSONableData") -> AD:
        """Construct from JSON-compatible dictionary"""

        return cls(k_points_data=KpointsData.from_dict(data["k_points_data"]), atoms_data=AtomsData.from_dict(data["atoms_data"]))
