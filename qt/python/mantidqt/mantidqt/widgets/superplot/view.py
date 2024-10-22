# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


from qtpy.QtWidgets import QDockWidget, QHeaderView, QTreeWidgetItem, QToolButton
from qtpy.QtGui import QColor
from qtpy.QtCore import Signal, QObject, QSize, Qt
from qtpy import uic

import os


class WorkspaceItemSignals(QObject):
    """
    Thrown when the delete button is pressed.
    """

    sig_del_clicked = Signal(str)

    def __init__(self):
        super().__init__()


class WorkspaceItem(QTreeWidgetItem):
    """
    Name of the workspace represented by this item.
    """

    _workspace_name = None

    """
    Delete button.
    """
    _del_button = None

    """
    WorkspaceItem signals.
    """
    signals = None

    def __init__(self, treeWidget, name):
        super().__init__(treeWidget)
        self._workspace_name = name
        self._del_button = QToolButton()
        self._del_button.setMinimumSize(20, 20)
        self._del_button.setMaximumSize(20, 20)
        self._del_button.setText("-")
        self._del_button.setToolTip("Remove the workspace from the list")
        self.signals = WorkspaceItemSignals()
        self._del_button.clicked.connect(lambda c: self.signals.sig_del_clicked.emit(self._workspace_name))
        self.treeWidget().setItemWidget(self, 1, self._del_button)
        self.treeWidget().resizeColumnToContents(1)

    def get_workspace_name(self):
        """
        Get the name of the workspace.

        Returns:
            str: name of the workspace
        """
        return self._workspace_name


class SpectrumItemSignals(QObject):
    """
    Thrown when the delete button is pressed.
    Args:
        str: workspace name
        int: spectrum index
    """

    sig_del_clicked = Signal(str, int)

    def __init__(self):
        super().__init__()


class SpectrumItem(QTreeWidgetItem):
    """
    Index of the spectrum represented by this item.
    """

    _spectrum_index = None

    """
    Name of the corresponding workspace.
    """
    _workspace_name = None

    """
    Delete button.
    """
    _del_button = None

    """
    Spectrum item signals.
    """
    signals = None

    def __init__(self, treeWidget, index):
        super().__init__(treeWidget)
        self._spectrum_index = index
        self._workspace_name = self.parent().get_workspace_name()
        self._del_button = QToolButton()
        self._del_button.setMinimumSize(20, 20)
        self._del_button.setMaximumSize(20, 20)
        self._del_button.setText("-")
        self._del_button.setToolTip("Remove from the list")
        self.signals = SpectrumItemSignals()
        self._del_button.clicked.connect(lambda c: self.signals.sig_del_clicked.emit(self._workspace_name, self._spectrum_index))
        self.treeWidget().setItemWidget(self, 1, self._del_button)
        self.treeWidget().resizeColumnToContents(1)

    def get_spectrum_index(self):
        """
        Get the index of the spectrum.

        Returns:
            int: index of the spectrum
        """
        return self._spectrum_index


class SuperplotViewSide(QDockWidget):
    UI = "side_dock_widget.ui"

    """
    Sent when the widget is resized.
    """
    resized = Signal()

    """
    Sent when a drop event is received.
    Args:
        str: the event mime type text (assumed to be a workspace name)
    """
    drop = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, SuperplotViewSide.UI), self)
        ws_list = self.workspacesList
        ws_list.header().setSectionResizeMode(0, QHeaderView.ResizeToContents)
        ws_list.header().setSectionResizeMode(1, QHeaderView.ResizeToContents)
        size0 = ws_list.header().sectionSize(0)
        size1 = ws_list.header().sectionSize(1)
        ws_list.header().setDefaultSectionSize(size1)
        ws_list.header().setSectionResizeMode(QHeaderView.Stretch)
        ws_list.header().setSectionResizeMode(1, QHeaderView.Interactive)
        ws_list.setMinimumSize(QSize(size0 + size1, 0))
        self.workspaceSelector.setWorkspaceTypes(["Workspace2D", "WorkspaceGroup", "EventWorkspace", "RebinnedOutput"])
        self.setAcceptDrops(True)

    def dragEnterEvent(self, event):
        """
        Override QDockWidget::dragEnterEvent.
        """
        event.acceptProposedAction()

    def dropEvent(self, event):
        """
        Override QDockWidget::dropEvent. Emit the drop signal.
        """
        self.drop.emit(event.mimeData().text())

    def resizeEvent(self, event):
        """
        Override QDockWidget::resizeEvent.
        """
        super().resizeEvent(event)
        self.resized.emit()


class SuperplotViewBottom(QDockWidget):
    UI = "bottom_dock_widget.ui"

    """
    Sent when the widget is resized.
    """
    resized = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))
        uic.loadUi(os.path.join(self.here, SuperplotViewBottom.UI), self)
        self.setFocusPolicy(Qt.ClickFocus)

    def keyPressEvent(self, event):
        """
        Override of QDockWidget::keyPressEvent. This method forward some key
        press event to some relevant actions of the contained widgets.

        Args:
            event (QKeyEvent): the key press event
        """
        if event.key() == Qt.Key_Left:
            self.spectrumSlider.keyPressEvent(event)
        elif event.key() == Qt.Key_Right:
            self.spectrumSlider.keyPressEvent(event)
        elif event.key() == Qt.Key_Space:
            self.holdButton.click()

    def resizeEvent(self, event):
        """
        Override QDockWidget::resizeEvent.
        """
        super().resizeEvent(event)
        self.resized.emit()


