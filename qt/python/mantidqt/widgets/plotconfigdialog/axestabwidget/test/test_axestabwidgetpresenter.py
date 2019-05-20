# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

import unittest

import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

from mantid.py3compat import mock
from mantidqt.widgets.plotconfigdialog.axestabwidget.axestabwidgetpresenter import AxesTabWidgetPresenter


AXES_TAB_WIDGET_VIEW = ("mantidqt.widgets.plotconfigdialog.axestabwidget." 
                        "axestabwidgetpresenter")


class AxesTabWidgetPresenterTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.mock_view = mock.Mock()
        fig = plt.figure()
        ax = fig.add_subplot(211)
        ax.plot([0, 1], [10, 12], 'rx')
        ax.set_title("My Axes \nnewline $\\mu$")
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        ax.set_yscale('Log')
        ax2 = fig.add_subplot(212)
        ax2.plot([-1000000, 10000], [10, 12], 'rx')
        ax2.set_title("Second Axes")
        cls.presenter = AxesTabWidgetPresenter(fig, view=cls.mock_view)
        
    def test_generate_ax_name_returns_correct_name(self):
        name = self.presenter._generate_ax_name(self.presenter.fig.get_axes()[0])
        self.assertEqual(name, "My Axes: (0, 0)")
