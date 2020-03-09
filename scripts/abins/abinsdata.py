# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Any, Dict

import abins
from abins.kpointsdata import KpointsData
from abins.atomsdata import AtomsData


class AbinsData:
    """
    Class for storing input DFT data.

    :param k_points_data: object of type KpointsData
    :param atoms_data: object of type AtomsData

    """
    def __init__(self, *,
                 k_points_data: KpointsData,
                 atoms_data: AtomsData) -> None:
        if not isinstance(k_points_data, KpointsData):
            raise TypeError("Invalid type of k-points data.: {}".format(type(k_points_data)))
        self._k_points_data = k_points_data

        if not isinstance(atoms_data, AtomsData):
            raise TypeError("Invalid type of atoms data.")
        self._atoms_data = atoms_data
        self._check_consistent_dimensions()

    @staticmethod
    def from_calculation_data(filename: str,
                              ab_initio_program: str) -> 'AbinsData':
        """
        Get AbinsData from ab initio calculation output file.

        :param filename: Path to vibration/phonon data file
        :type filename: str
        :param ab_initio_program: Program which generated data file; this should be a key in AbinsData.ab_initio_loaders
        :type ab_initio_program: str
        """
        import abins.input  # Defer import to avoid loops when abins.__init__ imports AbinsData

        # This should live closer to the Loaders but for now it is the only place the dict is used.
        ab_initio_loaders = {"CASTEP": abins.input.CASTEPLoader, "CRYSTAL": abins.input.CRYSTALLoader,
                             "DMOL3": abins.input.DMOL3Loader, "GAUSSIAN": abins.input.GAUSSIANLoader,
                             "VASP": abins.input.VASPLoader}

        if ab_initio_program.upper() not in ab_initio_loaders:
            raise ValueError("No loader available for {}: unknown program. "
                             "supported loaders: {}".format(ab_initio_program.upper(),
                                                            ' '.join(ab_initio_loaders.keys())))
        loader = ab_initio_loaders[ab_initio_program.upper()](input_ab_initio_filename=filename)
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
                raise ValueError("Abins data is inconsistent: number of atoms in structure does not match "
                                 "displacement data.")

    def extract(self) -> Dict[str, Any]:
        """Get a dict with k-points data and atoms data"""
        return {"k_points_data": self.get_kpoints_data().extract(),
                "atoms_data": self.get_atoms_data().extract()}

    def __str__(self) -> str:
        return "DFT data"
