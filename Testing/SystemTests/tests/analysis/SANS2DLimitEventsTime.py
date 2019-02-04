# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
import systemtesting
from mantid.simpleapi import *
from ISISCommandInterface import *


class SANS2DLimitEventsTime(systemtesting.MantidSystemTest):

    def runTest(self):
        SANS2D()
        MaskFile('MaskSANS2DReductionGUI_LimitEventsTime.txt')
        AssignSample('22048')
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '22048rear_1D_1.5_12.5','SANSReductionGUI_LimitEventsTime.nxs'
