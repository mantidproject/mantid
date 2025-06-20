# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from unittest.mock import patch, MagicMock
from mantid.api import AnalysisDataService as ADS

from Engineering.common.texture_sample_viewer import (
    has_no_valid_shape,
    get_xml_mesh,
    get_mesh_vertices_in_intrinsic_sample_axes,
    get_scaled_intrinsic_sample_directions_in_lab_frame,
    get_mesh_vertices,
)

texture_sample_viewer_path = "Engineering.common.texture_sample_viewer"


class TextureCorrectionModelTest(unittest.TestCase):
    def setUp(self):
        self.ws_name = "test_ws"

        # Mock workspace
        self.mock_ws = MagicMock()
        # Mock Sample
        mock_sample = MagicMock()
        self.mock_ws.sample.return_value = mock_sample

    def tearDown(self):
        if ADS.doesExist(self.ws_name):
            ADS.remove(self.ws_name)

    def get_cube_xml(self, name, side_len):
        return f"""
        <cuboid id='{name}'> \
        <height val='{side_len}'  /> \
        <width val='{side_len}' />  \
        <depth  val='{side_len}' />  \
        <centre x='0.0' y='0.0' z='0.0'  />  \
        </cuboid>  \
        <algebra val='{name}' /> \\ """

    def get_cube_mesh(self, side_len):
        p = side_len / 2
        return np.asarray(
            [
                [[p, -p, p], [p, -p, -p], [p, p, -p]],
                [[p, -p, p], [p, p, -p], [p, p, p]],
                [[p, p, p], [-p, p, p], [-p, -p, p]],
                [[p, p, p], [-p, -p, p], [p, -p, p]],
                [[p, p, p], [p, p, -p], [-p, p, -p]],
                [[p, p, p], [-p, p, -p], [-p, p, p]],
                [[p, p, -p], [-p, p, -p], [-p, -p, -p]],
                [[p, p, -p], [-p, -p, -p], [p, -p, -p]],
                [[p, -p, p], [p, -p, -p], [-p, -p, -p]],
                [
                    [p, -p, p],
                    [
                        -p,
                        -p,
                        -p,
                    ],
                    [-p, -p, p],
                ],
                [
                    [-p, -p, p],
                    [
                        -p,
                        -p,
                        -p,
                    ],
                    [-p, p, -p],
                ],
                [[-p, -p, p], [-p, p, -p], [-p, p, p]],
            ]
        )

    @patch(texture_sample_viewer_path + ".ADS")
    def test_has_no_valid_shape_returns_true_on_missing_mesh(self, mock_ads):
        shape = MagicMock()
        shape.getMesh.return_value = []  # empty mesh
        self.mock_ws.sample().getShape.return_value = shape
        mock_ads.retrieve.return_value = self.mock_ws
        self.assertTrue(has_no_valid_shape("ws"))

    @patch(texture_sample_viewer_path + ".ADS")
    def test_has_no_valid_shape_returns_false_with_mesh(self, mock_ads):
        shape = MagicMock()
        shape.getMesh.return_value = np.zeros((12, 3, 3))  # mesh has some data
        self.mock_ws.sample().getShape.return_value = shape
        mock_ads.retrieve.return_value = self.mock_ws
        self.assertTrue(not has_no_valid_shape("ws"))

    def test_get_xml_mesh_returns_mesh(self):
        mesh = get_xml_mesh(self.get_cube_xml("test-cube", 2))  # create 2m cube
        self.assertTrue(np.all((mesh.shape, (12, 3, 3))))  # expect cube to have 12 triangles (2 per face)
        self.assertTrue(np.all((np.unique(mesh), (-1, 1))))  # all coordinates should be +/- 1m
        self.assertTrue(np.all((mesh, self.get_cube_mesh(2))))

    @patch(texture_sample_viewer_path + ".logger")
    def test_get_xml_mesh_handles_bad_xml(self, mock_logger):
        bad_xml_str = "One cube, please"
        mesh = get_xml_mesh(bad_xml_str)
        self.assertTrue(len(mesh) == 0)
        mock_logger.error.assert_called_once_with(f"Shape mesh could not be constructed, check xml string is ok: '{bad_xml_str}'")

    def test_get_mesh_vertices(self):
        mesh = self.get_cube_mesh(2) + np.array((1, 2, 3))  # displace cube to help identify coordinates
        # cube in lab frame is:                  coords
        #  (0,3,4)    o--------o (2,3,4)         z  y
        #            /┃       /┃                 ┃ /
        # (0,1,4)   o--------o ┃ (2,1,4)         ┃/
        #  (0,3,2)  ┃ o------┃-o    (2,3,2)      +-----x
        #           ┃/       ┃/
        # (0,1,2)   o--------o   (2,1,2)
        expected_verts = np.array(((0, 1, 2), (2, 1, 2), (0, 3, 2), (2, 3, 2), (0, 1, 4), (2, 1, 4), (0, 3, 4), (2, 3, 4)))
        self.assertTrue(np.all(get_mesh_vertices(mesh) in expected_verts))

    def test_get_mesh_vertices_in_intrinsic_sample_axes(self):
        # set up the sample directions such that D1=y, D2=z, D3=x (see diagram)
        d1 = np.array((0, 1, 0))
        d2 = np.array((0, 0, 1))
        d3 = np.array((1, 0, 0))
        # combine into transform matrix
        ax_transform = np.concatenate((d1[:, None], d2[:, None], d3[:, None]), axis=1)
        # ax transform converts vectors in lab frame into an equivalent frame where x=D1, y=D2, and z=D3
        shape_mesh = self.get_cube_mesh(2) + np.array((0, 0, 1))  # translate cube along z in lab
        # cube in lab frame is:                  lab coords  sample directions
        #  (-1,1,2)   o--------o (1,1,2)         z  y        D2 D1
        #            /┃       /┃                 ┃ /         ┃ /
        # (-1,-1,2) o--------o ┃ (1,-1,2)        ┃/          ┃/
        #  (-1,1,0) ┃ o------┃-o    (1,1,0)      +-----x     +------D3
        #           ┃/   x   ┃/
        # (-1,-1,0) o--------o   (1,-1,0)
        #
        # cube in sample frame is:
        #  (-1,2,1)   o--------o (1,2,1)      D3(z) D2(y)
        #            /┃       /┃                 ┃ /
        # (-1,0,1)  o--------o ┃ (1,0,1)         ┃/
        #  (-1,2,-1)┃ o------┃-o    (1,2,-1)     +-----D1(x)
        #           ┃/       ┃/
        # (-1,0,-1) o--------o   (1,0,-1)

        # new mesh is same as a cube translated along y
        expected_mesh = self.get_cube_mesh(2) + np.array((0, 1, 0))
        permuted_mesh_verts = get_mesh_vertices_in_intrinsic_sample_axes(ax_transform, shape_mesh)
        # the order that the vertices appear changes, so we just check that each of the expected vertices are present
        self.assertTrue(np.all([v in permuted_mesh_verts.T for v in get_mesh_vertices(expected_mesh)]))

    def test_get_scaled_intrinsic_sample_directions_in_lab_frame(self):
        # set up the same sample as test_get_mesh_vertices_in_intrinsic_sample_axes but rotate it about x
        d1 = np.array((0, 1, 0))
        d2 = np.array((0, 0, 1))
        d3 = np.array((1, 0, 0))
        # combine into transform matrix
        ax_transform = np.concatenate((d1[:, None], d2[:, None], d3[:, None]), axis=1)
        rotation_matrix = np.array(((1, 0, 0), (0, -1, 0), (0, 0, -1)))  # 180 degrees around x
        # this time the cube needs to be displaced by -z (equivalent to applying R to the cube displaced by +z)
        sample_mesh = self.get_cube_mesh(2) + np.array((0, 0, -1))
        # cube in lab frame is:                  lab coords
        #  (-1,1,0)   o--------o (1,1,0)         z  y
        #            /┃  x    /┃                 ┃ /
        # (-1,-1,0) o--------o ┃ (1,-1,0)        ┃/          sample directions
        #  (-1,1,-2)┃ o------┃-o    (1,1,-2)     +-----x     +------D3
        #           ┃/       ┃/                             /┃
        # (-1,-1,-2)o--------o   (1,-1,-2)                 / ┃
        #                                                 D1 D2
        #
        rd, nd, td, arrow_lens = get_scaled_intrinsic_sample_directions_in_lab_frame(ax_transform, rotation_matrix, sample_mesh, scale=1)
        self.assertTrue(np.allclose(rd, (0, -1, 0)))
        self.assertTrue(np.allclose(nd, (0, 0, -1)))
        self.assertTrue(np.allclose(td, (1, 0, 0)))
        self.assertEqual(arrow_lens[0], 1)  # max distance along D1(-y) are the 4 points at y = -1 -> d = 1
        self.assertEqual(arrow_lens[1], 2)  # max distance along D2(-z) are the 4 points at z = -2 -> d = 2
        self.assertEqual(arrow_lens[2], 1)  # max distance along D3(+x) are the 4 points at x = 1 -> d = 1


if __name__ == "__main__":
    unittest.main()
