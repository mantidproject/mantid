# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QAction, QMenu, QTabWidget, QToolButton


class CodeEditorTabWidget(QTabWidget):

    def __init__(self, presenter=None, parent=None):
        super(CodeEditorTabWidget, self).__init__(parent)
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setContextMenuPolicy(Qt.ActionsContextMenu)
        self.setMovable(True)
        self.setTabsClosable(True)

        self.presenter = presenter

        self.last_tab_clicked = 0
        self.tabBarClicked.connect(self.tab_was_clicked)

        show_in_explorer = QAction("Show in Explorer", self)
        show_in_explorer.triggered.connect(self.presenter.action_show_in_explorer)
        self.addAction(show_in_explorer)

        # create a button to add new tabs
        plus_btn = QToolButton(self)
        plus_btn.setText('+')
        plus_btn.clicked.connect(parent.plus_button_clicked)
        self.setCornerWidget(plus_btn, Qt.TopLeftCorner)
        self.tabCloseRequested.connect(parent.close_tab)
        self.popMenu = QMenu(self)
        self.popMenu.addAction(QAction('test0', self))
        self.popMenu.addAction(QAction('test1', self))
        self.popMenu.addSeparator()
        self.popMenu.addAction(QAction('test2', self))

    def action_copy(self, e):
        print("CTX MENU WORKS", e, "Action on tab", self.last_tab_clicked)

    def closeEvent(self, event):
        self.deleteLater()
        super(CodeEditorTabWidget, self).closeEvent(event)

    def mousePressEvent(self, event):
        # right mouse button clicked
        if 4 == event.button():
            self.parent().close_tab(self.last_tab_clicked)

        QTabWidget(self).mousePressEvent(event)

    def tab_was_clicked(self, tab_index):
        self.last_tab_clicked = tab_index
