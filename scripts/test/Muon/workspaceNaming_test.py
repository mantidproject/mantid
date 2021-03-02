# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.ADSHandler import workspace_naming as ws_naming
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantid.api import AnalysisDataService, FileFinder
from mantid import ConfigService
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename


class WorkspaceNamingTest(unittest.TestCase):
    def setUp(self):
        return

    def test_getGroupOrPairName(self):
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; Group; bkwd; Asymmetry; Periods; #1; MA'),
                         "bkwd")
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; Group; fwd; Asymmetry; Periods; #2; MA'),
                         "fwd")
        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; Pair Asym; long; Periods; #1; MA'), "long")

        self.assertEqual(ws_naming.get_group_or_pair_from_name('MUSR62260; PhaseQuad; test_Re_;MA'),"test_Re_")

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

    def test_check_phasequad_name(self):
        self.assertEqual(True,ws_naming.check_phasequad_name("Ref_data_Re_"))
        self.assertEqual(True,ws_naming.check_phasequad_name("Ref_data_Im_"))
        self.assertEqual(True,ws_naming.check_phasequad_name("Image_data_Re_"))
        self.assertEqual(True,ws_naming.check_phasequad_name("Image_data_Im_"))
        self.assertEqual(False,ws_naming.check_phasequad_name("Ref_data"))
        self.assertEqual(False,ws_naming.check_phasequad_name("Image_data"))

    def test_add_phasequad_extension(self):
        self.assertEqual("Ref_data_Re__Im_",ws_naming.add_phasequad_extensions("Ref_data"))

    def test_get_pair_phasequad_name(self):
        AnalysisDataService.clear()
        ConfigService['MantidOptions.InvisibleWorkspaces'] = 'True'
        filepath = FileFinder.findRuns('EMU00019489.nxs')[0]

        load_result, run_number, filename, psi_data = load_workspace_from_filename(filepath)

        context = setup_context()
        context.gui_context.update({'RebinType': 'None'})
        context.data_context.instrument = 'EMU'

        context.data_context._loaded_data.add_data(workspace=load_result, run=[run_number], filename=filename,
                                                   instrument='EMU')
        context.data_context.current_runs = [[run_number]]
        context.data_context.update_current_data()

        self.assertEqual("EMU19489; PhaseQuad; test_Re; MA",ws_naming.get_pair_phasequad_name(context, "test_Re", "19489",False))
        self.assertEqual("EMU19489; PhaseQuad; test_Re; Rebin; MA",ws_naming.get_pair_phasequad_name(context, "test_Re", "19489",True))


if __name__ == '__main__':
    unittest.main()
