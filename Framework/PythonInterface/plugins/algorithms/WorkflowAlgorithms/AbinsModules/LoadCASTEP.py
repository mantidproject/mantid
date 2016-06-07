import numpy as np
import re
import hashlib

class LoadCASTEP:
    """
    Class which handles loading files from foo.phonon output CASTEP files.
    """

    # Functions to read phonon file taken from
    # SimulatedDensityOfStates. All credits for those methods to Elliot Oram.

    # ----------------------------------------------------------------------------------------

    def __init__(self, filename):

        self.filename = filename+".phonon"
        # Regex pattern for a floating point number
        self._float_regex = r'\-?(?:\d+\.?\d*|\d*\.?\d+)'

    # noinspection PyMethodMayBeStatic
    def _parse_block_header(self, header_match, block_count):
        """
        Parse the header of a block of frequencies and intensities

        @param header_match - the regex match to the header
        @param block_count - the count of blocks found so far
        @return weight for this block of values
        """
        # Found header block at start of frequencies
        q1, q2, q3, weight = [float(x) for x in header_match.groups()]
        q_vector = [q1, q2, q3]
        if block_count > 1 and sum(q_vector) == 0:
            weight = 0.0
        return weight, q_vector

        # ----------------------------------------------------------------------------------------

    def _parse_phonon_file_header(self, f_handle):
        """
        Read information from the header of a <>.phonon file

        @param f_handle - handle to the file.
        @return List of ions in file as list of tuple of (ion, mode number)
        """
        file_data = {'ions': []}

        while True:
            line = f_handle.readline()

            if not line:
                raise IOError("Could not find any header information.")

            if 'Number of ions' in line:
                self._num_ions = int(line.strip().split()[-1])
            elif 'Number of branches' in line:
                self._num_branches = int(line.strip().split()[-1])
            elif 'Unit cell vectors' in line:
                file_data['unit_cell'] = self._parse_phonon_unit_cell_vectors(f_handle)
            elif 'Fractional Co-ordinates' in line:
                if self._num_ions is None:
                    raise IOError("Failed to parse file. Invalid file header.")

                # Extract the mode number for each of the ion in the data file
                for _ in xrange(self._num_ions):
                    line = f_handle.readline()
                    line_data = line.strip().split()

                    species = line_data[4]
                    ion = {'species': species,
                           'fract_coord': np.array([float(line_data[1]), float(line_data[2]), float(line_data[3])]),
                           'index': int(line_data[0]) - 1,
                           'bond_number': len([i for i in file_data['ions'] if i['species'] == species]) + 1}
                    # -1 to convert to zero based indexing
                    file_data['ions'].append(ion)

            if 'END header' in line:
                if self._num_ions is None or self._num_branches is None:
                    raise IOError("Failed to parse file. Invalid file header.")
                return file_data

                # ----------------------------------------------------------------------------------------

    def _parse_phonon_freq_block(self, f_handle):
        """
        Iterator to parse a block of frequencies from a .phonon file.

        @param f_handle - handle to the file.
        """

        for _ in xrange(self._num_branches):
            line = f_handle.readline()
            line_data = line.strip().split()[1:]
            line_data = [float(x) for x in line_data]
            yield line_data

    # noinspection PyMethodMayBeStatic
    def _parse_phonon_unit_cell_vectors(self, f_handle):
        """
        Parses the unit cell vectors in a .phonon file.

        @param f_handle Handle to the file
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
        vectors = []
        for _ in xrange(self._num_ions * self._num_branches):
            line = f_handle.readline()

            if not line:
                raise IOError("Could not parse file. Invalid file format.")

            line_data = line.strip().split()
            vector_components = line_data[2::2]
            vector_components = [float(x) for x in vector_components]
            vectors.append(vector_components)

        return np.asarray(vectors)

    def _calculateHash(self):
        """
        Calculates hash of the phonon file according to SHA-2 alghoritm from hashlib library: sha512.
        @return: string representation of hash for phonon file which contains only hexadecimal digits
        """

        buf = 65536  # chop content of phonon file into 64kb chunks to minimize memory consumption for hash creation
        sha = hashlib.sha512()

        with open(self.filename, 'rU') as f:
            while True:
                data = f.read(buf)
                if not data:
                    break
                sha.update(data)

        return sha.hexdigest()

    def readPhononFile(self):
        """
        Read frequencies from a <>.phonon file

        @return dictionary with the frequencies for each k_point (frequencies),
                weights of k_points (weights),
                k_vectors (k_vectors),
                amplitudes of atom distortions for each k-point (eigenvectors),
                hash of the phonon file (hash) and
                whether we have Gamma point calculations or calculations for the whole Brillouin Zone ("gamma_calculations")
        """
        file_data = {}

        # Header regex. Looks for lines in the following format:
        #     q-pt=    1    0.000000  0.000000  0.000000      1.0000000000    0.000000  0.000000  1.000000
        header_regex_str = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) (?: *(%(s)s)){0,4}" % {'s': self._float_regex}
        header_regex = re.compile(header_regex_str)
        eigenvectors_regex = re.compile(r"\s*Mode\s+Ion\s+X\s+Y\s+Z\s*")
        block_count = 0

        frequencies, ir_intensities, raman_intensities, weights, k_vectors, eigenvectors = [], [], [], [], [], []
        data_lists = (frequencies, ir_intensities, raman_intensities)
        with open(self.filename, 'rU') as f_handle:
            file_data.update(self._parse_phonon_file_header(f_handle))

            while True:
                line = f_handle.readline()
                # Check we've reached the end of file
                if not line:
                    break

                # Check if we've found a block of frequencies
                header_match = header_regex.match(line)
                if header_match:
                    block_count += 1

                    weight, k_vector = self._parse_block_header(header_match, block_count)
                    weights.append(weight)
                    k_vectors.append(k_vector)

                    # Parse block of frequencies
                    for line_data in self._parse_phonon_freq_block(f_handle):
                        for data_list, item in zip(data_lists, line_data):
                            data_list.append(item)

                vector_match = eigenvectors_regex.match(line)
                if vector_match:

                    vectors = self._parse_phonon_eigenvectors(f_handle)
                    eigenvectors.append(vectors)

        frequencies = np.asarray(frequencies)
        eigenvectors = np.asarray(eigenvectors)
        warray = np.asarray(weights)
        hash_filename = self._calculateHash()
        # in the future it will be evaluated basing on the number of k-points:
        # one k-point => gamma_point_calculations=True
        # more then one k-point gamma_point_calculations=False
        # in this phase it is fixed to True so that only Gamma point is supported
        gamma_point_calculations = True

        file_data.update({"frequencies": frequencies,
                          "weights": warray,
                          "k_vectors": k_vectors,
                          "eigenvectors": eigenvectors,
                          "hash": hash_filename,
                          "gamma_calculations":gamma_point_calculations})

        return file_data
