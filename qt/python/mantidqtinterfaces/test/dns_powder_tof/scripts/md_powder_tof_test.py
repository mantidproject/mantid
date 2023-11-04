# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest.mock import call, patch

from mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof import convert_to_de, get_sqw, load_data, pre_load_data
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import get_fake_tof_binning


class DNSScriptsTof(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pass

    @staticmethod
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.CorrectKiKf")
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.ConvertToDistribution")
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.ConvertUnits")
    def test_convert_to_d_e(mock_convert_units, mock_convert_to_dist, mock_correct_ki_kf):
        convert_to_de("abc", 2)
        mock_convert_units.assert_called_once_with("abc", Target="DeltaE", EMode="Direct", EFixed=2, OutputWorkspace="abc_dE")
        mock_convert_to_dist.assert_called_once_with("abc_dE")
        mock_correct_ki_kf.assert_called_once_with("abc_dE", OutputWorkspace="abc_dE_S")

    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.mtd")
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.BinMD")
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.MergeMD")
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.ConvertToMD")
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.ConvertToMDMinMaxGlobal")
    def test_get_sqw(self, mock_convert_to_md_min_max, mock_convert_to_md, mock_merge_md, mock_bin_md, mock_mtd):
        mock_convert_to_md_min_max.return_value = [0, 1]
        mock_mtd.__getitem__.return_value.getNumberOfEntries.return_value = 0
        b = get_fake_tof_binning()
        get_sqw(gws_name="abc", out_ws_name="ouname", b=b)
        self.assertEqual(len(mock_mtd.__getitem__.mock_calls), 3)
        self.assertEqual(mock_mtd.__getitem__.mock_calls[0], unittest.mock.call("abc"))
        self.assertEqual(mock_mtd.__getitem__.mock_calls[2], unittest.mock.call().getNumberOfEntries())
        self.assertEqual(mock_mtd.__getitem__.return_value.__getitem__.mock_calls[0], unittest.mock.call(0))
        mock_convert_to_md_min_max.assert_called_once_with(mock_mtd.__getitem__().__getitem__(), "|Q|", "Direct")
        mock_convert_to_md.assert_called_once_with(
            mock_mtd.__getitem__(),
            QDimensions="|Q|",
            dEAnalysisMode="Direct",
            PreprocDetectorsWS="-",
            MinValues=0,
            MaxValues=1,
            OutputWorkspace="gouname_mde",
        )
        mock_merge_md.assert_not_called()
        mock_bin_md.assert_called_once_with(
            InputWorkspace="gouname_mde",
            AlignedDim0="|Q|,0.1849336923669811,2.8349336923669815,106",
            AlignedDim1="DeltaE,-3.1409856698897682,3.1215096823206343,86",
            OutputWorkspace="ouname_sqw",
        )

        mock_mtd.__getitem__.return_value.getNumberOfEntries.return_value = 2
        get_sqw("abc", "ouname", b)
        mock_merge_md.assert_called_once_with("gouname_mde", OutputWorkspace="ouname_mde")

    @staticmethod
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.pre_load_data")
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.GroupWorkspaces")
    def test_load_data(mock_group_ws, mock_preload):
        data = {-5: 1, -6: 3, "path": 4}
        p = ""
        prefix = "a"
        load_data(data, prefix, p)
        calls = [call(-6, "a_1", "", {-5: 1, -6: 3, "path": 4}), call(-5, "a_2", "", {-5: 1, -6: 3, "path": 4})]
        mock_preload.assert_has_calls(calls)
        mock_group_ws.assert_called_once_with(["a_1", "a_2"], OutputWorkspace="a")

    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.DeleteWorkspaces")
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.MergeRuns")
    @patch("mantidqtinterfaces.dns_powder_tof.scripts.md_powder_tof.LoadDNSLegacy")
    def test_pre_load_data(self, mock_load_leg, mock_merge, mock_delete):
        data = {-5: [1], -6: [3], "path": "4_*.d_dat"}
        p = {"wavelength": 4, "e_channel": 3, "delete_raw": False}
        prefix = "a"
        test_v = pre_load_data(-5, prefix, p, data)
        mock_load_leg.assert_called_once_with("4_1.d_dat", Normalization="no", ElasticChannel=3, Wavelength=4, OutputWorkspace="ws_1")
        mock_merge.assert_called_once_with(
            ["ws_1"], SampleLogsSum="mon_sum,duration", SampleLogsTimeSeries="deterota,T1,T2,Tsp", OutputWorkspace="a"
        )
        mock_delete.assert_not_called()
        self.assertEqual(test_v, mock_merge.return_value)
        p = {"wavelength": -4, "e_channel": 3, "delete_raw": True}
        mock_load_leg.reset_mock()
        pre_load_data(-5, prefix, p, data)
        mock_load_leg.assert_called_once_with("4_1.d_dat", Normalization="no", ElasticChannel=3, OutputWorkspace="ws_1")
        mock_delete.assert_called_once_with(["ws_1"])


if __name__ == "__main__":
    unittest.main()
