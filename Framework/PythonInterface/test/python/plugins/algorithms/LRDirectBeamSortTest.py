# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import LoadEventNexus, LRDirectBeamSort, DeleteWorkspace
import unittest


class LRDirectBeamSortTest(unittest.TestCase):
    medium = 'air'

    def __init__(self, *args):
        LoadEventNexus(Filename='REF_L_179926.nxs.h5', OutputWorkspace='REF_L_179926')
        LoadEventNexus(Filename='REF_L_179927.nxs.h5', OutputWorkspace='REF_L_179927')
        unittest.TestCase.__init__(self, *args)

    def test_RunsSuccessfully(self):
        LRDirectBeamSort(WorkspaceList=['REF_L_179926', 'REF_L_179927'],
                         ComputeScalingFactors=True,
                         OrderDirectBeamsByRunNumber=False,
                         SlitTolerance=0.06,
                         IncidentMedium=self.medium,
                         UseLowResCut=False,
                         TOFSteps=200)

    def __del__(self):
        DeleteWorkspace('REF_L_179926')
        DeleteWorkspace('REF_L_179927')


if __name__ == '__main__':
    unittest.main()
