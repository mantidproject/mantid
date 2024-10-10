# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication

from mantidqtinterfaces.Muon.GUI.Common.load_widget.load_presenter import LoadPresenter
from mantidqtinterfaces.Muon.GUI.Common.load_widget.load_view import LoadView
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget.load_model import LoadModel, CoLoadModel
from collections import OrderedDict


@start_qapplication
class LoadPresenterTest(unittest.TestCase):
    def setUp(self):
        self._view = mock.create_autospec(LoadView)
        self._load_model = mock.create_autospec(LoadModel)
        self._co_model = mock.create_autospec(CoLoadModel)
        self.presenter = LoadPresenter(self._view, self._load_model, self._co_model)
        self.view = self.presenter.view

    def test_equalise_last_loaded_run_empty(self):
        runs = OrderedDict()
        self.presenter.co_model.loaded_runs = runs
        self.presenter.load_model.loaded_runs = runs
        self.presenter.equalise_last_loaded_run(runs)
        self.assertEqual(self.presenter._current_run, None)

    def test_equalise_last_loaded_run_data(self):
        runs = OrderedDict({1: [], 2: [], 5: [], 3: []})
        self.presenter.co_model.loaded_runs = runs
        self.presenter.load_model.loaded_runs = runs
        self.presenter.equalise_last_loaded_run(runs)
        self.assertEqual(self.presenter._current_run, "3")

    def test_update_models(self):
        self.presenter.load_model = LoadModel()
        self.presenter.co_model = CoLoadModel()
        test_value = 10
        self.presenter.update_models(test_value)
        self.assertEqual(self.presenter.load_model.run, test_value)
        self.assertEqual(self.presenter.co_model.run, test_value)

    def test_enable_buttons(self):
        self.presenter.enable_buttons()
        self.assertEqual(self.view.enable_buttons.call_count, 1)

    def test_disable_buttons(self):
        self.presenter.disable_buttons()
        self.assertEqual(self.view.disable_buttons.call_count, 1)

    def test_set_coadd_loaded_run(self):
        self.presenter.equalise_last_loaded_run = mock.Mock()
        runs = OrderedDict()
        self.presenter.co_model.loaded_runs = runs
        self.presenter.load_model.loaded_runs = OrderedDict()
        self.presenter.set_coadd_loaded_run()
        self.presenter.equalise_last_loaded_run.assert_called_with(runs.keys())

    def test_set_loaded_run(self):
        self.presenter.equalise_last_loaded_run = mock.Mock()
        runs = OrderedDict()
        self.presenter.co_model.loaded_runs = OrderedDict()
        self.presenter.load_model.loaded_runs = runs
        self.presenter.set_loaded_run()
        self.presenter.equalise_last_loaded_run.assert_called_with(runs.keys())

    def test_end_load_thread(self):
        self.presenter.set_loaded_run = mock.Mock()
        self.presenter.enable_buttons = mock.Mock()
        self.presenter.load_thread = mock.Mock()
        self.presenter.load_thread.deleteLater = mock.Mock()

        self.presenter.end_load_thread()
        self.assertEqual(self.presenter.set_loaded_run.call_count, 1)
        self.assertEqual(self.presenter.enable_buttons.call_count, 1)
        self.assertEqual(self.presenter.load_thread, None)

    def test_end_co_thread(self):
        self.presenter.set_coadd_loaded_run = mock.Mock()
        self.presenter.enable_buttons = mock.Mock()
        self.presenter.co_thread = mock.Mock()
        self.presenter.co_thread.deleteLater = mock.Mock()

        self.presenter.end_co_thread()
        self.assertEqual(self.presenter.set_coadd_loaded_run.call_count, 1)
        self.assertEqual(self.presenter.enable_buttons.call_count, 1)
        self.assertEqual(self.presenter.co_thread, None)

    def test_last_loaded_run(self):
        self.presenter._current_run = 5
        self.assertEqual(self.presenter.last_loaded_run(), 5)


if __name__ == "__main__":
    unittest.main()
