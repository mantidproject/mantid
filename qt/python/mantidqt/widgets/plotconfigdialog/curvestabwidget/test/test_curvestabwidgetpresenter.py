# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

from matplotlib import use as mpl_use
mpl_use('Agg')  # noqa
from matplotlib.pyplot import figure

from mantid.simpleapi import CreateWorkspace
from mantid.plots import MantidAxes  # register MantidAxes projection  # noqa
from mantid.py3compat.mock import Mock, patch
from mantidqt.widgets.plotconfigdialog.curvestabwidget.presenter import CurvesTabWidgetPresenter

curves_tab_widget = "mantidqt.widgets.plotconfigdialog.curvestabwidget"
LINE_TAB = curves_tab_widget + ".linetabwidget.presenter.LineTabWidgetView"
MARKER_TAB = curves_tab_widget + ".markertabwidget.presenter.MarkerTabWidgetView"
ERRORBARS_TAB = curves_tab_widget + ".errorbarstabwidget.presenter.ErrorbarsTabWidgetView"


class CurvesTabWidgetPresenterTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.fig = figure()
        ax0 = cls.fig.add_subplot(221, projection='mantid')
        ax1 = cls.fig.add_subplot(222)
        ax2 = cls.fig.add_subplot(223)

        cls.ws = CreateWorkspace(DataX=[0, 1], DataY=[3, 4], DataE=[0.1, 0.1],
                                 NSpec=1, UnitX='TOF', OutputWorkspace='ws')
        ax0.set_title("Axes 0")
        ax0.errorbar(cls.ws, specNum=1, fmt=':g', marker=1, label='Workspace')
        ax0.plot([0, 1], [2, 3], '--r', marker='v', lw=1.1)

        ax1.set_title("Image")
        ax1.imshow([[0, 1], [0, 1]])

        ax2.errorbar([0, 1], [10, 11], yerr=[0.1, 0.2], label='Errorbars')

        # Mock out sub-tab views
        cls.line_tab_mock = patch(LINE_TAB)
        LineTabWidgetView = cls.line_tab_mock.start()  # noqa
        cls.marker_tab_mock = patch(MARKER_TAB)
        MarkerTabWidgetView = cls.marker_tab_mock.start()  # noqa
        cls.errorbars_tab_mock = patch(ERRORBARS_TAB)
        ErrorbarsTabWidgetView = cls.errorbars_tab_mock.start()  # noqa

    @classmethod
    def tearDownClass(cls):
        cls.ws.delete()
        cls.line_tab_mock.stop()
        cls.marker_tab_mock.stop()
        cls.errorbars_tab_mock.stop()

    def _generate_presenter(self, mock_view=None, fig=None):
        if not mock_view:
            mock_view = Mock(get_selected_ax_name=lambda: "Axes 0: (0, 0)",
                             get_selected_curve_name=lambda: "Workspace")
        if not fig:
            fig = self.fig
        return CurvesTabWidgetPresenter(fig=fig, view=mock_view)

    def test_axes_names_dict_gets_axes_on_init_(self):
        presenter = self._generate_presenter()
        self.assertIn("Axes 0: (0, 0)", presenter.axes_names_dict)
        self.assertNotIn("Image: (1, 0)", presenter.axes_names_dict)

    def test_populate_select_axes_combobox_called_on_init(self):
        presenter = self._generate_presenter()
        presenter.view.populate_select_axes_combo_box.assert_called_once_with(
            ["Axes 0: (0, 0)", "(1, 0)"])

    def test_populate_select_curve_combo_box_called_on_init(self):
        presenter = self._generate_presenter()
        presenter.view.populate_select_curve_combo_box.assert_called_once_with(
            ["_line1", "Workspace"])

    def test_curve_label_and_hide_curve_view_fields_updated_on_init(self):
        presenter = self._generate_presenter()
        presenter.view.set_curve_label.assert_called_once_with('Workspace')
        presenter.view.set_hide_curve.assert_called_once_with(False)

    def test_curve_names_not_equal_in_curve_names_dict_if_curves_have_same_labels(self):
        line = self.fig.get_axes()[0].get_lines()[1]
        # Patch unlabelled line in ax0 to have same label as the errobar curve
        # in the same axes
        with patch.object(line, 'get_label', lambda: 'Workspace'):
            presenter = self._generate_presenter()
        self.assertSequenceEqual(['Workspace', 'Workspace (1)'],
                                 sorted(presenter.curve_names_dict.keys()))

    def test_curves_with__no_legend__label_not_in_curves_name_dict(self):
        line = self.fig.get_axes()[0].get_lines()[1]
        with patch.object(line, 'get_label', lambda: '_nolegend_'):
            presenter = self._generate_presenter()
        self.assertNotIn(line, presenter.curve_names_dict.values())

    def test_updating_curve_label_updates_curve_names_dict(self):
        presenter = self._generate_presenter()
        line = self.fig.get_axes()[0].get_lines()[1]
        err_bars = self.fig.get_axes()[0].containers[0]
        presenter.set_curve_label(err_bars, 'new_label')
        self.assertEqual({'new_label': err_bars, '_line1': line},
                         presenter.curve_names_dict)


if __name__ == '__main__':
    unittest.main()
