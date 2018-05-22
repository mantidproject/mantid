from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui

from Muon.GUI.Common.dummy.dummy_widget import DummyWidget
from Muon.GUI.Common.dummy_label.dummy_label_widget import DummyLabelWidget
from Muon.GUI.Common.dock.dock_view import DockView


class DockWidget(QtGui.QWidget):

    """
    This is a special case of the widget class structure.
    Normally we would only store the presenter and would
    get the view via the presenter. However, the docks
    have no logic and therefore do not have a presenter.
    So this class simply wraps the dock (view) and
    populates it
    """

    def __init__(self, parent=None):
        super(DockWidget, self).__init__(parent)
        self.dockWidget = QtGui.QWidget()

        self.dock_view = DockView(self)

        self.btn = DummyWidget("moo", self)
        self.dock_view.addDock(self.btn.widget, "first")
        self.btn.setButtonConnection(self.handleButton)

        self.label = DummyLabelWidget("boo", self)
        self.dock_view.addDock(self.label.widget, "second")

        self.btn2 = DummyWidget("waaa", self)
        self.dock_view.addDock(self.btn2.widget, "third")
        self.btn2.setButtonConnection(self.handleButton)

        self.dock_view.makeTabs()
        self.dock_view.keepDocksOpen()

        QHbox = QtGui.QHBoxLayout()
        QHbox.addWidget(self.dock_view)

        self.dockWidget.setLayout(QHbox)

    def handleButton(self, message):
        self.label.updateLabel(message)

    @property
    def widget(self):
        return self.dockWidget

    def closeEvent(self, event):
        self.dock_view.closeEvent(event)
