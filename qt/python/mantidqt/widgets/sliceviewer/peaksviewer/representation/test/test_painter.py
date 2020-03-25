# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import, division, unicode_literals

# std imports
import unittest

# 3rdparty imports
from mantid.py3compat.mock import MagicMock

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation.painter import MplPainter


class MplPainterTest(unittest.TestCase):

    # --------------- success tests -----------------
    def test_remove_calls_remove_on_artists(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        mock_artist1, mock_artist2 = MagicMock(), MagicMock()

        painter.remove([mock_artist1, mock_artist2])

        mock_artist1.remove.assert_called_once()
        mock_artist2.remove.assert_called_once()

    def test_snap_to_sets_xy_limits_so_xy_at_center(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        x, y = 1.5, 2.5

        painter.snap_to(x, y)

        axes.set_xlim.assert_called_once_with(x - MplPainter.SNAP_WIDTH, x + MplPainter.SNAP_WIDTH)
        axes.set_ylim.assert_called_once_with(y - MplPainter.SNAP_WIDTH, y + MplPainter.SNAP_WIDTH)

    def test_circle_draws_circle_patch(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        x, y, radius = 1, 2, 0.8

        artist = painter.circle(x, y, radius)

        axes.add_patch.assert_called_once()
        self.assertTrue(artist is not None)

    def test_scatter_draws_with_only_xy(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        x, y = 1, 2

        artist = painter.scatter(x, y)

        axes.scatter.assert_called_once_with(x, y)
        self.assertTrue(artist is not None)

    def test_scatter_passes_kwargs_to_mpl(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        x, y = 1, 2

        painter.scatter(x, y, alpha=1)

        axes.scatter.assert_called_once_with(x, y, alpha=1)

    def test_update_properties_passes_keywords_to_set_for_each_artist(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        artist1, artist2 = MagicMock(), MagicMock()

        painter.update_properties([artist1, artist2], alpha=1)

        artist1.set.assert_called_once_with(alpha=1)
        artist2.set.assert_called_once_with(alpha=1)

    # --------------- failure tests -----------------
    def test_construction_raises_error_if_given_non_axes_instance(self):
        self.assertRaises(ValueError, MplPainter, 1)


if __name__ == '__main__':
    unittest.main()
