# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import FileFinder
from unittest import mock
from unittest.mock import call

import mantidqtinterfaces.Muon.GUI.Common.utilities.load_utils as load_utils
from mantidqtinterfaces.Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_model import HomeRunInfoWidgetModel
from mantidqtinterfaces.Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_presenter import HomeRunInfoWidgetPresenter
from mantidqtinterfaces.Muon.GUI.Common.home_runinfo_widget.home_runinfo_widget_view import HomeRunInfoWidgetView
from mantidqtinterfaces.Muon.GUI.Common.muon_pair import MuonPair

from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


class HomeTabRunInfoPresenterTest(unittest.TestCase):
    def setUp(self):
        setup_context_for_tests(self)
        self.data_context.instrument = 'MUSR'

        self.view = mock.Mock(spec=HomeRunInfoWidgetView)
        self.view.clear = mock.Mock()
        self.view.warning_popup = mock.Mock()
        self.view.add_text_line = mock.Mock()

        self.model = HomeRunInfoWidgetModel(self.context)
        self.presenter = HomeRunInfoWidgetPresenter(self.view, self.model)

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

        expected = [call("Instrument                : MUSR"),
                    call("Run                       : 22725"),
                    call("Title                     : FeTeSe T=1 F=100"),
                    call("Comment                   : FC first sample"),
                    call("Start                     : 2009-03-24T04:18:58"),
                    call("End                       : 2009-03-24T04:56:26"),
                    call("Counts (MEv)              : 20.076704"),
                    call("Good Frames               : 88540"),
                    call("Counts per Good Frame     : 226.753"),
                    call("Counts per Good Frame per det : 3.543"),
                    call("Average Temperature (K)   : 19.69992"),
                    call("Sample Temperature (K)    : 1.0"),
                    call("Sample Magnetic Field (G) : 100.0"),
                    call("Number of DAQ Periods     : 1")]

        self.assertEqual(self.view.add_text_line.call_args_list, expected)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
