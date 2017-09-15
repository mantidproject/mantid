#pylint: disable=redefined-builtin
from __future__ import (absolute_import, division, print_function)
from six.moves import range

import re
import numpy as np

import dos.load_helper as load_helper


def parse_castep_file(file_name, ir_or_raman):
    """
    Read frequencies from a <>.castep file

    @param file_name - file path of the file to read
    @return the frequencies, infra red and raman intensities and weights of frequency blocks
    """

    file_data = {}

    # Get Regex strings from load_helper
    header_regex = re.compile(load_helper.CASTEP_HEADER_REGEX)
    data_regex = re.compile(load_helper.CASTEP_DATA_REGEX)
    bond_regex = re.compile(load_helper.CASTEP_BOND_REGEX)

    block_count = 0
    frequencies, ir_intensities, raman_intensities, weights, q_vectors, bonds = [], [], [], [], [], []
    data_lists = (frequencies, ir_intensities, raman_intensities)
    with open(file_name, 'rU') as f_handle:
        file_data.update(_parse_castep_file_header(f_handle))

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

                # Move file pointer forward to start of intensity data
                _find_castep_freq_block(f_handle, data_regex)

                # Parse block of frequencies
                for line_data in _parse_castep_freq_block(f_handle, file_data['num_branches'], ir_or_raman):
                    for data_list, item in zip(data_lists, line_data):
                        data_list.append(item)

            # Check if we've found a bond
            bond_match = bond_regex.match(line)
            if bond_match:
                bonds.append(_parse_castep_bond(bond_match))

    frequencies = np.asarray(frequencies)
    ir_intensities = np.asarray(ir_intensities)
    raman_intensities = np.asarray(raman_intensities)
    warray = np.repeat(weights, file_data['num_branches'])

    file_data.update({
        'frequencies': frequencies,
        'ir_intensities': ir_intensities,
        'raman_intensities': raman_intensities,
        'weights': warray,
        'q_vectors':q_vectors
        })

    if len(bonds) > 0:
        file_data['bonds'] = bonds

    return file_data

#----------------------------------------------------------------------------------------


def _parse_castep_file_header(f_handle):
    """
    Read information from the header of a <>.castep file

    @param f_handle - handle to the file.
    @return tuple of the number of ions and branches in the file
    """
    num_species = 0
    file_data = {}
    while True:
        line = f_handle.readline()

        if not line:
            raise IOError("Could not find any header information.")

        if 'Total number of ions in cell =' in line:
            file_data['num_ions'] = int(line.strip().split()[-1])
        elif 'Total number of species in cell = ' in line:
            num_species = int(line.strip().split()[-1])

        if num_species > 0 and file_data['num_ions'] > 0:
            file_data['num_branches'] = num_species * file_data['num_ions']
            return file_data

#----------------------------------------------------------------------------------------


def _parse_castep_freq_block(f_handle, num_branches, ir_or_raman):
    """
    Iterator to parse a block of frequencies from a .castep file.

    @param f_handle - handle to the file.
    """

    for _ in range(num_branches):
        line = f_handle.readline()
        line_data = line.strip().split()[1:-1]
        freq = line_data[1]
        intensity_data = line_data[3:]

        # Remove non-active intensities from data
        intensities = []
        for value, active in zip(intensity_data[::2], intensity_data[1::2]):
            if ir_or_raman:
                if active == 'N' and value != 0:
                    value = 0.0
            intensities.append(value)

        line_data = [freq] + intensities
        line_data = [float(x) for x in line_data]
        yield line_data


#----------------------------------------------------------------------------------------

def _find_castep_freq_block(f_handle, data_regex):
    """
    Find the start of the frequency block in a .castep file.
    This will set the file pointer to the line before the start
    of the block.

    @param f_handle - handle to the file.
    """
    while True:
        pos = f_handle.tell()
        line = f_handle.readline()

        if not line:
            raise IOError("Could not parse frequency block. Invalid file format.")

        if data_regex.match(line):
            f_handle.seek(pos)
            return

#----------------------------------------------------------------------------------------


def _parse_castep_bond(bond_match):
    """
    Parses a regex match to obtain bond information.

    @param bond_match Regex match to bond data line
    @return A dictionary defining the bond
    """
    bond = dict()

    bond['atom_a'] = (bond_match.group(1), int(bond_match.group(2)))
    bond['atom_b'] = (bond_match.group(3), int(bond_match.group(4)))
    bond['population'] = float(bond_match.group(5))
    bond['length'] = float(bond_match.group(6))

    return bond

#----------------------------------------------------------------------------------------
