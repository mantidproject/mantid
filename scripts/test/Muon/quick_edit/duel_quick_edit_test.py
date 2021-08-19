# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock

from Muon.GUI.Common.plot_widget.quick_edit.quick_edit_view import QuickEditView
from Muon.GUI.Common.plot_widget.quick_edit.quick_edit_presenter import DuelQuickEditPresenter
from Muon.GUI.Common.contexts.plotting_context import PlottingContext

from mantid.simpleapi import AnalysisDataService
from mantidqt.utils.qt.testing import start_qapplication


def plot_at_index(index):
    return "plot "+str(index)


@start_qapplication
class DuelQuickEditTest(unittest.TestCase):

    def setUp(self):
        self.view = mock.Mock(spec=QuickEditView)
        self.context = PlottingContext()
        self.presenter = DuelQuickEditPresenter(self.view, self.context)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    # this is the only override
    def test_multiple_plots(self):
        self.view.number_of_plots = mock.Mock(return_value=4)
        self.view.plot_at_index = mock.Mock(side_effect = plot_at_index)
        # plot names at 0 and 1 are reserved so are excluded
        expected = ["plot 2", "plot 3"]
        self.assertEqual(self.presenter.multiple_plots(), expected)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
