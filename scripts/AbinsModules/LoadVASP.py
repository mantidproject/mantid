# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from itertools import chain
import re
from typing import Any, Dict, Optional
from xml.etree import ElementTree

import numpy as np
import AbinsModules
from AbinsModules.AbinsData import AbinsData
from AbinsModules.AbinsConstants import COMPLEX_TYPE, FLOAT_TYPE, HZ2INV_CM, VASP_FREQ_TO_THZ


class LoadVASP(AbinsModules.GeneralAbInitioProgram):
    """
    Class which handles loading files from VASP output files.

    Both OUTCAR and vasprun.xml files are supported for calculations performed
    with the internal VASP vibrations routines (IBRION=5,6,7,8). Note that
    these calculations only sample the Gamma-point in reciprocal space.

    """

    def __init__(self, input_ab_initio_filename) -> None:
        """

        :param input_ab_initio_filename: name of file with phonon data (foo.phonon)
        """
        super().__init__(input_ab_initio_filename=input_ab_initio_filename)
        self._ab_initio_program = "VASP"

    def read_vibrational_or_phonon_data(self) -> AbinsData:
        input_filename = self._clerk.get_input_filename()

        if input_filename[-4:] == '.xml':
            data = self._read_vasprun(input_filename)
            self._num_atoms = len(data['atoms'])
            self._num_k = 1

        elif 'OUTCAR' in input_filename:
            logger.warning("Reading OUTCAR file. This feature is included for development purposes "
                           "and may be removed in future versions. Please use the vasprun.xml file where possible.")
            data = self._read_outcar(input_filename)
            self._num_atoms = len(data['atoms'])
            self._num_k = 1

        else:
            raise ValueError('Cannot guess format from filename "{}". '
                             'Expected *.xml or *OUTCAR*')

        self.save_ab_initio_data(data=data)
        return self._rearrange_data(data=data)

    @classmethod
    def _read_outcar(cls, filename) -> Dict[str, Any]:
        # Vibration calculations within Vasp only calculate the Hessian
        # within the calculation cell: i.e. they only include Gamma-point

        file_data = {'k_vectors': np.array([[0, 0, 0]], dtype=FLOAT_TYPE),
                     'weights': np.array([1.], dtype=FLOAT_TYPE),
                     'atoms': {}}

        parser = AbinsModules.GeneralAbInitioParser()

        with open(filename, 'rb') as fd:
            # Lattice vectors are found first, with block formatted e.g.
            #
            #  Lattice vectors:

            # A1 = (  17.7648860000,   0.0000000000,   0.0000000000)
            # A2 = (   0.0000000000,  18.0379140000,   0.0000000000)
            # A3 = (   0.0000000000,   0.0000000000,  18.3144580000)
            unit_cell = np.empty((3, 3), dtype=float)
            _ = parser.find_first(file_obj=fd, msg='Lattice vectors:')
            _ = fd.readline() # skip a line

            for i in range(3):
                lattice_line = fd.readline().decode("utf-8")
                float_re = r'\s+(\d+\.\d+)'
                lattice_re = (r'^\sA' + str(i + 1) + r' = \('
                              + f'{float_re},{float_re},{float_re}\\)$')
                match = re.match(lattice_re, lattice_line)
                if not match:
                    print(lattice_re, lattice_line)
                    raise ValueError("Something went wrong while reading lattice vectors from OUTCAR")
                unit_cell[i, :] = list(map(float, match.groups()))
            file_data['unit_cell'] = unit_cell

            # getting element symbols from an OUTCAR is a bit cumbersome as
            # VASP tracks this in groups rather than per-atom
            ion_counts_line = parser.find_first(file_obj=fd, msg='ions per type =')
            ion_counts = list(map(int, ion_counts_line.split()[4:]))

            # Get ionic masses; these are reported for each PP, then grouped
            # together on a line e.g.
            #    POMASS =  12.01  1.00
            parser.find_last(file_obj=fd, msg="POMASS")
            masses_line = fd.readline()
            ion_count_masses = list(map(float, masses_line.decode("utf-8").split()[2:]))
            masses = list(chain(*[[mass] * ion_count
                                  for mass, ion_count in zip(ion_count_masses, ion_counts)]))

        # Re-open file as we need to backtrack to the pseudopotential info.
        # The rest of the file data can now be gathered in one pass.
        with open(filename, 'rb') as fd:
            symbol_lines = [parser.find_first(file_obj=fd, msg='VRHFIN') for _ in ion_counts]
            ion_count_symbols = [re.match(r'^\s+VRHFIN =(\w+):', line.decode("utf-8")).groups()[0]
                                 for line in symbol_lines]
            symbols = list(chain(*[[symbol] * ion_count
                                   for symbol, ion_count in zip(ion_count_symbols, ion_counts)]))

            eig_header = 'Eigenvectors and eigenvalues of the dynamical matrix'
            parser.find_first(file_obj=fd, msg=eig_header)

            eig_pattern = (r'^\s*\d+\s+f(/i|\s+)=\s+\d+\.\d+\s+THz'
                           r'\s+\d+\.\d+\s+2PiTHz\s+\d+\.\d+\s+cm\-1'
                           r'\s+\d+\.\d+\smeV$')

            first_eigenvalue_line = parser.find_first(file_obj=fd, regex=eig_pattern)

            # skip X Y Z header line
            _ = fd.readline()

            # Read positions and first eigenvector
            first_eigenvector_data = []
            for line in fd:
                data_line = line.decode("utf-8").split()
                if len(data_line) == 0:
                    break
                first_eigenvector_data.append(list(map(float, data_line)))
            else:
                raise ValueError("OUTCAR terminated during eigenvalue block")

            positions = [row[:3] for row in first_eigenvector_data]
            if len(positions) != len(symbols):
                raise IndexError("Number of eigenvectors in OUTCAR does not match "
                                 "the number element symbols determined from 'ions per type' line. "
                                 "Something must have gone wrong while reading these.")
            for i, (position, symbol, mass) in enumerate(zip(positions, symbols, masses)):
                ion_data = {"symbol": symbol,
                            "coord": np.array(position),
                            "sort": i,  # identifier for symmetry-equivalent sites; mark all as unique
                            "mass": mass}
                file_data["atoms"].update({f"atom_{i}": ion_data})

            n_atoms = len(positions)
            n_frequencies = n_atoms * 3

            file_data['frequencies'] = np.zeros((1, n_frequencies), dtype=FLOAT_TYPE)
            file_data['frequencies'][0, 0] = cls._line_to_eigenvalue(first_eigenvalue_line)

            # Eigenvectors are collected in order (kpt, mode, atom, direction) to suit the
            # output format.
            eigenvectors = np.zeros((1, n_frequencies, n_atoms, 3), dtype=COMPLEX_TYPE)
            eigenvectors[0, 0, :, :] = [row[-3:] for row in first_eigenvector_data]

            for i in range(1, n_frequencies):
                eigenvalue_line = parser.find_first(file_obj=fd, regex=eig_pattern)
                file_data['frequencies'][0, i] = cls._line_to_eigenvalue(eigenvalue_line)

                # skip X Y Z header line
                _ = fd.readline()

                eigenvector_data = [list(map(float, fd.readline().decode("utf-8").split()[-3:]))
                                    for _ in range(n_atoms)]
                eigenvectors[0, i, :, :] = eigenvector_data

            # Re-arrange eigenvectors to (kpt, atom, mode, direction) indices for Abins
            file_data['atomic_displacements'] = np.swapaxes(eigenvectors, 1, 2)

        return file_data

    @staticmethod
    def _line_to_eigenvalue(line: bytes) -> float:
        """Extract the frequency in cm-1 from OUTCAR bytes

        Typical line resembles:

        23 f/i=   25.989078 THz   163.294191 2PiTHz  866.902290 cm-1   107.482224 meV

        the f/i indicates that the mode is imaginary, this is
        represented by returning the negative number -866.902290

        """
        line = line.decode("utf-8")
        # Keep the value reported in cm-1
        imaginary_factor = -1 if 'f/i' in line else 1
        return float(line.split()[-4]) * imaginary_factor

    @staticmethod
    def _read_vasprun(filename: str,
                      diagonalize: bool = False,
                      apply_sum_rule: bool = False) -> Dict[str, Any]:

        file_data = {'k_vectors': np.array([[0, 0, 0]], dtype=FLOAT_TYPE),
                     'weights': np.array([1.], dtype=FLOAT_TYPE),
                     'atoms': {}}

        root = ElementTree.parse(filename).getroot()

        def _find_or_error(etree: ElementTree,
                           tag: str,
                           name: Optional[str] = None,
                           message: Optional[str] = None) -> ElementTree:
            for element in etree.findall(tag):
                if name is None:
                    return element
                elif name == element.get('name'):
                    return element
            else:
                if message is None:
                    raise ValueError("Could not find {tag}{name} in {parent} section of VASP XML file.".format(
                        tag=tag, name=(f' (name={name})' if name else ''), parent=etree.tag))
                else:
                    raise ValueError(message)

        structure_block = _find_or_error(root, 'structure', name='finalpos')
        varray = _find_or_error(_find_or_error(structure_block, 'crystal'),
                                'varray', name='basis')

        lattice_vectors = [list(map(float, v.text.split())) for v in varray.findall('v')]
        file_data['unit_cell'] = np.asarray(lattice_vectors, dtype=FLOAT_TYPE)
        if file_data['unit_cell'].shape != (3, 3):
            raise AssertionError("Lattice vectors in XML 'finalpos' don't look like a 3x3 matrix")

        varray = _find_or_error(structure_block, 'varray', name='positions')
        positions = np.asarray([list(map(float, v.text.split())) for v in varray.findall('v')],
                               dtype=FLOAT_TYPE)
        if len(positions.shape) != 2 or positions.shape[1] != 3:
            raise AssertionError("Positions in XML 'finalpos' don't look like an Nx3 array.")

        atom_info = _find_or_error(root, 'atominfo')

        # Get list of atoms by type e.g. [1, 1, 2, 2, 2, 2, 2, 2] for C2H6
        atom_types = [int(rc.findall('c')[1].text)
                      for rc in _find_or_error(_find_or_error(atom_info, 'array', name='atoms'),'set').findall('rc')]

        # Get symbols and masses corresponding to these types, and construct full lists of atom properties
        atom_data = [rc.findall('c')
                     for rc in _find_or_error(_find_or_error(atom_info, 'array', name='atomtypes'), 'set').findall('rc')]
        type_symbols = [data_row[1].text.strip() for data_row in atom_data]
        type_masses = [float(data_row[2].text) for data_row in atom_data]
        symbols = [type_symbols[i - 1] for i in atom_types]
        masses = [type_masses[i - 1] for i in atom_types]

        for i, (position, symbol, mass) in enumerate(zip(positions, symbols, masses)):
            ion_data = {"symbol": symbol,
                        "coord": np.array(position),
                        "sort": i,  # identifier for symmetry-equivalent sites; mark all as unique
                        "mass": mass}
            file_data["atoms"].update({f"atom_{i}": ion_data})

        calculation = _find_or_error(root, 'calculation')
        dynmat = _find_or_error(calculation, 'dynmat')
        # vasprun.xml reports raw eigenvectors in atomic units: convert to frequencies in cm-1
        eigenvalues = _find_or_error(dynmat, 'v', name='eigenvalues').text.split()
        frequencies = np.sqrt(-np.asarray(list(map(float, eigenvalues)),
                                          dtype=complex)) * VASP_FREQ_TO_THZ * 1e12 * HZ2INV_CM
        # store imaginary frequencies as -ve
        file_data['frequencies'] = np.asarray([frequencies.real + frequencies.imag], dtype=FLOAT_TYPE)

        eigenvectors_block = _find_or_error(dynmat, 'varray', name='eigenvectors')
        # Read data in (mode, atoms * 3) format
        eigenvectors = [list(map(float, row.text.split())) for row in eigenvectors_block.findall('v')]
        # Rearrange XYZ components onto a new axis to get (mode, atom, direction) indices
        eigenvectors = [np.array(row).reshape(-1,3) for row in eigenvectors]

        # Wrap in an outer list (1 k-point) then rearrange: (kpt, mode, atom, direction) -> (kpt, atom, mode, direction)
        file_data['atomic_displacements'] = np.swapaxes(np.asarray([eigenvectors], dtype=COMPLEX_TYPE),
                                                        1, 2)

        return file_data
