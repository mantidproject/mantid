# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from unittest.mock import patch

import numpy as np

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model import DNSElasticPowderPlotModel
from mantidqtinterfaces.dns_powder_elastic.plot import elastic_powder_plot_model


class DNSElasticPowderPlotModelTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    parent = None
    fake_workspace = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.model = DNSElasticPowderPlotModel(cls.parent)
        cls.fake_workspace = mock.Mock()
        cls.fake_workspace.extractX.return_value = np.asarray([[0, 1]])
        cls.fake_workspace.extractY.return_value = np.asarray([[4, 2]])
        cls.fake_workspace.extractE.return_value = np.asarray([[3, 4]])
        cls.test_array = np.asarray([1, 3])

    def test___init__(self):
        self.assertIsInstance(self.model, DNSElasticPowderPlotModel)
        self.assertIsInstance(self.model, DNSObsModel)
        self.assertTrue(hasattr(self.model, "_plotted_script_number"))

    @patch("mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model.mtd")
    def test__add_simulation_to_ws_list(self, mtd):
        ws_list = []
        mtd.__getitem__.return_value = 1
        elastic_powder_plot_model._add_simulation_to_ws_list(ws_list)
        test_v = elastic_powder_plot_model._add_simulation_to_ws_list(ws_list)
        self.assertEqual(test_v, ["mat_simulation"])
        ws_list = []
        mtd.__getitem__.side_effect = KeyError
        test_v = elastic_powder_plot_model._add_simulation_to_ws_list(ws_list)
        self.assertEqual(test_v, [])

    @patch("mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model.mtd")
    def test_get_max_int_of_workspaces(self, mtd):
        mtd.__getitem__.return_value = self.fake_workspace
        test_v = self.model.get_max_int_of_workspaces(["123"])
        self.assertEqual(test_v, 4)
        test_v = self.model.get_max_int_of_workspaces(["simulation"])
        self.assertEqual(test_v, 1)

    @patch("mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model.mtd")
    def test__get_x(self, mtd):
        mtd.__getitem__.return_value = self.fake_workspace
        test_v = elastic_powder_plot_model._get_x("123")
        self.assertEqual(test_v, np.asarray(1))

    @patch("mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model.mtd")
    def test__get_y(self, mtd):
        mtd.__getitem__.return_value = self.fake_workspace
        test_v = elastic_powder_plot_model._get_y("123")
        self.assertEqual(test_v[0], 4)
        self.assertEqual(len(test_v), 2)

    @patch("mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model.mtd")
    def test__get_yerr(self, mtd):
        mtd.__getitem__.return_value = self.fake_workspace
        test_v = elastic_powder_plot_model._get_yerr("123")
        self.assertEqual(test_v[0], 3)
        self.assertEqual(len(test_v), 2)

    def test__convert_x_axis(self):
        test_v = elastic_powder_plot_model._convert_x_axis(self.test_array, "tt", 4.74)
        self.assertEqual(test_v[0], 1)
        self.assertEqual(test_v[1], 3)
        test_v = elastic_powder_plot_model._convert_x_axis(self.test_array, "d", 4.74)
        self.assertAlmostEqual(test_v[0], 271.58544194790886)
        self.assertAlmostEqual(test_v[1], 90.53767353344176)
        test_v = elastic_powder_plot_model._convert_x_axis(self.test_array, "q", 4.74)
        self.assertAlmostEqual(test_v[0], 0.023135206593233835)
        self.assertAlmostEqual(test_v[1], 0.06939857257165744)

    def test__scale_simulation(self):
        test_v = elastic_powder_plot_model._scale_simulation("a", self.test_array, self.test_array, 5)
        self.assertEqual(test_v[0][0], 1)
        test_v = elastic_powder_plot_model._scale_simulation("simulation", self.test_array, self.test_array, 5)
        self.assertEqual(test_v[0][0], 0.5)
        self.assertEqual(test_v[1][0], 1 / 3 * 5)

    @patch("mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model.mtd")
    def test_get_x_y_yerr(self, mtd):
        mtd.__getitem__.return_value = self.fake_workspace
        test_v = self.model.get_x_y_yerr("a", "tt", 5, 4.74)
        self.assertEqual(test_v[0][0], 1)
        self.assertEqual(test_v[1][0], 4)
        self.assertEqual(test_v[2][0], 3)

    def test__datalist_updated(self):
        test_v = self.model._data_list_updated(["mat_a", "mat_b"], ["a", "b"], 0)
        self.assertFalse(test_v)
        test_v = self.model._data_list_updated(["mat_a", "mat_b"], ["a"], 0)
        self.assertTrue(test_v)
        test_v = self.model._data_list_updated(["mat_a", "mat_b"], ["a", "b"], 1)
        self.assertTrue(test_v)

    @patch("mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model.mtd")
    def test_get_updated_ws_list(self, mtd):
        self.model._plotted_script_number = 0
        mtd.__getitem__.return_value = self.fake_workspace
        test_v = self.model.get_updated_ws_list(["mat_b", "mat_a"], ["a"], 1)
        self.assertEqual(test_v[0], ["mat_a", "mat_b", "mat_simulation"])
        self.assertTrue(test_v[1])
        self.assertEqual(self.model._plotted_script_number, 1)

    def test_get_y_norm_label(self):
        test_v = self.model.get_y_norm_label(True)
        self.assertEqual(test_v, "Counts/Monitor Counts")
        test_v = self.model.get_y_norm_label(False)
        self.assertEqual(test_v, "Counts/s")

    def test_get_x_axis_label(self):
        test_v = self.model.get_x_axis_label("q")
        self.assertEqual(test_v, r"q $(\AA^{-1})$")
        test_v = self.model.get_x_axis_label("d")
        self.assertEqual(test_v, r"d $(\AA)$")
        test_v = self.model.get_x_axis_label("tt")
        self.assertEqual(test_v, r"2$\theta$ (deg)")

    def test__set_script_number(self):
        self.model._set_script_number(5)
        self.assertEqual(self.model._plotted_script_number, 5)
        self.model._plotted_script_number = 0

    @patch("mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model.mtd")
    def test__check_workspace_exists(self, mtd):
        mtd.__getitem__.return_value = "123"
        test_v = elastic_powder_plot_model._check_workspace_exists("123")
        self.assertTrue(test_v)
        mtd.__getitem__.side_effect = KeyError
        test_v = elastic_powder_plot_model._check_workspace_exists("123")
        self.assertFalse(test_v)


if __name__ == "__main__":
    unittest.main()
