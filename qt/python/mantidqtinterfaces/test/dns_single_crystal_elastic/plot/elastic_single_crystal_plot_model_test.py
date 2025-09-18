# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
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
        self.assertTrue(hasattr(self.model._data, "triang"))
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

    def test_get_interpolated_triangulation(self):
        self.model._single_crystal_map.interpolate_triangulation.return_value = [1, 2]
        test_v = self.model.generate_triangulation_mesh(True, "hkl", False)
        self.assertEqual(self.model._data.x, 0)
        self.assertEqual(self.model._data.y, 1)
        self.assertEqual(self.model._data.z, 2)
        self.model._single_crystal_map.triangulate.assert_called_once_with(mesh_name="hkl_mesh", switch=False)
        self.model._single_crystal_map.mask_triangles.assert_called_once_with(mesh_name="hkl_mesh")
        self.model._single_crystal_map.interpolate_triangulation.assert_called_once_with(True)
        self.assertEqual(test_v, (1, 2))
        self.model._single_crystal_map.triangulate.reset_mock()
        self.model._single_crystal_map.interpolate_triangulation.reset_mock()
        self.model.generate_triangulation_mesh(False, "hkl", True)
        self.model._single_crystal_map.triangulate.assert_called_once_with(mesh_name="hkl_mesh", switch=True)
        self.model._single_crystal_map.interpolate_triangulation.assert_called_once_with(False)

    def test_get_axis_labels(self):
        self.model._single_crystal_map.hkl1 = "abc"
        self.model._single_crystal_map.hkl2 = "cde"
        self.model._single_crystal_map.get_crystal_axis_names = mock.Mock(return_value=[1, 2])
        test_v = self.model.get_axis_labels("hkl", False, False)
        self.assertEqual(test_v, ["[abc] (r.l.u.)", "[cde] (r.l.u.)"])

    def test_get_changing_hkl_components(self):
        self.model._single_crystal_map.get_changing_hkl_components = mock.Mock(return_value=123)
        test_v = self.model.get_changing_hkl_components()
        self.assertEqual(test_v, 123)

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
        self.model._data.x = np.array([[1, 2], [3, 4]])
        self.model._data.y = np.array([[7, 8], [3, 4]])
        test_v = self.model.get_data_xy_lim(False)
        self.assertEqual(test_v, [[1, 4], [3, 8]])
        test_v = self.model.get_data_xy_lim(True)
        self.assertEqual(test_v, [[3, 8], [1, 4]])

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
