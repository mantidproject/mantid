#pylint: disable=redefined-builtin
from __future__ import (absolute_import, division, print_function)
from six.moves import range

import re
import numpy as np

import dos.load_helper as load_helper

element_isotope = dict()


def parse_phonon_file(file_name, record_eigenvectors):
    """
    Read frequencies from a <>.phonon file

    @param file_name - file path of the file to read
    @return the frequencies, infra red and raman intensities and weights of frequency blocks
    """
    file_data = {}

    # Get regex strings from load_helper
    header_regex = re.compile(load_helper.PHONON_HEADER_REGEX)
    eigenvectors_regex = re.compile(load_helper.PHONON_EIGENVEC_REGEX)

    block_count = 0
    frequencies, ir_intensities, raman_intensities, weights, q_vectors, eigenvectors = [], [], [], [], [], []
    data_lists = (frequencies, ir_intensities, raman_intensities)
    with open(file_name, 'rU') as f_handle:
        file_data.update(_parse_phonon_file_header(f_handle))

        while True:
            line = f_handle.readline()
            # Check we've reached the end of file
            if not line:
                break

            # Check if we've found a block of frequencies
            header_match = header_regex.match(line)
            if header_match:
                block_count += 1

                weight, q_vector = load_helper._parse_block_header(header_match, block_count)
                weights.append(weight)
                q_vectors.append(q_vector)

                # Parse block of frequencies
                for line_data in _parse_phonon_freq_block(f_handle,
                                                          file_data['num_branches']):
                    for data_list, item in zip(data_lists, line_data):
                        data_list.append(item)

            vector_match = eigenvectors_regex.match(line)
            if vector_match:
                if record_eigenvectors:
                    # Parse eigenvectors for partial dos
                    vectors = _parse_phonon_eigenvectors(f_handle,
                                                         file_data['num_ions'],
                                                         file_data['num_branches'])
                    eigenvectors.append(vectors)
                else:
                    # Skip over eigenvectors
                    for _ in range(file_data['num_ions'] * file_data['num_branches']):
                        line = f_handle.readline()
                        if not line:
                            raise IOError("Bad file format. Uexpectedly reached end of file.")

    frequencies = np.asarray(frequencies)
    ir_intensities = np.asarray(ir_intensities)
    raman_intensities = np.asarray(raman_intensities)
    warray = np.repeat(weights, file_data['num_branches'])
    eigenvectors = np.asarray(eigenvectors)

    file_data.update({
        'frequencies': frequencies,
        'ir_intensities': ir_intensities,
        'raman_intensities': raman_intensities,
        'weights': warray,
        'q_vectors':q_vectors,
        'eigenvectors': eigenvectors
        })

    return file_data, element_isotope

#----------------------------------------------------------------------------------------


def _parse_phonon_file_header(f_handle):
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
            file_data['num_ions'] = int(line.strip().split()[-1])
        elif 'Number of branches' in line:
            file_data['num_branches'] = int(line.strip().split()[-1])
        elif 'Unit cell vectors' in line:
            file_data['unit_cell'] = _parse_phonon_unit_cell_vectors(f_handle)
        elif 'Fractional Co-ordinates' in line:
            if file_data['num_ions'] is None:
                raise IOError("Failed to parse file. Invalid file header.")

            # Extract the mode number for each of the ion in the data file
            for _ in range(file_data['num_ions']):
                line = f_handle.readline()
                line_data = line.strip().split()

                species = line_data[4]
                ion = {'species': species}
                ion['fract_coord'] = np.array([float(line_data[1]), float(line_data[2]), float(line_data[3])])
                ion['isotope_number'] = float(line_data[5])
                element_isotope[species] = float(line_data[5])
                # -1 to convert to zero based indexing
                ion['index'] = int(line_data[0]) - 1
                ion['bond_number'] = len([i for i in file_data['ions'] if i['species'] == species]) + 1
                file_data['ions'].append(ion)

        if 'END header' in line:
            if file_data['num_ions'] is None or file_data['num_branches'] is None:
                raise IOError("Failed to parse file. Invalid file header.")
            return file_data

#----------------------------------------------------------------------------------------


def _parse_phonon_freq_block(f_handle, num_branches):
    """
    Iterator to parse a block of frequencies from a .phonon file.

    @param f_handle - handle to the file.
    """
    for _ in range(num_branches):
        line = f_handle.readline()
        line_data = line.strip().split()[1:]
        line_data = [float(x) for x in line_data]
        yield line_data

#----------------------------------------------------------------------------------------


def _parse_phonon_unit_cell_vectors(f_handle):
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

#----------------------------------------------------------------------------------------


def _parse_phonon_eigenvectors(f_handle, num_ions, num_branches):
    vectors = []
    for _ in range(num_ions * num_branches):
        line = f_handle.readline()

        if not line:
            raise IOError("Could not parse file. Invalid file format.")

        line_data = line.strip().split()
        vector_componets = line_data[2::2]
        vector_componets = [float(x) for x in vector_componets]
        vectors.append(vector_componets)

    return np.asarray(vectors)

#----------------------------------------------------------------------------------------
