from mantid.kernel import logger
from mantid.api import AlgorithmFactory, Progress,  FileAction, FileProperty, PythonAlgorithm

import numpy as np
import re


class ABINS(PythonAlgorithm):

    # ----------------------------------------------------------------------------------------

    def category(self):
        return "Simulation"

        # ----------------------------------------------------------------------------------------

    def summary(self):
        return "Calculates inelastic neutron scattering."

        # ----------------------------------------------------------------------------------------

    def PyInit(self):

        self.declareProperty(FileProperty('PHONONFile', '',
                             action=FileAction.OptionalLoad,
                             extensions=["phonon"]),
                             doc='Filename of the PHONON file.')

        

    def validateInputs(self):
        pass

    def PyExec(self):
        pass

    def _get_properties(self):
        pass


    # Functions to read phonon file taken from
    # SimulatedDensityOfStates
    # All credits for those methods to Elliot Oram
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
                    ion = {'species': species}
                    ion['fract_coord'] = np.array([float(line_data[1]), float(line_data[2]), float(line_data[3])])
                    # -1 to convert to zero based indexing
                    ion['index'] = int(line_data[0]) - 1
                    ion['bond_number'] = len([i for i in file_data['ions'] if i['species'] == species]) + 1
                    file_data['ions'].append(ion)

                logger.debug('All ions: ' + str(file_data['ions']))

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
        prog_reporter = Progress(self, 0.0, 1.0, 1)
        for _ in xrange(self._num_branches):
            line = f_handle.readline()
            line_data = line.strip().split()[1:]
            line_data = [float(x) for x in line_data]
            yield line_data

        prog_reporter.report("Reading frequencies.")

        # ----------------------------------------------------------------------------------------

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

        # ----------------------------------------------------------------------------------------

    def _parse_phonon_eigenvectors(self, f_handle):
        vectors = []
        prog_reporter = Progress(self, 0.0, 1.0, self._num_branches * self._num_ions)
        for _ in xrange(self._num_ions * self._num_branches):
            line = f_handle.readline()

            if not line:
                raise IOError("Could not parse file. Invalid file format.")

            line_data = line.strip().split()
            vector_componets = line_data[2::2]
            vector_componets = [float(x) for x in vector_componets]
            vectors.append(vector_componets)
            prog_reporter.report("Reading eigenvectors.")

        return np.asarray(vectors)

        # ----------------------------------------------------------------------------------------

    def _parse_phonon_file(self, file_name):
        """
        Read frequencies from a <>.phonon file

        @param file_name - file path of the file to read
        @return the frequencies, infra red and raman intensities and weights of frequency blocks
        """
        file_data = {}

        # Header regex. Looks for lines in the following format:
        #     q-pt=    1    0.000000  0.000000  0.000000      1.0000000000    0.000000  0.000000  1.000000
        header_regex_str = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) (?: *(%(s)s)){0,4}" % {
            's': self._float_regex}
        header_regex = re.compile(header_regex_str)
        eigenvectors_regex = re.compile(r"\s*Mode\s+Ion\s+X\s+Y\s+Z\s*")
        block_count = 0

        record_eigenvectors = self._calc_partial \
                              or (self._spec_type == 'DOS' and self._scale_by_cross_section != 'None') \
                              or self._spec_type == 'BondAnalysis'

        frequencies, ir_intensities, raman_intensities, weights, q_vectors, eigenvectors = [], [], [], [], [], []
        data_lists = (frequencies, ir_intensities, raman_intensities)
        with open(file_name, 'rU') as f_handle:
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

                    weight, q_vector = self._parse_block_header(header_match, block_count)
                    weights.append(weight)
                    q_vectors.append(q_vector)

                    # Parse block of frequencies
                    for line_data in self._parse_phonon_freq_block(f_handle):
                        for data_list, item in zip(data_lists, line_data):
                            data_list.append(item)

                vector_match = eigenvectors_regex.match(line)
                if vector_match:
                    if record_eigenvectors:
                        # Parse eigenvectors for partial dos
                        vectors = self._parse_phonon_eigenvectors(f_handle)
                        eigenvectors.append(vectors)
                    else:
                        # Skip over eigenvectors
                        for _ in xrange(self._num_ions * self._num_branches):
                            line = f_handle.readline()
                            if not line:
                                raise IOError("Bad file format. Uexpectedly reached end of file.")

        frequencies = np.asarray(frequencies)
        ir_intensities = np.asarray(ir_intensities)
        eigenvectors = np.asarray(eigenvectors)
        raman_intensities = np.asarray(raman_intensities)
        warray = np.repeat(weights, self._num_branches)

        file_data.update({
            'frequencies': frequencies,
            'ir_intensities': ir_intensities,
            'raman_intensities': raman_intensities,
            'weights': warray,
            'q_vectors': q_vectors,
            'eigenvectors': eigenvectors
        })

        return file_data


try:
    import scipy.constants
    AlgorithmFactory.subscribe(ABINS)
except ImportError:
    logger.debug('Failed to subscribe algorithm SimulatedDensityOfStates; The python package scipy may be missing.')
