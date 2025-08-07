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

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_model import DNSElasticSCPlotModel
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import (
    get_fake_elastic_single_crystal_dataset,
    get_fake_elastic_single_crystal_options,
)


class DNSElasticSCPlotModelTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    @classmethod
    def setUpClass(cls):
        parent = mock.Mock()
        parent.view = None
        cls.model = DNSElasticSCPlotModel(parent)

    def setUp(self):
        self.model._single_crystal_map = mock.Mock()
        self.model._single_crystal_map.hkl_mesh_intp = [1, 2, 3]
        self.model._single_crystal_map.hkl_mesh = [0, 1, 2]

    def test___init__(self):
        self.assertIsInstance(self.model, DNSElasticSCPlotModel)
        self.assertIsInstance(self.model, DNSObsModel)
        self.assertTrue(hasattr(self.model, "_single_crystal_map"))
        self.assertTrue(hasattr(self.model, "_data"))
        self.assertTrue(hasattr(self.model._data, "x"))
        self.assertTrue(hasattr(self.model._data, "y"))
        self.assertTrue(hasattr(self.model._data, "z"))
        self.assertTrue(hasattr(self.model._data, "z_min"))
        self.assertTrue(hasattr(self.model._data, "z_max"))
        self.assertTrue(hasattr(self.model._data, "pz_min"))
        self.assertTrue(hasattr(self.model._data, "triangulation"))
        self.assertTrue(hasattr(self.model._data, "z_triang"))

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_model.DNSScMap")
    def test_create_single_crystal_map(self, mock_dns_scd_map):
        mock_dns_scd_map.return_value = 123
        data_array = get_fake_elastic_single_crystal_dataset()
        options = get_fake_elastic_single_crystal_options()
        self.model.create_single_crystal_map(data_array, options, initial_values={"a": 1})
        mock_dns_scd_map.assert_called_once()
        self.assertEqual(self.model._single_crystal_map, 123)

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_model.helper.get_projection")
    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_model.helper.filter_flattened_meshes")
    def test_get_projections(self, mock_ffm, mock_gpj):
        mock_ffm.return_value = [1, 2, 3]
        mock_gpj.return_value = 2
        test_v = self.model.get_projections([0, 1], [2, 3])
        mock_ffm.assert_called_once()
        self.assertEqual(mock_gpj.call_count, 2)
        self.assertEqual(mock_gpj.mock_calls, [mock.call(1, 3), mock.call(2, 3)])
        self.assertEqual(test_v[1], 2)
        self.assertEqual(test_v[0], 2)

    def test_get_interpolated_quadmesh(self):
        test_v = self.model.get_interpolated_quadmesh(True, "hkl")
        self.assertEqual(test_v, (1, 2, 3))
        self.assertEqual(self.model._data.x, 1)
        self.assertEqual(self.model._data.y, 2)
        self.assertEqual(self.model._data.z, 3)
        test_v = self.model.get_interpolated_quadmesh(False, "hkl")
        self.assertEqual(test_v, (0, 1, 2))

    def test_get_interpolated_triangulation(self):
        self.model._single_crystal_map.interpolate_triangulation.return_value = [1, 2]
        test_v = self.model.get_interpolated_triangulation(True, "hkl", False)
        self.assertEqual(self.model._data.x, 0)
        self.assertEqual(self.model._data.y, 1)
        self.assertEqual(self.model._data.z, 2)
        self.model._single_crystal_map.triangulate.assert_called_once_with(mesh_name="hkl_mesh", switch=False)
        self.model._single_crystal_map.mask_triangles.assert_called_once_with(mesh_name="hkl_mesh")
        self.model._single_crystal_map.interpolate_triangulation.assert_called_once_with(True)
        self.assertEqual(test_v, (1, 2))
        self.model._single_crystal_map.triangulate.reset_mock()
        self.model._single_crystal_map.interpolate_triangulation.reset_mock()
        self.model.get_interpolated_triangulation(False, "hkl", True)
        self.model._single_crystal_map.triangulate.assert_called_once_with(mesh_name="hkl_mesh", switch=True)
        self.model._single_crystal_map.interpolate_triangulation.assert_called_once_with(False)

    def test_get_xy_dy_ratio(self):
        self.model._single_crystal_map.dx = 2
        self.model._single_crystal_map.dy = 3
        test_v = self.model.get_xy_dy_ratio()
        self.assertEqual(test_v, 2 / 3)

    def test_get_aspect_ratio(self):
        self.model._single_crystal_map.dx = 2
        self.model._single_crystal_map.dy = 3
        test_v = self.model.get_aspect_ratio({"fix_aspect": True, "type": "hkl"})
        self.assertEqual(test_v, 2 / 3)
        test_v = self.model.get_aspect_ratio({"fix_aspect": True, "type": "q"})
        self.assertEqual(test_v, 1)
        test_v = self.model.get_aspect_ratio({"fix_aspect": False, "type": "q"})
        self.assertEqual(test_v, "auto")

    def test_get_axis_labels(self):
        self.model._single_crystal_map.hkl1 = "abc"
        self.model._single_crystal_map.hkl2 = "cde"
        self.model._single_crystal_map.get_crystal_axis_names = mock.Mock(return_value=[1, 2])
        test_v = self.model.get_axis_labels("hkl", False, False)
        self.assertEqual(test_v, ["abc (r.l.u.)", "cde (r.l.u.)"])
        test_v = self.model.get_axis_labels("hkl", False, True)
        self.assertEqual(test_v, ["cde (r.l.u.)", "abc (r.l.u.)"])
        test_v = self.model.get_axis_labels("qxqy", False, False)
        self.assertEqual(test_v, ["qx (Ang^-1)", "qy (Ang^-1)"])
        test_v = self.model.get_axis_labels("tthomega", False, False)
        self.assertEqual(test_v, ["2 Theta (deg)", "Omega (deg)"])
        test_v = self.model.get_axis_labels("tthomega", True, False)
        self.assertEqual(test_v, [1, 2])

    def test_get_changing_hkl_components(self):
        self.model._single_crystal_map.get_changing_hkl_components = mock.Mock(return_value=123)
        test_v = self.model.get_changing_hkl_components()
        self.assertEqual(test_v, 123)

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_model.get_hkl_intensity_from_cursor")
    def test_get_format_coord(self, mock_get_hkl_from_c):
        mock_get_hkl_from_c.return_value = [1, 2, 3, 4, 5]
        test_format_cord = self.model.get_format_coord("hkl")
        self.assertEqual(test_format_cord(12, 15), "x = 12.0000, y= 15.0000, hkl = ( 1.00, 2.00, 3.00), Intensity=      4.00 ±       5.00")
        mock_get_hkl_from_c.assert_called_once_with(self.model._single_crystal_map, "hkl", 12, 15)

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_model.helper.string_range_to_float")
    def test_get_mlimits(self, mock_stringrange):
        mock_stringrange.return_value = "a"
        test_v = self.model.get_m_limits(1, 2)
        self.assertEqual(test_v, ["a", "a"])

    @patch("mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_plot_model.helper.get_z_min_max")
    def test_get_data_z_min_max(self, mock_get_z_min_max):
        mock_get_z_min_max.return_value = 123
        self.model._data.x = 0
        self.model._data.y = 1
        self.model._data.z = 2
        test_v = self.model.get_data_z_min_max(xlim=[1, 2], ylim=[2, 3])
        mock_get_z_min_max.assert_called_once_with(2, [1, 2], [2, 3], 0, 1)
        self.assertEqual(test_v, 123)

    def test_get_data_xy_lim(self):
        self.model._data.x = np.asarray([[1, 2], [3, 4]])
        self.model._data.y = np.asarray([[7, 8], [3, 4]])
        # self.model._data.z = np.asarray([[10, 11], [13, 14]])
        test_v = self.model.get_data_xy_lim(False)
        self.assertEqual(test_v, [[1, 4], [3, 8]])
        test_v = self.model.get_data_xy_lim(True)
        self.assertEqual(test_v, [[3, 8], [1, 4]])

    def test_switch_axis(self):
        x = np.asarray([[1, 2], [3, 4]])
        y = np.asarray([[7, 8], [6, 4]])
        z = np.asarray([[10, 11], [13, 14]])
        test_v = self.model.switch_axis(x, y, z, False)
        self.assertEqual(test_v, (x, y, z))
        test_v = self.model.switch_axis(x, y, z, True)
        self.assertEqual(test_v[0][0, 1], 6)
        self.assertEqual(test_v[1][0, 1], 3)
        self.assertEqual(test_v[2][0, 1], 13)

    def test_get_omega_offset(self):
        self.model._single_crystal_map = {"omega_offset": 123}
        test_v = self.model.get_omega_offset()
        self.assertEqual(test_v, 123)

    def test_get_dx_dy(self):
        self.model._single_crystal_map = {"dx": 123, "dy": 345}
        test_v = self.model.get_dx_dy()
        self.assertEqual(test_v, (123, 345))


if __name__ == "__main__":
    unittest.main()
