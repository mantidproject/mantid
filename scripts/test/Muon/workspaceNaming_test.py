# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.ADSHandler import workspace_naming as ws_naming


class WorkspaceNamingTest(unittest.TestCase):
    def setUp(self):
        return

    def test_getGroupOrPairName(self):
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; Group; bkwd; Asymmetry; #1'),"bkwd")
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; Group; fwd; Asymmetry; #2'),"fwd")
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; Pair Asym; long; #1'),"long") 

    def test_getGroupOrPairNameReturnsEmpty(self):
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; PhaseQuad; PhaseTable MUSR62260'),"")

if __name__ == '__main__':
    unittest.main()
