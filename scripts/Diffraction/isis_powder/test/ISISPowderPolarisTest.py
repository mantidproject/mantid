# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest.mock import MagicMock
from Diffraction.isis_powder.polaris_routines import polaris_algs


class ISISPowderPolarisTest(unittest.TestCase):
    def test_merge_banks_validates_stitch_params(self):
        mock_ws = MagicMock()
        mock_kwargs = MagicMock()
        mock_freq_params = MagicMock()

        stitch_points = [1, 2, 3, 4]
        stitch_lims = (0, 12)
        overlap_width = 0.07
        kwargs_list = [
            {"stitch_points": None, "stitch_lims": stitch_lims, "overlap_width": overlap_width},
            {"stitch_points": stitch_points, "stitch_lims": None, "overlap_width": overlap_width},
            {"stitch_points": stitch_points, "stitch_lims": stitch_lims, "overlap_width": None},
        ]
        for kwargs in kwargs_list:
            with self.assertRaisesRegex(ValueError, "All three of 'stitch_points', 'stitch_lims', and 'overlap_width'"):
                polaris_algs._merge_banks(mock_ws, mock_kwargs, mock_freq_params, None, **kwargs)

    def test_merge_banks_validates_stitch_lims(self):
        kwargs_list = [
            {"stitch_lims": (0,), "overlap_width": MagicMock()},
            {"stitch_lims": "a_string", "overlap_width": MagicMock()},
            {"stitch_lims": (0, 2, 4), "overlap_width": MagicMock()},
        ]

        for kwargs in kwargs_list:
            with self.assertRaisesRegex(ValueError, r"(\(.*\,.*\)|a_string) is not a valid value for 'stitch_lims'"):
                polaris_algs._merge_banks(MagicMock(), MagicMock(), MagicMock(), None, MagicMock(), **kwargs)


if __name__ == "__main__":
    unittest.main()
