# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-public-methods
from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import assertRaisesNothing
from testhelpers.tempfile_wrapper import TemporaryFileHelper

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *


class PoldiCreatePeaksFromFileTest(unittest.TestCase):
    testname = None

    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)

    def test_Init(self):
        assertRaisesNothing(self, AlgorithmManager.create, ("PoldiCreatePeaksFromFile"))

    def test_FileOneCompoundOneAtom(self):
        fileHelper = TemporaryFileHelper("""Silicon {
                                                Lattice: 5.43 5.43 5.43 90.0 90.0 90.0
                                                Spacegroup: F d -3 m
                                                Atoms: {
                                                    Si 0 0 0 1.0 0.05
                                                }
                                            }""")
        ws = PoldiCreatePeaksFromFile(fileHelper.getName(), 0.7, 10.0)

        # Check output GroupWorkspace
        self.assertEqual(ws.getNumberOfEntries(), 1)
        self.assertTrue(ws.contains("Silicon"))

        # Check that the ouput is identical to what's expected
        ws_expected = PoldiCreatePeaksFromCell("F d -3 m", "Si 0 0 0 1.0 0.05", a=5.43, LatticeSpacingMin=0.7)
        si_ws = AnalysisDataService.retrieve("Silicon")
        self._tablesAreEqual(si_ws, ws_expected)

        # Clean up
        self._cleanWorkspaces([ws, ws_expected])

    def test_FileOneCompoundTwoAtoms(self):
        # It's the same structure and the same reflections, just the structure factors are different
        fileHelper = TemporaryFileHelper("""SiliconCarbon {
                                                Lattice: 5.43 5.43 5.43 90.0 90.0 90.0
                                                Spacegroup: F d -3 m
                                                Atoms: {
                                                    Si 0 0 0 0.9 0.05
                                                    C 0 0 0 0.1 0.05
                                                }
                                                # Comment
                                            }""")
        ws = PoldiCreatePeaksFromFile(fileHelper.getName(), 0.7, 10.0)

        self.assertEqual(ws.getNumberOfEntries(), 1)
        self.assertTrue(ws.contains("SiliconCarbon"))

        ws_expected = PoldiCreatePeaksFromCell("F d -3 m", "Si 0 0 0 0.9 0.05; C 0 0 0 0.1 0.05", a=5.43,
                                               LatticeSpacingMin=0.7)
        si_ws = AnalysisDataService.retrieve("SiliconCarbon")
        self._tablesAreEqual(si_ws, ws_expected)

        # Clean up
        self._cleanWorkspaces([ws, ws_expected])

    def test_FileTwoCompounds(self):
        # Using two imaginary structures to check that two compounds are parsed correctly as well
        fileHelper = TemporaryFileHelper("""SiliconCarbon {
                                                Lattice: 5.43 5.43 5.43 90.0 90.0 120.0
                                                Spacegroup: P 63/m m c
                                                Atoms: {
                                                    Si 0 0 0 0.9 0.05
                                                    C 0 0 0 0.1 0.05
                                                }
                                            }
                                            Silicon {
                                                Lattice: 5.43 5.43 5.43 90.0 90.0 90.0
                                                Spacegroup: F d -3 m
                                                Atoms: {
                                                    Si 1/2 1/2 0 1.0 0.05
                                                }
                                            }""")
        ws = PoldiCreatePeaksFromFile(fileHelper.getName(), 0.7, 10.0)

        self.assertEqual(ws.getNumberOfEntries(), 2)
        self.assertTrue(ws.contains("SiliconCarbon"))
        self.assertTrue(ws.contains("Silicon"))

        self._cleanWorkspaces([ws])

    def test_FileFaultyLatticeStrings(self):
        fhLatticeMissing = TemporaryFileHelper("""Silicon {
                                                    Spacegroup: F d -3 m
                                                    Atoms: {
                                                        Si 0 0 0 1.0 0.05
                                                    }
                                                  }""")

        fhNoLattice = TemporaryFileHelper("""Silicon {
                                                Lattice:
                                                Spacegroup: F d -3 m
                                                Atoms: {
                                                    Si 0 0 0 1.0 0.05
                                                }
                                             }""")

        fhInvalidLattice = TemporaryFileHelper("""Silicon {
                                                    Lattice: invalid
                                                    Spacegroup: F d -3 m
                                                    Atoms: {
                                                        Si 0 0 0 1.0 0.05
                                                    }
                                                  }""")

        self.assertRaises(RuntimeError, PoldiCreatePeaksFromFile, *(fhLatticeMissing.getName(), 0.7, 10.0, 'ws'))
        self.assertRaises(RuntimeError, PoldiCreatePeaksFromFile, *(fhNoLattice.getName(), 0.7, 10.0, 'ws'))
        self.assertRaises(RuntimeError, PoldiCreatePeaksFromFile, *(fhInvalidLattice.getName(), 0.7, 10.0, 'ws'))

    def test_FileFaultySpaceGroupStrings(self):
        fhSgMissing = TemporaryFileHelper("""Silicon {
                                                Lattice: 5.43 5.43 5.43 90.0 90.0 90.0
                                                Atoms: {
                                                    Si 0 0 0 1.0 0.05
                                                }
                                             }""")

        fhSgInvalid = TemporaryFileHelper("""Silicon {
                                                    Lattice: 5.43 5.43 5.43 90.0 90.0 90.0
                                                    Spacegroup: invalid
                                                    Atoms: {
                                                        Si 0 0 0 1.0 0.05
                                                    }
                                                  }""")

        self.assertRaises(RuntimeError, PoldiCreatePeaksFromFile, *(fhSgMissing.getName(), 0.7, 10.0, 'ws'))
        self.assertRaises(RuntimeError, PoldiCreatePeaksFromFile, *(fhSgInvalid.getName(), 0.7, 10.0, 'ws'))

    def test_FileFaultyAtomStrings(self):
        fhAtomsMissing = TemporaryFileHelper("""Silicon {
                                                Lattice: 5.43 5.43 5.43 90.0 90.0 90.0
                                                Spacegroup: F d -3 m
                                             }""")

        fhAtomsNoBraces = TemporaryFileHelper("""Silicon {
                                                    Lattice: 5.43 5.43 5.43 90.0 90.0 90.0
                                                    Spacegroup: invalid
                                                    Atoms:
                                                        Sis 0 0 0 1.0 0.05
                                                  }""")
        fhAtomsEmpty = TemporaryFileHelper("""Silicon {
                                                    Lattice: 5.43 5.43 5.43 90.0 90.0 90.0
                                                    Spacegroup: invalid
                                                    Atoms: { }
                                                  }""")

        self.assertRaises(RuntimeError, PoldiCreatePeaksFromFile, *(fhAtomsMissing.getName(), 0.7, 10.0, 'ws'))
        self.assertRaises(RuntimeError, PoldiCreatePeaksFromFile, *(fhAtomsNoBraces.getName(), 0.7, 10.0, 'ws'))
        self.assertRaises(RuntimeError, PoldiCreatePeaksFromFile, *(fhAtomsEmpty.getName(), 0.7, 10.0, 'ws'))

    def _tablesAreEqual(self, lhs, rhs):
        self.assertEqual(lhs.rowCount(), rhs.rowCount(), msg="Row count of tables is different")

        for r in range(lhs.rowCount()):
            self.assertEqual(lhs.row(r), rhs.row(r), "Row " + str(r) + " of tables differ.")

    def _cleanWorkspaces(self, wsList):
        for ws in wsList:
            DeleteWorkspace(ws)


if __name__ == '__main__':
    # Only test if algorithm is registered (pyparsing dependency).
    if AlgorithmFactory.exists("PoldiCreatePeaksFromFile"):
        unittest.main()
