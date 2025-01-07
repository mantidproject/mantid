import unittest
from unittest import mock
from numpy import linspace, meshgrid, dstack
from numpy.testing import assert_array_equal
from matplotlib.collections import QuadMesh

from mantid.plots.quad_mesh_wrapper import QuadMeshWrapper


class QuadMeshWrapperTest(unittest.TestCase):
    def setUp(self):
        self.mock_mesh = mock.create_autospec(QuadMesh, instance=True)
        self.mock_mesh.axes = mock.MagicMock()
        xx, yy = meshgrid(linspace(-2, 2, 11), linspace(-1, 1, 5))
        self.mock_mesh._coordinates = dstack((xx, yy))
        self.mock_mesh.get_array.return_value = xx[:-1, :-1]
        self.quad_mesh_wrapper = QuadMeshWrapper(self.mock_mesh)

    def test_quad_mesh_clipped_array_for_all_data_within_axes_limits(self):
        # set axes limits to the extent of the data
        self.mock_mesh.axes.get_xlim.return_value = (-2, 2)
        self.mock_mesh.axes.get_ylim.return_value = (-1, 1)
        quad_mesh_wrapper = QuadMeshWrapper(self.mock_mesh)

        assert_array_equal(quad_mesh_wrapper.get_array_clipped_to_bounds(), self.mock_mesh.get_array().flatten())

    def test_quad_mesh_clipped_array_for_subset_data_within_axes_limits(self):
        # set axes limits to half the extent of the data
        self.mock_mesh.axes.get_xlim.return_value = (-2, 0)
        self.mock_mesh.axes.get_ylim.return_value = (-1, 0)
        quad_mesh_wrapper = QuadMeshWrapper(self.mock_mesh)

        assert_array_equal(quad_mesh_wrapper.get_array_clipped_to_bounds(), self.mock_mesh.get_array()[:2, :5].flatten())

    def test_quad_mesh_clipped_array_for_no_data_within_axes_limits(self):
        # set axes limits outside the extent of the data
        self.mock_mesh.axes.get_xlim.return_value = (3, 4)
        self.mock_mesh.axes.get_ylim.return_value = (3, 4)
        quad_mesh_wrapper = QuadMeshWrapper(self.mock_mesh)

        self.assertTrue(quad_mesh_wrapper.get_array_clipped_to_bounds().size == 0)


if __name__ == "__main__":
    unittest.main()
