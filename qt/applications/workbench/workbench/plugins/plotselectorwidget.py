#    This file is part of the mantid workbench.
#
#    Copyright (C) 2018 mantidproject
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, unicode_literals)

# system imports

# third-party library imports
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget
from workbench.plotting.currentfigure import CurrentFigure
from mantidqt.widgets.plotselector.presenter import PlotSelectorPresenter


class PlotSelector(PluginWidget):
    """Provides an algorithm selector widget for selecting algorithms to run"""

    def __init__(self, parent):
        super(PlotSelector, self).__init__(parent)

        plot_selector_presenter = PlotSelectorPresenter(CurrentFigure)

        # layout
        self.plot_selector_widget = plot_selector_presenter.widget
        layout = QVBoxLayout()
        layout.addWidget(self.plot_selector_widget)
        self.setLayout(layout)

# ----------------- Plugin API --------------------

    def register_plugin(self):
        self.main.add_dockwidget(self)

    def get_plugin_title(self):
        return "Plots"

    def read_user_settings(self, _):
        pass
