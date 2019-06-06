# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.plotconfigdialog.curvestabwidget import hide_errorbars
from mantidqt.widgets.plotconfigdialog.curvestabwidget.errorbarstabwidget import ErrorbarsProperties, errorbars_hidden
from mantidqt.widgets.plotconfigdialog.curvestabwidget.errorbarstabwidget.view import ErrorbarsTabWidgetView


class ErrorbarsTabWidgetPresenter:

    def __init__(self, line=None, view=None, parent=None):
        self.line = line
        if not view:
            self.view = ErrorbarsTabWidgetView(parent)
        else:
            self.view = view

    def apply_properties(self):
        view_props = self.get_view_properties()
        if view_props:
            self.apply_caps_properties(view_props)
            self.apply_bars_properties(view_props)
            hide_errorbars(self.line, view_props.hide)

    def apply_caps_properties(self, props):
        caps_set = self.line[1]
        if caps_set:
            for caps in caps_set:
                caps.set_markersize(2*props.capsize)
                caps.set_markeredgewidth(props.cap_thickness)
                caps.set_color(props.color)

    def apply_bars_properties(self, props):
        bars_set = self.line[2]
        if bars_set:
            for bars in bars_set:
                bars.set_linewidth(props.width)
                bars.set_color(props.color)

    def get_line_properties(self):
        return ErrorbarsProperties.from_container(self.line)

    def get_view_properties(self):
        return ErrorbarsProperties.from_view(self.view)

    def set_line(self, line):
        self.line = line

    def set_view_field(self, field, props, default=0):
        """
        Set view field but disable the field if no value is available to
        update with. This situation typically arises when errobars are
        plotted without caps or without bars.
        """
        setter = getattr(self.view, 'set_{}'.format(field))
        enabler = getattr(self.view, 'set_{}_enabled'.format(field))
        try:
            value = getattr(props, field)
            setter(value)
            enabler(True)
        except AttributeError:
            setter(default)
            enabler(False)

    def update_view(self):
        line_props = self.get_line_properties()
        if line_props:
            self.view.set_hide(line_props.hide)
            self.set_view_field('width', line_props)
            self.set_view_field('capsize', line_props)
            self.set_view_field('cap_thickness', line_props)
            self.view.set_color(line_props.color)
