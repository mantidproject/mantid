# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import Muon.GUI.Common.utilities.load_utils as utils
import unittest

from mantid import simpleapi
from mantid.kernel import ConfigService
from mantid.api import AnalysisDataService, ITableWorkspace
from unittest import mock


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
        ConfigService.Instance().setString("default.facility", "ISIS")
        filename = 'MUSR00022725.nsx'

        name = utils.load_dead_time_from_filename(filename)
        dead_time_table = AnalysisDataService.retrieve('MUSR00022725.nsx_deadtime_table')

        self.assertEqual(name, 'MUSR00022725.nsx_deadtime_table')
        self.assertTrue(isinstance(dead_time_table, ITableWorkspace))
        ConfigService.Instance().setString("default.facility", " ")

    def test_load_workspace_from_filename_for_existing_file(self):
        ConfigService.Instance().setString("default.facility", "ISIS")
        filename = 'MUSR00022725.nsx'
        load_result, run, filename, _ = utils.load_workspace_from_filename(filename)

        self.assertEqual(load_result['DeadTimeTable'], None)
        self.assertEqual(load_result['FirstGoodData'], 0.106)
        self.assertEqual(load_result['MainFieldDirection'], 'Transverse')
        self.assertAlmostEqual(load_result['TimeZero'], 0.55000, 5)
        self.assertEqual(run, 22725)
        ConfigService.Instance().setString("default.facility", " ")

    def test_create_load_alg_for_nxs_files(self):
        filename = "EMU00019489.nxs"
        inputs = {}

        alg, psi_data = utils.create_load_algorithm(filename, inputs)
        self.assertFalse(psi_data)

    def test_create_load_alg_for_nxs_v2_files(self):
        filename = "EMU00102347.nxs_v2"
        inputs = {}

        alg, psi_data = utils.create_load_algorithm(filename, inputs)
        self.assertFalse(psi_data)

    def test_create_load_alg_for_bin_files(self):
        filename = "deltat_tdc_dolly_1529.bin"
        inputs = {}

        alg, psi_data = utils.create_load_algorithm(filename, inputs)
        self.assertTrue(psi_data)

    @mock.patch('Muon.GUI.Common.utilities.load_utils.CloneWorkspace')
    def test_combine_loaded_runs_for_psi_data(self, clone_mock):
        workspace = mock.MagicMock()
        workspace.workspace.name = "name"
        model = mock.MagicMock()
        model._data_context.num_periods = mock.Mock(return_value=1)
        # Workspace is missing DeadTimeTable and DetectorGroupingTable which should be handled without raising
        model._loaded_data_store.get_data = mock.Mock(return_value={"workspace": {"OutputWorkspace": workspace,
                                                                                  "MainFieldDirection": 0,
                                                                                  "TimeZero": 0, "FirstGoodData": 0,
                                                                                  "DataDeadTimeTable": "dtt"}})
        run_list = [1529]
        utils.combine_loaded_runs(model, run_list)


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
