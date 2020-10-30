# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.ADSHandler import workspace_naming as ws_naming


class WorkspaceNamingTest(unittest.TestCase):
    def setUp(self):
        return

    def test_getGroupOrPairName(self):
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; Group; bkwd; Asymmetry; Periods; #1; MA'),
                         "bkwd")
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; Group; fwd; Asymmetry; Periods; #2; MA'),
                         "fwd")
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; Pair Asym; long; Periods; #1; MA'), "long")

    def test_getGroupOrPairNameReturnsEmpty(self):
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; PhaseQuad; PhaseTable MUSR62260'), "")

    def test_removeRebinFromName(self):
        # std name
        name = "MUSR62260; Group; bkwd; Asymmetry; MA"
        self.assertEqual(name, ws_naming.remove_rebin_from_name(name))
        # with periods
        period_name = 'MUSR62260; Group; bkwd; Asymmetry; Periods; #1; MA'
        self.assertEqual(period_name, ws_naming.remove_rebin_from_name(period_name))

        # std name and rebin
        name_rebin = 'MUSR62260; Group; bkwd; Asymmetry; Rebin; MA'
        self.assertEqual(name, ws_naming.remove_rebin_from_name(name_rebin))

        # with periods and rebin
        period_name_rebin = 'MUSR62260; Group; bkwd; Asymmetry; Periods; #1; Rebin; MA'
        self.assertEqual(period_name, ws_naming.remove_rebin_from_name(period_name_rebin))

    def test_add_rebin_to_name(self):
        # std name
        name = "MUSR62260; Group; bkwd; Asymmetry; MA"
        name_rebin = 'MUSR62260; Group; bkwd; Asymmetry; Rebin; MA'
        self.assertEqual(name_rebin, ws_naming.add_rebin_to_name(name))
        self.assertEqual(name_rebin, ws_naming.add_rebin_to_name(name_rebin))

        # with periods
        period_name = 'MUSR62260; Group; bkwd; Asymmetry; Periods; #1; MA'
        period_name_rebin = 'MUSR62260; Group; bkwd; Asymmetry; Periods; #1; Rebin; MA'
        self.assertEqual(period_name_rebin, ws_naming.add_rebin_to_name(period_name))
        self.assertEqual(period_name_rebin, ws_naming.add_rebin_to_name(period_name_rebin))


if __name__ == '__main__':
    unittest.main()
