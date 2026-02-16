# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest.mock import patch

from SANS import SANSadd2
from SANS.sans.common.enums import SampleShape
from mantid.simpleapi import Load
from SANSUtility import WorkspaceType


class TestSANSAddSampleMetadata(unittest.TestCase):
    def test_raw_files_get_correct_sample_info(self):
        result = SANSadd2.add_runs(("LOQ54432", "LOQ54432"), "LOQ", ".raw")
        assert result.startswith("The following file has been created:")
        assert result.endswith("LOQ54432-add.nxs")

        # load the output file and verify it has the sample information
        ws = Load("LOQ54432-add.nxs")
        sample = ws.sample()
        self.assertEqual(3, sample.getGeometryFlag())
        self.assertEqual(8.0, sample.getHeight())
        self.assertEqual(8.0, sample.getWidth())
        self.assertAlmostEqual(1.035, sample.getThickness())

    def test_raw_files_collect_sample_info_using_alg(self):
        with patch("SANS.SANSadd2.LoadSampleDetailsFromRaw") as mocked_load_sample:
            SANSadd2.add_runs(("LOQ54432", "LOQ54432"), "LOQ", ".raw")
            self.assertEqual(2, mocked_load_sample.call_count)

    def test_isis_neuxs_files_get_correct_sample_info(self):
        result = SANSadd2.add_runs(["74014"], "LOQ", ".nxs", rawTypes=(".add", ".raw", ".s*"), lowMem=False)
        assert result.startswith("The following file has been created:")
        assert result.endswith("LOQ74014-add.nxs")

        # load the output file and verify it has the sample information
        ws = Load("LOQ74014-add.nxs")
        sample = ws.sample()
        self.assertEqual(3, sample.getGeometryFlag())
        self.assertEqual(10.0, sample.getHeight())
        self.assertEqual(10.0, sample.getWidth())
        self.assertEqual(1.0, sample.getThickness())

    def test_isis_nexus_get_sample_info_using_file_information(self):
        with patch("SANS.SANSadd2.get_geometry_information_isis_nexus") as mocked_get_geo_info:
            mocked_get_geo_info.return_value = (8.0, 8.0, 2.0, SampleShape.DISC)
            SANSadd2.add_runs(("74014", "74014"), "LOQ", ".nxs", rawTypes=(".add", ".raw", ".s*"), lowMem=False)
            self.assertEqual(1, mocked_get_geo_info.call_count)

    @patch("SANS.SANSadd2.get_workspace_type")
    def test_is_not_allowed_instrument_branch(self, mocked_get_workspace_type):
        mocked_get_workspace_type.return_value = WorkspaceType.Event
        result = SANSadd2.add_runs(("LOQ54432", "LOQ54432"), "LOQ", ".raw")
        self.assertEqual(result, "")
