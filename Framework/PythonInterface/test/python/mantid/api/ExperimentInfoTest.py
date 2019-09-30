# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
###############################################################################
# This has to be tested through a workspace as it cannot be created in
# Python
###############################################################################
from testhelpers import run_algorithm, WorkspaceCreationHelper
from mantid.geometry import Instrument
from mantid.api import Sample, Run
from math import pi

class ExperimentInfoTest(unittest.TestCase):

    _expt_ws = None

    def setUp(self):
        if self.__class__._expt_ws is None:
            alg = run_algorithm('CreateWorkspace', DataX=[1,2,3,4,5], DataY=[1,2,3,4,5],NSpec=1, child=True)
            ws = alg.getProperty("OutputWorkspace").value
            ws.run().addProperty("run_number", 48127, True)
            self.__class__._expt_ws = ws

    def test_information_access(self):
        inst = self._expt_ws.getInstrument()
        self.assertTrue(isinstance(inst, Instrument))
        self.assertEqual(self._expt_ws.getRunNumber(), 48127)

    def test_sample_access_returns_sample_object(self):
        sample = self._expt_ws.sample()
        self.assertTrue(isinstance(sample, Sample))

    def test_run_access_returns_run_object(self):
        run = self._expt_ws.run()
        self.assertTrue(isinstance(run, Run))

    def test_get_energy_mode(self):
        emode = self._expt_ws.getEMode()
        self.assertEqual(emode, 0)

    def test_detectorInfo(self):
        detInfo = self._expt_ws.detectorInfo()
        # No instrument in test workspace, so size is 0.
        self.assertEqual(detInfo.size(), 0)

#    def test_set_and_get_efixed(self):
#      ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(1, 5, False, False)
#        ws.setEFixed(1, pi)
#      self.assertEqual(ws.getEFixed(1), pi)

if __name__ == '__main__':
    unittest.main()
