# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import AbinsModules


class AbinsData(AbinsModules.GeneralData):
    """
    Class for storing input DFT data.
    """
    def __init__(self, ):

        super(AbinsData, self).__init__()
        self._atoms_data = None
        self._kpoints_data = None
        self._data = None

    @staticmethod
    def from_calculation_data(filename, ab_initio_program):
        """
        Get AbinsData from ab initio calculation output file.

        :param filename: Path to vibration/phonon data file
        :type filename: str
        :param ab_initio_program: Program which generated data file; this should be a key in AbinsData.ab_initio_loaders
        :type ab_initio_program: str
        """
        # This should live closer to the Loaders but for now it is the only place the dict is used.
        ab_initio_loaders = {"CASTEP": AbinsModules.LoadCASTEP, "CRYSTAL": AbinsModules.LoadCRYSTAL,
                             "DMOL3": AbinsModules.LoadDMOL3, "GAUSSIAN": AbinsModules.LoadGAUSSIAN}

        if ab_initio_program.upper() not in ab_initio_loaders:
            raise ValueError("No loader available for {}: unknown program. "
                             "supported loaders: {}".format(ab_initio_program.upper(),
                                                            ' '.join(ab_initio_loaders.keys())))
        loader = ab_initio_loaders[ab_initio_program.upper()](input_ab_initio_filename=filename)
        return loader.get_formatted_data()

    def set(self, k_points_data=None, atoms_data=None):
        """

        :param k_points_data: object of type KpointsData
        :param atoms_data: object of type AtomsData
        """

        if isinstance(k_points_data, AbinsModules.KpointsData):
            self._kpoints_data = k_points_data
        else:
            raise ValueError("Invalid type of k-points data.")

        if isinstance(atoms_data, AbinsModules.AtomsData):
            self._atoms_data = atoms_data
        else:
            raise ValueError("Invalid type of atoms data.")

        self._data = {"k_points_data": k_points_data.extract(), "atoms_data": atoms_data.extract()}

    def get_kpoints_data(self):
        return self._kpoints_data

    def get_atoms_data(self):
        return self._atoms_data

    def extract(self):
        for k in self._data["k_points_data"]["atomic_displacements"]:
            if self._data["k_points_data"]["atomic_displacements"][k].shape[0] != len(self._data["atoms_data"]):
                raise ValueError("Abins data is inconsistent.")

        return self._data

    def __str__(self):
        return "DFT data"
