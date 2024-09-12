# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""
Main window for reduction UIs
"""

import sys
import os
import traceback
from mantidqt.gui_helper import get_qapplication
from qtpy.QtWidgets import QAction, QDialog, QFileDialog, QMainWindow, QMessageBox
from qtpy.QtCore import QCoreApplication, QFile, QFileInfo, QSettings
from mantid.kernel import Logger

# Check whether Mantid is available
CAN_REDUCE = False
try:
    CAN_REDUCE = True
    from mantid.kernel import ConfigService
except ImportError:
    pass
try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("ReductionGUI").information("Using legacy ui importer")
    from mantidplot import load_ui

unicode = str

STARTUP_WARNING = ""

if CAN_REDUCE:
    try:
        import reduction

        if os.path.splitext(os.path.basename(reduction.__file__))[0] == "reduction":
            home_dir = os.path.expanduser("~")
            if os.path.abspath(reduction.__file__).startswith(home_dir):
                STARTUP_WARNING = "The following file is in your home area, please delete it and restart Mantid:\n\n"
            else:
                STARTUP_WARNING = "If the following file is in your home area, please delete it and restart Mantid:\n\n"
            STARTUP_WARNING += os.path.abspath(reduction.__file__)
    except:
        STARTUP_WARNING = "Please contact the Mantid team with the following message:\n\n\n"
        STARTUP_WARNING += unicode(traceback.format_exc())

from mantidqtinterfaces.reduction_gui.instruments.instrument_factory import instrument_factory, INSTRUMENT_DICT
from mantidqtinterfaces.reduction_gui.settings.application_settings import GeneralSettings


class ReductionGUI(QMainWindow):
    def __init__(self, parent=None, window_flags=None, instrument=None, instrument_list=None):
        QMainWindow.__init__(self, parent)
        if window_flags:
            self.setWindowFlags(window_flags)
        self.ui = load_ui(__file__, "ui/reduction_main.ui", baseinstance=self)

        if STARTUP_WARNING:
            message = "The reduction application has problems starting:\n\n"
            message += STARTUP_WARNING
            QMessageBox.warning(self, "WARNING", message)

        # Application settings
        settings = QSettings(QCoreApplication.organizationName(), QCoreApplication.applicationName())

        # Name handle for the instrument
        if instrument is None:
            instrument = settings.value("instrument_name", "", type=unicode)
            if instrument_list is not None and instrument not in instrument_list:
                instrument = None

        self._instrument = instrument
        self._facility = None

        # List of allowed instrument
        self._instrument_list = instrument_list

        # Reduction interface
        self._interface = None

        # Recent files
        self._recent_files = settings.value("recent_files", [], type=list)
        if self._recent_files is None:  # An empty list saved to QSettings comes back as 'None'
            self._recent_files = []

        # Folder to open files in
        self._last_directory = settings.value("last_directory", ".", type=unicode)
        self._last_export_directory = settings.value("last_export_directory", ".", type=unicode)

        # Current file name
        self._filename = None

        # Internal flag for clearing all settings and restarting the application
        self._clear_and_restart = False

        # General settings shared by all widgets
        self.general_settings = GeneralSettings(settings)

        # Event connections
        if not CAN_REDUCE:
            self.reduce_button.hide()
        self.export_button.clicked.connect(self._export)
        self.reduce_button.clicked.connect(self.reduce_clicked)
        self.save_button.clicked.connect(self._save)
        self.interface_chk.clicked.connect(self._interface_choice)

        self.interface_chk.setChecked(self.general_settings.advanced)

        self.general_settings.progress.connect(self._progress_updated)

    def _set_window_title(self):
        """
        Sets the window title using the instrument name and the
        current settings file
        """
        title = "%s Reduction" % self._instrument
        if self._filename is not None:
            title += ": %s" % self._filename
        self.setWindowTitle(title)

    def _progress_updated(self, value):
        self.progress_bar.setValue(value)

    def setup_layout(self, load_last=False):
        """
        Sets up the instrument-specific part of the UI layout
        """
        # Clean up the widgets that have already been created
        self.tabWidget.clear()
        self.progress_bar.hide()

        if self._instrument == "" or self._instrument is None:
            return self._change_instrument()

        self._update_file_menu()

        if self._interface is not None:
            self._interface.destroy()

        self.general_settings.instrument_name = self._instrument
        # Find corresponding facility
        if self._facility is None:
            for facility in INSTRUMENT_DICT.keys():
                if self._instrument in INSTRUMENT_DICT[facility].keys():
                    self._facility = facility
                    break
        if self._facility is None:
            self._facility = ConfigService.Instance().getFacility().name()

        self.general_settings.facility_name = self._facility
        self._interface = instrument_factory(self._instrument, settings=self.general_settings)

        if self._interface is not None:
            tab_list = self._interface.get_tabs()
            for tab in tab_list:
                self.tabWidget.addTab(tab[1], tab[0])
            self._set_window_title()

            # Show the "advanced interface" check box if needed
            if self._interface.has_advanced_version():
                self.interface_chk.show()
            else:
                self.interface_chk.hide()

            if load_last:
                self._interface.load_last_reduction()
        else:
            print("Could not generate an interface for instrument %s" % self._instrument)
            self.close()

        return True

    def _update_file_menu(self):
        """
        Set up the File menu and update the menu with recent files
        """
        self.file_menu.clear()

        newAction = QAction("&New Reduction...", self)
        newAction.setShortcut("Ctrl+N")
        newAction.setStatusTip("Start a new reduction")
        newAction.triggered.connect(self._new)

        openAction = QAction("&Open...", self)
        openAction.setShortcut("Ctrl+O")
        openAction.setStatusTip("Open an XML file containing reduction parameters")
        openAction.triggered.connect(self._file_open)

        saveAsAction = QAction("Save as...", self)
        saveAsAction.setStatusTip("Save the reduction parameters to XML")
        saveAsAction.triggered.connect(self._save_as)

        saveAction = QAction("&Save...", self)
        saveAction.setShortcut("Ctrl+S")
        saveAction.setStatusTip("Save the reduction parameters to XML")
        saveAction.triggered.connect(self._save)

        exportAction = QAction("&Export...", self)
        exportAction.setShortcut("Ctrl+E")
        exportAction.setStatusTip("Export to python script for Mantid")
        exportAction.triggered.connect(self._export)

        quitAction = QAction("&Quit", self)
        quitAction.setShortcut("Ctrl+Q")
        quitAction.triggered.connect(self.close)

        self.file_menu.addAction(newAction)
        self.file_menu.addAction(openAction)
        self.file_menu.addAction(saveAction)
        self.file_menu.addAction(saveAsAction)
        self.file_menu.addAction(exportAction)
        self.file_menu.addSeparator()

        if self.general_settings.debug:
            clearAction = QAction("&Clear settings and quit", self)
            clearAction.setStatusTip("Restore initial application settings and close the application")
            clearAction.triggered.connect(self._clear_and_close)
            self.file_menu.addAction(clearAction)

        self.file_menu.addAction(quitAction)

        # TOOLS menu
        instrAction = QAction("Change &instrument...", self)
        instrAction.setShortcut("Ctrl+I")
        instrAction.setStatusTip("Select a new instrument")
        instrAction.triggered.connect(self._change_instrument)

        debug_menu_item_str = "Turn debug mode ON"
        if self.general_settings.debug:
            debug_menu_item_str = "Turn debug mode OFF"
        debugAction = QAction(debug_menu_item_str, self)
        debugAction.setStatusTip(debug_menu_item_str)
        debugAction.triggered.connect(self._debug_mode)

        self.tools_menu.clear()
        self.tools_menu.addAction(instrAction)
        self.tools_menu.addAction(debugAction)

        recent_files = []
        for fname in self._recent_files:
            if fname != self._filename and QFile.exists(fname) and fname not in recent_files:
                recent_files.append(fname)

        if len(recent_files) > 0:
            self.file_menu.addSeparator()
            for i, fname in enumerate(recent_files):
                action = QAction("&%d %s" % (i + 1, QFileInfo(fname).fileName()), self)
                action.setData(fname)
                action.triggered.connect(self.open_file)
                self.file_menu.addAction(action)

    def _debug_mode(self, mode=None):
        """
        Set debug mode
        @param mode: debug mode (True or False). If None, the debug mode will simply be flipped
        """
        if mode is None:
            mode = not self.general_settings.debug
        self.general_settings.debug = mode
        self._new()
        self.setup_layout()

    def _interface_choice(self, advanced_ui=None):
        if advanced_ui is None:
            advanced_ui = self.general_settings.advanced
        self.general_settings.advanced = advanced_ui
        self._new()
        self.setup_layout()

    def _change_instrument(self):
        """
        Invoke an instrument selection dialog
        """

        class InstrDialog(QDialog):
            def __init__(self, instrument_list=None):
                QDialog.__init__(self)
                self.ui = load_ui(__file__, "ui/instrument_dialog.ui", baseinstance=self)
                self.instrument_list = instrument_list
                self.instr_combo.clear()
                self.facility_combo.clear()
                facilities = sorted(
                    [fac for fac in INSTRUMENT_DICT.keys() if any([inst in INSTRUMENT_DICT[fac] for inst in instrument_list])]
                )
                facilities.reverse()
                for facility in facilities:
                    self.facility_combo.addItem(facility)

                self._facility_changed(facilities[0])
                self.facility_combo.activated.connect(self._facility_changed)

            def _facility_changed(self, facility):
                facility = self.facility_combo.currentText()
                self.instr_combo.clear()
                instr_list = sorted(INSTRUMENT_DICT[unicode(facility)].keys())
                for item in instr_list:
                    if self.instrument_list is None or item in self.instrument_list:
                        self.instr_combo.addItem(item)

        if self.general_settings.debug:
            dialog = InstrDialog()
        else:
            dialog = InstrDialog(self._instrument_list)
        dialog.exec_()
        if dialog.result() == 1:
            self._instrument = dialog.instr_combo.currentText()
            self._facility = dialog.facility_combo.currentText()
            self.setup_layout()
            self._new()
            return True
        else:
            self.close()
            return False

    def _clear_and_close(self):
        """
        Clear all QSettings parameters
        """
        self._clear_and_restart = True
        self.close()
        # If we make it here, the user canceled the close, which
        # means that we need to reset the clear&close flag so
        # that the state is properly saved on the next close.
        self._clear_and_restart = False

    def closeEvent(self, event):
        """
        Executed when the application closes
        """
        if False:
            reply = QMessageBox.question(
                self, "Message", "Are you sure you want to quit this application?", QMessageBox.Yes, QMessageBox.No
            )

            if reply == QMessageBox.Yes:
                event.accept()
            else:
                event.ignore()

        # Save application settings
        if self._clear_and_restart:
            self._clear_and_restart = False
            QSettings().clear()
        else:
            settings = QSettings()

            settings.setValue("instrument_name", self._instrument)
            settings.setValue("last_file", self._filename)
            settings.setValue("recent_files", self._recent_files)
            settings.setValue("last_directory", str(self._last_directory))
            settings.setValue("last_export_directory", str(self._last_export_directory))

    def reduce_clicked(self):
        """
        Create an object capable of using the information in the
        interface and turn it into a reduction process.
        """
        self.reduce_button.setEnabled(False)
        self.export_button.setEnabled(False)
        self.save_button.setEnabled(False)
        self.interface_chk.setEnabled(False)
        self.file_menu.setEnabled(False)
        self.tools_menu.setEnabled(False)

        if self._interface is not None:
            self._interface.reduce()

        self.reduce_button.setEnabled(True)
        self.export_button.setEnabled(True)
        self.save_button.setEnabled(True)
        self.interface_chk.setEnabled(True)
        self.file_menu.setEnabled(True)
        self.tools_menu.setEnabled(True)

    def open_file(self, file_path=None):
        """
        Open an XML file and populate the UI
        @param file_path: path to the file to be loaded
        """
        if file_path is None:
            action = self.sender()
            if isinstance(action, QAction):
                file_path = action.data()
        if not file_path:
            return

        # don't try to load if the file doesn't exist
        file_path = str(file_path)
        if not os.path.exists(file_path):
            return

        # Check whether the file describes the current instrument
        try:
            found_instrument = self._interface.scripter.verify_instrument(file_path)
        except:
            msg = (
                "The file you attempted to load doesn't have a recognized format:\n"
                + file_path
                + "\n\n"
                + "Please make sure it has been produced by this application."
            )
            QMessageBox.warning(self, "Error loading reduction parameter file", msg)
            print(sys.exc_info()[1])
            return

        if not found_instrument == self._instrument:
            self._instrument = found_instrument
            self.setup_layout()

        self.reduce_button.setEnabled(False)
        self.export_button.setEnabled(False)
        self.save_button.setEnabled(False)
        self.interface_chk.setEnabled(False)
        self._interface.load_file(file_path)
        self.reduce_button.setEnabled(True)
        self.export_button.setEnabled(True)
        self.save_button.setEnabled(True)
        self.interface_chk.setEnabled(True)

        self._filename = file_path
        self._update_file_menu()
        self._set_window_title()

        if file_path in self._recent_files:
            self._recent_files.remove(file_path)
        self._recent_files.insert(0, file_path)
        while len(self._recent_files) > 10:
            self._recent_files.pop()

    def _new(self, *argv):
        """
        Start new reduction
        """
        self._interface.reset()
        self._filename = None
        self._update_file_menu()
        self._set_window_title()

    def _file_open(self, *argv):
        """
        File chooser for loading UI parameters
        """
        fname = QFileDialog.getOpenFileName(
            self, "Reduction settings - Choose a settings file", self._last_directory, "Settings files (*.xml)"
        )
        if not fname:
            return

        if isinstance(fname, tuple):
            fname = fname[0]
        fname = str(QFileInfo(fname).filePath())
        # Store the location of the loaded file
        self._last_directory = str(QFileInfo(fname).path())
        self.open_file(fname)

    def _save(self):
        """
        Present a file dialog to the user and saves the content of the
        UI in XML format
        """
        if self._filename is None:
            self._save_as()
        else:
            try:
                self._interface.save_file(self._filename)
                self._update_file_menu()
                self.statusBar().showMessage("Saved as %s" % self._filename)
                self._set_window_title()
            except:
                # TODO: put this in a log window, and in a file
                print(sys.exc_info()[1])
                self.statusBar().showMessage("Failed to save %s" % self._filename)

    def _save_as(self):
        """
        Present a file dialog to the user and saves the content of
        the UI in XML format.
        """
        if self._filename is not None:
            fname = self._filename
        else:
            fname = self._instrument + "_"

        fname = QFileDialog.getSaveFileName(
            self, "Reduction settings - Save settings", self._last_directory + "/" + fname, "Settings files (*.xml)"
        )
        if not fname:
            return

        if isinstance(fname, tuple):
            fname = fname[0]
        fname = str(QFileInfo(fname).filePath())
        if not fname.endswith(".xml"):
            fname += ".xml"
        if fname in self._recent_files:
            self._recent_files.remove(fname)
        self._recent_files.insert(0, fname)
        while len(self._recent_files) > 10:
            self._recent_files.pop()
        self._last_directory = QFileInfo(fname).path()
        self._filename = fname
        self._save()

    def _export(self):
        """
        Exports the current content of the UI to a python script that can
        be run within MantidPlot
        """
        if self._interface is None:
            return

        fname = QFileDialog.getSaveFileName(self, "Mantid Python script - Save script", self._last_export_directory, "Python script (*.py)")
        if not fname:
            return

        if isinstance(fname, tuple):
            fname = fname[0]
        fname = str(fname)
        if not fname.endswith(".py"):
            fname += ".py"
        (folder, file_name) = os.path.split(fname)
        self._last_export_directory = folder
        script = self._interface.export(fname)
        if script is not None:
            self.statusBar().showMessage("Saved as %s" % fname)
        else:
            self.statusBar().showMessage("Could not save file")


# --------------------------------------------------------------------------------------------------------


def start():
    app, within_mantid = get_qapplication()

    app.setApplicationName("Mantid Reduction")
    reducer = ReductionGUI()
    reducer.setup_layout(load_last=True)
    reducer.show()
    if not within_mantid:
        sys.exit(app.exec_())


if __name__ == "__main__":
    start()
