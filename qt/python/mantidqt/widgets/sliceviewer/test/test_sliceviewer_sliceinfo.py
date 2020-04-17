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
from mantidqt.widgets.sliceviewer.model import SliceInfo


class SliceInfoTest(unittest.TestCase):
    def test_construction_with_named_fields(self):
        indices, frame, point, dimrange = (0, 1, 2), "HKL", (None, None, 0.5), (-15, 15)
        info = SliceInfo(indices=indices, frame=frame, point=point, range=dimrange)

        self.assertEqual(indices, info.indices)
        self.assertEqual(frame, info.frame)
        self.assertEqual(point, info.point)
        self.assertEqual(dimrange, info.range)

    def test_transform_uses_indices_to_transform_to_slice_frame(self):
        # Set slice info such that display(X,Y) = data(Z,Y)
        info = SliceInfo(indices=(2, 1, 0), frame="HKL", point=(None, None, 0.5), range=(-15, 15))

        frame_point = (0.5, 1.0, -1.5)
        slice_frame = info.transform(frame_point)

        self.assertEqual(frame_point[2], slice_frame[0])
        self.assertEqual(frame_point[1], slice_frame[1])
        self.assertEqual(frame_point[0], slice_frame[2])


if __name__ == '__main__':
    unittest.main()
