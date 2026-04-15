# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from unittest import TestCase, main
from unittest.mock import patch
from qtpy.QtGui import QColor

from workbench.config.fonts import _get_currentline_background_color


class FontsHelperFunctionTest(TestCase):
    @patch("workbench.config.fonts.IS_MAC", True)
    @patch("workbench.config.fonts.IS_DARK_MODE", True)
    def test_mac_dark_mode(self):
        self.assertEqual(_get_currentline_background_color(), QColor(0, 52, 110))

    @patch("workbench.config.fonts.IS_MAC", True)
    @patch("workbench.config.fonts.IS_DARK_MODE", False)
    def test_mac_light_mode(self):
        self.assertEqual(_get_currentline_background_color(), QColor(247, 236, 248))

    @patch("workbench.config.fonts.IS_MAC", False)
    def test_non_mac(self):
        self.assertEqual(_get_currentline_background_color(), QColor(247, 236, 248))


if __name__ == "__main__":
    main()
