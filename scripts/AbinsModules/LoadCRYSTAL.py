from __future__ import (absolute_import, division, print_function)
import numpy as np
from math import sqrt
from mantid.kernel import logger
import AbinsModules


class LoadCRYSTAL(AbinsModules.GeneralDFTProgram):
    """
    Class for loading CRYSTAL DFT phonon data. Main author of this module is Leonardo Bernasconi.
    """
    def __init__(self, input_dft_filename=None):
        super(LoadCRYSTAL, self).__init__(input_dft_filename=input_dft_filename)
        # TODO: currently only Gamma point calculations are supported. To be extended in the future
        self._num_k = 1
        self._dft_program = "CRYSTAL"

    def read_phonon_file(self):
        """
        Reads phonon data from CRYSTAL output files. Saves frequencies, weights of k-point vectors, k-point vectors,
        amplitudes of atomic displacements, hash of the phonon file (hash) to <>.hdf5

        @return  object of type AbinsData.
        """
        # TODO: to make LoadCRYSTAL more robust it would be good if masses can be read directly from CRYSTAL output file
        # Atomic masses
        masses = {'K': 39.0980,
                  'O': 15.9994,
                  'C': 12.0107,
                  'H': 1.00794,
                  'F': 18.9984,
                  'CL': 35.4530,
                  'N': 14.0067,
                  'NA': 22.99
                  }
        # determine system
        molecular = self._determine_system()

        # read data from output CRYSTAL file
        coord_lines = self._read_atomic_coordinates()
        freq, coordinates = self._read_modes()
        coord_lines = self._clear_coord(molecular=molecular, coord_lines=coord_lines)

        # put data into Abins data structure
        data = {}
        self._create_atoms_data(data=data, coord_lines=coord_lines, masses=masses)
        self._create_kpoints_data(data=data, freq=freq, atomic_displacements=coordinates,
                                  atomic_coordinates=coord_lines, masses=masses)

        # save data to hdf file
        self.save_dft_data(data=data)

        # return AbinsData object
        return self._rearrange_data(data=data)

    def _determine_system(self):
        """
        Determines whether the system is a molecule or a crystal.
        """
        found_type = False
        molecular = False
        with open(self._clerk.get_input_filename(),  "r") as crystal_file:
            for line in crystal_file:
                if "MOLECULAR CALCULATION" in line or "0D - MOLECULE" in line:
                    molecular = True
                    found_type = True
                    break
                if "CRYSTAL CALCULATION" in line or "3D - CRYSTAL" in line:
                    molecular = False
                    found_type = True
                    break

        if not found_type:
            raise ValueError("Only molecular or 3D CRYSTAL systems can be processed")

        if molecular:
            logger.notice("This run is for a MOLECULAR system")
        else:
            logger.notice("This run is for a 3D CRYSTAL system")

        return molecular

    def _read_atomic_coordinates(self):
        """
        Reads atomic coordinates from .out file.
        :return: list with atomic coordinates
        """
        filename = self._clerk.get_input_filename()
        with open(filename,  "r") as crystal_file:
            logger.notice("Reading from " + filename)
            coord_lines = []
            found_coord = False
            finished = False
            for line in crystal_file:
                if "ATOM          X(ANGSTROM)         Y(ANGSTROM)" in line:
                    found_coord = True
                if found_coord:
                    if "LOCAL ATOMIC FUNCTIONS BASIS SET" in line or \
                                    "ROTATION" in line or "T = ATOM BELONGING " in line or \
                                    "PSEUDOPOTENTIAL INFORMATION" in line or\
                                    "STARTING BASIS SET HANDLING" in line:

                        found_coord = False
                        if not found_coord:
                            finished = True
                if finished:
                    break
                if found_coord and "ATOM" not in line and \
                   "*" not in line and not len(line) == 1 and \
                   not len(line.split()) == 0:

                    coord_lines += [line.strip("\n")]

        for line in coord_lines:
            logger.debug(line.strip("\n"))

        return coord_lines

    def _read_modes(self):
        """
        Reads vibrational modes (frequencies and displacements)
        """
        with open(self._clerk.get_input_filename(), "r") as crystal_file:
            freq = []
            xdisp = []
            ydisp = []
            zdisp = []
            found_freq = False
            for line in crystal_file:
                if "NORMAL MODES NORMALIZED TO CLASSICAL AMPLITUDES" in line:
                    found_freq = True
                if "ESTIMATED" in line:
                    found_freq = False
                if "FREQ(CM**-1)" in line:
                    for item in line.strip("\n").strip("FREQ(CM**-1)").split()[1:]:
                        freq += [float(item)]
                if found_freq and "FREQ(CM**-1)" not in line and "ESTIMATED" not in line \
                        and "NORMALIZED" not in line and not len(line) == 1:
                    if "X" in line:
                        xdisp += line[14:].strip("\n").split()
                    if "Y" in line:
                        ydisp += line[14:].strip("\n").split()
                    if "Z" in line:
                        zdisp += line[14:].strip("\n").split()

        return freq, [xdisp, ydisp, zdisp]

    # noinspection PyMethodMayBeStatic
    def _clear_coord(self, molecular=None, coord_lines=None):
        """
        Clean molecular coordinates, removing 'T'
        """
        if molecular:
            coord_lines2 = []
            for item in coord_lines:
                coord_lines2 += [item[0:4] + item[6:]]
            coord_lines = coord_lines2

        return coord_lines

    # noinspection PyMethodMayBeStatic
    def _create_atoms_data(self, data=None, coord_lines=None, masses=None):
        """
        Create Python dictionary which can easily be converted to AbinsData object
        :return:
        """
        data.update({"atoms": dict()})
        for i, line in enumerate(coord_lines):
            l = line.split()
            if l[2] in masses:
                mass = masses[l[2]]
            else:
                raise ValueError("Mass not available for element " + l[2])
            data["atoms"]["atom_%s" % i] = {
                "symbol": l[2].capitalize(), "mass": mass, "sort": i,
                "fract_coord": np.asarray(l[3:6]).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)}

    def _create_kpoints_data(self, data=None, freq=None, atomic_displacements=None, atomic_coordinates=None,
                             masses=None):
        """
         Normalises atomic displacement.  Saves result to data Python dictionary.
        :param data: Python dictionary for in k points  data will be stored
        :param freq: normal modes
        :param atomic_displacements: atomic displacements
        :param atomic_coordinates: atomic coordinates
        """
        #     a) Put frequencies into dictionary
        data["frequencies"] = np.asarray([freq]).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE, casting="safe")

        #     b) Normalise atomic displacements and put them into data dictionary
        i = -1
        it = -1
        nb = 0
        displacements = []
        xdisp = atomic_displacements[0]
        ydisp = atomic_displacements[1]
        zdisp = atomic_displacements[2]

        for _ in freq:
            i += 1
            it += 1
            if i == 6:
                i = 0
                nb += 1
            if nb <= len(xdisp) / (6 * len(atomic_coordinates)) - 1:
                iat = -1
                # Compute normalisation constant for displacements
                # and build block of coordinates to print out
                block_to_print = []
                norm_const1 = 0.
                for line in atomic_coordinates:
                    iat += 1
                    l = line.split()
                    if nb * len(atomic_coordinates) * 6 + iat * 6 + i <= len(xdisp) - 1:
                        x = float(xdisp[nb * len(atomic_coordinates) * 6 + iat * 6 + i])
                        y = float(ydisp[nb * len(atomic_coordinates) * 6 + iat * 6 + i])
                        z = float(zdisp[nb * len(atomic_coordinates) * 6 + iat * 6 + i])
                        norm_const1 += x * x + y * y + z * z
                        block_to_print += [[iat + 1, l[2], int(l[1]), x, y, z]]
                # Normalise displacements -> xn, yn, zn
                xn = []
                yn = []
                zn = []
                norm_const1 = sqrt(norm_const1)
                norm = 0.
                for item in block_to_print:
                    x = item[3] / norm_const1
                    y = item[4] / norm_const1
                    z = item[5] / norm_const1
                    norm += x * x + y * y + z * z
                    xn += [x]
                    yn += [y]
                    zn += [z]
                # Multiply displacements by sqrt(mass) -> x2m, y2m, z2m
                x2m = []
                y2m = []
                z2m = []
                norm = 0.0
                ii = -1
                for item in block_to_print:
                    ii += 1
                    mass = masses[item[1]]
                    x = xn[ii] * sqrt(mass)
                    y = yn[ii] * sqrt(mass)
                    z = zn[ii] * sqrt(mass)
                    x2m += [x]
                    y2m += [y]
                    z2m += [z]
                    norm += x * x + y * y + z * z
                    # Normalise and print displacements
                normf = 0.0
                ii = -1
                # noinspection PyAssignmentToLoopOrWithParameter
                for _ in block_to_print:
                    ii += 1
                    x = x2m[ii] / sqrt(norm)
                    y = y2m[ii] / sqrt(norm)
                    z = z2m[ii] / sqrt(norm)
                    normf += x * x + y * y + z * z
                    displacements.append([complex(x), complex(y), complex(z)])
                logger.debug("Mode {0} normalised to {1}".format(str(it + 1), str(normf)))
            # At this point we have printed out all the modes that are
            # part of blocks of 6 in the crystal output; now we need to
            # consider the other blocks
            elif len(xdisp) % 6 != 0:
                lb = len(xdisp) % 6
                iat = -1
                # Compute normalisation constant for displacements
                # and build block of coordinates to print out
                block_to_print = []
                norm_const1 = 0.
                for line in atomic_coordinates:
                    iat += 1
                    l = line.split()
                    x = float(xdisp[nb * len(atomic_coordinates) * 6 + iat * lb + i])
                    y = float(ydisp[nb * len(atomic_coordinates) * 6 + iat * lb + i])
                    z = float(zdisp[nb * len(atomic_coordinates) * 6 + iat * lb + i])
                    logger.debug("{0:s} {1:s} {2:s}".format(x, y, z))
                    norm_const1 += x * x + y * y + z * z
                    block_to_print += [[iat + 1, l[2], int(l[1]), x, y, z]]
                # Normalise displacements -> xn, yn, zn
                xn = []
                yn = []
                zn = []
                norm_const1 = sqrt(norm_const1)
                norm = 0.
                for item in block_to_print:
                    x = item[3] / norm_const1
                    y = item[4] / norm_const1
                    z = item[5] / norm_const1
                    norm += x * x + y * y + z * z
                    xn += [x]
                    yn += [y]
                    zn += [z]
                # Multiply displacements by sqrt(mass) -> x2m, y2m, z2m
                x2m = []
                y2m = []
                z2m = []
                norm = 0.0
                ii = -1
                for item in block_to_print:
                    ii += 1
                    mass = masses[item[1]]
                    x = xn[ii] * sqrt(mass)
                    y = yn[ii] * sqrt(mass)
                    z = zn[ii] * sqrt(mass)
                    x2m += [x]
                    y2m += [y]
                    z2m += [z]
                    norm += x * x + y * y + z * z
                # Normalise and print displacements
                normf = 0.0
                ii = -1
                # noinspection PyAssignmentToLoopOrWithParameter
                for _ in block_to_print:
                    ii += 1
                    x = x2m[ii] / sqrt(norm)
                    y = y2m[ii] / sqrt(norm)
                    z = z2m[ii] / sqrt(norm)
                    normf += x * x + y * y + z * z
                    displacements.append([complex(x), complex(y), complex(z)])
                logger.debug("Mode " + str(it + 1) + " normalised to " + str(normf))

        # Reshape displacements so that Abins can use it to create its internal data objects
        # num_k  = 1: currently only data for Gamma point is used in the working powder equations
        # num_atoms: number of atoms in the system
        # num_freq: number of modes
        # dim: dimension for each atomic displacement (atoms vibrate in 3D space)
        #
        # The following conversion is necessary:
        # (num_freq, num_atom, dim) -> (num_k, num_atom, num_freq, dim)

        num_freq = len(freq)
        self._num_atoms = len(atomic_coordinates)
        dim = 3

        displacements = np.asarray(a=[displacements], order="C")
        displacements = np.reshape(a=displacements, newshape=(self._num_k, num_freq, self._num_atoms, dim))
        data["atomic_displacements"] = np.transpose(a=displacements, axes=(0, 2, 1, 3))

        # In order  to provide compatibility with Abins internal data structure same additional entries have to
        # be defined.
        data["weights"] = np.asarray([1.0]).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE, casting="safe")
        data["k_vectors"] = np.asarray([[0.0, 0.0, 0.0]]).astype(dtype=AbinsModules.AbinsConstants.FLOAT_TYPE,
                                                                 casting="safe")
        data["unit_cell"] = np.zeros(shape=(dim, dim), dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)
        # TODO: it would make LoadCRYSTAL more robust if these parameters can be read from CRYSTAL output
