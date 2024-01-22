# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest.mock import patch
from unittest.mock import call

from mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic import (
    background_subtraction,
    flipping_ratio_correction,
    load_all,
    load_binned,
    raise_error,
    vanadium_correction,
)

from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import get_fake_elastic_data_dic, get_fake_MD_workspace_unique
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_error import DNSError

from mantid.simpleapi import mtd
from mantid.api import IMDHistoWorkspace


class MDPowderElasticTest(unittest.TestCase):
    def setUp(self):
        mtd.clear()

    def test_background_subtraction(self):
        get_fake_MD_workspace_unique(name="vana_x_sf", factor=64)
        get_fake_MD_workspace_unique(name="vana_x_sf_norm", factor=1 / 18)
        test_v = background_subtraction("empty_x_sf")
        self.assertIsNone(test_v)
        with self.assertRaises(DNSError) as context:
            background_subtraction("vana_x_sf")
        self.assertTrue("No background file " in str(context.exception))

        get_fake_MD_workspace_unique(name="empty_x_sf", factor=8)
        get_fake_MD_workspace_unique(name="empty_x_sf_norm", factor=1)
        test_v = background_subtraction("vana_x_sf")
        self.assertIsInstance(test_v, IMDHistoWorkspace)
        value = test_v.getSignalArray()
        error = test_v.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 4, 1))
        self.assertAlmostEqual(value[0, 0, 0], 1144)
        self.assertAlmostEqual(error[0, 0, 0], 1220)

    def test_flipping_ratio_correction(self):
        get_fake_MD_workspace_unique(name="vana_x_sf", factor=27)
        get_fake_MD_workspace_unique(name="vana_x_sf_norm", factor=1)
        test_v = flipping_ratio_correction("vana_x_nsf")
        self.assertFalse(test_v)
        with self.assertRaises(DNSError) as context:
            flipping_ratio_correction("vana_x_sf")
        self.assertTrue("no matching nsf workspace found " "for" in str(context.exception))

        get_fake_MD_workspace_unique(name="vana_x_nsf", factor=64)
        get_fake_MD_workspace_unique(name="vana_x_nsf_norm", factor=27)
        with self.assertRaises(DNSError) as context:
            flipping_ratio_correction("vana_x_sf")
        self.assertTrue("no matching NiCr workspace found " "for " in str(context.exception))

        get_fake_MD_workspace_unique(name="nicr_x_sf", factor=27)
        get_fake_MD_workspace_unique(name="nicr_x_sf_norm", factor=1)
        get_fake_MD_workspace_unique(name="nicr_x_nsf", factor=8)
        get_fake_MD_workspace_unique(name="nicr_x_nsf_norm", factor=1)
        test_v = flipping_ratio_correction("vana_x_sf")
        self.assertTrue(test_v)
        test_v = mtd["vana_x_sf"]
        self.assertIsInstance(test_v, IMDHistoWorkspace)
        value = test_v.getSignalArray()
        error = test_v.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 4, 1))
        self.assertAlmostEqual(value[0, 0, 0], 1432.421052631579)
        self.assertAlmostEqual(error[0, 0, 0], 169533.7651184383)
        test_v = mtd["vana_x_nsf"]
        value = test_v.getSignalArray()
        error = test_v.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 4, 1))
        self.assertAlmostEqual(value[0, 0, 0], 205.57894736842104)
        self.assertAlmostEqual(error[0, 0, 0], 170199.7651184383)

    @staticmethod
    @patch("mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic.load_binned")
    def test_load_all(mock_load_binned):
        data_dict = get_fake_elastic_data_dic()
        load_all(data_dict, [0, 1, 2], normalize_to="monitor")
        calls = [
            call("knso_x_nsf", [0, 1, 2], "C:/data", range(554574, 554634, 6), "monitor"),
            call("knso_x_sf", [0, 1, 2], "C:/data", range(554573, 554633, 6), "monitor"),
            call("knso_y_nsf", [0, 1, 2], "C:/data", range(554576, 554636, 6), "monitor"),
            call("knso_y_sf", [0, 1, 2], "C:/data", range(554575, 554635, 6), "monitor"),
            call("knso_z_nsf", [0, 1, 2], "C:/data", range(554578, 554638, 6), "monitor"),
            call("knso_z_sf", [0, 1, 2], "C:/data", range(554577, 554637, 6), "monitor"),
        ]
        mock_load_binned.assert_has_calls(calls)

    @patch("mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic.mtd")
    @patch("mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic.BinMD")
    @patch("mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic.LoadDNSSCD")
    def test_load_binned(self, mock_load, mock_bin_md, mock_mtd):
        test_v = load_binned("knso_x_nsf", [0, 1, 2], "C:/data_******.d_dat", range(554574, 554578, 2), "monitor")
        mock_load.assert_called_once_with(
            "C:/data_554574.d_dat, C:/data_554576.d_dat",
            OutputWorkspace="knso_x_nsf",
            NormalizationWorkspace="knso_x_nsf_norm",
            Normalization="monitor",
            LoadAs="raw",
            TwoThetaLimits="0,1",
        )
        calls = [
            call(
                InputWorkspace="knso_x_nsf",
                OutputWorkspace="knso_x_nsf",
                AxisAligned=True,
                AlignedDim0="Theta,0.0,0.5,2",
                AlignedDim1="Omega,0.0,359.0,1",
                AlignedDim2="TOF,424.0,2000.0,1",
            ),
            call(
                InputWorkspace="knso_x_nsf_norm",
                OutputWorkspace="knso_x_nsf_norm",
                AxisAligned=True,
                AlignedDim0="Theta,0.0,0.5,2",
                AlignedDim1="Omega,0.0,359.0,1",
                AlignedDim2="TOF,424.0,2000.0,1",
            ),
        ]
        mock_bin_md.assert_has_calls(calls)
        mock_mtd.__getitem__.assert_called_once_with("knso_x_nsf")
        self.assertEqual(test_v, mock_mtd.__getitem__.return_value)

    def test_raise_error(self):
        with self.assertRaises(DNSError):
            raise_error("123")

    def test_vanadium_correction_1(self):
        get_fake_MD_workspace_unique(name="sample_x_sf", factor=64)
        get_fake_MD_workspace_unique(name="sample_x_sf_norm", factor=1 / 18)
        with self.assertRaises(DNSError) as context:
            vanadium_correction("sample_x_sf", binning=[10, 20, 11], vana_set=None, ignore_vana_fields=False, sum_vana_sf_nsf=False)
        self.assertTrue("No vanadium file for" in str(context.exception))
        get_fake_MD_workspace_unique(name="vana_x_sf", factor=27)
        get_fake_MD_workspace_unique(name="vana_x_sf_norm", factor=8)
        test_v = vanadium_correction("sample_x_sf", binning=[10, 20, 11], vana_set=None, ignore_vana_fields=False, sum_vana_sf_nsf=False)
        self.assertIsInstance(test_v, IMDHistoWorkspace)
        value = test_v.getSignalArray()
        error = test_v.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 4, 1))
        self.assertAlmostEqual(value[0, 0, 0], 1152)
        self.assertAlmostEqual(error[0, 0, 0], 1411172.9748371872)
        test_v = mtd["sample_x_sf_norm"]
        value = test_v.getSignalArray()
        error = test_v.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 4, 1))
        self.assertAlmostEqual(value[0, 0, 0], 1)
        self.assertAlmostEqual(error[0, 0, 0], 1.0624796359872226)

    def test_vanadium_correction_2(self):
        with self.assertRaises(DNSError) as context:
            vanadium_correction("sample_x_sf", binning=[5, 20, 16], vana_set=None, ignore_vana_fields=True, sum_vana_sf_nsf=False)
        self.assertTrue("Need to give vanadium" in str(context.exception))
        with self.assertRaises(DNSError) as context:
            vanadium_correction(
                "sample_x_sf", binning=[5, 20, 16], vana_set=["x_sf, x_nsf"], ignore_vana_fields=True, sum_vana_sf_nsf=False
            )
        self.assertTrue("No vanadium file for " in str(context.exception))
        get_fake_MD_workspace_unique(name="sample_x_sf", factor=64)
        get_fake_MD_workspace_unique(name="sample_x_sf_norm", factor=8)
        get_fake_MD_workspace_unique(name="vana_x_sf", factor=27)
        get_fake_MD_workspace_unique(name="vana_x_sf_norm", factor=1)
        get_fake_MD_workspace_unique(name="vana_x_nsf", factor=8)
        get_fake_MD_workspace_unique(name="vana_x_nsf_norm", factor=1 / 18)
        test_v = vanadium_correction(
            "sample_x_sf", binning=[5, 20, 16], vana_set={"x_sf": 1, "x_nsf": 2}, ignore_vana_fields=True, sum_vana_sf_nsf=False
        )
        self.assertIsInstance(test_v, IMDHistoWorkspace)

        value = test_v.getSignalArray()
        error = test_v.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 4, 1))
        self.assertAlmostEqual(value[0, 0, 0], 8)
        self.assertAlmostEqual(error[0, 0, 0], 17.758122739857274)
        test_v = mtd["sample_x_sf_norm"]
        value = test_v.getSignalArray()
        error = test_v.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 4, 1))
        self.assertAlmostEqual(value[0, 0, 0], 144)
        self.assertAlmostEqual(error[0, 0, 0], 5735.631767713756)

    def test_vanadium_correction_3(self):
        with self.assertRaises(DNSError) as context:
            vanadium_correction("sample_x_sf", binning=[5, 120, 116], vana_set=None, ignore_vana_fields=False, sum_vana_sf_nsf=True)
        self.assertTrue("No vanadium file for x_sf" in str(context.exception))
        get_fake_MD_workspace_unique(name="vana_x_sf", factor=27)
        get_fake_MD_workspace_unique(name="vana_x_sf_norm", factor=8)
        with self.assertRaises(DNSError) as context:
            vanadium_correction("sample_x_sf", binning=[5, 120, 116], vana_set=None, ignore_vana_fields=False, sum_vana_sf_nsf=True)
        self.assertTrue("No vanadium file for x_nsf" in str(context.exception))
        get_fake_MD_workspace_unique(name="sample_x_sf", factor=64)
        get_fake_MD_workspace_unique(name="sample_x_sf_norm", factor=1 / 18)
        get_fake_MD_workspace_unique(name="vana_x_nsf", factor=8)
        get_fake_MD_workspace_unique(name="vana_x_nsf_norm", factor=1)
        test_v = vanadium_correction("sample_x_sf", binning=[5, 120, 116], vana_set=None, ignore_vana_fields=False, sum_vana_sf_nsf=True)
        self.assertIsInstance(test_v, IMDHistoWorkspace)
        value = test_v.getSignalArray()
        error = test_v.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 4, 1))
        self.assertAlmostEqual(value[0, 0, 0], 1152)
        self.assertAlmostEqual(error[0, 0, 0], 1343922.983910595)
        test_v = mtd["sample_x_sf_norm"]
        value = test_v.getSignalArray()
        error = test_v.getErrorSquaredArray()
        self.assertEqual(value.shape, (5, 4, 1))
        self.assertAlmostEqual(value[0, 0, 0], 1)
        self.assertAlmostEqual(error[0, 0, 0], 1.0118053927277704)


if __name__ == "__main__":
    unittest.main()
