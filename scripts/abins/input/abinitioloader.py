# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS

# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod
from functools import wraps
from pathlib import Path
from typing import Sequence

import numpy as np
from pydantic import ConfigDict, validate_call
from typing_extensions import TypedDict

from mantid.kernel import logger
from mantid.kernel import Atom
import abins
from abins.abinsdata import AbinsData
from abins.atomsdata import _AtomData
from abins.constants import MASS_EPS


class AbinsDataDict(TypedDict):
    """Dict form of AbinsData used for caching"""

    __pydantic_config__ = ConfigDict(arbitrary_types_allowed=True)

    frequencies: np.ndarray
    weights: np.ndarray
    k_vectors: np.ndarray
    atomic_displacements: np.ndarray  # indexed: (kpt, atom, mode, direction)
    unit_cell: np.ndarray  # 3x3 array, Å
    atoms: dict[str, _AtomData]


# Make it easier to inspect program classes by cleaning up result from str()
class NamedAbstractClass(ABCMeta):
    def __str__(self):
        return self.__name__


class AbInitioLoader(metaclass=NamedAbstractClass):
    """
    Base class for loaders which import phonon data from ab initio output files

    In addition to a standard interface, this provides some caching functionality.
    Typically the user calls get_formatted_data() which checks the cache before calling
    read_formatted_data() if necessary and caching the results.
    """

    def __init__(self, input_ab_initio_filename: str = None, cache_directory: Path | None = None):
        """An object for loading phonon data from ab initio output files"""

        if not isinstance(input_ab_initio_filename, str):
            raise TypeError("Filename must be a string")
        elif not Path(input_ab_initio_filename).is_file():
            raise IOError(f"Ab initio file {input_ab_initio_filename} not found.")

        self._clerk = abins.IO(
            input_filename=input_ab_initio_filename,
            group_name=abins.parameters.hdf_groups["ab_initio_data"],
            cache_directory=cache_directory,
        )

    @property
    @abstractmethod
    def _ab_initio_program(self) -> str: ...

    @abstractmethod
    def read_vibrational_or_phonon_data(self) -> abins.AbinsData:
        """
        Read external data to an AbinsData object. This method must be implemented for each format supported by AbINS.

        Implementations of this method should always be decorated with ``@AbInitioLoader.abinsdata_saver``, which will use the object
        attributes _ab_initio_program and _clerk to cache the AbinsData from this method to .hdf5.

        Implementations should do the following:

          1) Open file with vibrational or phonon data (e.g. "foo.phonon" for CASTEP). The filename is
             accessed via self._clerk.get_input_filename().

          2) Read information about frequencies, atomic displacements, k-point vectors, weights of k-points and ions.

          3) Construct and return a valid AbinsData
        """
        ...

    def load_formatted_data(self) -> abins.AbinsData:
        """
        Loads data from hdf file. After data is loaded it is put into AbinsData object.
        :returns: object of type AbinsData
        """
        data = self._clerk.load(list_of_datasets=["frequencies", "weights", "k_vectors", "atomic_displacements", "unit_cell", "atoms"])
        datasets = data["datasets"]

        loaded_data = {
            "frequencies": datasets["frequencies"],
            "weights": datasets["weights"],
            "k_vectors": datasets["k_vectors"],
            "atomic_displacements": datasets["atomic_displacements"],
            "unit_cell": datasets["unit_cell"],
            "atoms": datasets["atoms"],
        }

        return self._rearrange_data(data=loaded_data)

    # Internal method for use by child classes which read ab initio phonon data
    @staticmethod
    @validate_call
    def _rearrange_data(data: AbinsDataDict) -> abins.AbinsData:
        """
        This method rearranges data read from input ab initio file.
        :param data: dictionary with the data to rearrange
        """

        k_points = abins.KpointsData(  # 1D [k] (one entry corresponds to weight of one k-point)
            weights=data["weights"],
            # 2D [k][3] (one entry corresponds to one coordinate of particular k-point)
            k_vectors=data["k_vectors"],
            # 2D  array [k][freq] (one entry corresponds to one frequency for the k-point k)
            frequencies=data["frequencies"],
            # 4D array [k][atom_n][freq][3] (one entry corresponds to
            # one coordinate for atom atom_n, frequency  freq and k-point k )
            atomic_displacements=data["atomic_displacements"],
            unit_cell=data["unit_cell"],
        )

        atoms = abins.AtomsData(data["atoms"])
        return abins.AbinsData(k_points_data=k_points, atoms_data=atoms)

    @staticmethod
    def abinsdata_saver(read_function):
        """Decorated function also passes return value to self.save_ab_initio_data

        read_function should return an object of type abins_data

        The class containing the wrapped method must have attributes "_ab_initio_program" (a str) and "_clerk" (instance of abins.io.IO).

        """

        @wraps(read_function)
        def wrapper(self, *args, **kwargs) -> AbinsData:
            abins_data = read_function(self, *args, **kwargs)
            self._save_ab_initio_data(abins_data=abins_data)
            return abins_data

        return wrapper

    def _save_ab_initio_data(self, abins_data: AbinsData) -> None:
        """
        Saves ab initio data to an HDF5 file.
        :param data: dictionary with data to be saved.
        """
        atoms_data = abins_data.get_atoms_data()
        kpoints_data = abins_data.get_kpoints_data()

        data = AbinsDataDict(
            frequencies=kpoints_data._frequencies,
            weights=kpoints_data._weights,
            k_vectors=kpoints_data._k_vectors,
            atomic_displacements=kpoints_data._atomic_displacements,
            unit_cell=kpoints_data.unit_cell,
            atoms=atoms_data.extract(),
        )

        for name in data:
            self._clerk.add_data(name=name, value=data[name])
        self._clerk.add_file_attributes()
        self._clerk.add_attribute("ab_initio_program", self._ab_initio_program)
        self._clerk.save()

    def get_formatted_data(self) -> abins.AbinsData:
        """
        Check for HD5 cache before reading from ab initio outputs if cache unavailable
        """
        # try to load ab initio data from *.hdf5 file
        try:
            self._clerk.check_previous_data()

            ab_initio_data = self.load_formatted_data()
            logger.notice(str(ab_initio_data) + " has been loaded from the HDF file.")

        # if loading from *.hdf5 file failed than read data directly from input ab initio file and erase hdf file
        except (IOError, ValueError) as err:
            logger.notice(str(err))
            self._clerk.erase_hdf_file()
            ab_initio_data = self.read_vibrational_or_phonon_data()
            logger.notice(str(ab_initio_data) + " from ab initio input file has been loaded.")

        return ab_initio_data

    @staticmethod
    def check_isotopes_substitution(atoms: dict, masses: Sequence[float], approximate: bool = False) -> None:
        """
        Update atomic masses to Mantid values if isotopic substitution detected

        The masses attached to "atoms", generally derived from atom symbols, are compared
        to the list of "masses" given in calculation output.

        In "approximate" mode, masses are rounded to the nearest integer for comparison;
        otherwise the tolerance depends on abins.constants.MASS_EPS

        :param atoms: dictionary of atom data to check and update if appropriate
        :param masses: atomic masses read from an ab initio file
        :param approximate: whether or not look for isotopes in the approximated way
        """
        num_atoms = len(atoms)
        eps = MASS_EPS
        if approximate:
            isotopes_found = [abs(round(atoms["atom_%s" % i]["mass"]) - round(masses[i])) > eps for i in range(num_atoms)]
        else:
            isotopes_found = [abs(atoms["atom_%s" % i]["mass"] - masses[i]) > eps for i in range(num_atoms)]

        if any(isotopes_found):
            for i in range(num_atoms):
                if isotopes_found[i]:
                    z_num = Atom(symbol=atoms["atom_{}".format(i)]["symbol"]).z_number
                    a_num = int(round(masses[i]))
                    try:
                        temp = Atom(a_number=a_num, z_number=z_num).mass
                        atoms["atom_{}".format(i)]["mass"] = temp
                    # no mass for isotopes available; assume no isotopic substitution for this atom
                    except RuntimeError:
                        pass
