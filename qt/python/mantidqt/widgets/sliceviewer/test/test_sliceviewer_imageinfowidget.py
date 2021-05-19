# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
# std imports
from numpy import radians, eye
from sys import float_info
import unittest
from unittest.mock import MagicMock, patch

from mantidqt.widgets.sliceviewer.imageinfowidget import ImageInfoWidget, ImageInfoTracker
from mantidqt.widgets.sliceviewer.transform import NonOrthogonalTransform  # noqa: E402


class ImageInfoTrackerTest(unittest.TestCase):

    def setUp(self):
        self.image = MagicMock()
        self.nonortho_transform = NonOrthogonalTransform(radians(60))
        self.image_info_widget = MagicMock(ImageInfoWidget)

    def test_nonortho_transform_not_applied_when_mesh_and_transfrom_false(self):
        del self.image.get_extent  # so image treated as mesh
        tracker = ImageInfoTracker(image=self.image,
                                   transform=self.nonortho_transform,
                                   do_transform=False,
                                   widget=self.image_info_widget)

        tracker.on_cursor_at(1.0, 1.0)

        self.image_info_widget.cursorAt.assert_called_with(1.0, 1.0, float_info.max)

    def test_nonortho_transform_applied_when_mesh_and_transfrom_true(self):
        del self.image.get_extent  # so image treated as mesh
        tracker = ImageInfoTracker(image=self.image,
                                   transform=self.nonortho_transform,
                                   do_transform=True,
                                   widget=self.image_info_widget)

        tracker.on_cursor_at(1.0, 1.0)

        self.image_info_widget.cursorAt.assert_called_with(0.42264973081037405, 1.1547005383792517, float_info.max)

    @patch("mantidqt.widgets.sliceviewer.imageinfowidget.cursor_info")
    def test_nonortho_transform_not_applied_when_not_mesh(self, mock_cursorinfo):
        mock_cursorinfo.return_value = (
            2 * eye(2, dtype=float), None, (1, 1))  # output simple 2D array and slice indices
        tracker = ImageInfoTracker(image=self.image,
                                   transform=self.nonortho_transform,
                                   do_transform=True,
                                   widget=self.image_info_widget)

        tracker.on_cursor_at(1.0, 1.0)

        self.image_info_widget.cursorAt.assert_called_with(1.0, 1.0, 2.0)


if __name__ == '__main__':
    unittest.main()
