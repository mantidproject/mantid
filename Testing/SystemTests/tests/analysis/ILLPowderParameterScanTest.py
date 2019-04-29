# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import systemtesting
from mantid.simpleapi import PowderILLParameterScan
from mantid import config, mtd


class ILLPowderParameterScanTest(systemtesting.MantidSystemTest):

    def __init__(self):
        super(ILLPowderParameterScanTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D20'
        config.appendDataSearchSubDir('ILL/D20/')

    def requiredFiles(self):
        return ['967087.nxs', '967088.nxs']

    def tearDown(self):
        mtd.clear()

    def runTest(self):
        PowderILLParameterScan(Run='967087,967088',OutputWorkspace='reduced')

    def validate(self):
        self.tolerance = 0.0001
        # something goes wrong with DetInfo when saving loading nexus processed
        self.disableChecking.append("Instrument")
        return ['reduced', 'ILL_D20_red_def.nxs']
