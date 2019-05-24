# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from qtpy.QtWidgets import QWidget

from mantidqt.utils.qt import load_ui


class CurvesTabWidgetView(QWidget):

    def __init__(self, parent=None):
        super(CurvesTabWidgetView, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'curves_tab.ui',
                          baseinstance=self)

        self.ui.tab_widget.addTab(CurvesLineTabWidget(self), "Line")
        self.ui.tab_widget.addTab(CurvesMarkerTabWidget(self), "Marker")
        self.ui.tab_widget.addTab(CurvesErrorbarsTabWidget(self), "Errorbars")


class CurvesLineTabWidget(QWidget):

    def __init__(self, parent=None):
        super(CurvesLineTabWidget, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'curves_tab_line_tab.ui',
                          baseinstance=self)


class CurvesMarkerTabWidget(QWidget):

    def __init__(self, parent=None):
        super(CurvesMarkerTabWidget, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'curves_tab_marker_tab.ui',
                          baseinstance=self)


class CurvesErrorbarsTabWidget(QWidget):

    def __init__(self, parent=None):
        super(CurvesErrorbarsTabWidget, self).__init__(parent=parent)

        self.ui = load_ui(__file__,
                          'curves_tab_errobars_tab.ui',
                          baseinstance=self)
