# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
import systemtesting
from BilbyReductionScript import RunBilbyReduction


class BilbySANSDataProcessorTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        run_bilby_reduction = RunBilbyReduction('mantid_reduction_settings_example.csv', '0', '0',
                                                'shift_assembled.csv', False)
        run_bilby_reduction.run_bilby_reduction()

    def validate(self):
        self.disableChecking.append('Instrument')
        return 'BBY0019749_1D_2.0_18.0_AgBeh', 'BilbyReductionExampleOutput.nxs'
