# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from qtpy import QtWidgets

from mantid.api import FileFinder
from mantid.py3compat import mock

from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.test_helpers import mock_widget
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests
from Muon.GUI.Common.utilities import load_utils
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_model
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_presenter_new
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_view


def retrieve_combobox_info(combo_box):
    output_list = []
    for i in range(combo_box.count()):
        output_list.append(str(combo_box.itemText(i)))

    return output_list


class MaxEntPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtWidgets.QWidget()
        setup_context_for_tests(self)

        self.data_context.instrument = 'MUSR'

        self.gui_context.update({'RebinType': 'None'})
        self.model = maxent_model.MaxEntModel()

        self.view = maxent_view.MaxEntView(self.obj)

        self.presenter = maxent_presenter_new.MaxEntPresenter(self.view, self.model, self.context)

        file_path = FileFinder.findRuns('MUSR00022725.nxs')[0]
        ws, run, filename = load_utils.load_workspace_from_filename(file_path)
        self.data_context._loaded_data.remove_data(run=run)
        self.data_context._loaded_data.add_data(run=[run], workspace=ws, filename=filename, instrument='MUSR')
        self.data_context.current_runs = [[22725]]

        self.context.update_current_data()
        test_pair = MuonPair('test_pair', 'top', 'bottom', alpha=0.75)
        self.group_context.add_pair(pair=test_pair)

        self.view.warning_popup = mock.MagicMock()

    def test_get_workspace_names_sets_comboboxes_appropriately(self):
        self.presenter.getWorkspaceNames()

        self.assertEquals(retrieve_combobox_info(self.view.ws), ['MUSR22725_raw_data'])
        self.assertEquals(retrieve_combobox_info(self.view.N_points), ['2048', '4096', '8192', '16384', '32768', '65536',
                                                                       '131072', '262144', '524288', '1048576'])

    def test_get_phase_table_inputs_returns_correctly(self):
        self.presenter.getWorkspaceNames()

        self.assertEquals(self.presenter.get_phase_table_inputs(), {'DataFitted': 'fits', 'DetectorTable': 'PhaseTable',
                                                                    'FirstGoodData': 0.1, 'InputWorkspace': 'MUSR22725_raw_data',
                                                                    'LastGoodData': 15.0})

    def test_get_input_run_returns_correctly(self):
        self.presenter.getWorkspaceNames()

        self.assertEquals(self.presenter.get_input_run(), 'MUSR22725')

    def test_get_max_ent_inputs_return_correctly(self):
        self.presenter.getWorkspaceNames()

        self.assertEquals(self.presenter.getMaxEntInput(), {'DefaultLevel': 0.1, 'DoublePulse': False, 'Factor': 1.04,
                                                            'FirstGoodTime': 0.1, 'FitDeadTime': True, 'InnerIterations': 10,
                                                            'InputWorkspace': 'MUSR22725_raw_data', 'LastGoodTime': 15.0,
                                                            'MaxField': 1000.0, 'Npts': 2048, 'OuterIterations': 10,
                                                            'OutputWorkspace': 'MUSR22725_raw_data;FrequencyDomain;MaxEnt'})




if __name__ == '__main__':
    unittest.main()