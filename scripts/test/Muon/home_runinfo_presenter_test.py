# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys

from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_view import HomeRunInfoWidgetView
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_presenter import HomeRunInfoWidgetPresenter
from Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_model import HomeRunInfoWidgetModel
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common import mock_widget
from mantid.api import FileFinder
import Muon.GUI.Common.utilities.load_utils as load_utils
from Muon.GUI.Common.muon_pair import MuonPair
import unittest
from PyQt4 import QtGui

if sys.version_info.major < 2:
    from unittest import mock
else:
    import mock


class HomeTabRunInfoPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.obj = QtGui.QWidget()
        self.context = MuonDataContext()
        self.view = HomeRunInfoWidgetView(self.obj)
        self.model = HomeRunInfoWidgetModel(self.context)
        self.presenter = HomeRunInfoWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.MagicMock()

    def tearDown(self):
        self.obj = None

    def test_runinfo_correct(self):
        file_path = FileFinder.findRuns('22725')[0]
        ws, run, filename = load_utils.load_workspace_from_filename(file_path)
        self.context._loaded_data.remove_data(run=run)
        self.context._loaded_data.add_data(run=run, workspace=ws, filename=filename)
        self.context.update_current_data()
        test_pair = MuonPair('test_pair', 'top', 'bottom', alpha=0.75)
        self.context.add_pair(pair=test_pair)

        self.presenter.update_view_from_model()

        self.assertEqual(self.view.run_info_box.toPlainText(),
                         'Instrument                : MUSR\n'
                         'Run                       : 22725\n'
                         'Title                     : FeTeSe T=1 F=100\n'
                         'Comment                   : FC first sample\n'
                         'Start                     : 2009-03-24T04:18:58\n'
                         'End                       : 2009-03-24T04:56:26\n'
                         'Good Frames               : 88540\n'
                         'Counts (MeV)              : 20.076704\n'
                         'Average Temperature (K)   : 2.5338574658342083\n'
                         'Sample Temperature (K)    : 1.0\n'
                         'Sample Magnetic Field (G) : 100.0')


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
