import unittest
from testhelpers import assertRaisesNothing
from testhelpers.tempfile_wrapper import TemporaryFileHelper

from tempfile import NamedTemporaryFile

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import os


class PoldiLoadCrystalDataTest(unittest.TestCase):
    testname = None

    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)

    def test_Init(self):
        assertRaisesNothing(self, AlgorithmManager.create, ("PoldiLoadCrystalData"))

    def test_FileOneCompoundOneAtom(self):
        fileHelper = TemporaryFileHelper("""Silicon {
    Lattice: 5.43 5.43 5.43 90.0 90.0 90.0
    Spacegroup: F d -3 m
    Atoms: {
        Si 0 0 0 1.0 0.05
    }
}""")
        ws = PoldiLoadCrystalData(fileHelper.getName(), 0.7, 10.0)

        # Check output GroupWorkspace
        self.assertEquals(ws.getNumberOfEntries(), 1)
        self.assertTrue(ws.contains("Silicon"))

        # Check that the ouput is identical to what's expected
        ws_expected = PoldiCreatePeaksFromCell("F d -3 m", "Si 0 0 0 1.0 0.05", a=5.43, LatticeSpacingMin=0.7)
        si_ws = AnalysisDataService.retrieve("Silicon")
        self._tablesAreEqual(si_ws, ws_expected)

        # Clean up
        self._cleanWorkspaces([ws, ws_expected])

    def _tablesAreEqual(self, lhs, rhs):
        self.assertEquals(lhs.rowCount(), rhs.rowCount(), msg="Row count of tables is different")

        for r in range(lhs.rowCount()):
            self.assertEquals(lhs.row(r), rhs.row(r), "Row " + str(r) + " of tables differ.")

    def _cleanWorkspaces(self, wsList):
        for ws in wsList:
            DeleteWorkspace(ws)


if __name__ == '__main__':
    unittest.main()