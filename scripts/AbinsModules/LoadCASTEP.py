from __future__ import (absolute_import, division, print_function)
import numpy as np
import re
import AbinsModules


class LoadCASTEP(AbinsModules.GeneralDFTProgram):
    """
    Class which handles loading files from foo.phonon output CASTEP files.
    Functions to read phonon file taken from SimulatedDensityOfStates (credits for Elliot Oram.).
    """

    def __init__(self, input_dft_filename):
        """

        @param input_dft_filename: name of file with phonon data (foo.phonon)
        """
        super(LoadCASTEP, self).__init__(input_dft_filename=input_dft_filename)

        # Regex pattern for a floating point number
        self._float_regex = r'\-?(?:\d+\.?\d*|\d*\.?\d+)'
        self._sum_rule = None
        self._dft_program = "CASTEP"

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
            q1, q2, q3, weight, q1_dir, q2_dir, q3_dir = [float(x) for x in header_match.groups()]
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
        file_data = {"atoms": {}}

        while True:
            line = f_handle.readline()

            if not line:
                raise IOError("Could not find any header information.")

            if 'Number of ions' in line:
                self._num_atoms = int(line.strip().split()[-1])
            elif 'Number of branches' in line:
                self._num_phonons = int(line.strip().split()[-1])
            elif 'Number of wavevectors' in line:
                self._num_k = int(line.strip().split()[-1])
            elif 'Unit cell vectors' in line:
                file_data['unit_cell'] = self._parse_phonon_unit_cell_vectors(f_handle)
            elif 'Fractional Co-ordinates' in line:
                if self._num_atoms is None:
                    raise IOError("Failed to parse file. Invalid file header.")

                # Extract the mode number for each of the ion in the data file
                for _ in range(self._num_atoms):
                    line = f_handle.readline()
                    line_data = line.strip().split()
                    indx = int(line_data[0]) - 1  # -1 to convert to zero based indexing
                    symbol = line_data[4]
                    ion = {"symbol": symbol,
                           "fract_coord": np.array([float(line_data[1]), float(line_data[2]), float(line_data[3])]),
                           # at the moment it is a dummy parameter, it will mark symmetry equivalent atoms
                           "sort": indx,
                           "mass": float(line_data[5])}
                    file_data["atoms"].update({"atom_%s" % indx: ion})

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

        freq = []
        for n in range(self._num_phonons):
            line = f_handle.readline()
            freq.append(float(line.strip().split()[1]))

        return freq

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

        dim = 3  # we have 3D space
        # in general case eigenvectors are complex
        vectors = np.zeros((self._num_atoms, self._num_phonons, dim), dtype=AbinsModules.AbinsConstants.COMPLEX_TYPE)
        for freq in range(self._num_phonons):
            for atom in range(self._num_atoms):

                line = f_handle.readline()

                if not line:
                    raise IOError("Could not parse file. Invalid file format.")

                line_data = line.split()
                vector_components = line_data[2:]
                # we have dimension 3, eigenvectors are complex, 3 * 2 = 6  number of all fields to parse
                for n in range(dim):
                    vectors[atom, freq, n] = complex(float(vector_components[2 * n]),
                                                     float(vector_components[2 * n + 1]))

        return np.asarray(vectors)

    def _check_acoustic_sum(self):
        """
        Checks if acoustic sum correction has been applied during calculations.
        @return: True is correction has been applied, otherwise False.
        """
        header_str_sum = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) +(%(s)s) + " \
                         r"(%(s)s) + (%(s)s) + (%(s)s)" % {'s': self._float_regex}
        header_sum = re.compile(header_str_sum)

        with open(self._clerk.get_input_filename(), "r") as f:
            found = False
            for line in f:  # iterate over the file one line at a time(memory efficient)
                if header_sum.match(line):
                    return True

            return found

    def read_phonon_file(self):
        """
        Reads frequencies, weights of k-point vectors, k-point vectors, amplitudes of atomic displacements
        from a <>.phonon file. Save frequencies, weights of k-point vectors, k-point vectors, amplitudes of atomic
        displacements, hash of the phonon file (hash) to <>.hdf5

        @return  object of type AbinsData.
        """
        file_data = {}

        # Header regex. Looks for lines in the following format:
        #     q-pt=    1    0.000000  0.000000  0.000000      1.0000000000    0.000000  0.000000  1.000000
        sum_rule_header = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) +(%(s)s) + " \
                          r"(%(s)s) + (%(s)s) + (%(s)s)" % {'s': self._float_regex}
        no_sum_rule_header = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) +(%(s)s)" % {'s': self._float_regex}

        if self._check_acoustic_sum():
            header_regex_str = sum_rule_header
            self._sum_rule = True
        else:
            header_regex_str = no_sum_rule_header
            self._sum_rule = False

        compiled_header_regex_str = re.compile(header_regex_str)
        compiled_no_sum_rule_header = re.compile(no_sum_rule_header)

        headers = {True: compiled_header_regex_str, False: compiled_no_sum_rule_header}
        eigenvectors_regex = re.compile(r"\s*Mode\s+Ion\s+X\s+Y\s+Z\s*")
        block_count = 0

        frequencies, weights, k_vectors, eigenvectors = [], [], [], []
        with open(self._clerk.get_input_filename(), "r") as f_handle:
            file_data.update(self._parse_phonon_file_header(f_handle))
            header_found = False
            while True:
                line = f_handle.readline()
                # Check we've reached the end of file
                if not line:
                    break

                # Check if we've found a block of frequencies
                header_match = headers[block_count == 0].match(line)
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
                          "atomic_displacements":
                              np.asarray(eigenvectors) * AbinsModules.AbinsConstants.ATOMIC_LENGTH_2_ANGSTROM
                          })

        self._recover_symmetry_points(data=file_data)

        # save stuff to hdf file
        data_to_save = ["frequencies", "weights", "k_vectors", "atomic_displacements", "unit_cell", "atoms"]
        data = {}
        for key in data_to_save:
            data[key] = file_data[key]

        self.save_dft_data(data=data)

        return self._rearrange_data(data=file_data)
