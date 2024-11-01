# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore, QtGui
from qtpy.QtCore import Signal
from mantidqtinterfaces.Muon.GUI.Common import message_box
from mantidqtinterfaces.Muon.GUI.Common.utilities import table_utils
from typing import List


def get_pair_columns() -> List[str]:
    return list(pair_columns.values())


pair_columns = {0: "pair_name", 1: "to_analyse", 2: "group_1", 3: "group_2", 4: "alpha", 5: "guess_alpha"}
inverse_pair_columns = {"pair_name": 0, "to_analyse": 1, "group_1": 2, "group_2": 3, "alpha": 4, "guess_alpha": 5}


class PairingTableView(QtWidgets.QWidget):
    dataChanged = Signal()

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(PairingTableView, self).__init__(parent)

        self._presenter = None

        self.pairing_table = QtWidgets.QTableWidget(self)
        self.set_up_table()
        self.setup_interface_layout()
        self.pairing_table.itemChanged.connect(self.on_item_changed)
        self.pairing_table.cellChanged.connect(self.on_cell_changed)

        # Table entry validation
        self._validate_pair_name_entry = lambda text: True
        self._validate_alpha = lambda text: True

        self._on_table_data_changed = lambda row, column: 0
        self._on_guess_alpha_clicked = lambda row: 0

        # The active groups that can be selected from the group combo box
        self._group_selections = []

        # whether the table is updating and therefore we shouldn't respond to signals
        self._updating = False

        # the right-click context menu
        self.menu = None
        self._disabled = False
        self.add_pair_action = None
        self.remove_pair_action = None

    def setup_interface_layout(self):
        self.setObjectName("PairingTableView")
        self.resize(500, 500)

        self.add_pair_button = QtWidgets.QToolButton()
        self.remove_pair_button = QtWidgets.QToolButton()

        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)
        size_policy.setHeightForWidth(self.add_pair_button.sizePolicy().hasHeightForWidth())
        size_policy.setHeightForWidth(self.remove_pair_button.sizePolicy().hasHeightForWidth())

        self.add_pair_button.setSizePolicy(size_policy)
        self.add_pair_button.setObjectName("addGroupButton")
        self.add_pair_button.setToolTip("Add a pair to the end of the table")
        self.add_pair_button.setText("+")

        self.remove_pair_button.setSizePolicy(size_policy)
        self.remove_pair_button.setObjectName("removeGroupButton")
        self.remove_pair_button.setToolTip("Remove selected/last pair(s) from the table")
        self.remove_pair_button.setText("-")

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.add_pair_button)
        self.horizontal_layout.addWidget(self.remove_pair_button)
        self.spacer_item = QtWidgets.QSpacerItem(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.horizontal_layout.addItem(self.spacer_item)
        self.horizontal_layout.setAlignment(QtCore.Qt.AlignLeft)

        self.vertical_layout = QtWidgets.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addWidget(self.pairing_table)
        self.vertical_layout.addLayout(self.horizontal_layout)

        self.setLayout(self.vertical_layout)

    def update_group_selections(self, group_name_list):
        self._group_selections = group_name_list

    def set_up_table(self):
        self.pairing_table.setColumnCount(6)
        self.pairing_table.setHorizontalHeaderLabels(["Pair Name", "Analyse (plot/fit)", "Group 1", " Group 2", "Alpha", "Guess Alpha"])
        header = self.pairing_table.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(3, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(4, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(5, QtWidgets.QHeaderView.ResizeToContents)
        vertical_headers = self.pairing_table.verticalHeader()
        vertical_headers.setSectionsMovable(False)
        vertical_headers.setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)
        vertical_headers.setVisible(True)

        self.pairing_table.horizontalHeaderItem(0).setToolTip(
            "The name of the pair :"
            "\n    - The name must be unique across all groups/pairs"
            "\n    - The name can only use digits, characters and _"
        )
        self.pairing_table.horizontalHeaderItem(1).setToolTip("Group 1 of the pair, selected from the grouping table")
        self.pairing_table.horizontalHeaderItem(2).setToolTip("Group 2 of the pair, selected from the grouping table")
        self.pairing_table.horizontalHeaderItem(3).setToolTip(
            "The value of Alpha for the pair asymmetry:" "\n   - The number must be >= 0.0"
        )

        self.pairing_table.horizontalHeaderItem(4).setToolTip("Whether to include this pair in the analysis")
        self.pairing_table.horizontalHeaderItem(5).setToolTip("Replace the current value of Alpha with one estimated" " from the data.")

    def subscribe(self, presenter):
        self._presenter = presenter

    def subscribe_notifiers_to_presenter(self):
        self.on_add_pair_button_clicked(self._presenter.handle_add_pair_button_checked_state)
        self.on_remove_pair_button_clicked(self._presenter.handle_remove_pair_button_clicked)

        self.on_user_changes_pair_name(self._presenter.validate_pair_name)
        self.on_user_changes_alpha(self._presenter.validate_alpha)
        self.on_guess_alpha_clicked(self._presenter.handle_guess_alpha_clicked)
        self.on_table_data_changed(self._presenter.handle_data_change)

    @property
    def get_pairing_table(self):
        return self.pairing_table

    @property
    def is_updating(self):
        return self._updating

    def set_is_updating(self, value):
        self._updating = value

    @property
    def validate_alpha(self):
        return self._validate_alpha

    @property
    def is_disabled(self):
        return self._disabled

    def set_is_disabled(self, value):
        self._disabled = value

    @property
    def validate_pair_name_entry(self):
        return self._validate_pair_name_entry

    def insert_row_in_table(self):
        row_position = self.pairing_table.rowCount()
        self.pairing_table.insertRow(row_position)
        return row_position

    def set_item_in_table(self, row, col, item, flags=None):
        # Set item flags if provided
        if flags:
            item.setFlags(flags)

        self.pairing_table.setItem(row, col, item)
        return item

    def create_table_item(self, entry, color, tooltip="", alpha=127):
        item = QtWidgets.QTableWidgetItem(entry)
        q_brush = self.create_brush_with_color(color, alpha)  # Using the method for creating brush
        item.setBackground(q_brush)
        item.setToolTip(tooltip)
        return item

    def create_brush_with_color(self, color, alpha=127):
        q_color = QtGui.QColor(*color, alpha=alpha)
        q_brush = QtGui.QBrush(q_color)
        return q_brush

    def set_widget_in_table(self, row, col, widget):
        self.pairing_table.setCellWidget(row, col, widget)

    def set_checkbox_in_table(self, item, checked):
        if checked:
            item.setCheckState(QtCore.Qt.Checked)
        else:
            item.setCheckState(QtCore.Qt.Unchecked)

    def set_combo_box_index(self, widget, text):
        index = widget.findText(text)
        widget.setCurrentIndex(index)

    def add_pair_name_entry(self, row, col, entry):
        """Add a validated pair name entry."""
        pair_name_widget = table_utils.ValidatedTableItem(self.validate_pair_name_entry)
        pair_name_widget.setText(entry)
        self.set_item_in_table(row, col, pair_name_widget, flags=self.get_default_item_flags)

    def add_group_selector_entry(self, row, col, entry, group):
        """Add a group selector widget for group_1 or group_2."""
        group_selector_widget = self.group_selection_cell_widget()

        if group == "group_1":
            group_selector_widget.currentIndexChanged.connect(lambda: self.on_cell_changed(row, 2))
        elif group == "group_2":
            group_selector_widget.currentIndexChanged.connect(lambda: self.on_cell_changed(row, 3))

        self.set_combo_box_index(group_selector_widget, entry)
        self.set_widget_in_table(row, col, group_selector_widget)

    def add_alpha_entry(self, row, col, entry):
        """Add an alpha entry to the table."""
        alpha_widget = table_utils.ValidatedTableItem(self.validate_alpha)
        alpha_widget.setText(entry)
        self.set_item_in_table(row, col, alpha_widget)

    def add_to_analyse_checkbox(self, item, entry):
        """Set the 'to_analyse' checkbox state."""
        self.set_checkbox_in_table(item, checked=bool(entry))

    def add_guess_alpha_button(self, row):
        """Add the 'Guess Alpha' button to the last column."""
        guess_alpha_widget = self.guess_alpha_button()
        guess_alpha_widget.clicked.connect(self.guess_alpha_clicked_from_row)
        self.set_widget_in_table(row, 5, guess_alpha_widget)

    @property
    def get_default_item_flags(self):
        return QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled

    def group_selection_cell_widget(self):
        # The widget for the group selection columns
        selector = QtWidgets.QComboBox(self)
        selector.setToolTip("Select a group from the grouping table")
        selector.addItems(self._group_selections)
        return selector

    def guess_alpha_button(self):
        # The widget for the guess alpha column
        guess_alpha = QtWidgets.QPushButton(self)
        guess_alpha.setToolTip("Estimate the alpha value for this pair")
        guess_alpha.setText("Guess")
        return guess_alpha

    def guess_alpha_clicked_from_row(self):
        self._on_guess_alpha_clicked(self.pairing_table.currentIndex().row())

    def get_table_item(self, row, col):
        return self.pairing_table.item(row, col)

    def set_widget_enabled(self, row, col, enabled):
        cell_widget = self.pairing_table.cellWidget(row, col)
        cell_widget.setEnabled(enabled)

    def set_item_selectable(self, row, col):
        item = self.get_table_item(row, col)
        item.setFlags(QtCore.Qt.ItemIsSelectable)

    def set_item_selectable_and_enabled(self, row, col):
        item = self.get_table_item(row, col)
        item.setFlags(self.get_default_item_flags)

    def set_item_editable_and_enabled(self, row, col):
        item = self.get_table_item(row, col)
        item.setFlags(QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsEnabled)

    def set_item_checkable_and_enabled(self, row, col):
        item = self.get_table_item(row, col)
        item.setFlags(QtCore.Qt.ItemIsUserCheckable | QtCore.Qt.ItemIsEnabled)

    def get_widget_current_text(self, row, col):
        cell_widget = self.pairing_table.cellWidget(row, col)
        return str(cell_widget.currentText()) if cell_widget else ""

    def get_item_text(self, row, col):
        item = self.get_table_item(row, col)
        return str(item.text()) if item else ""

    # ------------------------------------------------------------------------------------------------------------------
    # Signal / Slot connections
    # ------------------------------------------------------------------------------------------------------------------

    def on_user_changes_pair_name(self, slot):
        self._validate_pair_name_entry = slot

    def on_user_changes_alpha(self, slot):
        self._validate_alpha = slot

    def on_guess_alpha_clicked(self, slot):
        self._on_guess_alpha_clicked = slot

    def on_add_pair_button_clicked(self, slot):
        self.add_pair_button.clicked.connect(slot)

    def on_remove_pair_button_clicked(self, slot):
        self.remove_pair_button.clicked.connect(slot)

    def on_table_data_changed(self, slot):
        self._on_table_data_changed = slot

    def on_item_changed(self):
        """Not yet implemented."""
        if not self._updating:
            pass

    def on_cell_changed(self, row, column):
        if not self._updating:
            if not (isinstance(self.sender(), QtWidgets.QTableWidget) or self.sender().pos().isNull()):
                pos_index = self.pairing_table.indexAt(self.sender().pos())
                row = pos_index.row()
                column = pos_index.column()
            self._on_table_data_changed(row, column)

    # ------------------------------------------------------------------------------------------------------------------
    # Context Menu
    # ------------------------------------------------------------------------------------------------------------------
    def create_context_menu(self):
        self.menu = QtWidgets.QMenu(self)
        return self.menu

    @property
    def cursor_position(self):
        return QtGui.QCursor.pos()

    # ------------------------------------------------------------------------------------------------------------------
    # Context Menu
    # ------------------------------------------------------------------------------------------------------------------
    def contextMenuEvent(self, _event):
        """Overridden method for dealing with the right-click context menu"""
        menu = self.create_context_menu()

        add_pair_action = self._context_menu_add_pair_action(self.add_pair_button.clicked.emit)
        remove_pair_action = self._context_menu_remove_pair_action(self.remove_pair_button.clicked.emit)

        if self.is_disabled:
            self.add_pair_action.setEnabled(False)
            self.remove_pair_action.setEnabled(False)

        # set-up the menu
        menu.addAction(add_pair_action)
        menu.addAction(remove_pair_action)
        menu.popup(self.cursor_position)

    def _context_menu_add_pair_action(self, slot):
        add_pair_action = self.get_add_pair_action()
        add_pair_action.setCheckable(False)
        if len(self._presenter.get_selected_row_indices()) > 0:
            add_pair_action.setEnabled(False)
        add_pair_action.triggered.connect(slot)
        return add_pair_action

    def _context_menu_remove_pair_action(self, slot):
        if len(self._presenter.get_selected_row_indices()) > 1:
            # use plural if >1 item selected
            remove_pair_action = self.get_remove_pair_action(True)
        else:
            remove_pair_action = self.get_remove_pair_action()
        if self.get_pairing_table.rowCount() == 0:
            remove_pair_action.setEnabled(False)
        remove_pair_action.triggered.connect(slot)
        return remove_pair_action

    # ------------------------------------------------------------------------------------------------------------------
    # Adding / Removing pairs
    # ------------------------------------------------------------------------------------------------------------------
    def get_add_pair_action(self):
        self.add_pair_action = QtWidgets.QAction("Add Pair", self)
        return self.add_pair_action

    def get_remove_pair_action(self, plural=False):
        """
        Create and return the 'Remove Pair' or 'Remove Pairs' QAction based on the plural flag.
        """
        if plural:
            self.remove_pair_action = QtWidgets.QAction("Remove Pairs", self)
            return self.remove_pair_action
        else:
            self.remove_pair_action = QtWidgets.QAction("Remove Pair", self)
            return self.remove_pair_action

    def enter_pair_name(self):
        new_pair_name, ok = QtWidgets.QInputDialog.getText(self, "Pair Name", "Enter name of new pair:")
        if ok:
            return new_pair_name

    def enable_all_buttons(self):
        self.add_pair_button.setEnabled(True)
        self.remove_pair_button.setEnabled(True)

    def disable_all_buttons(self):
        self.add_pair_button.setEnabled(False)
        self.remove_pair_button.setEnabled(False)

    def set_to_analyse_state(self, row, state):
        checked_state = QtCore.Qt.Checked if state is True else QtCore.Qt.Unchecked
        item = self.get_table_item(row, inverse_pair_columns["to_analyse"])
        item.setCheckState(checked_state)

    def set_to_analyse_state_quietly(self, row, state):
        checked_state = QtCore.Qt.Checked if state is True else QtCore.Qt.Unchecked
        item = self.get_table_item(row, 1)
        self.pairing_table.blockSignals(True)
        item.setCheckState(checked_state)
        self.pairing_table.blockSignals(False)
