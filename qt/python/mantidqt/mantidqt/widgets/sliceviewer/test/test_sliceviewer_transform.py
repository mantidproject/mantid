# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

from mantidqt.widgets.sliceviewer.transform import NonOrthogonalTransform
from numpy.testing import assert_allclose
import numpy as np


class TransformTest(unittest.TestCase):
    def test_nonorthogonal_transform_skews_as_expected(self):
        transform = NonOrthogonalTransform(angle=np.radians(45.))  # 45deg

        x = np.array([0, 1])
        y = np.array([1, 0])
        xp, yp = transform.tr(x, y)

        assert_allclose(xp, np.array([1. / np.sqrt(2.), 1.]))
        assert_allclose(yp, np.array([1. / np.sqrt(2.), 0.]))

    def test_nonorthogonal_transform_round_trip(self):
        transform = NonOrthogonalTransform(angle=np.radians(40.))
        x, y = np.array([1., 2., 3.]), np.array([4., 5., 6.])
        xp, yp = transform.tr(x, y)
        xpinv, ypinv = transform.inv_tr(xp, yp)

        assert_allclose(x, xpinv)
        assert_allclose(y, ypinv)


if __name__ == '__main__':
    unittest.main()
