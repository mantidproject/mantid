# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,no-init,too-many-public-methods,too-many-arguments
from __future__ import (absolute_import, division, print_function)
import stresstesting

import mantid.simpleapi as ms


class LoadExedTest(stresstesting.MantidStressTest):
    def runTest(self):
        rawfile = "V15_0000016544_S000_P01.raw"
        print("Rawfilename:"+rawfile)
        ms.LoadEXED(Filename=rawfile, OutputWorkspace='test')
        # check that it did create a workspace.
        self.assertTrue(ms.mtd.doesExist('test'))
        #check that it has the correct number of histograms
        self.assertEquals(ms.mtd['test'].getNumberHistograms(),20400)
        #check that phi sample Log is correct
        self.assertEquals(ms.mtd['test'].getRun().getLogData('phi').value,-6.000005)
