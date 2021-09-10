# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
# std imports
import unittest
from numpy import radians

# 3rd party
from mantid.kernel import SpecialCoordinateSystem

# local imports
from mantidqt.widgets.sliceviewer.sliceinfo import SliceInfo
from mantidqt.widgets.sliceviewer.transform import NonOrthogonalTransform


class FakeTransform:
    def tr(self, x, y):
        return x + 1, y - 1

    def inv_tr(self, x, y):
        pass


class SliceInfoTest(unittest.TestCase):
    def test_construction_with_named_fields(self):
        frame, point, transpose, = SpecialCoordinateSystem.HKL, (None, None, 0.5), False
        dimrange, spatial = (None, None, (-15, 15)), [True] * 3
        info = SliceInfo(
            frame=frame, point=point, transpose=transpose, range=dimrange, qflags=spatial)

        self.assertEqual(frame, info.frame)
        self.assertEqual(point, info.slicepoint)
        self.assertEqual(dimrange, info.range)
        self.assertEqual(2, info.z_index)
        self.assertEqual(point[2], info.z_value)
        self.assertEqual(dimrange[2][1] - dimrange[2][0], info.z_width)

    def test_no_spatial_dimensions_sets_z_properties_None(self):
        frame, point, transpose, = SpecialCoordinateSystem.HKL, (None, None, 0.5), False
        dimrange, spatial = (None, None, (-15, 15)), [False] * 3
        info = SliceInfo(
            frame=frame, point=point, transpose=transpose, range=dimrange, qflags=spatial)

        self.assertTrue(info.z_index is None)
        self.assertTrue(info.z_value is None)
        self.assertTrue(info.z_width is None)

    def test_non_HKL_workspace_cannot_support_nonorthogonal_axes(self):
        self._assert_can_support_nonorthogonal_axes(
            False, SpecialCoordinateSystem.QLab, qflags=[True] * 3)

    def test_HKL_workspace_with_one_slice_spatial_dimension_cannot_support_nonorthogonal_axes(self):
        self._assert_can_support_nonorthogonal_axes(
            False, SpecialCoordinateSystem.HKL, qflags=[True, False, False])

    def test_HKL_workspace_with_no_slice_spatial_dimension_cannot_support_nonorthogonal_axes(self):
        self._assert_can_support_nonorthogonal_axes(
            False, SpecialCoordinateSystem.HKL, qflags=[False] * 3)

    def test_HKL_workspace_with_two_slice_spatial_dimensions_can_support_nonorthogonal_axes(self):
        self._assert_can_support_nonorthogonal_axes(
            True, SpecialCoordinateSystem.HKL, qflags=[True, True, False], transform=FakeTransform())

    def test_transform_selects_dimensions_correctly_when_not_transposed(self):
        # Set slice info such that display(X,Y) = data(Y,Z)
        slice_pt = 0.5
        info = SliceInfo(
            frame=SpecialCoordinateSystem.HKL,
            point=(slice_pt, None, None),
            transpose=False,
            range=[(-15, 15), None, None],
            qflags=[True, True, True])

        frame_point = (0.5, 1.0, -1.5)
        slice_frame = info.transform(frame_point)

        self.assertEqual(frame_point[1], slice_frame[0])
        self.assertEqual(frame_point[2], slice_frame[1])
        self.assertEqual(frame_point[0], slice_frame[2])
        self.assertEqual(slice_pt, info.z_value)

    def test_transform_selects_dimensions_correctly_when_transposed(self):
        # Set slice info such that display(X,Y) = data(Z,Y)
        slice_pt = 0.5
        info = SliceInfo(
            frame=SpecialCoordinateSystem.HKL,
            point=(slice_pt, None, None),
            transpose=True,
            range=[(-15, 15), None, None],
            qflags=[True, True, True])

        frame_point = (0.5, 1.0, -1.5)
        slice_frame = info.transform(frame_point)

        self.assertEqual(frame_point[2], slice_frame[0])
        self.assertEqual(frame_point[1], slice_frame[1])
        self.assertEqual(frame_point[0], slice_frame[2])
        self.assertEqual(slice_pt, info.z_value)

    def test_inverse_transform_selects_dimensions_correctly_when_not_transposed(self):
        # Set slice info such that display(X,Y) = data(Y,Z)
        slice_pt = 0.5
        info = SliceInfo(
            frame=SpecialCoordinateSystem.HKL,
            point=(slice_pt, None, None),
            transpose=False,
            range=[(-15, 15), None, None],
            qflags=[True, True, True])

        slice_frame = (0.5, 1.0, -1.5)
        data_frame = info.inverse_transform(slice_frame)

        self.assertEqual(data_frame[0], slice_frame[2])
        self.assertEqual(data_frame[1], slice_frame[0])
        self.assertEqual(data_frame[2], slice_frame[1])
        self.assertEqual(slice_pt, info.z_value)

    def test_inverse_transform_selects_dimensions_correctly_when_transposed(self):
        # Set slice info such that display(X,Y) = data(Z,Y)
        slice_pt = 0.5
        info = SliceInfo(
            frame=SpecialCoordinateSystem.HKL,
            point=(slice_pt, None, None),
            transpose=True,
            range=[(-15, 15), None, None],
            qflags=[True, True, True])

        frame_point = (-1.5, 1.0, 0.5)
        slice_frame = info.inverse_transform(frame_point)

        self.assertEqual(frame_point[0], slice_frame[2])
        self.assertEqual(frame_point[1], slice_frame[1])
        self.assertEqual(frame_point[2], slice_frame[0])
        self.assertEqual(slice_pt, info.z_value)

    def test_transform_respects_nonortho_tr_if_given(self):
        # Set slice info such that display(X,Y) = data(Z,Y)
        info = SliceInfo(
            frame=SpecialCoordinateSystem.HKL,
            point=(None, None, 0.5),
            transpose=False,
            range=[None, None, (-15, 15)],
            qflags=[True, True, True],
            nonortho_transform=FakeTransform())

        frame_point = (0.5, 1.0, -1.5)
        slice_frame = info.transform(frame_point)

        self.assertEqual(frame_point[0] + 1, slice_frame[0])
        self.assertEqual(frame_point[1] - 1, slice_frame[1])
        self.assertEqual(frame_point[2], slice_frame[2])

    def test_slicepoint_with_greater_than_three_qflags_true_raises_errors(self):
        self.assertRaises(
            AssertionError,
            SliceInfo,
            frame=SpecialCoordinateSystem.HKL,
            point=(1, None, None, 4),
            transpose=True,
            range=[(-15, 15), None, None, (-5, -5)],
            qflags=(True, True, True, True))

    def test_set_transform(self):
        # make sliceinfo with unit transform (default)
        info = SliceInfo(
            frame=SpecialCoordinateSystem.HKL,
            point=(None, None, 0.5),
            transpose=False,
            range=[None, None, (-15, 15)],
            qflags=[True, True, True])

        info.set_transform(NonOrthogonalTransform(angle=radians(120)))

        self.assertEqual(info.can_support_nonorthogonal_axes(), True)

    # private
    def _assert_can_support_nonorthogonal_axes(self, expectation, frame, qflags, transform=None):
        frame, point, transpose, = frame, (None, None, 0.5), False
        dimrange, spatial = (None, None, (-15, 15)), qflags

        info = SliceInfo(
            frame=frame, point=point, transpose=transpose, range=dimrange, qflags=spatial,
            nonortho_transform=transform)

        self.assertEqual(expectation, info.can_support_nonorthogonal_axes())


if __name__ == '__main__':
    unittest.main()
