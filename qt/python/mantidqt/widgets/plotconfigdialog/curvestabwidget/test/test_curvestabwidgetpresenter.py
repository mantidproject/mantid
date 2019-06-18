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
from mantidqt.widgets.plotconfigdialog.curvestabwidget import CurveProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget.presenter import (
    CurvesTabWidgetPresenter, remove_curve_from_ax, curve_has_errors)


class CurvesTabWidgetPresenterTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.fig = figure()
        cls.ws = CreateWorkspace(DataX=[0, 1], DataY=[3, 4], DataE=[0.1, 0.1],
                                 NSpec=1, UnitX='TOF', OutputWorkspace='ws')
        cls.ax0 = cls.fig.add_subplot(221, projection='mantid')
        cls.ax0.set_title("Axes 0")
        cls.curve0 = cls.ax0.errorbar(cls.ws, specNum=1, fmt=':g', marker=1,
                                      label='Workspace')
        cls.ax0.plot([0, 1], [2, 3], '--r', marker='v', lw=1.1)

        ax1 = cls.fig.add_subplot(222)
        ax1.set_title("Image")
        ax1.imshow([[0, 1], [0, 1]])

        cls.ax2 = cls.fig.add_subplot(223)
        cls.ax2.errorbar([0, 1], [10, 11], yerr=[0.1, 0.2], label='Errorbars')

    @classmethod
    def tearDownClass(cls):
        cls.ws.delete()

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

    def test_update_view_called_on_init(self):
        presenter = self._generate_presenter()
        presenter.view.update_fields.assert_called_once_with(
            CurveProperties.from_curve(self.curve0))

    def test_curve_names_not_equal_in_curve_names_dict_if_curves_have_same_labels(self):
        line = self.fig.get_axes()[0].get_lines()[1]
        # Patch unlabelled line in ax0 to have same label as the errorbar curve
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

    def test_remove_curve_from_mantid_ax_with_workspace_artist(self):
        mantid_ax = self.ax0
        ws_curve = mantid_ax.errorbar(self.ws)
        remove_curve_from_ax(ws_curve)
        self.assertNotIn(ws_curve[0], mantid_ax.lines)
        self.assertNotIn(ws_curve, mantid_ax.containers)
        tracked_artists = []
        for ws_artists_list in mantid_ax.tracked_workspaces.values():
            for ws_artists in ws_artists_list:
                for artist in ws_artists._artists:
                    tracked_artists.append(artist)
        self.assertNotIn(ws_curve, tracked_artists)

    def test_remove_curve_from_mantid_ax_with_non_workspace_artist(self):
        mantid_ax = self.ax0
        curve = mantid_ax.plot([0], [0])[0]
        remove_curve_from_ax(curve)
        self.assertNotIn(curve, mantid_ax.lines)

    def test_remove_curve_from_mpl_axes(self):
        ax = self.ax2
        line = ax.plot([0], [0])[0]
        err_cont = ax.errorbar([0], [0], [1])
        remove_curve_from_ax(line)
        self.assertNotIn(line, ax.lines)
        remove_curve_from_ax(err_cont)
        self.assertNotIn(err_cont[0], ax.lines)
        self.assertNotIn(err_cont, ax.containers)

    def test_remove_selected_curve_removes_curve_from_curves_names_and_combo_box(self):
        self.ax2.plot([0], [0], label='new_line')
        mock_view = Mock(get_selected_ax_name=lambda: "(1, 0)",
                         get_selected_curve_name=lambda: "new_line")
        presenter = self._generate_presenter(mock_view=mock_view)
        with patch.object(presenter, 'update_view', lambda: None):
            presenter.remove_selected_curve()
        self.assertNotIn('new_line', presenter.curve_names_dict)
        presenter.view.remove_select_curve_combo_box_selected_item.assert_called_once_with()

    def test_axes_removed_from_axes_names_dict_when_all_curves_removed(self):
        fig = figure()
        ax0 = fig.add_subplot(211)
        ax0.set_title("First Axes")
        ax1 = fig.add_subplot(212)
        ax1.set_title("Second Axes")
        ax0.plot([0, 1], [0, 1], label='ax0 curve')
        ax1.plot([0], [0], label='ax1 curve')
        mock_view = Mock(get_selected_ax_name=lambda: "First Axes: (0, 0)",
                         get_selected_curve_name=lambda: "ax0 curve")
        presenter = self._generate_presenter(fig=fig, mock_view=mock_view)
        with patch.object(presenter, 'update_view', lambda: None):
            presenter.remove_selected_curve()
        self.assertNotIn('First Axes', presenter.axes_names_dict)
        self.assertNotIn("ax0 curve", presenter.curve_names_dict)

    def test_curve_has_errors_on_workspace_with_no_errors(self):
        try:
            ws = CreateWorkspace(DataX=[0], DataY=[0], NSpec=1,
                                 OutputWorkspace='test_ws')
            fig = figure()
            ax = fig.add_subplot(111, projection='mantid')
            curve = ax.plot(ws, specNum=1)[0]
            self.assertFalse(curve_has_errors(curve))
        finally:
            ws.delete()

    def test_curve_has_errors_on_workspace_with_errors(self):
        fig = figure()
        ax = fig.add_subplot(111, projection='mantid')
        curve = ax.plot(self.ws, specNum=1)[0]
        self.assertTrue(curve_has_errors(curve))

    def test_replot_selected_curve(self):
        fig = figure()
        ax = fig.add_subplot(111, projection='mantid')
        ax.set_title('Axes 0')
        ax.plot(self.ws, specNum=1, label='Workspace')
        presenter = self._generate_presenter(fig=fig)
        new_plot_kwargs = {'errorevery': 2, 'linestyle': '-.', 'color': 'r',
                           'marker': 'v'}
        presenter.replot_selected_curve(new_plot_kwargs)
        new_err_container = presenter.get_selected_curve()
        self.assertEqual(new_err_container[0].get_linestyle(), '-.')
        self.assertEqual(new_err_container[0].get_color(), 'r')
        self.assertEqual(new_err_container[0].get_marker(), 'v')
        # Test only one errorbar is plotted
        self.assertEqual(1, len(new_err_container[2][0].get_segments()))

    def test_curve_has_all_errorbars_on_replot_after_error_every_increase(self):
        fig = figure()
        ax = fig.add_subplot(111)
        curve = ax.errorbar([0, 1, 2, 4], [0, 1, 2, 4], yerr=[0.1, 0.2, 0.3, 0.4])
        new_curve = CurvesTabWidgetPresenter.replot_curve(ax, curve,
                                                          {'errorevery': 2})
        self.assertEqual(2, len(new_curve[2][0].get_segments()))
        new_curve = CurvesTabWidgetPresenter.replot_curve(ax, new_curve,
                                                          {'errorevery': 1})
        self.assertTrue(hasattr(new_curve, 'errorbar_data'))
        self.assertEqual(4, len(new_curve[2][0].get_segments()))


if __name__ == '__main__':
    unittest.main()
