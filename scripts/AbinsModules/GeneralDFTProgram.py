from __future__ import (absolute_import, division, print_function)
from mantid.kernel import logger
import AbinsModules


# noinspection PyMethodMayBeStatic
class GeneralDFTProgram(object):
    """
    A general class which groups all methods which should be inherited or implemented by a DFT program used
    in INS analysis.
    """
    def __init__(self, input_dft_filename=None):

        self._num_k = None
        self._num_atoms = None
        self._sample_form = None
        self._dft_program = None
        self._clerk = AbinsModules.IOmodule(input_filename=input_dft_filename,
                                            group_name=AbinsModules.AbinsParameters.dft_group)

    def read_phonon_file(self):
        """
        This method is different for different DFT programs. It has to be overridden by inheriting class.
        This method should do the following:

          1) Open file with phonon data (CASTEP: foo.phonon). Name of a file should be stored in self._input_filename.
          Only one dot '.' is expected in the name of a file. There must be no spaces in the name of a file. Extension
          of file (part of a name after '.') is arbitrary.

          2) Method should read from a phonon file an information about frequencies, atomic displacements,
          k-point vectors, weights of k-points and ions.

          3) Method should reconstruct data for symmetry equivalent k-points
             (protected method _recover_symmetry_points).

             **Notice: this step is not implemented now. At the moment only Gamma point calculations are supported.**

          4) Method should determine symmetry equivalent atoms

              **Notice: this step is not implemented now.**

          5) Method should calculate hash of a file with phonon data (protected method _calculateHash).

          6) Method should store phonon data in an hdf file (inherited method save()). The name of an hdf file is
          foo.hdf5 (CASTEP: foo.phonon -> foo.hdf5). In order to save the data to hdf file the following fields
          should be set:

                    self._hdf_filename
                    self._group_name
                    self._attributes
                    self._datasets

              The datasets should be a dictionary with the following entries:

                        "frequencies"  - frequencies for all k-points grouped in one numpy.array

                        "weights"      - weights of all k-points in one numpy.array

                        "k_vectors"    - all k-points in one numpy array

                                         **Notice: both symmetry equivalent and inequivalent points should be stored; at
                                          the moment only Gamma point calculations are supported**

                        "atomic_displacements" - atomic displacements for all atoms and all k-points in one numpy array

                        "unit_cell"      -   numpy array with unit cell vectors

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

                                               "fract_coord" - equilibrium position of atom; it has a form of numpy
                                                               array with three floats

                                               "mass" - mass of atom

              The attributes should be a dictionary with the following entries:

                        "hash"  - hash of a file with the phonon data. It should be a string representation of hash.

                        "DFT_program" - name of the DFT program which was used to obtain phonon data (for CASTEP ->
                                        CASTEP).

                        "filename" - name of input DFT file

          For more details about these fields please look at the documentation of IOmodule class.

        @return: Method should return an object of type AbinsData.

        """
        return None

    def load_formatted_data(self):
        """
        Loads data from hdf file. After data is loaded it is put into AbinsData object.
        @return:
        """
        data = self._clerk.load(list_of_datasets=["frequencies", "weights", "k_vectors",
                                                  "atomic_displacements", "unit_cell", "atoms"])
        datasets = data["datasets"]
        self._num_k = datasets["k_vectors"].shape[0]
        self._num_atoms = len(datasets["atoms"])

        loaded_data = {"frequencies": datasets["frequencies"],
                       "weights": datasets["weights"],
                       "k_vectors": datasets["k_vectors"],
                       "atomic_displacements": datasets["atomic_displacements"],
                       "unit_cell": datasets["unit_cell"],
                       "atoms": datasets["atoms"]}

        return self._rearrange_data(data=loaded_data)

    # Protected methods which should be reused by classes which read DFT phonon data
    def _recover_symmetry_points(self, data=None):
        """
        This method reconstructs symmetry equivalent k-points.
        @param data: dictionary with the data for only symmetry inequivalent k-points. This methods
        adds to this dictionary phonon data for symmetry equivalent k-points.
        """

        pass

    def _rearrange_data(self, data=None):
        """
        This method rearranges data read from phonon DFT file. It converts  masses and frequencies Hartree atomic units.
        It converts atomic displacements from atomic units to Angstroms

        @param data: dictionary with the data to rearrange
        @return: Returns an object of type AbinsData
        """

        k_points = AbinsModules.KpointsData(num_atoms=self._num_atoms, num_k=self._num_k)

        # 1D [k] (one entry corresponds to weight of one k-point)
        k_points.set({"weights": data["weights"],
                      # 2D [k][3] (one entry corresponds to one coordinate of particular k-point)
                      "k_vectors": data["k_vectors"],
                      # 2D  array [k][freq] (one entry corresponds to one frequency for the k-point k)
                      "frequencies": data["frequencies"],
                      # 4D array [k][atom_n][freq][3] (one entry corresponds to
                      # one coordinate for atom atom_n, frequency  freq and k-point k )
                      "atomic_displacements": data["atomic_displacements"]
                      })

        atoms = AbinsModules.AtomsDaTa(num_atoms=self._num_atoms)
        atoms.set(data["atoms"])

        result_data = AbinsModules.AbinsData()
        result_data.set(k_points_data=k_points, atoms_data=atoms)
        return result_data

    def save_dft_data(self, data=None):
        """
        Saves DFT data to an HDF5 file.
        :param data: dictionary with data to be saved.
        """
        for name in data:
            self._clerk.add_data(name=name, value=data[name])
        self._clerk.add_file_attributes()
        self._clerk.add_attribute("DFT_program", self._dft_program)
        self._clerk.save()

    def get_formatted_data(self):

        # try to load DFT data from *.hdf5 file
        try:
            if self._dft_program != self._clerk.get_previous_dft_program():
                raise ValueError("Different DFT program was used in the previous calculation. Data in the hdf file "
                                 "will be erased.")

            self._clerk.check_previous_data()

            dft_data = self.load_formatted_data()
            logger.notice(str(dft_data) + " has been loaded from the HDF file.")

        # if loading from *.hdf5 file failed than read data directly from input DFT file and erase hdf file
        except (IOError, ValueError) as err:

            logger.notice(str(err))
            self._clerk.erase_hdf_file()
            dft_data = self.read_phonon_file()
            logger.notice(str(dft_data) + " from DFT input file has been loaded.")

        return dft_data
