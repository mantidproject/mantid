# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from unittest.mock import patch
import numpy as np
from mantidqtinterfaces.dns_single_crystal_elastic.data_structures.dns_single_crystal_map import DNSScMap
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import (
    get_fake_elastic_single_crystal_dataset,
    get_fake_elastic_single_crystal_options,
)
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_single_crystal_elastic.data_structures.dns_single_crystal_map import (
    _get_mesh,
    _correct_omega_offset,
    _get_unique,
    _get_q_mesh,
    _get_hkl_mesh,
)


class DNSScMapTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods, too-many-arguments

    @classmethod
    def setUpClass(cls):
        data_array = get_fake_elastic_single_crystal_dataset()
        options = get_fake_elastic_single_crystal_options()
        two_theta = data_array["two_theta_array"]
        omega = data_array["omega_array"]
        z_mesh = data_array["intensity"]
        error = data_array["error"]
        parameter = {
            "wavelength": options["wavelength"],
            "dx": options["dx"],
            "dy": options["dy"],
            "hkl1": options["hkl1"],
            "hkl2": options["hkl2"],
            "omega_offset": options["omega_offset"],
        }
        cls.map = DNSScMap(two_theta=two_theta, omega=omega, z_mesh=z_mesh, error_mesh=error, parameter=parameter)

    def setUp(self):
        self.map.hkl1 = "1,2,3"
        self.map.hkl2 = "2,3,4"

    def test___init__(self):
        self.assertIsInstance(self.map, DNSScMap)
        self.assertIsInstance(self.map, ObjectDict)
        self.assertTrue(hasattr(self.map, "omega_interpolated"))
        self.assertTrue((self.map.two_theta == [0, 1, 2]).all())
        self.assertTrue((self.map.omega == [4, 5]).all())
        self.assertEqual(self.map.omega_offset, 0)
        self.assertEqual(self.map.dx, 1)
        self.assertEqual(self.map.dy, 2)
        self.assertEqual(self.map.hkl1, "1,2,3")
        self.assertEqual(self.map.hkl2, "2,3,4")
        self.assertEqual(self.map.wavelength, 4.74)
        self.assertEqual(len(self.map.hkl_mesh), 3)
        self.assertTrue(np.allclose(self.map.hklx_mesh, [[0.0, 0.0], [-0.00022479, -0.00028889], [-0.0003854, -0.00051368]]))
        self.assertTrue(np.allclose(self.map.hkly_mesh, [[0.0, 0.0], [-0.00735043, -0.00734146], [-0.01470759, -0.01469189]]))
        self.assertTrue((self.map.z_mesh == np.array([[8.0, 11.0], [9.0, 12.0], [10.0, 13.0]])).all())
        self.assertTrue((self.map.error_mesh == np.array([[14.0, 17.0], [15.0, 18.0], [16.0, 19.0]])).all())

    def test__get_mesh(self):
        data_array = get_fake_elastic_single_crystal_dataset()
        two_theta = data_array["two_theta_array"]
        omega = data_array["omega_array"]
        z_mesh = data_array["intensity"]
        z_error_mesh = data_array["error"]
        z_mesh[0, 1] = np.nan
        test_v = _get_mesh(omega, two_theta, z_mesh, z_error_mesh)
        self.assertTrue((test_v[0] == np.array([4, 4, 5, 4, 5])).all())
        self.assertTrue((test_v[1] == np.array([0, 1, 1, 2, 2])).all())
        self.assertTrue((test_v[2] == np.array([8, 9, 12, 10, 13])).all())
        self.assertTrue((test_v[3] == np.array([14.0, 15.0, 18.0, 16.0, 19.0])).all())

    def test__correct_omega_offset(self):
        data_array = get_fake_elastic_single_crystal_dataset()
        omega = data_array["omega_array"]
        test_v = _correct_omega_offset(omega, 3)
        self.assertEqual(test_v[0], 1)
        self.assertEqual(test_v[1], 2)

    def test_get_unique(self):
        test_v = _get_unique([1, 1, 2], [3, 3, 3])
        self.assertTrue((test_v[0] == [1, 2]).all())
        self.assertTrue((test_v[1] == [3]).all())

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.data_structures.dns_single_crystal_map.angle_to_q")
    def test__get_q_mesh(self, mock_angle_to_q):
        mock_angle_to_q.return_value = ([1], [4])
        test_v = _get_q_mesh([5, 5.5], [12, 0], 3)
        mock_angle_to_q.assert_called_once_with(two_theta=[12, 0], omega=[5, 5.5], wavelength=3)
        self.assertEqual(test_v, mock_angle_to_q.return_value)

    def test__get_hkl_mesh(self):
        test_v = _get_hkl_mesh(2, 3, 4, 5)
        self.assertAlmostEqual(test_v[0], 1.2732395447351628)
        self.assertAlmostEqual(test_v[1], 2.3873241463784303)

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.data_structures.dns_single_crystal_map.tri")
    def test_triangulate(self, mock_tri):
        self.map.hkl_mesh = [np.asarray([0, 1]), np.asarray([2, 3]), np.asarray([4, 5])]
        test_v = self.map.triangulate("hkl_mesh", switch=False)
        mock_tri.Triangulation.assert_called_once()
        self.assertTrue((mock_tri.Triangulation.call_args_list[0][0][0] == np.asarray([0, 1])).all())
        self.assertTrue((mock_tri.Triangulation.call_args_list[0][0][1] == np.asarray([2, 3])).all())
        self.assertEqual(test_v, mock_tri.Triangulation.return_value)
        mock_tri.reset_mock()

    def test_interpolate_triangulation(self):
        z_mock = mock.Mock()
        self.map.triangulation = None
        self.map.z_mesh = z_mock
        self.assertIsNone(self.map.interpolate_triangulation())
        self.map.triangulation = 1
        test_v = self.map.interpolate_triangulation()
        self.assertEqual(test_v[0], 1)
        self.assertEqual(test_v[1], z_mock.flatten.return_value)

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.data_structures.dns_single_crystal_map.path")
    def test_get_dns_map_border(self, mock_path):
        test_v = self.map.get_dns_map_border("qxqy")
        self.assertEqual(test_v, mock_path.Path.return_value)
        test_array = np.array(
            [
                [0.0, 0.0],
                [0.0, 0.0],
                [0.0, 0.0],
                [-0.00141237, -0.02309205],
                [-0.00242151, -0.04620524],
                [-0.00242151, -0.04620524],
                [-0.00322754, -0.04615594],
                [-0.00322754, -0.04615594],
                [-0.00181517, -0.02306389],
                [0.0, 0.0],
            ]
        )
        self.assertTrue(np.allclose(mock_path.Path.call_args_list[0][0][0], test_array))
        mock_path.reset_mock()
        self.map.get_dns_map_border("hkl")
        test_array = np.array(
            [
                [0.0, 0.0],
                [0.0, 0.0],
                [0.0, 0.0],
                [-0.00022479, -0.00735043],
                [-0.0003854, -0.01470759],
                [-0.0003854, -0.01470759],
                [-0.00051368, -0.01469189],
                [-0.00051368, -0.01469189],
                [-0.00028889, -0.00734146],
                [0.0, 0.0],
            ]
        )
        self.assertTrue(np.allclose(mock_path.Path.call_args_list[0][0][0], test_array))

    def test_mask_triangles(self):
        mock_triang = mock.Mock()
        self.map.triangulation = mock_triang
        self.map.triangulation.triangles = np.array([[5, 4, 2], [2, 4, 0], [5, 2, 3], [3, 2, 0]])
        test_v = self.map.mask_triangles("hkl_mesh")
        self.assertEqual(test_v, mock_triang)
        mock_triang.set_mask.assert_called_once()
        carg = mock_triang.set_mask.call_args_list[0][0][0]
        self.assertTrue((carg == np.array([False, True, False, False])).all())

    def test_get_changing_indexes(self):
        test_v = self.map.get_changing_indexes()
        self.assertEqual(test_v, [1, 2])  # 3 changing indexes, does not work
        self.map.hkl1 = "0, 1, 0"
        self.map.hkl2 = "1, 1, 0"
        test_v = self.map.get_changing_indexes()
        self.assertEqual(test_v, [0, 1])
        self.map.hkl1 = "0, 1, 2"
        self.map.hkl2 = "0, 1, 1"
        test_v = self.map.get_changing_indexes()
        self.assertEqual(test_v, [1, 2])

    def test_get_changing_hkl_components(self):
        test_v = self.map.get_changing_hkl_components()
        self.assertEqual(test_v, (2.0, 3.0, 3.0, 4.0))


if __name__ == "__main__":
    unittest.main()
