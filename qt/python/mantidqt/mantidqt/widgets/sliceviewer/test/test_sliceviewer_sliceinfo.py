# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
# std imports
import unittest

# 3rd party
from numpy import full, radians, sin, eye

# local imports
from mantidqt.widgets.sliceviewer.models.sliceinfo import SliceInfo


class SliceInfoTest(unittest.TestCase):
    def test_construction_with_named_fields(self):
        point = (None, None, 0.5)
        dimrange = (None, None, (-15, 15))
        info = make_sliceinfo(point=point, dimrange=dimrange)

        self.assertEqual(point, info.slicepoint)
        self.assertEqual(dimrange, info.range)
        self.assertEqual(2, info.z_index)
        self.assertEqual(point[2], info.z_value)
        self.assertEqual(dimrange[2][1] - dimrange[2][0], info.z_width)
        self.assertTrue(info.can_support_nonorthogonal_axes())

    def test_no_spatial_dimensions_sets_z_properties_None_and_disables_nonorthogonal_axes(self):
        info = make_sliceinfo(qflags=[False] * 3)

        self.assertTrue(info.z_index is None)
        self.assertTrue(info.z_value is None)
        self.assertTrue(info.z_width is None)
        self.assertFalse(info.can_support_nonorthogonal_axes())

    def test_cannot_support_nonorthogonal_axes_when_no_angles_supplied_with_spatial_dimensions(self):
        info = make_sliceinfo(qflags=[True] * 3, axes_angles=None)
        self.assertFalse(info.can_support_nonorthogonal_axes())

    def test_cannot_support_nonorthogonal_axes_in_slice_with_one_spatial_dimensions(self):
        info = make_sliceinfo(qflags=[True, False, False])
        self.assertFalse(info.can_support_nonorthogonal_axes())

    def test_transform_selects_dimensions_correctly_when_not_transposed(self):
        # Set slice info such that display(X,Y) = data(Y,Z)
        slice_pt = 0.5
        info = make_sliceinfo(point=(slice_pt, None, None), dimrange=[(-15, 15), None, None])

        frame_point = (0.5, 1.0, -1.5)
        slice_frame = info.transform(frame_point)

        self.assertAlmostEqual(frame_point[1], slice_frame[0], delta=1e-6)
        self.assertAlmostEqual(frame_point[2], slice_frame[1], delta=1e-6)
        self.assertAlmostEqual(frame_point[0], slice_frame[2], delta=1e-6)
        self.assertEqual(slice_pt, info.z_value)

    def test_transform_selects_dimensions_correctly_when_transposed(self):
        # Set slice info such that display(X,Y) = data(Z,Y)
        slice_pt = 0.5
        info = make_sliceinfo(point=(slice_pt, None, None), dimrange=[(-15, 15), None, None], transpose=True)

        frame_point = (0.5, 1.0, -1.5)
        slice_frame = info.transform(frame_point)

        self.assertAlmostEqual(frame_point[2], slice_frame[0], delta=1e-6)
        self.assertAlmostEqual(frame_point[1], slice_frame[1], delta=1e-6)
        self.assertAlmostEqual(frame_point[0], slice_frame[2], delta=1e-6)
        self.assertEqual(slice_pt, info.z_value)

    def test_transform_preceding_nonQdim_4D_MD_ws_nonortho_transform(self):
        angles = full((3, 3), radians(90))
        angles[0, 1] = radians(60)
        angles[1, 0] = radians(60)
        # dims: E,H,K,L - viewing (X,Y) = (K,H) with H,K non-orthog (angle=60 deg)
        info = make_sliceinfo(
            point=(0.0, None, None, -1.0),
            dimrange=[(-1.0, 1.0), None, None, (-2.0, 2.0)],
            qflags=(False, True, True, True),
            axes_angles=angles,
            transpose=True,
        )
        slice_pt = -1.0
        frame_point = (2.0, 0.0, slice_pt)
        slice_frame = info.transform(frame_point)
        self.assertAlmostEqual(slice_frame[0], 1.0, delta=1e-6)
        self.assertAlmostEqual(slice_frame[1], 2 * sin(angles[0, 1]), delta=1e-6)
        self.assertAlmostEqual(slice_frame[2], slice_pt, delta=1e-6)
        self.assertEqual(slice_pt, info.z_value)

    def test_inverse_transform_selects_dimensions_correctly_when_not_transposed(self):
        # Set slice info such that display(X,Y) = data(Y,Z)
        slice_pt = 0.5
        info = make_sliceinfo(point=(slice_pt, None, None), dimrange=[(-15, 15), None, None])

        slice_frame = (0.5, 1.0, -1.5)
        data_frame = info.inverse_transform(slice_frame)

        self.assertAlmostEqual(data_frame[0], slice_frame[2], delta=1e-6)
        self.assertAlmostEqual(data_frame[1], slice_frame[0], delta=1e-6)
        self.assertAlmostEqual(data_frame[2], slice_frame[1], delta=1e-6)
        self.assertEqual(slice_pt, info.z_value)

    def test_inverse_transform_selects_dimensions_correctly_when_transposed(self):
        # Set slice info such that display(X,Y) = data(Z,Y)
        slice_pt = 0.5
        info = make_sliceinfo(point=(slice_pt, None, None), dimrange=[(-15, 15), None, None], transpose=True)

        frame_point = (-1.5, 1.0, 0.5)
        slice_frame = info.inverse_transform(frame_point)

        self.assertAlmostEqual(frame_point[0], slice_frame[2], delta=1e-6)
        self.assertAlmostEqual(frame_point[1], slice_frame[1], delta=1e-6)
        self.assertAlmostEqual(frame_point[2], slice_frame[0], delta=1e-6)
        self.assertEqual(slice_pt, info.z_value)

    def test_inverse_transform_preceding_nonQdim_4D_MD_ws(self):
        info = make_sliceinfo(
            point=(0.0, None, None, -1.0), dimrange=[(-1.0, 1.0), None, None, (-2.0, 2.0)], qflags=(False, True, True, True), transpose=True
        )  # dims: E,H,K,L - viewing (X,Y) = (K,H)
        slice_pt = -1.0
        frame_point = (1.0, 2.0, slice_pt)
        slice_frame = info.inverse_transform(frame_point)
        self.assertAlmostEqual(slice_frame[0], 2.0, delta=1e-6)
        self.assertAlmostEqual(slice_frame[1], 1.0, delta=1e-6)
        self.assertAlmostEqual(slice_frame[2], slice_pt, delta=1e-6)
        self.assertEqual(slice_pt, info.z_value)

    def test_inverse_transform_preceding_nonQdim_4D_MD_ws_nonortho_transform(self):
        angles = full((3, 3), radians(90))
        angles[0, 1] = radians(60)
        angles[1, 0] = radians(60)
        # dims: E,H,K,L - viewing (X,Y) = (K,H) with H,K non-orthog (angle=60 deg)
        info = make_sliceinfo(
            point=(0.0, None, None, -1.0),
            dimrange=[(-1.0, 1.0), None, None, (-2.0, 2.0)],
            qflags=(False, True, True, True),
            axes_angles=angles,
            transpose=True,
        )
        slice_pt = -1.0
        frame_point = (1.0, 2 * sin(angles[0, 1]), slice_pt)
        slice_frame = info.inverse_transform(frame_point)
        self.assertAlmostEqual(slice_frame[0], 2.0, delta=1e-6)
        self.assertAlmostEqual(slice_frame[1], 0.0, delta=1e-6)
        self.assertAlmostEqual(slice_frame[2], slice_pt, delta=1e-6)
        self.assertEqual(slice_pt, info.z_value)

    def test_slicepoint_with_greater_than_three_qflags_true_raises_errors(self):
        self.assertRaises(
            AssertionError,
            SliceInfo,
            point=(1, None, None, 4),
            transpose=True,
            range=[(-15, 15), None, None, (-5, -5)],
            qflags=(True, True, True, True),
        )

    def test_slicepoint_proj_matrix(self):
        info = make_sliceinfo(proj_matrix=[[1.0, 0.0, 1.0], [1.0, 0.0, -1.0], [0.0, 1.0, 0.0]])

        ## point to HKL

        # HKL=[0,0,1]
        HKL = info.inverse_transform([0, 1, 0])
        self.assertEqual(HKL[0], 0)
        self.assertEqual(HKL[1], 0)
        self.assertEqual(HKL[2], 1)

        # HKL=[1,1,0]
        HKL = info.inverse_transform([1, 0, 0])
        self.assertEqual(HKL[0], 1)
        self.assertEqual(HKL[1], 1)
        self.assertEqual(HKL[2], 0)

        ## HKL to point

        # HKL=[0,0,1]
        point = info.transform([0, 0, 1])
        self.assertEqual(point[0], 0)
        self.assertEqual(point[1], 1)
        self.assertEqual(point[2], 0)

        # HKL=[1,1,0]
        point = info.transform([1, 1, 0])
        self.assertEqual(point[0], 1)
        self.assertEqual(point[1], 0)
        self.assertEqual(point[2], 0)


def make_sliceinfo(
    point=(None, None, 0.5),
    transpose=False,
    dimrange=(None, None, (-15, 15)),
    qflags=[True, True, True],
    axes_angles=full((3, 3), radians(90)),
    proj_matrix=eye(3),
):
    return SliceInfo(point=point, transpose=transpose, range=dimrange, qflags=qflags, axes_angles=axes_angles, proj_matrix=proj_matrix)


if __name__ == "__main__":
    unittest.main()
