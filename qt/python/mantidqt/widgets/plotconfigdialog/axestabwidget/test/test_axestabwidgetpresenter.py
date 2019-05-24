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
from mantidqt.widgets.plotconfigdialog.axestabwidget import AxProperties
from mantidqt.widgets.plotconfigdialog.axestabwidget.axestabwidgetpresenter import AxesTabWidgetPresenter as Presenter


class AxesTabWidgetPresenterTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.fig = figure()
        ax = cls.fig.add_subplot(211)
        ax.plot([0, 1], [10, 12], 'rx')
        ax.set_title("My Axes \nnewline $\\mu$")
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        ax.set_yscale('Log')
        ax2 = cls.fig.add_subplot(212)
        ax2.plot([-1000000, 10000], [10, 12], 'rx')
        ax2.set_title("Second Axes")

    def _generate_presenter(self):
        mock_view = mock.MagicMock()
        mock_view.get_selected_ax_name.return_value = "My Axes: (0, 0)"
        return Presenter(self.fig, view=mock_view)
        
    def test_generate_ax_name_returns_correct_name(self):
        ax0_name = Presenter.generate_ax_name(self.fig.get_axes()[0])
        self.assertEqual("My Axes: (0, 0)", ax0_name)
        ax1_name = Presenter.generate_ax_name(self.fig.get_axes()[1])
        self.assertEqual("Second Axes: (1, 0)", ax1_name)

    def test_apply_properties_calls_setters_with_correct_properties(self):
        ax_mock = mock.MagicMock()
        presenter = self._generate_presenter()
        with mock.patch.object(presenter, 'get_current_ax', lambda: ax_mock):
            presenter.apply_properties()
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
        presenter = self._generate_presenter()
        actual_dict = presenter.get_axes_names_dict()
        expected = {"My Axes: (0, 0)": self.fig.get_axes()[0],
                    "Second Axes: (1, 0)": self.fig.get_axes()[1]}
        self.assertEqual(expected, actual_dict)

    def test_get_current_ax(self):
        presenter = self._generate_presenter()
        self.assertEqual(self.fig.get_axes()[0], presenter.get_current_ax())

    def test_get_current_ax_name(self):
        presenter = self._generate_presenter()
        self.assertEqual("My Axes: (0, 0)", presenter.get_current_ax_name())

    def test_get_current_ax_properties(self):
        presenter = self._generate_presenter()
        actual_props = presenter.get_current_ax_properties()
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
            ["My Axes: (0, 0)", "Second Axes: (1, 0)"]
        )

    def test_rename_current_axes_calls_set_current_axes_selector_text(self):
        presenter = self._generate_presenter()
        new_name = "New Axes Name: (0, 0)"
        presenter.rename_current_axes(new_name)
        presenter.view.set_current_axes_selector_text.assert_called_once_with(
            new_name
        )

    def test_rename_current_axes_updates_axes_names_dict(self):
        presenter = self._generate_presenter()
        new_name = "New Axes Name: (0, 0)"
        presenter.rename_current_axes(new_name)
        self.assertEqual(presenter.axes_names_dict,
                         {new_name: self.fig.get_axes()[0],
                          "Second Axes: (1, 0)": self.fig.get_axes()[1]})

    def test_set_ax_title_sets_title_and_updates_axes_names_dict(self):
        presenter = self._generate_presenter()
        new_title = "New Title"
        presenter.set_ax_title(self.fig.get_axes()[0], new_title)
        self.assertIn((new_title + ": (0, 0)", self.fig.get_axes()[0]),
                      presenter.get_axes_names_dict().items())
        self.assertEqual(new_title, self.fig.get_axes()[0].title.get_text())
