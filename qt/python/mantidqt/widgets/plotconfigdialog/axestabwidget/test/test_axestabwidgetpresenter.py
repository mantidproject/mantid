# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import unittest

from matplotlib import use as mpl_use
mpl_use('Agg')  # noqa
from matplotlib.pyplot import figure

from mantid.py3compat import mock
from mantidqt.widgets.plotconfigdialog import generate_ax_name, get_axes_names_dict
from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties
from mantidqt.widgets.plotconfigdialog.axestabwidget.presenter import AxesTabWidgetPresenter as Presenter


class AxesTabWidgetPresenterTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.fig = figure()
        ax = cls.fig.add_subplot(211)
        ax.plot([0, 1], [10, 12], 'rx')
        cls.title = "My Axes \nnewline $\\mu$"
        ax.set_title(cls.title)
        cls.x_label = 'X'
        ax.set_xlabel(cls.x_label)
        cls.x_scale = 'Linear'
        cls.y_label = 'Y'
        ax.set_ylabel(cls.y_label)
        cls.y_scale = 'Log'
        ax.set_yscale(cls.y_scale)
        ax2 = cls.fig.add_subplot(212)
        ax2.plot([-1000000, 10000], [10, 12], 'rx')

    def _generate_presenter(self):
        mock_view = mock.Mock(get_selected_ax_name=lambda: "My Axes: (0, 0)")
        return Presenter(self.fig, view=mock_view)
        
    def test_generate_ax_name_returns_correct_name(self):
        ax0_name = generate_ax_name(self.fig.get_axes()[0])
        self.assertEqual("My Axes: (0, 0)", ax0_name)
        ax1_name = generate_ax_name(self.fig.get_axes()[1])
        self.assertEqual("(1, 0)", ax1_name)

    def test_apply_properties_calls_setters_with_correct_properties(self):
        ax_mock = mock.MagicMock()
        presenter = self._generate_presenter()
        with mock.patch.object(presenter, 'get_selected_ax', lambda: ax_mock):
            presenter.apply_properties()
            # Mock properties object and view then test that the view's setters
            # are called with the correct property values
            view_mock = presenter.view
            get_props_mock = view_mock.get_properties
            view_mock.get_properties.assert_called_once_with()
            ax_mock.set_title.assert_called_once_with(
                get_props_mock().title)
            ax_mock.set_xlim.assert_called_once_with(
                get_props_mock().xlim)
            ax_mock.set_xlabel.assert_called_once_with(
                get_props_mock().xlabel)
            ax_mock.set_xscale.assert_called_once_with(
                get_props_mock().xscale)
            ax_mock.set_ylim.assert_called_once_with(
                get_props_mock().ylim)
            ax_mock.set_ylabel.assert_called_once_with(
                get_props_mock().ylabel)
            ax_mock.set_yscale.assert_called_once_with(
                get_props_mock().yscale)

    def test_get_axes_names_dict(self):
        actual_dict = get_axes_names_dict(self.fig)
        expected = {"My Axes: (0, 0)": self.fig.get_axes()[0],
                    "(1, 0)": self.fig.get_axes()[1]}
        self.assertEqual(expected, actual_dict)

    def test_get_selected_ax(self):
        presenter = self._generate_presenter()
        self.assertEqual(self.fig.get_axes()[0], presenter.get_selected_ax())

    def test_get_selected_ax_name(self):
        presenter = self._generate_presenter()
        self.assertEqual("My Axes: (0, 0)", presenter.get_selected_ax_name())

    def test_get_selected_ax_properties(self):
        presenter = self._generate_presenter()
        actual_props = presenter.get_selected_ax_properties()
        self.assertIsInstance(actual_props, AxProperties)
        self.assertEqual("My Axes \\nnewline $\\\\mu$", actual_props.title)
        self.assertEqual('X', actual_props.xlabel)
        self.assertEqual('Linear', actual_props.xscale)
        self.assertEqual('Y', actual_props.ylabel)
        self.assertEqual('Log', actual_props.yscale)

    def test_populate_select_axes_combo_box_called_once_on_construction(self):
        presenter = self._generate_presenter()
        view_mock = presenter.view
        view_mock.populate_select_axes_combo_box.assert_called_once_with(
            ["My Axes: (0, 0)", "(1, 0)"]
        )

    def test_rename_selected_axes_calls_set_selected_axes_selector_text(self):
        presenter = self._generate_presenter()
        new_name = "New Axes Name: (0, 0)"
        presenter.rename_selected_axes(new_name)
        presenter.view.set_selected_axes_selector_text.assert_called_once_with(
            new_name
        )

    def test_rename_selected_axes_updates_axes_names_dict(self):
        presenter = self._generate_presenter()
        new_name = "New Axes Name: (0, 0)"
        presenter.rename_selected_axes(new_name)
        self.assertEqual(presenter.axes_names_dict,
                         {new_name: self.fig.get_axes()[0],
                          "(1, 0)": self.fig.get_axes()[1]})

    def test_set_ax_title_sets_title_and_updates_axes_names_dict(self):
        presenter = self._generate_presenter()
        new_title = "New Title"
        ax = self.fig.get_axes()[1]
        presenter.set_ax_title(ax, new_title)
        self.assertIn((new_title + ": (1, 0)", ax),
                      get_axes_names_dict(self.fig).items())
        self.assertEqual(new_title, ax.title.get_text())

    def test_update_view_calls_correct_setters_with_correct_values(self):
        presenter = self._generate_presenter()
        new_view_mock = mock.Mock(get_selected_ax_name=lambda: "My Axes: (0, 0)")
        ax = self.fig.get_axes()[0]
        setters = ['set_title', 'set_xlower_limit', 'set_xupper_limit',
                   'set_ylower_limit', 'set_yupper_limit', 'set_xlabel',
                   'set_ylabel', 'set_xscale', 'set_yscale']
        expected_vals = [self.title.encode('unicode_escape'), ax.get_xlim()[0],
                         ax.get_xlim()[1], ax.get_ylim()[0], ax.get_ylim()[1],
                         self.x_label, self.y_label, self.x_scale, self.y_scale]
        with mock.patch.object(presenter, 'view', new_view_mock):
            presenter.update_view()
            for setter, value in zip(setters, expected_vals):
                getattr(new_view_mock, setter).assert_called_once_with(value)
