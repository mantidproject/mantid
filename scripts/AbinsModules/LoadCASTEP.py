import numpy as np
import re

# ABINS modules
from GeneralDFTProgram import GeneralDFTProgram
from AbinsData import AbinsData
import AbinsParameters

class LoadCASTEP(GeneralDFTProgram):
    """
    Class which handles loading files from foo.phonon output CASTEP files.
    Functions to read phonon file taken from SimulatedDensityOfStates (credits for Elliot Oram.).
    """

    def __init__(self, input_DFT_filename):
        """

        @param input_DFT_filename: name of file with phonon data (foo.phonon)
        """
        super(LoadCASTEP, self).__init__(input_DFT_filename=input_DFT_filename)

        # Regex pattern for a floating point number
        self._float_regex = r'\-?(?:\d+\.?\d*|\d*\.?\d+)'
        self._sum_rule = None


    # noinspection PyMethodMayBeStatic
    def _parse_block_header(self, header_match, block_count):
        """
        Parses the header of a block of frequencies and intensities

        @param header_match: the regex match to the header
        @param block_count: the count of blocks found so far
        @return weight for this block of values
        """
        # Found header block at start of frequencies
        if self._sum_rule and block_count == 0:
            q1, q2, q3, weight, q1_dir,q2_dir,q3_dir = [float(x) for x in header_match.groups()]
        else:
            q1, q2, q3, weight = [float(x) for x in header_match.groups()]

        q_vector = [q1, q2, q3]

        return weight, q_vector

        # ----------------------------------------------------------------------------------------

    def _parse_phonon_file_header(self, f_handle):
        """
        Reads information from the header of a <>.phonon file

        @param f_handle: handle to the file.
        @return List of ions in file as list of tuple of (ion, mode number)
        """
        file_data = {"atoms": []}

        while True:
            line = f_handle.readline()

            if not line:
                raise IOError("Could not find any header information.")

            if 'Number of ions' in line:
                self._num_atoms = int(line.strip().split()[-1])
            elif 'Number of branches' in line:
                self._num_phonons = int(line.strip().split()[-1])
            elif 'Number of wavevectors' in line:
                self._num_k =  int(line.strip().split()[-1])
            elif 'Unit cell vectors' in line:
                file_data['unit_cell'] = self._parse_phonon_unit_cell_vectors(f_handle)
            elif 'Fractional Co-ordinates' in line:
                if self._num_atoms is None:
                    raise IOError("Failed to parse file. Invalid file header.")

                # Extract the mode number for each of the ion in the data file
                for _ in range(self._num_atoms):
                    line = f_handle.readline()
                    line_data = line.strip().split()

                    symbol = line_data[4]
                    ion = {"symbol": symbol,
                           "fract_coord": np.array([float(line_data[1]), float(line_data[2]), float(line_data[3])]),
                           "atom": int(line_data[0]) - 1,
                           "sort": int(line_data[0]) - 1, # at the moment it is a dummy parameter, it will mark symmetry equivalent atoms
                           "mass": float(line_data[5])}
                    # -1 to convert to zero based indexing
                    file_data["atoms"].append(ion)

            if 'END header' in line:
                if self._num_atoms is None or self._num_phonons is None:
                    raise IOError("Failed to parse file. Invalid file header.")
                return file_data

                # ----------------------------------------------------------------------------------------

    def _parse_phonon_freq_block(self, f_handle):
        """
        Reads frequencies block from <>.phonon file.

        @param f_handle: handle to the file.
        """

        # for _ in range(self._num_branches):
        #     line = f_handle.readline()
            # line_data = line.strip().split()[1:]
            # line_data = [float(x) for x in line_data]
            # yield line_data

        _freq = []
        for n in range(self._num_phonons):
            line = f_handle.readline()
            _freq.append(float(line.strip().split()[1]))

        return _freq



    # noinspection PyMethodMayBeStatic
    def _parse_phonon_unit_cell_vectors(self, f_handle):
        """
        Parses the unit cell vectors in a .phonon file.

        @param f_handle: Handle to the file
        @return Numpy array of unit vectors
        """
        data = []
        for _ in range(3):
            line = f_handle.readline()
            line_data = line.strip().split()
            line_data = [float(x) for x in line_data]
            data.append(line_data)

        return np.array(data)

    def _parse_phonon_eigenvectors(self, f_handle):
        """

        @param f_handle: file object to read
        @return: eigenvectors (atomic displacements) for all k-points
        """

        dim = 3 # we have 3D space
        vectors = np.zeros((self._num_atoms, self._num_phonons, dim), dtype=AbinsParameters.complex_type) # in general case eigenvectors are complex
        for freq in range(self._num_phonons):
            for atom in range(self._num_atoms):

                line = f_handle.readline()

                if not line:
                    raise IOError("Could not parse file. Invalid file format.")

                line_data = line.split()
                vector_components = line_data[2:]
                for n in range(dim): # we have dimension 3, eigenvectors are complex, 3 * 2 = 6  number of all fields to parse
                    vectors[atom , freq, n] = complex(float(vector_components[2 * n]), float(vector_components[2 * n + 1]))

        return np.asarray(vectors)


    def _check_acoustic_sum(self):
        """
        Cheks if acoustic sum correction has been applied during calculations.
        @return: True is correction has been applied, otherwise False.
        """
        header_str_sum = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) +(%(s)s) + (%(s)s) + (%(s)s) + (%(s)s)" % {'s': self._float_regex}
        header_sum = re.compile(header_str_sum)

        header_str =  r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) +(%(s)s)" % {'s': self._float_regex}
        header = re.compile(header_str)

        with open(self._input_filename, "rU") as f:
            found = False
            for line in f:  #iterate over the file one line at a time(memory efficient)
                if header_sum.match(line):
                    return True

            return found


    def readPhononFile(self):
        """
        Reads frequencies, weights of k-point vectors, k-point vectors, amplitudes of atomic displacements
        from a <>.phonon file. Save frequencies, weights of k-point vectors, k-point vectors, amplitudes of atomic
        displacements, information about Gamma-[pint calculation (True/False) hash of the phonon file (hash) to <>.hdf5

        @return dictionary with the frequencies for each k_point (frequencies),
                weights of k_points (weights),
                k_vectors (k_vectors),
                amplitudes of atom distortions for each k-point (eigenvectors) and
                whether we have Gamma point calculations or calculations for the whole Brillouin Zone ("gamma_calculations")
        """
        file_data = {}

        # Header regex. Looks for lines in the following format:
        #     q-pt=    1    0.000000  0.000000  0.000000      1.0000000000    0.000000  0.000000  1.000000
        _sum_rule_header =   r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) +(%(s)s) + (%(s)s) + (%(s)s) + (%(s)s)" % {'s': self._float_regex}
        _no_sum_rule_header =  r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) +(%(s)s)" % {'s': self._float_regex}

        if self._check_acoustic_sum():
            header_regex_str = _sum_rule_header
            self._sum_rule = True
        else:
            header_regex_str = _no_sum_rule_header
            self._sum_rule = False

        _compiled_header_regex_str = re.compile(header_regex_str)
        _compiled_no_sum_rule_header = re.compile(_no_sum_rule_header)

        headers = {True:_compiled_header_regex_str, False: _compiled_no_sum_rule_header}
        eigenvectors_regex = re.compile(r"\s*Mode\s+Ion\s+X\s+Y\s+Z\s*")
        block_count = 0

        frequencies, weights, k_vectors, eigenvectors = [], [], [], []
        with open(self._input_filename, 'rU') as f_handle:
            file_data.update(self._parse_phonon_file_header(f_handle))
            header_found = False
            while True:
                line = f_handle.readline()
                # Check we've reached the end of file
                if not line:
                    break

                # Check if we've found a block of frequencies
                header_match =  headers[block_count == 0].match(line)
                if header_match:
                    header_found = True
                    weight, k_vector = self._parse_block_header(header_match, block_count)
                    weights.append(weight)
                    k_vectors.append(k_vector)

                    # Parse block of frequencies
                    frequencies.append(self._parse_phonon_freq_block(f_handle=f_handle))

                    block_count += 1

                vector_match = eigenvectors_regex.match(line)
                if vector_match and header_found:
                    header_found = False
                    vectors = self._parse_phonon_eigenvectors(f_handle=f_handle)
                    eigenvectors.append(vectors)

        file_data.update({"frequencies": np.asarray(frequencies),
                          "weights": np.asarray(weights),
                          "k_vectors": np.asarray(k_vectors),
                          "atomic_displacements": np.asarray(eigenvectors)})

        self._recoverSymmetryPoints(data=file_data)

        # save stuff to hdf file
        _numpy_datasets_to_save=["frequencies", "weights", "k_vectors", "atomic_displacements", "unit_cell"]
        for name in _numpy_datasets_to_save:
            self.addNumpyDataset(name, file_data[name])

        _structured_datasets=["atoms"]
        for item in _structured_datasets:
            self.addStructuredDataset(name=item, value=file_data[item])

        self.addFileAttributes()
        self.addAttribute("DFT_program", "CASTEP")

        self.save()

        return self._rearrange_data(data=file_data)





