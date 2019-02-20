# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

import os

from mantid.kernel import logger
from mantidqt.widgets.codeeditor.tab_widget.codeeditor_tab_view import CodeEditorTabWidget


class CodeEditorTabPresenter(object):
    def __init__(self, multifileinterpreter=None, view=None):
        self.view = view if view else CodeEditorTabWidget(self, multifileinterpreter)

    def action_show_in_explorer(self):
        filename = self.view.widget(self.view.last_tab_clicked).editor.fileName()
        if "" != filename:
            os.startfile(os.path.dirname(filename))
        else:
            logger.notice("Selected tab is not saved to a file, and cannot be shown in explorer.")

    # TODO reconsider just accessing them by accessing the view :thinking:
    def count(self):
        return self.view.count()

    def widget(self, index):
        return self.view.widget(index)

    def addTab(self, interpreter, tab_title):
        return self.view.addTab(interpreter, tab_title)

    def removeTab(self, index):
        self.view.removeTab(index)

    def setTabToolTip(self, tab_idx, tab_tooltip):
        self.view.setTabToolTip(tab_idx, tab_tooltip)

    def setCurrentIndex(self, tab_idx):
        self.view.setCurrentIndex(tab_idx)

    def currentWidget(self):
        return self.view.currentWidget()

    def currentIndex(self):
        return self.view.currentIndex()

    def tabText(self, index):
        return self.view.tabText(index)

    def setTabText(self, index, text):
        return self.view.setTabText(index, text)
