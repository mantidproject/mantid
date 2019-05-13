# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from qtpy import QtWidgets

from mantid.api import FileFinder
from mantid.py3compat import mock

import Muon.GUI.Common.utilities.load_utils as load_utils
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_model import HomeRunInfoWidgetModel
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_presenter import HomeRunInfoWidgetPresenter
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_view import HomeRunInfoWidgetView
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.test_helpers import mock_widget
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


class HomeTabRunInfoPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.obj = QtWidgets.QWidget()
        setup_context_for_tests(self)
        self.data_context.instrument = 'MUSR'
        self.view = HomeRunInfoWidgetView(self.obj)
        self.model = HomeRunInfoWidgetModel(self.context)
        self.presenter = HomeRunInfoWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.MagicMock()

    def tearDown(self):
        self.obj = None

    def test_runinfo_correct(self):
        file_path = FileFinder.findRuns('MUSR00022725.nxs')[0]
        ws, run, filename, _ = load_utils.load_workspace_from_filename(file_path)
        self.data_context._loaded_data.remove_data(run=run)
        self.data_context._loaded_data.add_data(run=[run], workspace=ws, filename=filename, instrument='MUSR')
        self.data_context.current_runs = [[22725]]
        self.context.update_current_data()
        test_pair = MuonPair('test_pair', 'top', 'bottom', alpha=0.75)
        self.group_context.add_pair(pair=test_pair)

        self.presenter.update_view_from_model()

        expected_string_list = ['Instrument:MUSR', 'Run:22725', 'Title:FeTeSeT=1F=100', 'Comment:FCfirstsample',
                                'Start:2009-03-24T04:18:58', 'End:2009-03-24T04:56:26', 'Counts(MEv):20.076704',
                                'GoodFrames:88540', 'CountsperGoodFrame:226.753',
                                'CountsperGoodFrameperdet:3.543', 'AverageTemperature(K):2.53386',
                                'SampleTemperature(K):1.0', 'SampleMagneticField(G):100.0']

        self.assertEqual(str(self.view.run_info_box.toPlainText()).replace(' ', '').splitlines(), expected_string_list)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
