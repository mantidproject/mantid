# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS path tab view of DNS reduction GUI.
"""

from mantidqt.utils.qt import load_ui
from qtpy.QtCore import Signal
from qtpy.QtWidgets import QFileDialog
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view import DNSView


class DNSPathView(DNSView):
    """
    Widget that lets user select DNS data directories.
    """

    NAME = "Paths"

    def __init__(self, parent):
        super().__init__(parent)
        self._ui = load_ui(__file__, "path.ui", baseinstance=self)

        self._map = {
            "data_dir": self._ui.lE_data_dir,
            "data_dir_asterisk": self._ui.l_data_dir_req,
            "psd_dir": self._ui.lE_psd_dir,
            "user": self._ui.lE_user,
            "prop_nb": self._ui.lE_prop_nb,
            "standards_dir": self._ui.lE_standards_dir,
            "script_dir": self._ui.lE_script_dir,
            "export_dir": self._ui.lE_export_dir,
            "nexus": self._ui.cB_nexus,
            "ascii": self._ui.cB_ascii,
            "export": self._ui.gB_export,
            "auto_set_other_dir": self._ui.cB_auto_set_other_dir,
        }
        # connect signals
        self._attach_signal_slots()

    # signals
    sig_data_path_is_set = Signal(str)
    sig_clear_cache = Signal()
    sig_file_dialog_requested = Signal(str)
    sig_data_dir_editing_finished = Signal()

    def _file_dialog(self):
        sender = self.sender().objectName()[8:]
        self.sig_file_dialog_requested.emit(sender)

    def open_file_dialog(self, start_path):
        dir_name = QFileDialog.getExistingDirectory(self, "Select folder", start_path, QFileDialog.ShowDirsOnly)
        return dir_name

    def set_data_path(self, dir_name):
        self.set_path("data_dir", dir_name)
        self.sig_data_path_is_set.emit(dir_name)

    def _clear_cache(self):
        self.sig_clear_cache.emit()

    def _clear_user_and_proposal_number(self):
        self._map["user"].setText("")
        self._map["prop_nb"].setText("")

    def _toggle_editable_directories(self):
        state = not self._map["auto_set_other_dir"].checkState()
        self._map["standards_dir"].setEnabled(state)
        self._map["script_dir"].setEnabled(state)
        self._map["psd_dir"].setEnabled(state)
        self._map["export_dir"].setEnabled(state)
        self._ui.pB_file_psd.setEnabled(state)
        self._ui.pB_file_standards.setEnabled(state)
        self._ui.pB_file_script.setEnabled(state)
        self._ui.pB_export.setEnabled(state)

    def get_path(self, path_type):
        return self._map[path_type].text()

    def get_prop_number(self):
        return self._map["prop_nb"].text()

    def get_user(self):
        return self._map["user"].text()

    def set_path(self, path_type, directory):
        self._map[path_type].setText(directory)

    def set_prop_number(self, prop_nb):
        self._map["prop_nb"].setText(prop_nb)

    def set_user(self, user):
        self._map["user"].setText(user)
        self._map["user"].setCursorPosition(0)

    def set_asterisk(self, hide):
        if hide:
            self._map["data_dir_asterisk"].setText(" ")
        else:
            self._map["data_dir_asterisk"].setText("*")

    def hide_save(self, hide=True):
        self._map["nexus"].setVisible(not hide)
        self._map["ascii"].setVisible(not hide)

    def _attach_signal_slots(self):
        self._ui.cB_auto_set_other_dir.stateChanged.connect(self._toggle_editable_directories)
        self._ui.pB_file_data.clicked.connect(self._file_dialog)
        self._ui.pB_file_psd.clicked.connect(self._file_dialog)
        self._ui.pB_file_standards.clicked.connect(self._file_dialog)
        self._ui.pB_file_script.clicked.connect(self._file_dialog)
        self._ui.pB_export.clicked.connect(self._file_dialog)
        self._ui.pB_clear_cache.clicked.connect(self._clear_cache)
