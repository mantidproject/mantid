import hashlib
from IOmodule import IOmodule
from AbinsData import AbinsData

class GeneralDFTProgram(IOmodule):
    """
    A general class which groups all methods which should be inherited or implemented by a DFT program used
    in INS analysis.
    """

    def __init__(self, input_DFT_filename=None):

        super(GeneralDFTProgram,self).__init__(input_filename=input_DFT_filename, group_name="PhononAB")
        self._filename = input_DFT_filename # name of a filename with the phonon data (for example CASTEP: foo.phonon;
                                            # filename can include path to the file as well)


    def readPhononFile(self):
        """
        This method is different for different DFT programs. It has to be overridden by inheriting class.
        This method should do the following:

          1) Open file with phonon data (CASTEP: foo.phonon). Name of a file should be stored in self._filename.
          Only one dot '.' is expected in the name of a file. There must be no spaces in the name of a file. Extension
          of file (part of a name after '.') is arbitrary.

          2) Method should read from a phonon file an information about frequencies, atomic displacements,
          k-point vectors, weights of k-points and ions.

          3) Method should reconstruct data for symmetry equivalent k-points (protected method _recoverSymmetryPoints).

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

                        "ions"          - Python list with the information about ions. Each entry in the list is a
                                          dictionary with the following entries:

                                               "symbol" - chemical symbol of the element (for example hydrogen -> H)

                                               "sort"   - defines symmetry equivalent atoms, e.g, atoms with the same
                                                          sort are symmetry equivalent

                                                          **Notice at the moment this parameter is not functional
                                                            in LoadCastep**

                                               "fract_coord" - equilibrium position of atom; it has a form of numpy
                                                               array with three floats

                                               "atom" - number of atom in the unit cell


              The attributes should be an dictionary with the following entries:

                        "hash"  - hash of a file with the phonon data. It should be a string representation of hash.

                        "DFT_program" - name of the DFT program which was used to obtain phonon data (for CASTEP -> CASTEP).

          For more details about these fields please look at the documentation of IOmodule class.

        @return: Method should return a list of dictionaries with the following structure:

               data= [ {"frequencies": numpy.array, "atomic_displacements: numpy.array, "weight": numpy._float, "value":numpy.array},
                       {"frequencies": numpy.array, "atomic_displacements: numpy.array, "weight": numpy._float, "value":numpy.array}
                     ]

             Each entry in the list corresponds to one k-point. Each item in the list is a dictionary. The meaning of
             keys in each dictionary is as follows:

                      "frequencies" - frequencies for the given k-point

                      "value"  - value of k-point (numpy array of dimension 3)

                      "atomic_displacements - atomic displacements for the given k-point

                      "weight" - weight of k-point

        """
        return None

    def validData(self):
        """
        Checks if input DFT file and content of HDF file are consistent.
        @return: True if consistent, otherwise False.
        """
        current_hash = self._calculateHash()
        saved_hash = None
        try:
            saved_hash = self.load(list_of_attributes=["hash"])
        except ValueError:
            return False # no hash was found

        return current_hash == saved_hash["attributes"]["hash"]

    def loadData(self):
        """
        Loads data from hdf file.
        @return:
        """
        data = self.load(list_of_numpy_datasets=["frequencies", "weights", "k_vectors", "atomic_displacements", "unit_cell"])
        return self._rearrange_data(data=data["datasets"])


    # Protected methods which should be reused by classes which read DFT phonon data
    def _recoverSymmetryPoints(self, data=None):
        """
        This method reconstructs symmetry equivalent k-points.
        @param data: dictionary with the data for only symmetry inequivalent k-points. This methods
        adds to this dictionary phonon data for symmetry equivalent k-points.
        """

        pass

    def _calculateHash(self):
        """
        This method calculates hash of the phonon file according to SHA-2 algorithm from hashlib library: sha512.
        @return: string representation of hash for phonon file which contains only hexadecimal digits
        """

        buf = 65536  # chop content of phonon file into 64kb chunks to minimize memory consumption for hash creation
        sha = hashlib.sha512()

        with open(self._filename, 'rU') as f:
            while True:
                data = f.read(buf)
                if not data:
                    break
                sha.update(data)

        return sha.hexdigest()

    def _rearrange_data(self, data=None):
        """
        This method rearranges data read from phonon DFT file.

        @param data: dictionary with the data to rearrange
        @return: Returns an object of type ABINSData
        """
        _num_k_points = data["k_vectors"].shape[0]

        # here we multiply by _num_k_points because data["frequencies"] is one dimensional numpy array which stores frequencies for all k-points
        _number_of_atoms = int(float(data["atomic_displacements"].shape[1])/data["frequencies"].shape[0] * _num_k_points)
        _number_of_phonons = 3 * _number_of_atoms
        _return_data = AbinsData(num_atoms=_number_of_atoms, num_k=_num_k_points)

        for i in range(_num_k_points):

            temp_1 = i * _number_of_phonons
            _return_data.append({"weight":data["weights"][i],
                                     "value" :data["k_vectors"][i],
                                    "frequencies":data["frequencies"][temp_1:temp_1 + _number_of_phonons],
                                    "atomic_displacements":data["atomic_displacements"][i]
                                    } )
        return _return_data


