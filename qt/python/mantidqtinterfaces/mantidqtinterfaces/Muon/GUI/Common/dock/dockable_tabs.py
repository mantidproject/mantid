# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt import icons

from qtpy import QtWidgets, QtCore, QtGui
from qtpy.QtCore import Slot
from qtpy.QtGui import QIcon

"""
Original code by user Blackwood, Jan 2018.
https://stackoverflow.com/questions/47267195/in-pyqt4-is-it-possible-to-detach-tabs-from-a-qtabwidget
"""


class DetachableTabWidget(QtWidgets.QTabWidget):
    """
    The detached tabs are QMainWindow type
    """

    def __init__(self, parent=None):
        QtWidgets.QTabWidget.__init__(self, parent)

        self.tab_bar = self.TabBar(self)

        self.tab_bar.onDetachTabSignal.connect(self.detach_tab)
        # self.tab_bar.detachedTabDropSignal.connect(self.detached_tab_drop)

        self.setTabBar(self.tab_bar)

        # Used to keep a reference to detached tabs since their QMainWindow
        # does not have a parent
        self.detachedTabs = {}

        self.tab_order = []
        self.attached_tab_names = []

        self.valid_tab_icon = QIcon()
        self.invalid_tab_icon = icons.get_icon("mdi.asterisk", "red", 0.5)

        # Close all detached tabs if the application is closed explicitly
        # QtGui.qApp.aboutToQuit.connect(self.close_detached_tabs)  # @UndefinedVariable

    def closeEvent(self, event):
        self.close_detached_tabs()

    def set_tab_warning(self, index: int, message: str) -> None:
        """Adds a tooltip to a tab with a warning message."""
        self.setTabIcon(index, self.invalid_tab_icon if message != "" else self.valid_tab_icon)
        self.setTabToolTip(index, message)

    def setMovable(self, movable):
        """
        The default movable functionality of QTabWidget must remain disabled
        so as not to conflict with the added features.
        """
        pass

    def addTabWithOrder(self, tab, name):
        self.tab_order.append(name)
        self.attached_tab_names.append(name)
        self.addTab(tab, name)

    def addTab(self, tab, name):
        super(DetachableTabWidget, self).addTab(tab, name)
        self.setTabToolTip(self.indexOf(tab), name)

    def insertTab(self, to_index, widget, *args):
        index = super(DetachableTabWidget, self).insertTab(to_index, widget, *args)
        self.setTabToolTip(to_index, args[-1])
        return index

    def set_slot_for_tab_changed(self, slot):
        self.currentChanged.connect(slot)

    @Slot(int, QtCore.QPoint)
    def detach_tab(self, index, point):
        """
        Detach the tab by removing it's contents and placing them in
        a DetachedTab window
        :param index: the index location of the tab to be detached.
        :param point: the screen position for creating the new DetachedTab window.
        :return: None
        """
        # Get the tab content
        name = self.tabText(index)
        self.attached_tab_names.remove(name)
        icon = self.tabIcon(index)
        if icon.isNull():
            icon = self.window().windowIcon()
        content_widget = self.widget(index)

        try:
            content_widget_rect = content_widget.frameGeometry()
        except AttributeError:
            return

        # Create a new detached tab window
        detached_tab = self.DetachedTab(name, content_widget)
        detached_tab.setWindowModality(QtCore.Qt.NonModal)
        detached_tab.setWindowIcon(icon)
        detached_tab.setGeometry(content_widget_rect)
        detached_tab.onCloseSignal.connect(self.attach_tab)
        detached_tab.move(point)
        detached_tab.show()

        # Create a reference to maintain access to the detached tab
        self.detachedTabs[name] = detached_tab

    def determine_insert_position(self, name):
        tabs_to_check = self.tab_order[self.tab_order.index(name) + 1:]
        for tab in tabs_to_check:
            if tab in self.attached_tab_names:
                return self.attached_tab_names.index(tab)
        return len(self.attached_tab_names)

    def attach_tab(self, content_widget, name, icon, insert_at=None):
        """
        Re-attach the tab by removing the content from the DetachedTab window,
        closing it, and placing the content back into the DetachableTabWidget
        :param content_widget: the content widget from the DetachedTab window.
        :param name: the name of the detached tab.
        :param icon: the window icon for the detached tab.
        :param insert_at: insert the re-attached tab at the given index.
        :return: None
        """
        if not insert_at:
            insert_at = self.determine_insert_position(name)

        self.attached_tab_names.insert(insert_at, name)
        # Make the content widget a child of this widget
        content_widget.setParent(self)
        # Remove the signal
        self.detachedTabs[name].onCloseSignal.disconnect()
        # Remove the reference
        del self.detachedTabs[name]

        # Create an image from the given icon (for comparison)
        if not icon.isNull():
            try:
                tab_icon_pixmap = icon.pixmap(icon.availableSizes()[0])
                tab_icon_image = tab_icon_pixmap.toImage()
            except IndexError:
                tab_icon_image = None
        else:
            tab_icon_image = None

        # Create an image of the main window icon (for comparison)
        if not icon.isNull():
            try:
                window_icon_pixmap = self.window().windowIcon().pixmap(icon.availableSizes()[0])
                window_icon_image = window_icon_pixmap.toImage()
            except IndexError:
                window_icon_image = None
        else:
            window_icon_image = None

        # Determine if the given image and the main window icon are the same.
        # If they are, then do not add the icon to the tab
        if tab_icon_image == window_icon_image:
            if insert_at is None:
                index = self.addTab(content_widget, name)
            else:
                index = self.insertTab(insert_at, content_widget, name)
        else:
            if insert_at is None:
                index = self.addTab(content_widget, icon, name)
            else:
                index = self.insertTab(insert_at, content_widget, icon, name)

        # Make this tab the current tab
        if index > -1:
            self.setCurrentIndex(index)

    def close_detached_tabs(self):
        """
        Close all tabs that are currently detached.
        """
        list_of_detached_tabs = []
        for key in self.detachedTabs:
            list_of_detached_tabs.append(self.detachedTabs[key])

        for detachedTab in list_of_detached_tabs:
            detachedTab.onCloseSignal.disconnect()
            detachedTab.close()

    class DetachedTab(QtWidgets.QMainWindow):
        """
        When a tab is detached, the contents are placed into this QDialog.  The tab
        can be re-attached by closing the dialog or by double clicking on its
        window frame.
        """
        onCloseSignal = QtCore.Signal(QtWidgets.QWidget, str, QtGui.QIcon)
        onDropSignal = QtCore.Signal(str, QtCore.QPoint)

        def __init__(self, name, content_widget):
            QtWidgets.QMainWindow.__init__(self, None)

            self.setObjectName(name)
            self.setWindowTitle(name)

            self.contentWidget = content_widget
            self.setCentralWidget(self.contentWidget)
            self.contentWidget.show()

        def closeEvent(self, _event):
            """
            If the window is closed, emit the onCloseSignal and give the
            content widget back to the DetachableTabWidget.
            """
            self.onCloseSignal.emit(self.contentWidget, self.objectName(), self.windowIcon())

    class TabBar(QtWidgets.QTabBar):
        """
        The TabBar class re-implements some of the functionality of the QTabBar widget.
        """

        onDetachTabSignal = QtCore.Signal(int, QtCore.QPoint)
        onMoveTabSignal = QtCore.Signal(int, int)
        detachedTabDropSignal = QtCore.Signal(str, int, QtCore.QPoint)

        def __init__(self, parent=None):
            QtWidgets.QTabBar.__init__(self, parent)

            self.setAcceptDrops(True)
            self.setElideMode(QtCore.Qt.ElideRight)
            self.setSelectionBehaviorOnRemove(QtWidgets.QTabBar.SelectLeftTab)

            self.drag_start_pos = QtCore.QPoint()
            self.drag_end_pos = QtCore.QPoint()
            self.mouse_cursor = QtGui.QCursor()
            self.drag_initiated = False

            self.drag_pixmap = None

        def mouseDoubleClickEvent(self, event):
            """
            Send the onDetachTabSignal when a tab is double clicked.
            :param event: a mouse double click event.
            """
            event.accept()
            self.onDetachTabSignal.emit(self.tabAt(event.pos()), self.mouse_cursor.pos())

        def mousePressEvent(self, event):
            """
            Set the starting position for a drag event when the mouse button is pressed.
            :param event: a mouse press event.
            """
            if event.button() == QtCore.Qt.LeftButton:
                self.drag_start_pos = event.pos()

            self.drag_end_pos.setX(0)
            self.drag_end_pos.setY(0)

            self.drag_initiated = False

            QtWidgets.QTabBar.mousePressEvent(self, event)
