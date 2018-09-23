from PyQt4 import QtGui, QtCore
from PyQt4.QtCore import pyqtSignal, pyqtSlot

"""
Original code by user Blackwood, Jan 2018.
https://stackoverflow.com/questions/47267195/in-pyqt4-is-it-possible-to-detach-tabs-from-a-qtabwidget
"""


class DetachableTabWidget(QtGui.QTabWidget):
    """

    The detached tabs are QMainWindow type
    """

    def __init__(self, parent=None):
        QtGui.QTabWidget.__init__(self, parent)

        self.tab_bar = self.TabBar(self)

        self.tab_bar.onDetachTabSignal.connect(self.detach_tab)
        self.tab_bar.onMoveTabSignal.connect(self.move_tab)
        self.tab_bar.detachedTabDropSignal.connect(self.detached_tab_drop)

        self.setTabBar(self.tab_bar)

        # Used to keep a reference to detached tabs since their QMainWindow
        # does not have a parent
        self.detachedTabs = {}

        # Close all detached tabs if the application is closed explicitly
        QtGui.qApp.aboutToQuit.connect(self.closeDetachedTabs)  # @UndefinedVariable

    def closeEvent(self, event):
        print("closeEvent in detatchable tab widget")
        self.closeDetachedTabs()

    def setMovable(self, movable):
        """
        The default movable functionality of QTabWidget must remain disabled
        so as not to conflict with the added features.
        """
        pass

    @pyqtSlot(int, int)
    def move_tab(self, from_index, to_index):
        """
        Move a tab from one position (index) to another
        :param from_index: the original index location of the tab.
        :param to_index: the new index location of the tab.
        :return: None
        """
        widget = self.widget(from_index)
        icon = self.tabIcon(from_index)
        text = self.tabText(from_index)

        self.removeTab(from_index)
        self.insertTab(to_index, widget, icon, text)
        self.setCurrentIndex(to_index)

    @pyqtSlot(int, QtCore.QPoint)
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
        detached_tab.onCloseSignal.connect(self.attachTab)
        detached_tab.onDropSignal.connect(self.tab_bar.detachedTabDrop)
        detached_tab.move(point)
        detached_tab.show()

        # Create a reference to maintain access to the detached tab
        self.detachedTabs[name] = detached_tab

    def attachTab(self, content_widget, name, icon, insert_at=None):
        """
        Re-attach the tab by removing the content from the DetachedTab window,
        closing it, and placing the content back into the DetachableTabWidget
        :param content_widget: the content widget from the DetachedTab window.
        :param name: the name of the detached tab.
        :param icon: the window icon for the detached tab.
        :param insert_at: insert the re-attached tab at the given index.
        :return: None
        """

        # Make the content widget a child of this widget
        content_widget.setParent(self)

        # Remove the reference
        del self.detachedTabs[name]

        # Create an image from the given icon (for comparison)
        if not icon.isNull():
            try:
                tabIconPixmap = icon.pixmap(icon.availableSizes()[0])
                tab_icon_image = tabIconPixmap.toImage()
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

    ##
    #  Remove the tab with the given name, even if it is detached
    #
    #  @param    name    the name of the tab to be removed
    def removeTabByName(self, name):

        # Remove the tab if it is attached
        attached = False
        for index in xrange(self.count()):
            if str(name) == str(self.tabText(index)):
                self.removeTab(index)
                attached = True
                break

        # If the tab is not attached, close it's window and
        # remove the reference to it
        if not attached:
            for key in self.detachedTabs:
                if str(name) == str(key):
                    self.detachedTabs[key].onCloseSignal.disconnect()
                    self.detachedTabs[key].close()
                    del self.detachedTabs[key]
                    break

    @QtCore.pyqtSlot(str, int, QtCore.QPoint)
    def detached_tab_drop(self, name, index, dropPos):
        """
        Handle dropping of a detached tab inside the DetachableTabWidget.
        :param name: the name of the detached tab.
        :param index: the index of an existing tab (if the tab bar determined that the drop occurred on an existing tab)
        :param dropPos: the mouse cursor position when the drop occurred.
        """

        # If the drop occurred on an existing tab, insert the detached
        # tab at the existing tab's location
        if index > -1:

            # Create references to the detached tab's content and icon
            contentWidget = self.detachedTabs[name].contentWidget
            icon = self.detachedTabs[name].windowIcon()

            # Disconnect the detached tab's onCloseSignal so that it
            # does not try to re-attach automatically
            self.detachedTabs[name].onCloseSignal.disconnect()

            # Close the detached
            self.detachedTabs[name].close()

            # Re-attach the tab at the given index
            self.attachTab(contentWidget, name, icon, index)


        # If the drop did not occur on an existing tab, determine if the drop
        # occurred in the tab bar area (the area to the side of the QTabBar)
        else:

            # Find the drop position relative to the DetachableTabWidget
            tabDropPos = self.mapFromGlobal(dropPos)

            # If the drop position is inside the DetachableTabWidget...
            if self.rect().contains(tabDropPos):

                # If the drop position is inside the tab bar area (the
                # area to the side of the QTabBar) or there are not tabs
                # currently attached...
                if tabDropPos.y() < self.tab_bar.height() or self.count() == 0:
                    # Close the detached tab and allow it to re-attach
                    # automatically
                    self.detachedTabs[name].close()

    def closeDetachedTabs(self):
        """
        Close all tabs that are currently detached.
        """
        print("close detatched tabs")
        listOfDetachedTabs = []

        for key in self.detachedTabs:
            listOfDetachedTabs.append(self.detachedTabs[key])

        for detachedTab in listOfDetachedTabs:
            detachedTab.close()

    class DetachedTab(QtGui.QMainWindow):
        """
        When a tab is detached, the contents are placed into this QDialog.  The tab
        can be re-attached by closing the dialog or by double clicking on its
        window frame.
        """
        onCloseSignal = pyqtSignal(QtGui.QWidget, str, QtGui.QIcon)
        onDropSignal = pyqtSignal(str, QtCore.QPoint)

        def __init__(self, name, contentWidget):
            QtGui.QMainWindow.__init__(self, None)

            self.setObjectName(name)
            self.setWindowTitle(name)

            self.contentWidget = contentWidget
            self.setCentralWidget(self.contentWidget)
            self.contentWidget.show()

            self.windowDropFilter = self.WindowDropFilter()
            self.installEventFilter(self.windowDropFilter)
            self.windowDropFilter.onDropSignal.connect(self.windowDropSlot)

        @pyqtSlot(QtCore.QPoint)
        def windowDropSlot(self, drop_pos):
            """
            Handle a window drop event
            :param drop_pos: the mouse cursor position of the drop
            """
            print("windowDropSlot")
            self.onDropSignal.emit(self.objectName(), drop_pos)

        class WindowDropFilter(QtCore.QObject):
            """
            An event filter class to detect a QMainWindow drop event.
            """
            onDropSignal = pyqtSignal(QtCore.QPoint)

            def __init__(self):
                QtCore.QObject.__init__(self)
                self.lastEvent = None

            def eventFilter(self, obj, event):
                """
                Detect a QMainWindow drop event by looking for a NonClientAreaMouseMove (173)
                event that immediately follows a Move event
                :param obj: the object that generated the event.
                :param event: the current event.
                """

                # If a NonClientAreaMouseMove (173) event immediately follows a Move event
                if self.lastEvent == QtCore.QEvent.Move and event.type() == 173:

                    # Determine the position of the mouse cursor and emit it with the onDropSignal
                    mouse_drop_pos = QtGui.QCursor().pos()
                    self.onDropSignal.emit(mouse_drop_pos)
                    self.lastEvent = event.type()
                    return True
                else:
                    self.lastEvent = event.type()
                    return False

        def closeEvent(self, event):
            """
            If the window is closed, emit the onCloseSignal and give the
            content widget back to the DetachableTabWidget.
            """
            self.onCloseSignal.emit(self.contentWidget, self.objectName(), self.windowIcon())

    class TabBar(QtGui.QTabBar):
        """
        The TabBar class re-implements some of the functionality of the QTabBar widget.
        """

        onDetachTabSignal = pyqtSignal(int, QtCore.QPoint)
        onMoveTabSignal = pyqtSignal(int, int)
        detachedTabDropSignal = pyqtSignal(str, int, QtCore.QPoint)

        def __init__(self, parent=None):
            QtGui.QTabBar.__init__(self, parent)

            self.setAcceptDrops(True)
            self.setElideMode(QtCore.Qt.ElideRight)
            self.setSelectionBehaviorOnRemove(QtGui.QTabBar.SelectLeftTab)

            self.drag_start_pos = QtCore.QPoint()
            self.drag_end_pos = QtCore.QPoint()
            self.mouseCursor = QtGui.QCursor()
            self.drag_initiated = False

        def mouseDoubleClickEvent(self, event):
            """
            Send the onDetachTabSignal when a tab is double clicked.
            :param event: a mouse double click event.
            """
            event.accept()
            self.onDetachTabSignal.emit(self.tabAt(event.pos()), self.mouseCursor.pos())

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

            QtGui.QTabBar.mousePressEvent(self, event)

        def mouseMoveEvent(self, event):
            """
            Determine if the current movement is a drag.  If it is, convert it into a QDrag.  If the
            drag ends inside the tab bar, emit an onMoveTabSignal.  If the drag ends outside the tab
            bar, emit an onDetachTabSignal.
            :param event: a mouse move event.
            """

            # Determine if the current movement is detected as a drag
            if not self.drag_start_pos.isNull() and (
                    (event.pos() - self.drag_start_pos).manhattanLength() < QtGui.QApplication.startDragDistance()):
                self.drag_initiated = True

            # If the current movement is a drag initiated by the left button
            if (event.buttons() & QtCore.Qt.LeftButton) and self.drag_initiated:

                # Stop the move event
                finishMoveEvent = QtGui.QMouseEvent(QtCore.QEvent.MouseMove, event.pos(), QtCore.Qt.NoButton,
                                                    QtCore.Qt.NoButton, QtCore.Qt.NoModifier)
                QtGui.QTabBar.mouseMoveEvent(self, finishMoveEvent)

                # Convert the move event into a drag
                drag = QtGui.QDrag(self)
                mimeData = QtCore.QMimeData()
                # mimeData.setData('action', 'application/tab-detach')
                drag.setMimeData(mimeData)
                # screen = QScreen(self.parentWidget().currentWidget().winId())
                # Create the appearance of dragging the tab content
                pixmap = QtGui.QPixmap.grabWindow(self.parentWidget().currentWidget().winId())
                targetPixmap = QtGui.QPixmap(pixmap.size())
                targetPixmap.fill(QtCore.Qt.transparent)
                painter = QtGui.QPainter(targetPixmap)
                painter.setOpacity(0.85)
                painter.drawPixmap(0, 0, pixmap)
                painter.end()
                drag.setPixmap(targetPixmap)

                # Initiate the drag
                dropAction = drag.exec_(QtCore.Qt.MoveAction | QtCore.Qt.CopyAction)

                # For Linux:  Here, drag.exec_() will not return MoveAction on Linux.  So it
                #             must be set manually
                if self.drag_end_pos.x() != 0 and self.drag_end_pos.y() != 0:
                    dropAction = QtCore.Qt.MoveAction

                # If the drag completed outside of the tab bar, detach the tab and move
                # the content to the current cursor position
                if dropAction == QtCore.Qt.IgnoreAction:
                    event.accept()
                    self.onDetachTabSignal.emit(self.tabAt(self.drag_start_pos), self.mouseCursor.pos())

                # Else if the drag completed inside the tab bar, move the selected tab to the new position
                elif dropAction == QtCore.Qt.MoveAction:
                    if not self.drag_end_pos.isNull():
                        event.accept()
                        self.onMoveTabSignal.emit(self.tabAt(self.drag_start_pos), self.tabAt(self.drag_end_pos))
            else:
                QtGui.QTabBar.mouseMoveEvent(self, event)

        def dragEnterEvent(self, event):
            QtGui.QTabBar.dragMoveEvent(self, event)

        def dropEvent(self, event):
            """
            Get the position of the end of the drag.
            """
            self.drag_end_pos = event.pos()
            QtGui.QTabBar.dropEvent(self, event)

        def detachedTabDrop(self, name, drop_pos):
            """
            Determine if the detached tab drop event occurred on an existing tab,
            then send the event to the DetachableTabWidget.
            """
            tab_drop_pos = self.mapFromGlobal(drop_pos)
            index = self.tabAt(tab_drop_pos)
            self.detachedTabDropSignal.emit(name, index, drop_pos)
