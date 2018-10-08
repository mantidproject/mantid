# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,unused-variable
import stresstesting
from mantid.simpleapi import *


class RawVNexus(stresstesting.MantidStressTest):
    ''' Simply tests that our LoadRaw and LoadISISNexus algorithms produce the same workspace'''

    def runTest(self):
        LoadRaw(Filename='SANS2D00000808.raw', OutputWorkspace='Raw')

    def validate(self):
        return 'Raw','SANS2D00000808.nxs'
