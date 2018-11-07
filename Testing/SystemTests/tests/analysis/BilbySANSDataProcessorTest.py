# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

from __future__ import (absolute_import, division, print_function)
import stresstesting
import BilbyCustomFunctions_Reduction
from BilbyReductionScript import run_bilby_reduction
from mantid.simpleapi import *


class BilbySANSDataProcessorTest(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        run_bilby_reduction('mantid_reduction_settings_example.csv', '0', '0', 'shift_assembled.csv', False)

    def validate(self):
        self.disableChecking.append('Instrument')
        return 'BBY0019749_1D_2.0_18.0_AgBeh', 'BilbyReductionExampleOutput.nxs'
