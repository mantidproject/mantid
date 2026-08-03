# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
# system imports
import os.path as osp
from dataclasses import dataclass

# third-party library imports
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from mantid.kernel import logger
from mantidqt.widgets.codeeditor.multifileinterpreter import MultiPythonFileInterpreter
from workbench.config import CONF
from ..config import DEFAULT_SCRIPT_CONTENT
from ..config.fonts import text_font
from ..plugins.base import PluginWidget

# from mantidqt.utils.qt import toQSettings when readSettings/writeSettings are implemented


# Accepted extensions for drag-and-drop to editor
ACCEPTED_FILE_EXTENSIONS = [".py", ".pyw"]
# QSettings key for session tabs
TAB_SETTINGS_KEY = "Editors/SessionTabs"
ZOOM_LEVEL_KEY = "Editors/ZoomLevel"
ENABLE_COMPLETION_KEY = "Editors/completion_enabled"


@dataclass(frozen=True)
class EditorSettings:
    """Immutable editor state read from persistent settings."""

    session_tabs: tuple[str, ...]
    zoom_level: int


class MultiFileEditor(PluginWidget):
    """
    Provides the container for the widget containing the CodeEditors in the Workbench
    """

    def __init__(self, parent, font=None):
        super(MultiFileEditor, self).__init__(parent)
        if not font:
            font = text_font()

        completion_enabled = True
        if CONF.has(ENABLE_COMPLETION_KEY):
            completion_enabled = CONF.get(ENABLE_COMPLETION_KEY, type=bool)

        # layout
        self.editors = MultiPythonFileInterpreter(
            font=font, default_content=DEFAULT_SCRIPT_CONTENT, parent=self, completion_enabled=completion_enabled
        )
        layout = QVBoxLayout()
        layout.addWidget(self.editors)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)

        self.setAcceptDrops(True)

        # attributes
        self.tabs_open_on_closing = None
        self.editors_zoom_level = None

    def load_settings_from_config(self, config):
        self.editors.load_settings_from_config(config)

    def execute_current_async(self):
        """
        Executes the selection in the currently active code editor.
        This is used by MainWindow to execute a file after opening it.

        """
        return self.editors.execute_current_async()

    def execute_async(self):
        """
        Executes _everything_ in currently active code editor.
        :return:
        """
        return self.editors.execute_async()

    def restore_session_tabs(self, session_tabs):
        self.open_files_in_new_tabs(session_tabs, startup=True)
        self.editors.close_tab(0)  # close default empty tab

    # ----------- Plugin API --------------------

    def app_closing(self):
        """
        Tries to close all editors
        :return: True if editors can be closed, false if cancelled
        """
        self.tabs_open_on_closing = self.editors.tab_filepaths
        self.editors_zoom_level = self.editors.current_editor().editor.getZoom()
        return self.editors.close_all()

    def dragEnterEvent(self, event):
        data = event.mimeData()
        if data.hasText() and data.hasUrls():
            filepaths = [url.toLocalFile() for url in data.urls()]
            for filepath in filepaths:
                if osp.splitext(filepath)[1] in ACCEPTED_FILE_EXTENSIONS:
                    event.acceptProposedAction()

    def dropEvent(self, event):
        data = event.mimeData()
        for url in data.urls():
            filepath = url.toLocalFile()
            if osp.splitext(filepath)[1] in ACCEPTED_FILE_EXTENSIONS:
                try:
                    self.open_file_in_new_tab(filepath)
                except IOError as io_error:
                    logger.warning("Could not load file:\n  '{}'".format(io_error))

    def get_plugin_title(self):
        return "Editor"

    def readSettings(self, settings):
        """Read and return editor settings without changing the editor or store."""
        try:
            prev_session_tabs = settings.get(TAB_SETTINGS_KEY, type=list)
            zoom_level = settings.get(ZOOM_LEVEL_KEY, type=int)
        except (KeyError, TypeError):
            return None
        return EditorSettings(tuple(prev_session_tabs), zoom_level)

    def restoreSettings(self, settings):
        """Apply a previously read editor snapshot without persistent writes."""
        if settings is None:
            return
        self.restore_session_tabs(settings.session_tabs)
        self.editors.current_editor().editor.zoomTo(settings.zoom_level)

    def captureSettings(self):
        """Capture the editor state prepared during application shutdown."""
        return EditorSettings(tuple(dict.fromkeys(self.tabs_open_on_closing)), self.editors_zoom_level)

    def saveSettings(self, settings, values):
        """Persist an explicitly captured editor snapshot."""
        settings.set(ZOOM_LEVEL_KEY, values.zoom_level)
        settings.set(TAB_SETTINGS_KEY, list(values.session_tabs))

    def register_plugin(self):
        self.main.add_dockwidget(self)

    # ----------- Plugin Behaviour --------------------

    def open_file_in_new_tab(self, filepath, startup=False):
        return self.editors.open_file_in_new_tab(filepath, startup)

    def open_files_in_new_tabs(self, filepaths, startup=False):
        for filepath in filepaths:
            try:
                self.open_file_in_new_tab(filepath, startup)
            except IOError as io_error:
                logger.warning("Could not load file:\n  {}".format(io_error))

    def open_script_in_new_tab(self, content):
        self.editors.append_new_editor(content=content)
        self.editors.mark_current_tab_modified(True)

    def save_current_file(self):
        self.editors.save_current_file()

    def save_current_file_as(self):
        self.editors.save_current_file_as()
