from __future__ import (absolute_import, division, print_function)

import io
from math import sqrt

import numpy as np

import AbinsModules
from mantid.kernel import Atom, logger


class LoadCRYSTAL(AbinsModules.GeneralDFTProgram):
    """
    Class for loading CRYSTAL DFT phonon data. Special thanks to Leonardo Bernasconi for contributing to this module.
    """
    def __init__(self, input_dft_filename=None):
        """
        :param input_dft_filename: name of a file with phonon data (foo.out)
        """
        super(LoadCRYSTAL, self).__init__(input_dft_filename=input_dft_filename)

        self._num_k = None
        self._num_modes = None
        self._num_atoms = None

        # Transformation (expansion) matrix E
        # More info in 'Creating a super cell' at
        # http://www.theochem.unito.it/crystal_tuto/mssc2008_cd/tutorials/geometry/geom_tut.html
        self._inv_expansion_matrix = np.eye(3, dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)
        self._parser = AbinsModules.GeneralDFTParser()

        self._dft_program = "CRYSTAL"

    def read_phonon_file(self):
        """
        Reads phonon data from CRYSTAL output files. Saves frequencies, weights of k-point vectors, k-point vectors,
        amplitudes of atomic displacements, hash of the phonon file (hash) to <>.hdf5

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

            if system is AbinsModules.AbinsConstants.CRYSTAL:
                lattice_vectors = self._read_lattice_vectors(file_obj=crystal_file)
            else:
                lattice_vectors = [[0, 0, 0]] * 3

            coord_lines = self._read_atomic_coordinates(file_obj=crystal_file)
            freq, coordinates, weights, k_coordinates = self._read_modes(file_obj=crystal_file,
                                                                         phonon_dispersion=phonon_dispersion)

        # put data into Abins data structure
        data = {}
        self._create_atoms_data(data=data, coord_lines=coord_lines[:self._num_atoms])
        self._create_kpoints_data(data=data, freq=freq, atomic_displacements=coordinates,
                                  atomic_coordinates=coord_lines[:self._num_atoms], weights=weights,
                                  k_coordinates=k_coordinates, unit_cell=lattice_vectors)

        # save data to hdf file
        self.save_dft_data(data=data)

        # return AbinsData object
        return self._rearrange_data(data=data)

    def _determine_system(self):
        """
        Determines whether the system is a molecule or a crystal.
        :return: True if calculation for molecule otherwise False
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
        :return: True if many k-points included in calculations otherwise False
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
            temp = np.asarray(vectors).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE, casting="safe")
            self._inv_expansion_matrix = np.linalg.inv(temp)

        return phonon_dispersion

    def _read_lattice_vectors(self, file_obj=None):
        """
        Reads lattice vectors from .out CRYSTAL file.
        :param file_obj:  file object from which we read
        :return: list with lattice vectors
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
        :return: list with atomic coordinates
        """
        coord_lines = []
        self._parser.find_first(file_obj=file_obj,
                                msg="ATOM          X(ANGSTROM)         Y(ANGSTROM)         Z(ANGSTROM)")

        file_obj.readline()  # Line: *******************************************************************************

        while not self._parser.file_end(file_obj=file_obj):
            line = file_obj.readline().replace(b"T", b"")
            # At the end of this section there is always empty line.
            if not line.strip():
                break
            coord_lines += [line.strip(b"\n")]

        for line in coord_lines:
            # convert from unicode to str in case of Python 2
            temp = str(line.strip(b"\n"))

            logger.debug(temp)

        return coord_lines

    def _read_modes(self, file_obj=None, phonon_dispersion=None):
        """
        Reads vibrational modes (frequencies and atomic displacements).
        :param phonon_dispersion: True if more then one k-point to parse, otherwise False.
        :param file_obj: file object from which we read
        :return: Tuple with frequencies and corresponding atomic displacements, weights of k-points and coordinates of
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
                        self._read_coord_block(file_obj=file_obj, xdisp=real_partial_xdisp,
                                               ydisp=real_partial_ydisp, zdisp=real_partial_zdisp, part="real")
                        if self._parser.block_end(file_obj=file_obj, msg=["IMAGINARY"]):
                            break

                    while not self._parser.file_end(file_obj=file_obj):

                        self._read_freq_block(file_obj=file_obj, freq=partial_freq, append=False)
                        self._read_coord_block(file_obj=file_obj, xdisp=complex_partial_xdisp,
                                               ydisp=complex_partial_ydisp, zdisp=complex_partial_zdisp,
                                               part="imaginary")

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
        :return: True if end of block otherwise False
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

    def _create_atoms_data(self, data=None, coord_lines=None):
        """
        Creates Python dictionary with atoms data which can be easily converted to AbinsData object.
        :param data: Python dictionary to which found atoms data should be added
        :param coord_lines: list with information about atoms
        """
        data.update({"atoms": dict()})
        for i, line in enumerate(coord_lines):
            l = line.split()
            symbol = str(l[2].decode("utf-8").capitalize())
            atom = Atom(symbol=symbol)
            data["atoms"]["atom_{}".format(i)] = {
                "symbol": symbol, "mass": atom.mass, "sort": i,
                "coord": np.asarray(l[3:6]).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)}

    def _create_kpoints_data(self, data=None, freq=None, atomic_displacements=None, atomic_coordinates=None,
                             weights=None, k_coordinates=None, unit_cell=None):
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
        data["frequencies"] = np.asarray(freq).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE, casting="safe")

        #     b) Normalise atomic displacements and put them into data dictionary
        all_kpoints = []
        for k in range(self._num_k):
            all_kpoints.append(self._create_kpoint_data(freq=freq[k], atomic_displacements=atomic_displacements[k],
                               atomic_coordinates=atomic_coordinates))

        data["atomic_displacements"] = np.asarray(all_kpoints)
        data["weights"] = np.asarray(weights).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE, casting="safe")
        data["k_vectors"] = np.asarray(k_coordinates).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE,
                                                             casting="safe")

        temp = np.asarray(unit_cell).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE, casting="safe")
        data["unit_cell"] = np.dot(self._inv_expansion_matrix, temp)

    def _create_kpoint_data(self, freq=None, atomic_displacements=None, atomic_coordinates=None):
        """
        Normalises atomic displacements.
        :param freq: normal modes for the given k-point
        :param atomic_displacements: atomic displacements for the given k-point
        :param atomic_coordinates: atomic coordinates
        :return normalised atomic displacements in the form of numpy array
        """
        #  Normalise atomic displacements and put them into data dictionary for the given k-point
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
                displacements.extend(self.create_kpoints_data_helper(
                    atomic_displacements=atomic_displacements, atomic_coordinates=atomic_coordinates, row=row_num,
                    column=column_num, freq_num=freq_num))

            # At this point we have parsed all the modes that are
            # part of blocks of 6 in the crystal output; now we need to
            # consider the other blocks
            elif num_displacements % default_row_width != 0:
                current_row_width = num_displacements % default_row_width
                displacements.extend(self.create_kpoints_data_helper(
                    atomic_displacements=atomic_displacements, atomic_coordinates=atomic_coordinates, row=row_num,
                    column=column_num, freq_num=freq_num, row_width=current_row_width))

        # Reshape displacements so that Abins can use it to create its internal data objects
        # num_atoms: number of atoms in the system
        # num_freq: number of modes
        # dim: dimension for each atomic displacement (atoms vibrate in 3D space)
        #
        # The following conversion is necessary:
        # (num_freq * num_atom * dim) -> (num_freq, num_atom, dim) -> (num_atom, num_freq, dim)

        num_freq = len(freq)
        dim = 3

        displacements = np.asarray(a=[displacements], order="C")
        displacements = np.reshape(a=displacements, newshape=(num_freq, self._num_atoms, dim))
        displacements = np.transpose(a=displacements, axes=(1, 0, 2))

        return displacements

    def create_kpoints_data_helper(self, atomic_displacements=None, atomic_coordinates=None, row=None, column=None,
                                   freq_num=None, row_width=6):
        """
        Computes normalisation constant for displacements and builds a block of coordinates.
        :param atomic_displacements: list with atomic displacements
        :param atomic_coordinates: list with atomic coordinates
        :param row: number of atomic_displacements row to parse
        :param column: number of atomic_displacements column to parse
        :param freq_num: number of mode (frequency)
        :param row_width: current width of row to parse
        :return normalised atomic displacements
        """
        xdisp = atomic_displacements[0]
        ydisp = atomic_displacements[1]
        zdisp = atomic_displacements[2]
        atom_num = -1

        # Compute normalisation constant for displacements
        # and build block of normalised coordinates.
        normalised_coordinates = []
        norm_const1 = 0.
        for line in atomic_coordinates:
            atom_num += 1
            l = line.split()
            indx = row * len(atomic_coordinates) * 6 + atom_num * row_width + column
            if indx <= len(xdisp) - 1:
                x, y, z = xdisp[indx], ydisp[indx], zdisp[indx]
                norm_const1 += (x * x.conjugate() + y * y.conjugate() + z * z.conjugate()).real
                normalised_coordinates += [[l[2], x, y, z]]

        # Normalise displacements and multiply displacements by sqrt(mass)-> xn, yn, zn
        xn = []
        yn = []
        zn = []
        norm_const1 = sqrt(norm_const1)
        norm = 0.0

        for item in normalised_coordinates:
            atom = Atom(symbol=str(item[0].decode("utf-8").capitalize()))
            mass = atom.mass
            x = item[1] / norm_const1 * sqrt(mass)
            y = item[2] / norm_const1 * sqrt(mass)
            z = item[3] / norm_const1 * sqrt(mass)
            xn += [x]
            yn += [y]
            zn += [z]
            norm += (x * x.conjugate() + y * y.conjugate() + z * z.conjugate()).real

        # Final normalization
        local_displacements = np.transpose(np.asarray([xn, yn, zn])) / sqrt(norm)
        return local_displacements
