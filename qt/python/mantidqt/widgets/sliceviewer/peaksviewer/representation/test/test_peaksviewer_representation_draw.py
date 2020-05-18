# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

# std imports
import json
import unittest
from unittest.mock import MagicMock

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.representation.draw \
    import draw_peak_representation
from mantidqt.widgets.sliceviewer.peaksviewer.representation.test.shapetesthelpers \
    import create_ellipsoid_info, create_sphere_info


def draw_shape(shape_name, shape_info=None):
    """
    Draw a shape with the given name and properties
    :param shape_name: The str name of the shape
    :param shape_info: The properties of the shape as an optional JSON-encoded str
    """
    peak_origin, fg_color, bg_color = (1, 3, 5), "r", "g"
    peak_shape, slice_info, painter = (MagicMock(), ) * 3
    peak_shape.shapeName.return_value = shape_name
    if shape_info is not None:
        peak_shape.toJSON.return_value = shape_info
    # identity transform
    slice_info.transform.side_effect = lambda x: x
    slice_info.z_value = 5.
    slice_info.z_width = 10.

    draw_peak_representation(peak_origin, peak_shape, slice_info, painter, fg_color, bg_color)

    return painter


class DrawTest(unittest.TestCase):
    def test_draw_peak_representation_falls_back_to_nonintegrated_if_shape_unknown(self):
        painter = draw_shape("unknown_shape")

        painter.cross.assert_called_once()

    def test_draw_peak_representation_for_none_shape(self):
        painter = draw_shape("none")

        painter.cross.assert_called_once()

    def test_draw_peak_representation_for_spherical_shape_without_background(self):
        shape_descr = create_sphere_info(0.5)
        painter = draw_shape("spherical", json.dumps(shape_descr))

        painter.cross.assert_called_once()
        painter.circle.assert_called_once()

    def test_draw_peak_representation_for_spherical_shape_including_background(self):
        shape_descr = create_sphere_info(0.5, (1.1, 1.2))
        painter = draw_shape("spherical", json.dumps(shape_descr))

        painter.cross.assert_called_once()
        painter.circle.assert_called_once()
        painter.shell.assert_called_once()

    def test_draw_peak_representation_for_ellipsoid_shape(self):
        shape_descr = create_ellipsoid_info((0.5, 0.3, 0.2), ("1 0 0", "0 1 0", "0 0 1"),
                                            ((1.45, 1.25, 1.15), (1.5, 1.3, 1.2)))
        painter = draw_shape("ellipsoid", json.dumps(shape_descr))

        painter.cross.assert_called_once()
        painter.ellipse.assert_called_once()
        painter.elliptical_shell.assert_called_once()


if __name__ == "__main__":
    unittest.main()
