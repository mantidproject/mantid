# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
import stresstesting
from BilbyReductionScript import run_bilby_reduction


class BilbySANSDataProcessorTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        run_bilby_reduction('mantid_reduction_settings_example.csv', '0', '0', 'shift_assembled.csv', False)

    def validate(self):
        self.disableChecking.append('Instrument')
        return 'BBY0019749_1D_2.0_18.0_AgBeh', 'BilbyReductionExampleOutput.nxs'
