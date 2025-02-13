# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import io
from pathlib import Path

import numpy as np

from .textparser import TextParser
from .abinitioloader import AbInitioLoader
from abins.constants import CRYSTAL, FLOAT_TYPE
from mantid.kernel import Atom, logger


class CRYSTALLoader(AbInitioLoader):
    """
    Class for loading CRYSTAL ab initio vibrational or phonon data. Special thanks to Leonardo Bernasconi for
    contributing to this module.
    """

    def __init__(self, input_ab_initio_filename=None, cache_directory: Path | None = None):
        """
        :param input_ab_initio_filename: name of a file with vibrational or phonon data (foo.out)
        """
        super().__init__(input_ab_initio_filename=input_ab_initio_filename, cache_directory=cache_directory)

        self._num_k = None
        self._num_modes = None
        self._num_atoms = None

        # Transformation (expansion) matrix E
        # More info in 'Creating a super cell' at
        # http://www.theochem.unito.it/crystal_tuto/mssc2008_cd/tutorials/geometry/geom_tut.html
        self._inv_expansion_matrix = np.eye(3, dtype=FLOAT_TYPE)
        self._parser = TextParser()

    @property
    def _ab_initio_program(self) -> str:
        return "CRYSTAL"

    def read_vibrational_or_phonon_data(self):
        """
        Reads vibrational or phonon data from CRYSTAL output files. Saves frequencies, weights of k-point vectors,
        k-point vectors, amplitudes of atomic displacements, hash of the vibrational or phonon data file (hash) to
        <>.hdf5.
        :return  object of type AbinsData.
        """

        # determine system (molecule or crystal?)
        system = self._determine_system()

        # check if one or more k-points to parse
        phonon_dispersion = self._determine_dispersion()

        # read data from output CRYSTAL file
        filename = self._clerk.get_input_filename()
        with io.open(filename, "rb") as crystal_file:
            logger.notice("Reading from " + filename)

            if system is CRYSTAL:
                lattice_vectors = self._read_lattice_vectors(file_obj=crystal_file)
            else:
                lattice_vectors = [[0, 0, 0]] * 3

            coord_lines = self._read_atomic_coordinates(file_obj=crystal_file)
            masses = self._read_masses_from_file(file_obj=crystal_file)

            freq, coordinates, weights, k_coordinates = self._read_modes(file_obj=crystal_file, phonon_dispersion=phonon_dispersion)

        # put data into Abins data structure
        data = {}
        self._create_atoms_data(data=data, coord_lines=coord_lines[: self._num_atoms], atoms_masses=masses[: self._num_atoms])
        self._create_kpoints_data(
            data=data,
            freq=freq,
            atomic_displacements=coordinates,
            atomic_coordinates=coord_lines[: self._num_atoms],
            weights=weights,
            k_coordinates=k_coordinates,
            unit_cell=lattice_vectors,
        )

        # save data to hdf file
        self.save_ab_initio_data(data=data)

        # return AbinsData object
        return self._rearrange_data(data=data)

    def _determine_system(self):
        """
        Determines whether the system is a molecule or a crystal.
        :returns: True if calculation for molecule otherwise False
        """
        with io.open(self._clerk.get_input_filename(), "rb") as crystal_file:
            lines = crystal_file.read()

        if b"MOLECULAR CALCULATION" in lines or b"0D - MOLECULE" in lines:
            molecular = True
        elif b"CRYSTAL CALCULATION" in lines or b"3D - CRYSTAL" in lines:
            molecular = False
        else:
            raise ValueError("Only molecular or 3D CRYSTAL systems can be processed")

        if molecular:
            logger.notice("This run is for a MOLECULAR system")
        else:
            logger.notice("This run is for a 3D CRYSTAL system")

        return molecular

    def _determine_dispersion(self):
        """
        Checks if we have data for more than one k-point. If data for more than one k-point then calculates
        transformation matrix to primitive unit cell from super cell.
        :returns: True if many k-points included in calculations otherwise False
        """
        with io.open(self._clerk.get_input_filename(), "rb") as crystal_file:
            lines = crystal_file.read()

        phonon_dispersion = lines.count(b"DISPERSION K ") > 1

        if phonon_dispersion:
            # In case there is more than one k-point super-cell is constructed. In order to obtain metric tensor we
            # need to find expansion transformation.
            with io.open(self._clerk.get_input_filename(), "rb") as crystal_file:
                self._parser.find_first(file_obj=crystal_file, msg="EXPANSION MATRIX OF PRIMITIVE CELL")
                dim = 3
                vectors = []
                for i in range(dim):
                    line = crystal_file.readline().split()[1:]
                    vector = [float(item) for item in line]
                    vectors.append(vector)
            temp = np.asarray(vectors).astype(dtype=FLOAT_TYPE, casting="safe")
            self._inv_expansion_matrix = np.linalg.inv(temp)

        return phonon_dispersion

    def _read_lattice_vectors(self, file_obj=None):
        """
        Reads lattice vectors from .out CRYSTAL file.
        :param file_obj:  file object from which we read
        :returns: list with lattice vectors
        """
        self._parser.find_first(file_obj=file_obj, msg="DIRECT LATTICE VECTORS CARTESIAN COMPONENTS (ANGSTROM)")
        file_obj.readline()  # Line: X                    Y                    Z
        dim = 3
        vectors = []
        for i in range(dim):
            line = file_obj.readline()
            line = line.split()
            vector = [float(item) for item in line]
            vectors.append(vector)
        return vectors

    def _read_atomic_coordinates(self, file_obj=None):
        """
        Reads atomic coordinates from .out CRYSTAL file.
        :param file_obj:  file object from which we read
        :returns: list with atomic coordinates
        """
        coord_lines = []
        self._parser.find_first(file_obj=file_obj, regex=r".*\s+ATOM\s+X\(ANGSTROM\)\s+Y\(ANGSTROM\)\s+Z\(ANGSTROM\)\s*$")

        file_obj.readline()  # Line: *******************************************************************************

        while not self._parser.file_end(file_obj=file_obj):
            line = file_obj.readline().replace(b"T", b"")
            # At the end of this section there is always empty line.
            if not line.strip():
                break
            coord_lines += [line.strip(b"\n")]

        for line in coord_lines:
            logger.debug(line)

        return coord_lines

    def _read_modes(self, file_obj=None, phonon_dispersion=None):
        """
        Reads vibrational or phonon modes (frequencies and atomic displacements).
        :param phonon_dispersion: True if more then one k-point to parse, otherwise False.
        :param file_obj: file object from which we read
        :returns: Tuple with frequencies and corresponding atomic displacements, weights of k-points and coordinates of
                 k-points
        """
        # case of more than one k-point
        if phonon_dispersion:
            num_k = self._get_num_kpoints(file_obj=file_obj)
            weights = []
            k_coordinates = []
            freq = []

            all_coord = []
            # parse all k-points
            for k in range(num_k):
                line = self._parser.find_first(file_obj=file_obj, msg="DISPERSION K POINT NUMBER")
                partial_freq = []
                xdisp = []
                ydisp = []
                zdisp = []

                local_line = line.replace(b"(", b" ").replace(b")", b" ").split()

                k_coordinates.append([float(local_line[7]), float(local_line[8]), float(local_line[9])])
                weights.append(float(local_line[11]))
                k_point_type = local_line[6]

                # parse k-points for which atomic displacements are real
                if k_point_type == b"R":
                    while not self._parser.file_end(file_obj=file_obj):
                        self._read_freq_block(file_obj=file_obj, freq=partial_freq)
                        self._read_coord_block(file_obj=file_obj, xdisp=xdisp, ydisp=ydisp, zdisp=zdisp)

                        if self._parser.block_end(file_obj=file_obj, msg=["DISPERSION K POINT NUMBER"]):
                            break
                        if not self._inside_k_block(file_obj=file_obj):
                            break

                # parse k-points for which atomic displacements are complex
                elif k_point_type == b"C":
                    real_partial_xdisp = []
                    real_partial_ydisp = []
                    real_partial_zdisp = []

                    complex_partial_xdisp = []
                    complex_partial_ydisp = []
                    complex_partial_zdisp = []

                    while not self._parser.file_end(file_obj=file_obj):
                        self._read_freq_block(file_obj=file_obj, freq=partial_freq)
                        self._read_coord_block(
                            file_obj=file_obj, xdisp=real_partial_xdisp, ydisp=real_partial_ydisp, zdisp=real_partial_zdisp, part="real"
                        )
                        if self._parser.block_end(file_obj=file_obj, msg=["IMAGINARY", "MODES IN ANTI-PHASE"]):
                            break

                    while not self._parser.file_end(file_obj=file_obj):
                        self._read_freq_block(file_obj=file_obj, freq=partial_freq, append=False)
                        self._read_coord_block(
                            file_obj=file_obj,
                            xdisp=complex_partial_xdisp,
                            ydisp=complex_partial_ydisp,
                            zdisp=complex_partial_zdisp,
                            part="imaginary",
                        )

                        if self._parser.block_end(file_obj=file_obj, msg=["DISPERSION K POINT NUMBER"]):
                            break
                        if not self._inside_k_block(file_obj=file_obj):
                            break

                    # reconstruct complex atomic displacements
                    for el in range(len(real_partial_xdisp)):
                        xdisp.append(real_partial_xdisp[el] + complex_partial_xdisp[el])
                        ydisp.append(real_partial_ydisp[el] + complex_partial_ydisp[el])
                        zdisp.append(real_partial_zdisp[el] + complex_partial_zdisp[el])

                else:
                    raise ValueError("Invalid format of input file ", self._clerk.get_input_filename())

                freq.append(partial_freq)
                all_coord.append([xdisp, ydisp, zdisp])

        # only one k-point
        else:
            end_msgs = ["******", "ACLIMAX"]
            inside_block = True
            freq = []
            xdisp = []
            ydisp = []
            zdisp = []

            # parse block with frequencies and atomic displacements
            while not self._parser.file_end(file_obj=file_obj) and inside_block:
                self._read_freq_block(file_obj=file_obj, freq=freq)
                self._read_coord_block(file_obj=file_obj, xdisp=xdisp, ydisp=ydisp, zdisp=zdisp)

                if self._parser.block_end(file_obj=file_obj, msg=end_msgs):
                    break

            freq = [freq]
            weights = [1.0]
            k_coordinates = [[0.0, 0.0, 0.0]]
            all_coord = [[xdisp, ydisp, zdisp]]

        self._num_k = len(freq)

        self._num_modes = len(freq[0])
        if self._num_modes % 3 == 0:
            self._num_atoms = int(self._num_modes / 3)
        else:
            raise ValueError("Invalid number of modes.")

        return freq, all_coord, weights, k_coordinates

    def _read_freq_block(self, file_obj=None, freq=None, append=True):
        """
        Parses block with frequencies.
        :param append:
        :param file_obj: file object from which we read
        :param freq: list with frequencies which we update
        """
        line = self._parser.find_first(file_obj=file_obj, msg="FREQ(CM**-1)")

        if append:
            freq.extend([float(item) for item in line.replace(b"\n", b" ").replace(b"FREQ(CM**-1)", b" ").split()])

    def _read_coord_block(self, file_obj=None, xdisp=None, ydisp=None, zdisp=None, part="real"):
        """
        Parses block with coordinates.
        :param file_obj: file object from which we read
        :param xdisp: list with x coordinates which we update
        :param ydisp: list with y coordinates which we update
        :param zdisp: list with z coordinates which we update
        """
        self._parser.move_to(file_obj=file_obj, msg="AT.")
        while not self._parser.file_end(file_obj=file_obj):
            pos = file_obj.tell()
            line = file_obj.readline()

            if line.strip():
                if b" X " in line:
                    for item in line[14:].strip(b"\n").split():
                        self._parse_item(item=item, container=xdisp, part=part)
                elif b" Y " in line:
                    for item in line[14:].strip(b"\n").split():
                        self._parse_item(item=item, container=ydisp, part=part)
                elif b" Z " in line:
                    for item in line[14:].strip(b"\n").split():
                        self._parse_item(item=item, container=zdisp, part=part)
                else:
                    file_obj.seek(pos)
                    break

    def _parse_item(self, item=None, container=None, part=None):
        if part == "real":
            container.append(complex(float(item), 0.0))
        elif part == "imaginary":
            container.append(complex(0.0, float(item)))
        else:
            raise ValueError("Real or imaginary part of complex number was expected.")

    def _inside_k_block(self, file_obj=None):
        """
        Checks if end of k-points block.
        :param file_obj: file object from which we read
        :returns: True if end of block otherwise False
        """
        allowed_keywords = [b" X ", b" Y ", b" Z ", b"-", b"REAL", b"IMAGINARY", b"MODES", b"DISPERSION"]

        # remove empty lines:
        pos = None
        while not self._parser.file_end(file_obj=file_obj):
            pos = file_obj.tell()
            line = file_obj.readline()
            if line.strip():
                break
        file_obj.seek(pos)

        # non empty line to check
        pos = file_obj.tell()
        line = file_obj.readline()
        file_obj.seek(pos)

        # if there isn't any keyword from set "allowed_keywords" it means that we reached end of k-block
        # if any keyword in line we are still in k-block
        return any([key in line for key in allowed_keywords])

    def _get_num_kpoints(self, file_obj=None):
        self._parser.find_first(file_obj=file_obj, msg="K       WEIGHT       COORD")
        num_k = 0
        while not self._parser.file_end(file_obj=file_obj):
            line = file_obj.readline()
            if b"WITH SHRINKING FACTORS:" in line:
                return num_k
            num_k += 1

    def _create_atoms_data(self, data=None, coord_lines=None, atoms_masses=None):
        """
        Creates Python dictionary with atoms data which can be easily converted to AbinsData object.
        :param atoms_masses: atom masses from output ab-initio file
        :param data: Python dictionary to which found atoms data should be added
        :param coord_lines: list with information about atoms
        """
        data.update({"atoms": dict()})

        for i, line in enumerate(coord_lines):
            elements = line.split()
            symbol = str(elements[2].decode("utf-8").capitalize())

            atom = Atom(symbol=symbol)
            data["atoms"]["atom_{}".format(i)] = {
                "symbol": symbol,
                "mass": atom.mass,
                "sort": i,
                "coord": np.asarray(elements[3:6]).astype(dtype=FLOAT_TYPE),
            }

        self.check_isotopes_substitution(atoms=data["atoms"], masses=atoms_masses, approximate=True)

    def _create_kpoints_data(
        self, data=None, freq=None, atomic_displacements=None, atomic_coordinates=None, weights=None, k_coordinates=None, unit_cell=None
    ):
        """
        Creates Python dictionary with k-points data which can  be easily converted to AbinsData object.
        :param data: Python dictionary to which found k-points data should be added
        :param freq: normal modes
        :param atomic_displacements: atomic displacements
        :param atomic_coordinates: equilibrium atomic coordinates
        :param weights: weights of k-points
        :param k_coordinates: coordinates of k-points
        :param unit_cell: list with unit cell vectors
        """
        #     a) Put frequencies into dictionary
        data["frequencies"] = np.asarray(freq).astype(dtype=FLOAT_TYPE, casting="safe")

        #     b) Extract atomic displacements, normalize them and put them into data dictionary
        # Extract
        all_k_atomic_disp = [
            self._create_kpoint_data(freq=freq[k], atomic_displacements=atomic_displacements[k], atomic_coordinates=atomic_coordinates)
            for k in range(self._num_k)
        ]

        # normalise
        all_k_atomic_disp = np.asarray(all_k_atomic_disp)
        masses = np.asarray([data["atoms"]["atom_%s" % atom]["mass"] for atom in range(self._num_atoms)])

        # [num_k ,num_freq, num_atoms, dim] -> [num_k, num_freq, num_atoms, dim, dim] -> [num_k, num_freq, num_atoms]
        temp1 = np.trace(np.einsum("mlki, mlkj->mlkij", all_k_atomic_disp, all_k_atomic_disp.conjugate()), axis1=3, axis2=4)
        temp2 = np.einsum("mij, j->mij", temp1, masses)

        # [num_k, num_freq, num_atoms] -> [num_k, num_freq]
        norm = np.sum(temp2, axis=2)

        # noinspection PyTypeChecker
        all_k_atomic_disp = np.einsum("mijk,mi->mijk", all_k_atomic_disp, 1.0 / np.sqrt(norm))
        all_k_atomic_disp = np.einsum("mijk,j->mijk", all_k_atomic_disp, np.sqrt(masses))

        # [num_k, num_freq, num_atoms, dim] ->  [num_k, num_atoms, num_freq, dim]
        data["atomic_displacements"] = np.transpose(a=all_k_atomic_disp, axes=(0, 2, 1, 3))

        # c) Put weights into dictionary
        data["weights"] = np.asarray(weights).astype(dtype=FLOAT_TYPE, casting="safe")

        # d) Put k-vectors into dictionary
        data["k_vectors"] = np.asarray(k_coordinates).astype(dtype=FLOAT_TYPE, casting="safe")
        # e) put unit cell into dictionary
        temp = np.asarray(unit_cell).astype(dtype=FLOAT_TYPE, casting="safe")
        data["unit_cell"] = np.dot(self._inv_expansion_matrix, temp)

    def _create_kpoint_data(self, freq=None, atomic_displacements=None, atomic_coordinates=None):
        """
        Extracts atomic displacements.
        :param freq: normal modes for the given k-point
        :param atomic_displacements: atomic displacements for the given k-point
        :param atomic_coordinates: atomic coordinates (equilibrium positions)
        :return normalised atomic displacements in the form of numpy array
        """
        column_num = -1
        freq_num = -1
        row_num = 0
        default_row_width = 6  # default width of block with modes
        displacements = []
        num_displacements = len(atomic_displacements[0])
        num_coordinates = len(atomic_coordinates)

        for _ in freq:
            column_num += 1
            freq_num += 1
            if column_num == default_row_width:
                column_num = 0
                row_num += 1

            # Parse blocks with default row width (6)
            if row_num <= num_displacements / (default_row_width * num_coordinates) - 1:
                displacements.extend(
                    self.create_kpoints_data_helper(
                        atomic_displacements=atomic_displacements, atomic_coordinates=atomic_coordinates, row=row_num, column=column_num
                    )
                )

            # At this point we have parsed all the modes that are
            # part of blocks of 6 in the crystal output; now we need to
            # consider the other blocks
            elif num_displacements % default_row_width != 0:
                current_row_width = num_displacements % default_row_width
                displacements.extend(
                    self.create_kpoints_data_helper(
                        atomic_displacements=atomic_displacements,
                        atomic_coordinates=atomic_coordinates,
                        row=row_num,
                        column=column_num,
                        row_width=current_row_width,
                    )
                )

        # Reshape displacements so that Abins can use it to create its internal data objects
        # num_atoms: number of atoms in the system
        # num_freq: number of modes
        # dim: dimension for each atomic displacement (atoms vibrate in 3D space)
        #
        # The following conversion is necessary:
        # (num_freq * num_atom * dim) -> (num_freq, num_atom, dim)

        num_freq = len(freq)
        dim = 3

        displacements = np.asarray(a=displacements, order="C").reshape(num_freq, self._num_atoms, dim)

        return displacements

    def create_kpoints_data_helper(self, atomic_displacements=None, atomic_coordinates=None, row=None, column=None, row_width=6):
        """
        Extracts atomic displacements for the given row and column.
        :param atomic_displacements: list with atomic displacements
        :param atomic_coordinates: list with atomic coordinates
        :param row: number of atomic_displacements row to parse
        :param column: number of atomic_displacements column to parse
        :param row_width: current width of row to parse
        :return normalised atomic displacements
        """
        xdisp = atomic_displacements[0]
        ydisp = atomic_displacements[1]
        zdisp = atomic_displacements[2]

        atomic_coordinates_length = len(atomic_coordinates)
        atomic_coordinates_iter = range(atomic_coordinates_length)
        const = row * atomic_coordinates_length * 6 + column

        indices = [const + atom_num * row_width for atom_num in atomic_coordinates_iter]
        x = [xdisp[indx] for indx in indices]
        y = [ydisp[indx] for indx in indices]
        z = [zdisp[indx] for indx in indices]

        local_displacements = np.transpose(np.asarray([x, y, z]))

        return local_displacements

    def _read_masses_from_file(self, file_obj):
        masses = []
        pos = file_obj.tell()
        self._parser.find_first(file_obj=file_obj, msg="ATOMS ISOTOPIC MASS (AMU) FOR FREQUENCY CALCULATION ")
        file_obj.readline()  # blank line
        end_message = ["INFORMATION", "*******************************************************************************", "GAMMA"]

        for i, item in enumerate(end_message):
            end_message[i] = bytes(item, "utf8")

        while not self._parser.file_end(file_obj=file_obj):
            line = file_obj.readline()
            if any([word in line for word in end_message]):
                break

            items = line.split()
            length = len(items)
            for i in range(2, length, 3):
                masses.append(float(items[i]))
        file_obj.seek(pos)  # revert position of file pointer to the initial state
        return masses
