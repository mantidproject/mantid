# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget import MarkerProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget.view import MarkerTabWidgetView


class MarkerTabWidgetPresenter:

    def __init__(self, line=None, view=None, parent=None):
        self.line = line
        if not view:
            self.view = MarkerTabWidgetView(parent)
        else:
            self.view = view

    def apply_properties(self):
        view_props = self.get_view_properties()
        if view_props:
            self.line.set_marker(view_props.style)
            self.line.set_markersize(view_props.size)
            self.line.set_markerfacecolor(view_props.face_color)
            self.line.set_markeredgecolor(view_props.edge_color)

    def get_line_properties(self):
        return MarkerProperties.from_line(self.line)

    def get_view_properties(self):
        return MarkerProperties.from_view(self.view)

    def set_line(self, line):
        self.line = line

    def update_view(self):
        line_props = self.get_line_properties()
        if line_props:
            self.view.set_style(line_props.style)
            self.view.set_size(line_props.size)
            self.view.set_face_color(line_props.face_color)
            self.view.set_edge_color(line_props.edge_color)
