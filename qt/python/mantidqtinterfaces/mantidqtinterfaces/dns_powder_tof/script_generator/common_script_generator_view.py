# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Path Configuration Widget = View - Tab of DNS Reduction GUI
"""
from mantidqt.utils.qt import load_ui
from qtpy.QtCore import Qt, Signal
from qtpy.QtWidgets import QProgressDialog
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view import DNSView


class DNSScriptGeneratorView(DNSView):
    """
        Common Widget for DNS Script generator, shows mantid script
    """
    NAME = "Script generator"

    def __init__(self, parent):
        super().__init__(parent)
        content = load_ui(__file__, 'script_generator.ui', baseinstance=self)
        self._map = {
            'script_filename': content.lE_filename,
            'automatic_filename': content.cB_automatic_name,
            'generate_script': content.pB_generate_script,
            'copy_script': content.pB_copy_to_clipboard,
            'script_output': content.tE_script_output,
        }
        self.progress = None

        # self connect signals
        self._map['generate_script'].clicked.connect(self._generate_script)
        self._map['copy_script'].clicked.connect(self._copy_to_clip)
        self._map['automatic_filename'].stateChanged.connect(self._autom)

    # Signals

    sig_generate_script = Signal()
    sig_progress_canceled = Signal()

    def _autom(self, on):
        self._map['script_filename'].setReadOnly(on)

    def _copy_to_clip(self):
        self._map['script_output'].selectAll()
        self._map['script_output'].copy()
        text_cursor = self._map['script_output'].textCursor()
        text_cursor.clearSelection()
        self._map['script_output'].setTextCursor(text_cursor)

    def _generate_script(self):
        self.sig_generate_script.emit()

    def _progress_canceled(self):
        self.sig_progress_canceled.emit()

    def open_progress_dialog(self, numberofsteps):
        self.progress = QProgressDialog("Script running please wait",
                                        "Abort Loading", 0, numberofsteps)
        self.progress.setWindowModality(Qt.WindowModal)
        self.progress.setMinimumDuration(200)
        self.progress.open(self._progress_canceled)

    def set_filename(self, filename='script.py'):
        self._map['script_filename'].setText(filename)

    def set_progress(self, step):
        self.progress.setValue(step)

    def set_script_output(self, scripttext):
        self._map['script_output'].setPlainText(scripttext)

    def set_state(self, state_dict):
        """
        sets the gui state from a dictionary containing the shortnames of
        the widgets as keys and the values
        """
        self.set_script_output(state_dict.get('script_text', ''))
        for key, target_object in self._map.items():
            self.set_single_state(target_object,
                                  value=state_dict.get(key, None))
