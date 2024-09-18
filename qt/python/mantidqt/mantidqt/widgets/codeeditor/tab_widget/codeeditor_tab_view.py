# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from qtpy.QtCore import Qt
from qtpy.QtGui import QColor
from qtpy.QtWidgets import QAction, QHBoxLayout, QMenu, QPushButton, QSizePolicy, QTabWidget, QWidget

from mantidqt.icons import get_icon
from mantidqt.utils.qt import add_actions, create_action
from mantidqt.widgets.codeeditor.tab_widget.codeeditor_tab_presenter import CodeEditorTabPresenter

ABORT_BUTTON_RED_COLOR = QColor(230, 84, 80).name()

PLAY_BUTTON_GREEN_COLOR = QColor(73, 156, 84).name()


class CodeEditorTabWidget(QTabWidget):
    ABORT_BUTTON_OBJECT_NAME = "abort-button"
    NEW_EDITOR_PLUS_BTN_OBJECT_NAME = "plus-button"
    OPTIONS_BUTTON_OBJECT_NAME = "options-button"
    RUN_BUTTON_OBJECT_NAME = "run-button"
    SHOW_IN_EXPLORER_ACTION_OBJECT_NAME = "show-in-explorer-action"

    def __init__(self, parent=None, presenter=None):
        self.presenter = presenter if presenter else CodeEditorTabPresenter(self)
        super(CodeEditorTabWidget, self).__init__(parent)

        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setContextMenuPolicy(Qt.ActionsContextMenu)
        self.setMovable(True)
        self.setUsesScrollButtons(True)
        self.setTabsClosable(True)
        self.setDocumentMode(True)

        self.last_tab_clicked = 0

        self.setup_tabs_context_menu(parent)
        self.setup_options_actions(parent)

        # create a button to add new tabs
        plus_button = QPushButton(self)
        plus_button.setObjectName(self.NEW_EDITOR_PLUS_BTN_OBJECT_NAME)
        plus_button.clicked.connect(parent.plus_button_clicked)
        plus_button.setShortcut("Ctrl+N")
        plus_button.setIcon(get_icon("mdi.plus", "black", 1.2))
        self.setCornerWidget(plus_button, Qt.TopLeftCorner)

    def setup_tabs_context_menu(self, parent):
        """
        Setup the actions for the context menu (right click). These are handled by the presenter
        """
        self.tabBarClicked.connect(self.tab_was_clicked)

        show_in_explorer = QAction("Show in Explorer", self)
        show_in_explorer.setObjectName(self.SHOW_IN_EXPLORER_ACTION_OBJECT_NAME)
        show_in_explorer.triggered.connect(self.presenter.action_show_in_explorer)

        separator = QAction(self)
        separator.setSeparator(True)

        self.addAction(show_in_explorer)

    def setup_options_actions(self, parent):
        """
        Setup the actions for the Options menu. The actions are handled by the parent widget
        """
        container_widget = QWidget(self)
        layout = QHBoxLayout(container_widget)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        container_widget.setLayout(layout)
        self.setCornerWidget(container_widget, Qt.TopRightCorner)

        run_button = QPushButton(container_widget)
        run_button.setObjectName(self.RUN_BUTTON_OBJECT_NAME)
        run_button.setIcon(get_icon("mdi.play", PLAY_BUTTON_GREEN_COLOR, 1.6))
        run_button.clicked.connect(parent.execute_current_async)
        run_button.setToolTip("Run Ctrl+Enter")
        layout.addWidget(run_button)

        abort_button = QPushButton(container_widget)
        abort_button.setObjectName(self.ABORT_BUTTON_OBJECT_NAME)
        abort_button.setIcon(get_icon("mdi.square", ABORT_BUTTON_RED_COLOR, 1.1))
        abort_button.clicked.connect(parent.abort_current)
        abort_button.setToolTip("Abort Ctrl+D")
        layout.addWidget(abort_button)

        options_button = QPushButton(container_widget)
        options_button.setObjectName(self.OPTIONS_BUTTON_OBJECT_NAME)
        options_button.setSizePolicy(QSizePolicy.Maximum, QSizePolicy.Maximum)
        options_button.setText("Options")

        options_menu = QMenu("", self)
        options_button.setMenu(options_menu)
        layout.addWidget(options_button)

        self.tabCloseRequested.connect(parent.close_tab)

        run_action = create_action(
            self,
            "Run",
            on_triggered=parent.execute_current_async,
            shortcut=("Ctrl+Enter", "Ctrl+Return"),
            shortcut_context=Qt.ApplicationShortcut,
            shortcut_visible_in_context_menu=True,
        )

        run_all_action = create_action(
            self,
            "Run All",
            on_triggered=parent.execute_async,
            shortcut=("Ctrl+Shift+Enter", "Ctrl+Shift+Return"),
            shortcut_context=Qt.ApplicationShortcut,
            shortcut_visible_in_context_menu=True,
        )

        abort_action = create_action(
            self,
            "Abort",
            on_triggered=parent.abort_current,
            shortcut="Ctrl+D",
            shortcut_context=Qt.ApplicationShortcut,
            shortcut_visible_in_context_menu=True,
        )

        # menu action to toggle the find/replace dialog
        toggle_find_replace = create_action(
            self,
            "Find/Replace...",
            on_triggered=parent.toggle_find_replace_dialog,
            shortcut="Ctrl+F",
            shortcut_visible_in_context_menu=True,
        )

        toggle_comment_action = create_action(
            self,
            "Comment/Uncomment",
            on_triggered=parent.toggle_comment_current,
            shortcut="Ctrl+/",
            shortcut_context=Qt.ApplicationShortcut,
            shortcut_visible_in_context_menu=True,
        )

        tabs_to_spaces_action = create_action(
            self, "Tabs to Spaces", on_triggered=parent.tabs_to_spaces_current, shortcut_visible_in_context_menu=True
        )

        spaces_to_tabs_action = create_action(
            self, "Spaces to Tabs", on_triggered=parent.spaces_to_tabs_current, shortcut_visible_in_context_menu=True
        )

        toggle_whitespace_action = create_action(
            self, "Toggle Whitespace Visible", on_triggered=parent.toggle_whitespace_visible_all, shortcut_visible_in_context_menu=True
        )

        # Store actions for adding to menu bar; None will add a separator
        editor_actions = [
            run_action,
            run_all_action,
            abort_action,
            None,
            toggle_find_replace,
            None,
            toggle_comment_action,
            toggle_whitespace_action,
            None,
            tabs_to_spaces_action,
            spaces_to_tabs_action,
        ]

        add_actions(options_menu, editor_actions)

    def closeEvent(self, event):
        self.deleteLater()
        super(CodeEditorTabWidget, self).closeEvent(event)

    def mousePressEvent(self, event):
        # the user did not click on a tab at all, stop all further mouseEvents processing
        if self.last_tab_clicked < 0:
            return

        if Qt.MiddleButton == event.button():
            self.tabCloseRequested.emit(self.last_tab_clicked)

        QTabWidget(self).mousePressEvent(event)

    def tab_was_clicked(self, tab_index):
        self.last_tab_clicked = tab_index
