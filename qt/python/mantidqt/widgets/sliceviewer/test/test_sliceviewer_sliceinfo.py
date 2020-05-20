# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
# std imports
import unittest

# local imports
from mantidqt.widgets.sliceviewer.sliceinfo import SliceInfo


class SliceInfoTest(unittest.TestCase):
    def test_construction_with_named_fields(self):
        frame, point, transpose, dimrange = "HKL", (None, None, 0.5), False, (-15, 15)
        info = SliceInfo(frame=frame, point=point, transpose=transpose, range=dimrange)

        self.assertEqual(frame, info.frame)
        self.assertEqual(point, info.slicepoint)
        self.assertEqual(dimrange, info.range)
        self.assertEqual(point[2], info.value)

    def test_transform_selects_dimensions_correctly_when_not_transposed(self):
        # Set slice info such that display(X,Y) = data(Y,Z)
        slice_pt = 0.5
        info = SliceInfo(frame="HKL",
                         point=(slice_pt, None, None),
                         transpose=False,
                         range=(-15, 15))

        frame_point = (0.5, 1.0, -1.5)
        slice_frame = info.transform(frame_point)

        self.assertEqual(frame_point[1], slice_frame[0])
        self.assertEqual(frame_point[2], slice_frame[1])
        self.assertEqual(frame_point[0], slice_frame[2])
        self.assertEqual(slice_pt, info.value)

    def test_transform_selects_dimensions_correctly_when_transposed(self):
        # Set slice info such that display(X,Y) = data(Z,Y)
        slice_pt = 0.5
        info = SliceInfo(frame="HKL", point=(slice_pt, None, None), transpose=True, range=(-15, 15))

        frame_point = (0.5, 1.0, -1.5)
        slice_frame = info.transform(frame_point)

        self.assertEqual(frame_point[2], slice_frame[0])
        self.assertEqual(frame_point[1], slice_frame[1])
        self.assertEqual(frame_point[0], slice_frame[2])
        self.assertEqual(slice_pt, info.value)

    def test_slicepoint_not_length_three_raises_errors(self):
        self.assertRaises(AssertionError,
                          SliceInfo,
                          frame="HKL",
                          point=(1, None, None, 4),
                          transpose=True,
                          range=(-15, 15))


if __name__ == '__main__':
    unittest.main()
