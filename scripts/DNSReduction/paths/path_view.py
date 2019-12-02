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
from os.path import expanduser

from qtpy.QtWidgets import QFileDialog
from qtpy.QtCore import Signal

from DNSReduction.data_structures.dns_view import DNSView

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantidplot import load_ui
##from DNSReduction.DNSHelpers.mapping_creator import mapping_creator


class DNSPath_view(DNSView):
    """
        Widget that lets user select DNS data directories
    """
    def __init__(self, parent):
        super(DNSPath_view, self).__init__(parent)
        self.name = "Paths"
        self._content = load_ui(__file__, 'path.ui', baseinstance=self)

        self._mapping = {
            'data_dir': self._content.lE_data_dir,
            'logbook_dir': self._content.lE_logbook_dir,
            'psd_dir': self._content.lE_psd_dir,
            'user': self._content.lE_user,
            'prop_nb': self._content.lE_prop_nb,
            'standards_dir': self._content.lE_standards_dir,
            'script_dir': self._content.lE_script_dir,
        }

        ## connect signals
        self._content.pB_file_data.clicked.connect(self.filedialog)
        self._content.pB_file_psd.clicked.connect(self.filedialog)
        self._content.pB_file_logbook.clicked.connect(self.filedialog)
        self._content.pB_file_standards.clicked.connect(self.filedialog)
        self._content.pB_file_script.clicked.connect(self.filedialog)
        self._content.pB_clear.clicked.connect(self.clear_directories)

    ### for testing
        self._content.lE_data_dir.setText('C:/Daten/mantid_test')
        self._content.lE_standards_dir.setText('C:/Daten/mantid_test/standards')
        self._content.lE_script_dir.setText('C:/Daten/mantid_test/scripts')

    #self._content.lE_data_dir.setText('C:/data/p13656')


##Signals
    sig_data_path_set = Signal(str)

    def filedialog(self):
        """
        opens a file dialog to set data paths, if data directory is set
        others are tried to set automatically
        """
        sender = self.sender().objectName()
        if self.get_path('data_dir'):
            startpath = self.get_path('data_dir')
        else:
            startpath = expanduser("~")
        dir_name = QFileDialog.getExistingDirectory(
            self, "Select folder", startpath,
            QFileDialog.ShowDirsOnly | QFileDialog.DontUseNativeDialog)
        if dir_name:
            if sender == 'pB_file_data':
                self.set_path('data_dir', dir_name)
                self.sig_data_path_set.emit(dir_name)
                if not self.get_path('psd_dir'):
                    self.set_path('psd_dir', dir_name + '/psd')
                if not self.get_path('logbook_dir'):
                    self.set_path('logbook_dir', dir_name + '/logbook')
                if not self.get_path('standards_dir'):
                    self.set_path('standards_dir', dir_name + '/standards')
                if not self.get_path('script_dir'):
                    self.set_path('script_dir', dir_name + '/scripts')
            elif sender == 'pB_file_psd':
                self.set_path('psd_dir', dir_name)
            elif sender == 'pB_file_logbook':
                self.set_path('logbook_dir', dir_name)
            elif sender == 'pB_file_standards':
                self.set_path('standards_dir', dir_name)
            elif sender == 'pB_file_script':
                self.set_path('script_dir', dir_name)
        return dir_name

    def clear_directories(self):
        self.set_user('')
        self.set_prop_number('')
        self.set_path('logbook_dir', '')
        self.set_path('data_dir', '')
        self.set_path('standards_dir', '')
        self.set_path('script_dir', '')
        self.set_path('psd_dir', '')

    def get_path(self, pathtype):
        return self._mapping[pathtype].text()

    def get_prop_number(self):
        return self._mapping['prop_nb'].text()

    def get_user(self):
        return self._mapping['user'].text()

    def set_path(self, pathtype, directory):
        self._mapping[pathtype].setText(directory)
        return

    def set_prop_number(self, prop_nb):
        self._mapping['prop_nb'].setText(prop_nb)
        return

    def set_user(self, user):
        self._mapping['user'].setText(user)
