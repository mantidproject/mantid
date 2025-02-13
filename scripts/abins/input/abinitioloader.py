# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS

# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod
from pathlib import Path
from typing import Sequence

from mantid.kernel import logger
from mantid.kernel import Atom
import abins
from abins.constants import MASS_EPS


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
        This method is different for different ab initio programs. It has to be overridden by inheriting class.
        This method reads vibrational or phonon data produced by an ab initio program.
        This method should do the following:

          1) Open file with vibrational or phonon data (CASTEP: foo.phonon). Name of a file is
             accessed via self._clerk.get_input_filename(). There must be no spaces in the name of a
             file. Extension of a file (part of a name after '.') is arbitrary.

          2) Method should read from an ab initio file information about frequencies, atomic displacements,
          k-point vectors, weights of k-points and ions.

          3) Method should reconstruct data for symmetry equivalent k-points
             (protected method _recover_symmetry_points).

             **Notice: this step is not implemented now. At the moment only Gamma point calculations are supported.**

          4) Method should determine symmetry equivalent atoms

              **Notice: this step is not implemented now.**

          5) Method should calculate hash of a file with vibrational or phonon data (protected method _calculateHash).

          6) Method should store vibrational or phonon data in an hdf file (using inherited method
             save_ab_initio_data()). The name of an hdf file is foo.hdf5 (CASTEP: foo.phonon -> foo.hdf5). In order to
             save the data to hdf file the following fields should be set:

                    self._hdf_filename
                    self._group_name
                    self._attributes
                    self._datasets

              The datasets should be a dictionary with the following entries:

                        "frequencies"  - frequencies for all k-points grouped in one numpy.array in cm^-1

                        "weights"      - weights of all k-points in one numpy.array

                        "k_vectors"    - all k-points in one numpy array

                                         **Notice: both symmetry equivalent and inequivalent points should be stored; at
                                          the moment only Gamma point calculations are supported**

                        "atomic_displacements" - atomic displacements for all atoms and all k-points in one numpy array
                                                 indexed: (kpt, atom, mode, direction)

                        "unit_cell"      -   numpy array with unit cell vectors in Angstroms

              The following structured datasets should be also defined:

                        "atoms"          - Python dictionary with the information about ions. Each entry in the
                                           dictionary has the following format 'atom_n'. Here n means number of
                                           atom in the unit cell.

                                           Each entry 'atom_n' in the  dictionary is a dictionary with the following
                                           entries:

                                               "symbol" - chemical symbol of the element (for example hydrogen -> H)

                                               "sort"   - defines symmetry equivalent atoms, e.g, atoms with the same
                                                          sort are symmetry equivalent

                                                          **Notice at the moment this parameter is not functional
                                                            in LoadCastep**

                                               "coord" - equilibrium position of atom in Angstroms;
                                                         it has a form of numpy array with three floats

                                               "mass" - mass of atom

              The attributes should be a dictionary with the following entries:

                        "hash"  - hash of a file with the vibrational or phonon data. It should be a string
                                  representation of hash.

                        "ab_initio_program" - name of the ab initio program which was used to obtain vibrational or
                                              phonon data (for CASTEP -> CASTEP).

                        "filename" - name of input ab initio file

          For more details about these fields please look at the documentation of abins.IO class.

        :returns: Method should return an object of type AbinsData.

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
    def _rearrange_data(data: dict) -> abins.AbinsData:
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

    def save_ab_initio_data(self, data: dict) -> None:
        """
        Saves ab initio data to an HDF5 file.
        :param data: dictionary with data to be saved.
        """
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
