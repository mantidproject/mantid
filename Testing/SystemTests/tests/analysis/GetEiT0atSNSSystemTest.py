# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init
"""
GetEiT0atSNS
"""
from systemtesting import MantidSystemTest
import mantid.simpleapi as sm


class GetEiT0atSNSSystemTest(MantidSystemTest):

    def runTest(self):
        # old DAS. all data in first frame
        ws = sm.LoadNexusMonitors('SEQ_169004.nxs.h5')
        Ei, T0 = sm.GetEiT0atSNS(ws)
        assert abs(Ei-8.31)< 0.01
        # new DAS. unwrapped
        ws = sm.LoadNexusMonitors('SEQ_176472.nxs.h5')
        Ei, T0 = sm.GetEiT0atSNS(ws)
        print Ei, T0
        # assert abs(Ei-8.31)< 0.01
        return
