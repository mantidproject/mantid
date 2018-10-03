# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from tempfile import gettempdir
from os import path, remove
import stresstesting
from mantid.simpleapi import PowderDiffILLDetEffCorr, SaveNexusProcessed
from mantid import config, mtd


class ILLPowderDiffDetEffCorrClosureTest(stresstesting.MantidStressTest):

    _m_tmp_file = None

    def __init__(self):
        super(ILLPowderDiffDetEffCorrClosureTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D20'
        config.appendDataSearchSubDir('ILL/D20/')
        self._m_tmp_file = path.join(gettempdir(), 'D20Calib1stIteration.nxs')

    def requiredFiles(self):
        return ['967076.nxs']

    def tearDown(self):
        mtd.clear()
        remove(self._m_tmp_file)

    def runTest(self):

        PowderDiffILLDetEffCorr(CalibrationRun='967076.nxs', OutputWorkspace='calib')

        SaveNexusProcessed(InputWorkspace='calib', Filename=self._m_tmp_file)

        PowderDiffILLDetEffCorr(CalibrationRun='967076.nxs', CalibrationFile=self._m_tmp_file, OutputWorkspace='calib-2nd')

        for i in range(mtd['calib-2nd'].getNumberHistograms()):
            self.assertDelta(mtd['calib-2nd'].readY(i), 1., 1E-3)
