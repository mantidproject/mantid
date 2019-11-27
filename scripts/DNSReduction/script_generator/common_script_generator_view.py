# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Path Configuration Widget = View - Tab of DNS Reduction GUI
"""
from __future__ import (absolute_import, division, print_function)

from qtpy.QtWidgets import QProgressDialog
from qtpy.QtCore import Signal, Qt

from DNSReduction.data_structures.dns_view import DNSView

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantidplot import load_ui


class DNSScriptGenerator_view(DNSView):
    """
        Common Widget for DNS Script generator, shows mantid script
    """
    def __init__(self, parent):
        super(DNSScriptGenerator_view, self).__init__(parent)
        self.name = "Script generator"
        self._content = load_ui(__file__,
                                'script_generator.ui',
                                baseinstance=self)
        self._mapping = {
            'script_filename': self._content.lE_filename,
            'generate_script': self._content.pB_generate_script,
            'copy_script': self._content.pB_copy_to_clipboard,
            'script_output': self._content.tE_script_output,
        }
        self.progress = None

        ## self connect signals
        self._mapping['generate_script'].clicked.connect(self.generate_script)
        self._mapping['copy_script'].clicked.connect(self.copy_to_clip)


## Signals

    sig_generate_script = Signal()
    sig_progress_canceled = Signal()

    def copy_to_clip(self):
        self._mapping['script_output'].selectAll()
        self._mapping['script_output'].copy()
        text_cursor = self._mapping['script_output'].textCursor()
        text_cursor.clearSelection()
        self._mapping['script_output'].setTextCursor(text_cursor)

    def generate_script(self):
        self.sig_generate_script.emit()

    def open_progress_dialog(self, numberofsteps):
        self.progress = QProgressDialog("Script running please wait",
                                        "Abort Loading", 0, numberofsteps)
        self.progress.setWindowModality(Qt.WindowModal)
        self.progress.setMinimumDuration(200)
        self.progress.open(self.progress_canceled)

    def progress_canceled(self):
        self.sig_progress_canceled.emit()

    def set_filename(self, filename='script.py'):
        self._mapping['script_filename'].setText(filename)

    def set_progress(self, step):
        self.progress.setValue(step)

    def set_script_output(self, scripttext):
        self._mapping['script_output'].setPlainText(scripttext)

    def set_state(self, state_dict):
        """
        sets the gui state from a dictionary containing the shortnames of
        the widgets as keys and the values
        """
        self.set_script_output(state_dict.get('script_text', ''))
        for key, target_object in self._mapping.items():
            self.set_single_state(target_object,
                                  value=state_dict.get(key, None))
        return
