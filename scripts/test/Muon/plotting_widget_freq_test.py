# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.plotting_widget.plotting_widget_presenter import PlotWidgetPresenter
from Muon.GUI.Common.test_helpers.context_setup import setup_context


@start_qapplication
class PlottingWidgetPresenterFreqTest(unittest.TestCase):
    def setUp(self):
        self.context = setup_context(True)
        self.view = mock.MagicMock()
        self.model = mock.MagicMock()
        self.workspace_list = ['MUSR62260; Group; bottom; Asymmetry; FD',
                               'MUSR62261; Group; bottom; Asymmetry; FD',
                               'FFT; Re MUSR62260; Group; fwd; Asymmetry; FD; Im MUSR62260; Group; fwd; Asymmetry; FD_Im'
                               'FFT; Re MUSR62260; Group; fwd; Asymmetry; FD; Im MUSR62260; Group; fwd; Asymmetry; FD_Re'
                               'FFT; Re MUSR62260; Group; fwd; Asymmetry; FD; Im MUSR62260; Group; fwd; Asymmetry; FD_mod'
                               'MUSR62260_raw_data FD; MaxEnt']

        self.presenter = PlotWidgetPresenter(self.view, self.model, self.context)

        self.view.plot_options.get_errors = mock.MagicMock(return_value=True)
        self.presenter.get_plot_title = mock.MagicMock(return_value='MUSR62260-62261 bottom')
        self.presenter.get_x_lim_from_subplot = mock.MagicMock(return_value=[0, 15])
        self.presenter.get_y_lim_from_subplot = mock.MagicMock(return_value=[-1, 1])

    def test_time_plot_in_FDA(self):
        self.presenter.workspace_finder.get_workspace_list_to_plot = mock.MagicMock(
            return_value=[self.workspace_list[0], self.workspace_list[1]])
        self.presenter.get_plot_title = mock.MagicMock(return_value='MUSR62260-62261 bottom')
        self.presenter.get_workspace_legend_label = mock.MagicMock(return_value='Label')
        self.presenter.get_workspace_plot_axis = mock.MagicMock()
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False,
                       'label': 'Label'}

        self.presenter.handle_use_raw_workspaces_changed()
        self.model.add_workspace_to_plot.assert_any_call(self.presenter.get_workspace_plot_axis(),
                                                         self.workspace_list[0], [0], errors=True,
                                                         plot_kwargs=plot_kwargs)

        self.model.add_workspace_to_plot.assert_called_with(self.presenter.get_workspace_plot_axis(),
                                                            self.workspace_list[1], [0], errors=True,
                                                            plot_kwargs=plot_kwargs)

    def test_plot_type_changed(self):
        self.view.get_selected.return_value = "Asymmetry"
        self.assertEquals(self.view.get_selected(), "Asymmetry")

        self.view.get_selected.return_value = "Frequency Re"
        self.presenter.handle_plot_type_changed()
        self.assertEquals(self.context._frequency_context.plot_type, "Re")

    def test_plot_type_changed_to_time(self):
        self.view.get_selected.return_value = "Frequency Re"

        self.view.get_selected.return_value = "Asymmetry"
        self.presenter.handle_plot_type_changed()
        self.assertEquals(self.context._frequency_context.plot_type, "")

    def test_get_domain_freq(self):
        self.view.get_selected.return_value = "Frequency Re"
        self.assertEquals(self.presenter.get_domain(), "Frequency")

    def test_get_domain_time(self):
        self.view.get_selected.return_value = "Asymmetry"
        self.assertEquals(self.presenter.get_domain(), "Time")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