class SuperplotView:
    _presenter = None
    _side_view = None
    _bottom_view = None

    def __init__(self, presenter, parent=None):
        self._parent = parent
        self._presenter = presenter
        self._side_view = SuperplotViewSide(parent)
        self._bottom_view = SuperplotViewBottom(parent)

        side = self._side_view
        side.visibilityChanged.connect(self._presenter.on_visibility_changed)
        side.normaliseCheckbox.clicked.connect(self._presenter.on_normalise_checked)
        side.addButton.clicked.connect(self._presenter.on_add_button_clicked)
        side.workspacesList.itemSelectionChanged.connect(self._presenter.on_workspace_selection_changed)
        side.resized.connect(self._presenter.on_resize)
        side.drop.connect(self._presenter.on_drop)
        bottom = self._bottom_view
        bottom.holdButton.clicked.connect(self._presenter.on_hold_button_clicked)
        bottom.spectrumSlider.valueChanged.connect(self._presenter.on_spectrum_slider_moved)
        bottom.spectrumSpinBox.valueChanged.connect(self._presenter.on_spectrum_spin_box_changed)
        bottom.modeComboBox.currentTextChanged.connect(self._presenter.on_mode_changed)
        bottom.resized.connect(self._presenter.on_resize)

    def show(self):
        """
        Show the superplot view. This will add the two dockwidgets to their
        respective position.
        """
        self._parent.addDockWidget(Qt.LeftDockWidgetArea, self._side_view)
        self._parent.addDockWidget(Qt.BottomDockWidgetArea, self._bottom_view)
        self._bottom_view.setFocus()

    def close(self):
        self._side_view.close()
        self._bottom_view.close()

    def get_selected_workspace(self):
        """
        Get the workspace selected in the workspace selector.

        Return:
            str: name of the workspace
        """
        return self._side_view.workspaceSelector.currentText()

    def set_selection(self, selection):
        """
        Set the selected workspaces and spectra in list.

        Args:
            selection (dict(str: list(int))): workspace names and list of
                                              spectrum numbers
        """
        self._side_view.workspacesList.blockSignals(True)
        self._side_view.workspacesList.clearSelection()
        for i in range(self._side_view.workspacesList.topLevelItemCount()):
            ws_item = self._side_view.workspacesList.topLevelItem(i)
            ws_name = ws_item.get_workspace_name()
            if ws_name in selection:
                if -1 in selection[ws_name]:
                    ws_item.setSelected(True)
                for j in range(ws_item.childCount()):
                    sp_item = ws_item.child(j)
                    if sp_item.get_spectrum_index() in selection[ws_name]:
                        sp_item.setSelected(True)
        self._side_view.workspacesList.blockSignals(False)

    def get_selection(self):
        """
        Get the current selection.

        Returns:
            dict(str: list(int)): workspace names and list of spectrum numbers
        """
        selection = dict()
        items = self._side_view.workspacesList.selectedItems()
        for item in items:
            if item.parent() is None:
                ws_name = item.get_workspace_name()
                if ws_name not in selection:
                    selection[ws_name] = [-1]
            else:
                spectrum = item.get_spectrum_index()
                ws_name = item.parent().get_workspace_name()
                if ws_name in selection:
                    selection[ws_name].append(spectrum)
                else:
                    selection[ws_name] = [spectrum]
        return selection

    def modify_spectrum_label(self, ws_name, spectrum_index, label, color):
        """
        Modify spectrum label text and color.

        Args:
            ws_name (str): name of the concerned workspace
            spectrum_index (int): index of the spectrum
            label (str): new label
            color (str): new color (#RRGGBB)
        """
        ws_item = self._side_view.workspacesList.findItems(ws_name, Qt.MatchExactly, 0)
        if not ws_item:
            return
        ws_item = ws_item[0]

        for i in range(ws_item.childCount()):
            sp_item = ws_item.child(i)
            if sp_item.get_spectrum_index() == spectrum_index:
                brush = sp_item.foreground(0)
                brush.setColor(QColor(color))
                sp_item.setForeground(0, brush)
                sp_item.setText(0, label)

    def set_workspaces_list(self, names):
        """
        Set the list of selected workspaces and update the workspace slider
        length.

        Args:
            names (list(str)): list if the workspace names
        """
        self._side_view.workspacesList.blockSignals(True)
        self._side_view.workspacesList.clear()
        for name in names:
            item = WorkspaceItem(self._side_view.workspacesList, name)
            item.signals.sig_del_clicked.connect(self._presenter.on_del_button_clicked)
            item.setText(0, name)
        self._side_view.workspacesList.blockSignals(False)

    def set_spectra_list(self, name, nums):
        """
        Set the list of spectrum index for a workspace in the list.

        Args:
            name (str): name of the workspace
            nums (list(int)): list of the spectrum indexes
        """
        self._side_view.workspacesList.blockSignals(True)
        ws_item = self._side_view.workspacesList.findItems(name, Qt.MatchExactly, 0)[0]
        ws_item.takeChildren()
        for num in nums:
            item = SpectrumItem(ws_item, num)
            item.signals.sig_del_clicked.connect(self._presenter.on_del_spectrum_button_clicked)
            item.setText(0, str(num))
        self._side_view.workspacesList.expandAll()
        self._side_view.workspacesList.blockSignals(False)

    def get_spectra_list(self, name):
        """
        Get the list of spectrum indexes for a workspace in the list.

        Args:
            name (str): name of the workspace

        Returns:
            list(int): list of the spectrum indexes
        """
        ws_item = self._side_view.workspacesList.findItems(name, Qt.MatchExactly, 0)
        if ws_item:
            ws_item = ws_item[0]
        else:
            return []
        nums = list()
        for i in range(ws_item.childCount()):
            nums.append(ws_item.child(i).get_spectrum_index())
        return nums

    def set_hold_button_text(self, text):
        """
        Set the text of the hold button.

        Args:
            text (str): text
        """
        self._bottom_view.holdButton.setText(text)

    def get_hold_button_text(self):
        """
        Get the text of the hold button.

        Returns:
            str: text
        """
        return self._bottom_view.holdButton.text()

    def set_hold_button_size(self, width, height):
        """
        Set the hold button fixed size.

        Args:
            width (int): button width
            height (int): button height
        """
        self._bottom_view.holdButton.setFixedSize(width, height)

    def get_hold_button_size(self):
        """
        Get the hold button size.

        Returns:
            (int, int): button width and height
        """
        self._bottom_view.holdButton.adjustSize()
        size = self._bottom_view.holdButton.size()
        return size.width(), size.height()

    def set_spectrum_selection_disabled(self, state):
        """
        Disable/enable the spectrum selection widgets (slider and spinbox).

        Args:
            state (bool): if True, disable the widgets
        """
        self._bottom_view.spectrumSlider.setDisabled(state)
        self._bottom_view.spectrumSpinBox.setDisabled(state)
        self._bottom_view.holdButton.setDisabled(state)

    def is_spectrum_selection_disabled(self):
        """
        Get the state of the spectrum selection widgets.

        Returns:
            True if disabled
        """
        return not self._bottom_view.spectrumSlider.isEnabled()

    def set_spectrum_slider_max(self, length):
        """
        Set the max value of the spectrum slider.

        Args:
            value (int): slider maximum value
        """
        self._bottom_view.spectrumSlider.setMaximum(length)

    def set_spectrum_slider_position(self, position):
        """
        Set the spectrum slider position. This function does not trigger any
        QSlider signals.

        Args:
            position (int): position
        """
        self._bottom_view.spectrumSlider.blockSignals(True)
        self._bottom_view.spectrumSlider.setSliderPosition(position)
        self._bottom_view.spectrumSlider.blockSignals(False)

    def get_spectrum_slider_position(self):
        """
        Get the spectrum slider position.

        Returns:
            int: slider position
        """
        return self._bottom_view.spectrumSlider.value()

    def set_spectrum_spin_box_max(self, value):
        """
        Set the max value of the spectrum spinbox.

        Args:
            value (int): spinbox max value
        """
        self._bottom_view.spectrumSpinBox.setMaximum(value)

    def set_spectrum_spin_box_value(self, value):
        """
        Set the spectrum spinbox value. This function does not trigger any
        spinbox signals.

        Args:
            value (int): spinbox value
        """
        self._bottom_view.spectrumSpinBox.blockSignals(True)
        self._bottom_view.spectrumSpinBox.setValue(value)
        self._bottom_view.spectrumSpinBox.blockSignals(False)

    def get_spectrum_spin_box_value(self):
        """
        Get the current spectrum spinbox value.

        Returns:
            int: spinbox value
        """
        return self._bottom_view.spectrumSpinBox.value()

    def set_available_modes(self, modes):
        """
        Set available modes in the mode combobox.

        Args:
            modes (list(str)): list of modes
        """
        self._bottom_view.modeComboBox.blockSignals(True)
        self._bottom_view.modeComboBox.clear()
        self._bottom_view.modeComboBox.addItems(modes)
        self._bottom_view.modeComboBox.blockSignals(False)

    def set_mode(self, mode):
        """
        Set the selected mode. This method does not trigegr any combobox signal.

        Args:
            mode (str): mode
        """
        self._bottom_view.modeComboBox.blockSignals(True)
        self._bottom_view.modeComboBox.setCurrentText(mode)
        self._bottom_view.modeComboBox.blockSignals(False)

    def get_mode(self):
        """
        Get the selected mode from the combobox.

        Returns:
            str: mode
        """
        return self._bottom_view.modeComboBox.currentText()
