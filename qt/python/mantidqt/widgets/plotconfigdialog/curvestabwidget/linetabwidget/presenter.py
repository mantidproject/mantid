# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.axes import ErrorbarContainer
from matplotlib.lines import Line2D

from mantidqt.widgets.plotconfigdialog.curvestabwidget.linetabwidget import LineProperties
from mantidqt.widgets.plotconfigdialog.curvestabwidget.linetabwidget.view import LineTabWidgetView


class LineTabWidgetPresenter:

    def __init__(self, line=None, view=None, parent=None):
        self.line = line
        if not view:
            self.view = LineTabWidgetView(parent)
        else:
            self.view = view

    def apply_properties(self):
        view_props = self.get_view_properties()
        if view_props:
            self.line.set_linestyle(view_props.style)
            self.line.set_drawstyle(view_props.draw_style)
            self.line.set_linewidth(view_props.width)
            self.line.set_color(view_props.color)

    def get_line_properties(self):
        return LineProperties.from_line(self.line)

    def get_view_properties(self):
        return LineProperties.from_view(self.view)

    def set_line(self, line):
        if not isinstance(line, Line2D) and not isinstance(line, ErrorbarContainer):
            raise ValueError("'line' must be a Line2D or ErrorbarContainer "
                             "object. Found '{}'".format(type(line)))
        self.line = line

    def update_view(self):
        line_props = self.get_line_properties()
        if line_props:
            self.view.set_style(line_props.style)
            self.view.set_draw_style(line_props.draw_style)
            self.view.set_width(line_props.width)
            self.view.set_color(line_props.color)
