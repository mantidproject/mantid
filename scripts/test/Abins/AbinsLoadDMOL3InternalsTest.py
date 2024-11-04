# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from io import BytesIO
import unittest

import numpy as np
from numpy.testing import assert_allclose

from abins.constants import ATOMIC_LENGTH_2_ANGSTROM as BOHR_TO_ANGSTROM
from abins.input import DMOL3Loader
from mantid.kernel import Atom


class AbinsLoadDMOL3InternalsTest(unittest.TestCase):
    """Check fragments of DMOL3 parser separately from whole file"""

    def test_read_lattice_vectors(self):
        file_without_vectors = BytesIO(b"No vectors in this file")
        unit_cell = DMOL3Loader._read_lattice_vectors(file_without_vectors)

        assert_allclose(unit_cell, np.zeros((3, 3)))

        file_with_vectors = BytesIO(
            b"""
INCOOR, atomic coordinates in au (for archive):
______________________________________________________________________>8

$cell vectors
             16.74108374131821    0.00000000000000    0.00000000000000
             -8.37054187065910   14.49820380686420    0.00000000000000
              0.00000000000000    0.00000000000000    9.52044021771771
$coordinates
F             2.22656413759532   -1.33383475061772   -1.80983568538814
F             2.42243481736875    8.68442407988897    6.67097246055480
F            -2.34542583215868   10.72867081788549    2.94943237944894
"""
        )
        unit_cell = DMOL3Loader._read_lattice_vectors(file_with_vectors)

        assert_allclose(unit_cell, np.array([[8.859, 0.0, 0.0], [-4.4295, 7.67211905, 0.0], [0.0, 0.0, 5.038]]))

    def test_read_positions(self):
        file_with_coords_end = BytesIO(
            b"""
             -8.37054187065910   14.49820380686420    0.00000000000000
              0.00000000000000    0.00000000000000    9.52044021771771
$coordinates
F             2.22656413759532   -1.33383475061772   -1.80983568538814
F             2.42243481736875    8.68442407988897    6.67097246055480
Na           -2.34542583215868   10.72867081788549    2.94943237944894
Si            0.04185270935330    2.59517848080392   -1.80983568538814
$end
DUMMY CONTENT
"""
        )
        atoms = DMOL3Loader._read_atomic_coordinates(file_with_coords_end)

        assert_allclose(
            [atom["coord"] for _, atom in atoms.items()],
            np.array(
                [
                    [2.22656413759532, -1.33383475061772, -1.80983568538814],
                    [2.42243481736875, 8.68442407988897, 6.67097246055480],
                    [-2.34542583215868, 10.72867081788549, 2.94943237944894],
                    [0.04185270935330, 2.59517848080392, -1.80983568538814],
                ]
            )
            * BOHR_TO_ANGSTROM,
        )

        self.assertEqual(["F", "F", "Na", "Si"], [atom["symbol"] for _, atom in atoms.items()])

        # At this point the atomic mass should still be the Mantid value
        self.assertEqual(atoms["atom_0"]["mass"], Atom(symbol="F").mass)

        self.assertEqual(list(range(len(atoms))), [atom["sort"] for _, atom in atoms.items()])

    def test_read_masses(self):
        # usual case: break on Molecular Mass:
        file_with_masses = BytesIO(
            b"""
    z     -0.5431      -0.4170      -0.0002      -0.0001      -0.3565      -0.0000       0.0000       0.0000       0.0000

 STANDARD THERMODYNAMIC QUANTITIES AT   298.15 K  AND     1.00 ATM

   Zero point vibrational energy:       38.528 kcal/mol

   Atom    1 Element F  Has Mass   18.50000
   Atom    2 Element F  Has Mass   18.50000
   Atom    3 Element F  Has Mass   21.00000
   Atom    4 Element F  Has Mass   22.00000
   Molecular Mass:   564.166320 amu
   Principal axes and moments of inertia in atomic units:
                               1              2              3
    Eigenvalues --        8975.59891    26615.40273    29256.87074
          X                  0.82797        0.50000        0.25389
"""
        )

        # If not found, parser should still break cleanly at file end
        file_with_masses_noend = BytesIO(
            b"""
    z     -0.5431      -0.4170      -0.0002      -0.0001      -0.3565      -0.0000       0.0000       0.0000       0.0000

 STANDARD THERMODYNAMIC QUANTITIES AT   298.15 K  AND     1.00 ATM

   Zero point vibrational energy:       38.528 kcal/mol

   Atom    1 Element F  Has Mass   18.50000
   Atom    2 Element F  Has Mass   18.50000
   Atom    3 Element F  Has Mass   21.00000
   Atom    4 Element F  Has Mass   22.00000
   Principal axes and moments of inertia in atomic units:
                               1              2              3
    Eigenvalues --        8975.59891    26615.40273    29256.87074
          X                  0.82797        0.50000        0.25389
"""
        )

        for file_stream in (file_with_masses, file_with_masses_noend):
            masses = DMOL3Loader._read_masses_from_file(file_stream)
            assert_allclose(masses, [18.5, 18.5, 21.0, 22.0])

    def test_reshape_displacements(self):
        # Say we have 4 modes and two atoms (so 6 degrees of freedom).
        raw_disp = np.array(
            [
                [0, 1, 2, 3],
                [4, 5, 6, 7],
                [8, 9, 10, 11],
                [12, 13, 14, 15],
                [16, 17, 18, 19],
                [20, 21, 22, 23],
            ]  # 1x  # 1y  # 1z  # 2x  # 2y
        )  # 2z

        expected = np.array(
            [  # kpts
                [  # atoms
                    [[0, 4, 8], [1, 5, 9], [2, 6, 10], [3, 7, 11]],  # modes  # directions
                    [[12, 16, 20], [13, 17, 21], [14, 18, 22], [15, 19, 23]],
                ]
            ]
        )

        assert_allclose(DMOL3Loader._reshape_displacements(raw_disp), expected)

    def test_read_modes(self):
        """Check internal reader of eigenvalue blocks

        Check the following cases:
        - multiple blocks, using all 9 columns
        - multiple blocks, with final block using fewer columns
        - only one block of data, fewer than 9 columns
        """

        full_frequencies = [
            1045.7,
            1047.6,
            1049.7,
            1051.0,
            1051.8,
            1052.7,
            1053.4,
            1053.7,
            1054.7,
            1057.4,
            1062.6,
            1065.5,
            1066.9,
            1067.0,
            1067.7,
            1070.0,
            1084.5,
            1097.8,
        ]

        full_disp_block = np.array(
            [
                [
                    0.0072,
                    0.0268,
                    -0.0375,
                    0.0055,
                    0.0316,
                    0.1012,
                    0.0200,
                    0.0205,
                    0.0119,
                    -0.0709,
                    -0.0254,
                    0.0002,
                    0.0899,
                    0.1173,
                    0.0038,
                    -0.0008,
                    0.0118,
                    -0.0022,
                ],
                [
                    0.0062,
                    -0.0568,
                    0.0104,
                    -0.0921,
                    -0.0009,
                    -0.0170,
                    -0.0244,
                    -0.0052,
                    0.0035,
                    -0.0161,
                    0.0189,
                    0.0000,
                    -0.0027,
                    -0.0381,
                    0.0033,
                    0.0004,
                    -0.0012,
                    -0.0035,
                ],
                [
                    -0.0044,
                    -0.0101,
                    0.0183,
                    0.0825,
                    -0.0096,
                    -0.0162,
                    0.0128,
                    0.0013,
                    -0.0016,
                    0.0255,
                    -0.0044,
                    0.0007,
                    -0.0340,
                    0.0036,
                    -0.0070,
                    -0.0002,
                    -0.0057,
                    0.0044,
                ],
                [
                    0.0783,
                    -0.0072,
                    -0.0411,
                    0.0207,
                    -0.0214,
                    0.0246,
                    0.0174,
                    0.0184,
                    -0.1040,
                    -0.0171,
                    0.0115,
                    -0.1400,
                    -0.0007,
                    -0.0003,
                    -0.0100,
                    0.0000,
                    -0.0029,
                    -0.0023,
                ],
                [
                    0.0996,
                    -0.0124,
                    -0.0267,
                    0.0060,
                    0.0362,
                    -0.0169,
                    -0.0070,
                    0.0056,
                    0.0110,
                    0.0073,
                    0.0167,
                    -0.0241,
                    -0.0019,
                    0.0013,
                    0.0012,
                    -0.0002,
                    -0.0176,
                    -0.0016,
                ],
                [
                    -0.0102,
                    -0.0177,
                    -0.0405,
                    0.0090,
                    0.0633,
                    -0.0362,
                    0.0065,
                    0.0122,
                    -0.0178,
                    -0.0073,
                    0.0014,
                    -0.0015,
                    -0.0039,
                    0.0013,
                    0.0151,
                    0.0001,
                    0.0012,
                    -0.0014,
                ],
            ]
        )

        file_with_18_modes = BytesIO(
            b"""
      370: 1045.7  371: 1047.6  372: 1049.7  373: 1051.0  374: 1051.8  375: 1052.7  376: 1053.4  377: 1053.7  378: 1054.7

 Si x      0.0072       0.0268      -0.0375       0.0055       0.0316       0.1012       0.0200       0.0205       0.0119
    y      0.0062      -0.0568       0.0104      -0.0921      -0.0009      -0.0170      -0.0244      -0.0052       0.0035
    z     -0.0044      -0.0101       0.0183       0.0825      -0.0096      -0.0162       0.0128       0.0013      -0.0016
 Si x      0.0783      -0.0072      -0.0411       0.0207      -0.0214       0.0246       0.0174       0.0184      -0.1040
    y      0.0996      -0.0124      -0.0267       0.0060       0.0362      -0.0169      -0.0070       0.0056       0.0110
    z     -0.0102      -0.0177      -0.0405       0.0090       0.0633      -0.0362       0.0065       0.0122      -0.0178


      379: 1057.4  380: 1062.6  381: 1065.5  382: 1066.9  383: 1067.0  384: 1067.7  385: 1070.0  386: 1084.5  387: 1097.8

 Si x     -0.0709      -0.0254       0.0002       0.0899       0.1173       0.0038      -0.0008       0.0118      -0.0022
    y     -0.0161       0.0189       0.0000      -0.0027      -0.0381       0.0033       0.0004      -0.0012      -0.0035
    z      0.0255      -0.0044       0.0007      -0.0340       0.0036      -0.0070      -0.0002      -0.0057       0.0044
 Si x     -0.0171       0.0115      -0.1400      -0.0007      -0.0003      -0.0100       0.0000      -0.0029      -0.0023
    y      0.0073       0.0167      -0.0241      -0.0019       0.0013       0.0012      -0.0002      -0.0176      -0.0016
    z     -0.0073       0.0014      -0.0015      -0.0039       0.0013       0.0151       0.0001       0.0012      -0.0014



 STANDARD THERMODYNAMIC QUANTITIES AT   298.15 K  AND     1.00 ATM

"""
        )

        file_with_14_modes = BytesIO(
            b"""
      370: 1045.7  371: 1047.6  372: 1049.7  373: 1051.0  374: 1051.8  375: 1052.7  376: 1053.4  377: 1053.7  378: 1054.7

 Si x      0.0072       0.0268      -0.0375       0.0055       0.0316       0.1012       0.0200       0.0205       0.0119
    y      0.0062      -0.0568       0.0104      -0.0921      -0.0009      -0.0170      -0.0244      -0.0052       0.0035
    z     -0.0044      -0.0101       0.0183       0.0825      -0.0096      -0.0162       0.0128       0.0013      -0.0016
 Si x      0.0783      -0.0072      -0.0411       0.0207      -0.0214       0.0246       0.0174       0.0184      -0.1040
    y      0.0996      -0.0124      -0.0267       0.0060       0.0362      -0.0169      -0.0070       0.0056       0.0110
    z     -0.0102      -0.0177      -0.0405       0.0090       0.0633      -0.0362       0.0065       0.0122      -0.0178


      379: 1057.4  380: 1062.6  381: 1065.5  382: 1066.9  383: 1067.0

 Si x     -0.0709      -0.0254       0.0002       0.0899       0.1173
    y     -0.0161       0.0189       0.0000      -0.0027      -0.0381
    z      0.0255      -0.0044       0.0007      -0.0340       0.0036
 Si x     -0.0171       0.0115      -0.1400      -0.0007      -0.0003
    y      0.0073       0.0167      -0.0241      -0.0019       0.0013
    z     -0.0073       0.0014      -0.0015      -0.0039       0.0013



 STANDARD THERMODYNAMIC QUANTITIES AT   298.15 K  AND     1.00 ATM

"""
        )

        file_with_7_modes = BytesIO(
            b"""
      370: 1045.7  371: 1047.6  372: 1049.7  373: 1051.0  374: 1051.8  375: 1052.7  376: 1053.4

 Si x      0.0072       0.0268      -0.0375       0.0055       0.0316       0.1012       0.0200
    y      0.0062      -0.0568       0.0104      -0.0921      -0.0009      -0.0170      -0.0244
    z     -0.0044      -0.0101       0.0183       0.0825      -0.0096      -0.0162       0.0128
 Si x      0.0783      -0.0072      -0.0411       0.0207      -0.0214       0.0246       0.0174
    y      0.0996      -0.0124      -0.0267       0.0060       0.0362      -0.0169      -0.0070
    z     -0.0102      -0.0177      -0.0405       0.0090       0.0633      -0.0362       0.0065



 STANDARD THERMODYNAMIC QUANTITIES AT   298.15 K  AND     1.00 ATM

"""
        )

        frequencies, modes = DMOL3Loader._read_modes(file_with_18_modes)
        assert_allclose(frequencies, full_frequencies)
        assert_allclose(modes, full_disp_block)

        frequencies, modes = DMOL3Loader._read_modes(file_with_14_modes)
        assert_allclose(frequencies, full_frequencies[:14])
        assert_allclose(modes, full_disp_block[:, :14])

        frequencies, modes = DMOL3Loader._read_modes(file_with_7_modes)
        assert_allclose(frequencies, full_frequencies[:7])
        assert_allclose(modes, full_disp_block[:, :7])
