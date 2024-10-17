# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
import unittest
from unittest import mock
from numpy import eye, array, array_equal, zeros, allclose, c_, tile, sqrt, ones

from mantidqt.widgets.sliceviewer.cutviewer.model import CutViewerModel


class TestCutViewerModel(unittest.TestCase):
    def setUp(self):
        # load empty instrument so can create a peak table
        self.model = CutViewerModel(eye(3))
        self.mock_dims = mock.MagicMock()
        # setup dimensions corresponding to HK plane with (X,Y) = (K,H)
        self.mock_dims.get_states.side_effect = lambda: [1, 0, None]  # so return can be modified
        self.mock_dims.get_bin_params.return_value = [100, 50, 0.1]  # e.g 100 bins in H which is y-axis of colorfill

    def test_get_default_bin_params(self):
        vectors, extents, nbins = self.model.get_default_bin_params(self.mock_dims, ax_lims=((-4.0, 4.0), (-5.0, 5.0)), slicept=0)

        self.assertTrue(array_equal(vectors, array([[0, 1, 0], [1, 0, 0], [0, 0, 1]])))
        self.assertTrue(array_equal(extents, array([[-2, -0.2, -0.05], [2, 0.2, 0.05]])))
        self.assertTrue(array_equal(nbins, array([25, 1, 1])))

    def test_invalid_bin_params_when_vectors_have_zero_det(self):
        self.assertFalse(self.model.valid_bin_params(zeros((3, 3)), tile(c_[[0.0, 1.0]], (1, 3)), array([10, 1, 1])))

    def test_invalid_bin_params_when_max_less_min_in_extents(self):
        self.assertFalse(self.model.valid_bin_params(eye(3), tile(c_[[1.0, 0.0]], (1, 3)), array([10, 1, 1])))

    def test_invalid_bin_params_when_nbins_all_equal_one(self):
        self.assertFalse(self.model.valid_bin_params(eye(3), tile(c_[[0.0, 1.0]], (1, 3)), array([1, 1, 1])))

    def test_validate_vectors_when_new_vector_in_slice_plane(self):
        self.assertTrue(
            array_equal(
                self.model.validate_vectors(0, 1, array([[1, 1, 0], [0, 1, 0], [0, 0, 1]])), array([[1, 1, 0], [1, -1, 0], [0, 0, 1]])
            )
        )

    def test_validate_vectors_when_new_vector_out_of_slice_plane(self):
        # e.g. if last elem of first vec changed such that u1 not perp u3 - chooses new u1 vector chosen in slice plane
        self.assertTrue(array_equal(self.model.validate_vectors(0, 1, array([[1, 0, 1], [0, 1, 0], [0, 0, 1]])), eye(3)))

    def test_validate_nbins_with_two_nbins_grtr_one(self):
        self.assertTrue(array_equal(self.model.validate_nbins(0, 1, nbins=array([10, 10, 1])), array([10, 1, 1])))

    def test_validate_nbins_when_nbins_equal_one(self):
        # check nbins along ivec changed to default value of nbins along a cut (50)
        self.assertTrue(array_equal(self.model.validate_nbins(0, 1, nbins=ones(3)), array([1, 50, 1])))

    def test_validate_step_when_integer_nbins_in_extents(self):
        in_extents = tile(c_[[0.0, 1.0]], (1, 3))
        in_nbins = array([10, 1, 1])

        nbins, extents = self.model.validate_step(0, 1, in_nbins, in_extents, step=0.2)

        self.assertTrue(array_equal(nbins, array([5, 1, 1])))  # i.e. nbins along u1 halved when step doubled
        self.assertTrue(array_equal(extents, in_extents))

    def test_validate_step_when_noninteger_nbins_in_extents(self):
        in_extents = tile(c_[[0.0, 1.0]], (1, 3))
        in_nbins = array([10, 1, 1])

        nbins, extents = self.model.validate_step(0, 1, in_nbins, in_extents, step=0.3)

        # check u1 max changed from 1->0.9 (3 bins of width 0.3)
        self.assertTrue(array_equal(nbins, array([3, 1, 1])))
        self.assertTrue(allclose(extents, array([[0, 0, 0], [0.9, 1, 1]]), rtol=0.0, atol=1e-10))

    def test_calc_cut_representation_parameters(self):
        xmin, xmax, ymin, ymax, thick = self.model.calc_cut_representation_parameters(
            eye(3)[:-1, :], tile(c_[[0.0, 1.0]], (1, 2)), array([10, 1]), [0, 1, None]
        )

        self.assertTrue(array_equal(self.model.xvec, array([1, 0, 0])))
        self.assertTrue(array_equal(self.model.yvec, array([0, 1, 0])))
        self.assertAlmostEqual(xmin, 0.0, delta=1e-10)
        self.assertAlmostEqual(xmax, 1.0, delta=1e-10)
        self.assertAlmostEqual(ymin, 0.5, delta=1e-10)
        self.assertAlmostEqual(ymax, 0.5, delta=1e-10)
        self.assertAlmostEqual(thick, 1.0, delta=1e-10)

    def test_calc_bin_params_from_cut_representation(self):
        self.model.xvec = array([1, 0, 0])
        self.model.yvec = array([0, 1, 0])
        thickness = 0.1  # defined for unit vector perp to cut
        # get bin params corresponding to start (x,y) = (-1,-1) to end (1, 1)
        vectors, extents, nbins = self.model.calc_bin_params_from_cut_representation(-1, 1, -1, 1, 0.1, array([0, 0, 1]))

        self.assertTrue(array_equal(nbins, array([50, 1])))  # 50 is default nbins
        self.assertTrue(allclose(vectors, array([[1, 1, 0], [1, -1, 0]]) / sqrt(2), rtol=0.0, atol=1e-10))
        self.assertTrue(allclose(extents, array([[-sqrt(2), -thickness / 2], [sqrt(2), thickness / 2]]), rtol=0.0, atol=1e-10))


if __name__ == "__main__":
    unittest.main()
