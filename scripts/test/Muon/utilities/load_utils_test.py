# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import Muon.GUI.Common.utilities.load_utils as utils
import os
import unittest

from mantid import simpleapi, ConfigService
from mantid.api import AnalysisDataService, ITableWorkspace


def create_simple_workspace(data_x, data_y, run_number=0):
    alg = simpleapi.AlgorithmManager.create("CreateWorkspace")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setLogging(False)
    alg.setProperty("dataX", data_x)
    alg.setProperty("dataY", data_y)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.execute()
    ws = alg.getProperty("OutputWorkspace").value
    ws.getRun().addProperty('run_number', run_number, 'NonDim', True)
    return ws

class MuonFileUtilsTest(unittest.TestCase):
    def test_get_run_from_multi_period_data(self):
        simple_workspace = create_simple_workspace(data_x=[1,2,3,4], data_y=[10,20,30,40], run_number=74044)
        workspace_list = [simple_workspace] * 5

        run_number = utils.get_run_from_multi_period_data(workspace_list)

        self.assertEqual(run_number, 74044)

    def test_get_run_from_multi_period_data_raises_a_value_error_if_not_all_run_numbers_same(self):
        simple_workspace = create_simple_workspace(data_x=[1, 2, 3, 4], data_y=[10, 20, 30, 40], run_number=74044)
        simple_workspace_1 = create_simple_workspace(data_x=[1, 2, 3, 4], data_y=[10, 20, 30, 40], run_number=74045)
        workspace_list = [simple_workspace] * 4 + [simple_workspace_1]

        self.assertRaises(ValueError, utils.get_run_from_multi_period_data, workspace_list)

    def test_default_instrument_returns_default_instrument_if_muon_instrument(self):
        ConfigService['default.instrument'] = 'MUSR'

        instrument = utils.get_default_instrument()

        self.assertEqual(instrument, 'MUSR')

    def test_default_instrument_returns_MUSR_if_default_instrument_is_not_muon_instrument(self):
        ConfigService['default.instrument'] = 'LOQ'

        instrument = utils.get_default_instrument()

        self.assertEqual(instrument, 'MUSR')

    def test_that_load_dead_time_from_filename_places_table_in_ADS(self):
        filename = 'MUSR00022725.nsx'

        name = utils.load_dead_time_from_filename(filename)
        dead_time_table = AnalysisDataService.retrieve(name)

        self.assertEqual(name, 'MUSR00022725_deadTimes')
        self.assertTrue(AnalysisDataService.doesExist(name))
        self.assertTrue(isinstance(dead_time_table, ITableWorkspace))

    def test_load_workspace_from_filename_for_existing_file(self):
        filename = 'MUSR00022725.nsx'
        load_result, run, filename = utils.load_workspace_from_filename(filename)

        self.assertEqual(load_result['DeadTimeTable'], None)
        self.assertEqual(load_result['FirstGoodData'], 0.11)
        self.assertEqual(load_result['MainFieldDirection'], 'Transverse')
        self.assertAlmostEqual(load_result['TimeZero'], 0.55000, 5)
        self.assertEqual(run, 22725)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)