# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import SANSILLParameterScan, config, mtd


class SANSILLIntegrationTest(unittest.TestCase):

    _facility = None

    def setUp(self):
        self._facility = config['default.facility']
        self._data_search_dirs = config.getDataSearchDirs()
        config.appendDataSearchSubDir('ILL/D16/')
        config.setFacility("ILL")

    def tearDown(self):
        config.setFacility(self._facility)
        config.setDataSearchDirs(self._data_search_dirs)
        mtd.clear()

    def test_D16_omega(self):
        SANSILLParameterScan(SampleRuns="023583:023585",
                             OutputWorkspace="output2d",
                             OutputJoinedWorkspace="reduced",
                             Observable="Omega.value",
                             PixelYmin=3,
                             PixelYMax=317)
        self._check_output(mtd["output2d"], 3)
        self.assertTrue(mtd["reduced"])

    def _check_output(self, ws, spectra=1):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, MatrixWorkspace))
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), "Degrees")
        self.assertEqual(ws.getNumberHistograms(), spectra)
        self.assertTrue(ws.getInstrument())
        self.assertTrue(ws.getRun())
        self.assertTrue(ws.getHistory())


if __name__ == '__main__':
    unittest.main()
