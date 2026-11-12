# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import TestCase
from unittest.mock import patch

from qtpy.QtGui import QFont, QFontInfo
from qtpy.QtWidgets import QComboBox, QWidget

from mantidqt.utils.qt.testing import start_qapplication
from workbench.widgets.about.view import AboutViewWidget


@start_qapplication
class AboutViewWidgetTest(TestCase):
    INSTRUMENT_SELECTOR_CLASSPATH = "workbench.widgets.about.view.instrumentselector.InstrumentSelector"

    def setUp(self):
        with patch(self.INSTRUMENT_SELECTOR_CLASSPATH, QComboBox):
            self.widget = AboutViewWidget(parent=None, version_text="7.0.0", date_text=None)

    def tearDown(self):
        self.widget.deleteLater()

    def test_all_fonts_have_positive_point_size(self):
        # pixel-sized fonts report a point size of -1, which causes Qt warnings with the Windows 11 style
        for child in [self.widget] + self.widget.findChildren(QWidget):
            child.ensurePolished()
            self.assertGreater(child.font().pointSize(), 0, f"{type(child).__name__} has a pixel-sized font")

    def test_rescale_pixels_to_points_preserves_pixel_size(self):
        for px_value in (8, 12, 14, 18, 22, 28):
            font = QFont(self.widget.font())
            font.setPointSizeF(self.widget.rescale_pixels_to_points(px_value))
            self.assertEqual(self.widget.rescale_w(px_value), QFontInfo(font).pixelSize())


if __name__ == "__main__":
    unittest.main()
