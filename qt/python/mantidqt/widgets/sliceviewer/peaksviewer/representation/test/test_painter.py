# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# std imports
import unittest
from unittest.mock import MagicMock

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation.painter import MplPainter


class MplPainterTest(unittest.TestCase):

    # --------------- success tests -----------------
    def test_remove_calls_remove_on_artist(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        mock_artist = MagicMock()

        painter.remove(mock_artist)

        mock_artist.remove.assert_called_once()

    def test_snap_to_sets_xy_limits_so_xy_at_center(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        x, y, snap_width = 1.5, 2.5, 0.1

        painter.snap_to(x, y, snap_width)

        axes.set_xlim.assert_called_once_with(x - snap_width, x + snap_width)
        axes.set_ylim.assert_called_once_with(y - snap_width, y + snap_width)

    def test_circle_draws_circle_patch(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        x, y, radius = 1, 2, 0.8

        artist = painter.circle(x, y, radius)

        axes.add_patch.assert_called_once()
        self.assertTrue(artist is not None)

    def test_cross_draws_with_only_xy(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        x, y, half_width = 1, 2, 0.1

        artist = painter.cross(x, y, half_width)

        axes.add_patch.assert_called_once()
        self.assertTrue(artist is not None)

    def test_cross_passes_kwargs_to_mpl(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        x, y, half_width = 1, 2, 0.1

        painter.cross(x, y, half_width, alpha=1)

        axes.add_patch.assert_called_once()

    def test_shell_adds_patch(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        x, y, outer_radius, thick = 1, 2, 0.8, 0.2

        painter.shell(x, y, outer_radius, thick)

        axes.add_patch.assert_called_once()

    def test_update_properties_passes_keywords_to_set(self):
        axes = MagicMock()
        painter = MplPainter(axes)
        mock_artist = MagicMock()

        painter.update_properties(mock_artist, alpha=1)

        mock_artist.set.assert_called_once_with(alpha=1)

    # --------------- failure tests -----------------
    def test_construction_raises_error_if_given_non_axes_instance(self):
        self.assertRaises(ValueError, MplPainter, 1)


if __name__ == '__main__':
    unittest.main()
