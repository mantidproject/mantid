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
        fileHelper = TemporaryFileHelper("""Compound Silicon 1.0 0.01 {
    Lattice: 5.43 5.43 5.43 90.0 90.0 90.0
    Spacegroup: F d -3 m
    Atoms: {
        Si 0 0 0 1.0 0.05
    }
}""")

        ws = PoldiLoadCrystalData(fileHelper.getName(), 0.75, 10.0)

        self.assertEquals(ws.rowCount(), 18)



if __name__ == '__main__':
    unittest.main()