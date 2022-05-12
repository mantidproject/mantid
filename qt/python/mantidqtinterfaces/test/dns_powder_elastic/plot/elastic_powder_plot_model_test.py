# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import patch

import numpy as np

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import \
    DNSObsModel
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model \
    import DNSElasticPowderPlotModel
from mantidqtinterfaces.dns_powder_elastic.plot import \
    elastic_powder_plot_model


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
        cls.testarray = np.asarray([1, 3])

    def test___init__(self):
        self.assertIsInstance(self.model, DNSElasticPowderPlotModel)
        self.assertIsInstance(self.model, DNSObsModel)
        self.assertTrue(hasattr(self.model, '_plotted_script_number'))

    @patch('mantidqtinterfaces.dns_powder_elastic.plot.'
           'elastic_powder_plot_model.mtd')
    def test__add_simulation_to_ws_list(self, mtd):
        wslist = []
        mtd.__getitem__.return_value = 1
        elastic_powder_plot_model._add_simulation_to_ws_list(wslist)
        testv = elastic_powder_plot_model._add_simulation_to_ws_list(wslist)
        self.assertEqual(testv, ['mat_simulation'])
        wslist = []
        mtd.__getitem__.side_effect = KeyError
        testv = elastic_powder_plot_model._add_simulation_to_ws_list(wslist)
        self.assertEqual(testv, [])

    @patch('mantidqtinterfaces.dns_powder_elastic.plot.'
           'elastic_powder_plot_model.mtd')
    def test_get_max_int_of_workspaces(self, mtd):
        mtd.__getitem__.return_value = self.fake_workspace
        testv = self.model.get_max_int_of_workspaces(['123'])
        self.assertEqual(testv, 4)
        testv = self.model.get_max_int_of_workspaces(['simulation'])
        self.assertEqual(testv, 1)

    @patch('mantidqtinterfaces.dns_powder_elastic.plot.'
           'elastic_powder_plot_model.mtd')
    def test__get_x(self, mtd):
        mtd.__getitem__.return_value = self.fake_workspace
        testv = elastic_powder_plot_model._get_x('123')
        self.assertEqual(testv, np.asarray(1))

    @patch('mantidqtinterfaces.dns_powder_elastic.plot.'
           'elastic_powder_plot_model.mtd')
    def test__get_y(self, mtd):
        mtd.__getitem__.return_value = self.fake_workspace
        testv = elastic_powder_plot_model._get_y('123')
        self.assertEqual(testv[0], 4)
        self.assertEqual(len(testv), 2)

    @patch('mantidqtinterfaces.dns_powder_elastic.plot.'
           'elastic_powder_plot_model.mtd')
    def test__get_yerr(self, mtd):
        mtd.__getitem__.return_value = self.fake_workspace
        testv = elastic_powder_plot_model._get_yerr('123')
        self.assertEqual(testv[0], 3)
        self.assertEqual(len(testv), 2)

    def test__convert_x_axis(self):
        testv = elastic_powder_plot_model._convert_x_axis(self.testarray, 'tt',
                                                          4.74)
        self.assertEqual(testv[0], 1)
        self.assertEqual(testv[1], 3)
        testv = elastic_powder_plot_model._convert_x_axis(self.testarray, 'd',
                                                          4.74)
        self.assertAlmostEqual(testv[0], 271.58544194790886)
        self.assertAlmostEqual(testv[1], 90.53767353344176)
        testv = elastic_powder_plot_model._convert_x_axis(self.testarray, 'q',
                                                          4.74)
        self.assertAlmostEqual(testv[0], 0.023135206593233835)
        self.assertAlmostEqual(testv[1], 0.06939857257165744)

    def test__scale_simulation(self):
        testv = elastic_powder_plot_model._scale_simulation('a',
                                                            self.testarray,
                                                            self.testarray, 5)
        self.assertEqual(testv[0][0], 1)
        testv = elastic_powder_plot_model._scale_simulation('simulation',
                                                            self.testarray,
                                                            self.testarray, 5)
        self.assertEqual(testv[0][0], 0.5)
        self.assertEqual(testv[1][0], 1 / 3 * 5)

    @patch('mantidqtinterfaces.dns_powder_elastic.plot.'
           'elastic_powder_plot_model.mtd')
    def test_get_x_y_yerr(self, mtd):
        mtd.__getitem__.return_value = self.fake_workspace
        testv = self.model.get_x_y_yerr('a', 'tt', 5, 4.74)
        self.assertEqual(testv[0][0], 1)
        self.assertEqual(testv[1][0], 4)
        self.assertEqual(testv[2][0], 3)

    def test__datalist_updated(self):
        testv = self.model._datalist_updated(['mat_a', 'mat_b'], ['a', 'b'], 0)
        self.assertFalse(testv)
        testv = self.model._datalist_updated(['mat_a', 'mat_b'], ['a'], 0)
        self.assertTrue(testv)
        testv = self.model._datalist_updated(['mat_a', 'mat_b'], ['a', 'b'], 1)
        self.assertTrue(testv)

    @patch('mantidqtinterfaces.dns_powder_elastic.plot.'
           'elastic_powder_plot_model.mtd')
    def test_get_updated_ws_list(self, mtd):
        self.model._plotted_script_number = 0
        mtd.__getitem__.return_value = self.fake_workspace
        testv = self.model.get_updated_ws_list(['mat_b', 'mat_a'], ['a'], 1)
        self.assertEqual(testv[0], ['mat_a', 'mat_b', 'mat_simulation'])
        self.assertTrue(testv[1])
        self.assertEqual(self.model._plotted_script_number, 1)

    def test_get_y_norm_label(self):
        testv = self.model.get_y_norm_label(True)
        self.assertEqual(testv, 'normed to monitor')
        testv = self.model.get_y_norm_label(False)
        self.assertEqual(testv, 'Counts/s')

    def test_get_x_axis_label(self):
        testv = self.model.get_x_axis_label('q')
        self.assertEqual(testv, r'$q (\AA^{-1})$')
        testv = self.model.get_x_axis_label('d')
        self.assertEqual(testv, r'$d (\AA)$')
        testv = self.model.get_x_axis_label('tt')
        self.assertEqual(testv, '2 theta (degree)')

    def test__set_script_number(self):
        self.model._set_script_number(5)
        self.assertEqual(self.model._plotted_script_number, 5)
        self.model._plotted_script_number = 0

    @patch('mantidqtinterfaces.dns_powder_elastic.plot.'
           'elastic_powder_plot_model.mtd')
    def test__check_workspace_exists(self, mtd):
        mtd.__getitem__.regturn_value = '123'
        testv = elastic_powder_plot_model._check_workspace_exists('123')
        self.assertTrue(testv)
        mtd.__getitem__.side_effect = KeyError
        testv = elastic_powder_plot_model._check_workspace_exists('123')
        self.assertFalse(testv)


if __name__ == '__main__':
    unittest.main()
