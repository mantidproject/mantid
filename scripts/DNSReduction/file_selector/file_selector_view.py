# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS File selector Widget = View - Tab of DNS Reduction GUI
"""
from __future__ import (absolute_import, division, print_function)

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantidplot import load_ui

#from DNSReduction.helpers.mapping_creator import mapping_creator

from qtpy.QtWidgets import QProgressDialog
from qtpy.QtCore import Qt, Signal

from DNSReduction.data_structures.dns_view import DNSView


class DNSFileSelector_view(DNSView):
    """
        Widget that lets user select DNS data directories
    """
    def __init__(self, parent):
        super(DNSFileSelector_view, self).__init__(parent)
        self.name = 'Data'
        self._content = load_ui(__file__,
                                'file_selector.ui',
                                baseinstance=self)

        self.sample_treeview = self._content.DNS_sample_view
        self.sample_treeview.setUniformRowHeights(True)
        self.treeview = self.sample_treeview
        self.standard_treeview = self._content.DNS_standard_view
        self.standard_treeview.setUniformRowHeights(True)

        self._mapping = {
            'filter_scans': self._content.cB_filter_scans,
            'file_to': self._content.sB_td_file_to,
            'file_nb': self._content.sB_td_file_nb,
            'filter_free': self._content.cB_filter_free,
            'autoload': self._content.cB_autoload,
            'filter_free_text': self._content.lE_filter_free_text,
            'filter_empty': self._content.cB_filter_empty,
            'filter_cscans': self._content.cB_filter_cscans,
            'filter_sample_rot': self._content.cB_filter_sample_rot,
            'filter_nicr': self._content.cB_filter_nicr,
            'filter_vanadium': self._content.cB_filter_vanadium,
            'last_scans': self._content.sB_last_scans,
            'filter_det_rot': self._content.cB_filter_det_rot,
            'auto_standard': self._content.cB_auto_standard,
        }

        ## buttons
        self._content.pB_td_read_all.clicked.connect(self.read_all_clicked)
        self._content.pB_td_read_filtered.clicked.connect(
            self.read_all_clicked)
        self._content.cB_filter_det_rot.stateChanged.connect(
            self.filter_scans_checked)
        self._content.cB_filter_sample_rot.stateChanged.connect(
            self.filter_scans_checked)
        self._content.cB_filter_scans.stateChanged.connect(
            self.filter_scans_checked)
        self._content.cB_filter_cscans.stateChanged.connect(
            self.filter_scans_checked)
        self._content.cB_filter_free.stateChanged.connect(
            self.filter_scans_checked)
        self._content.lE_filter_free_text.textChanged.connect(
            self.filter_scans_checked)
        self._content.pB_expand_all.clicked.connect(self.expand)
        self._content.pB_expand_none.clicked.connect(self.expand)
        self._content.pB_check_all.clicked.connect(self.check_buttons_clicked)
        self._content.pB_check_none.clicked.connect(self.check_buttons_clicked)
        self._content.pB_check_last_scan.clicked.connect(
            self.check_buttons_clicked)
        self._content.pB_check_last_complete_scan.clicked.connect(
            self.check_buttons_clicked)
        self._content.pB_check_selected.clicked.connect(
            self.check_buttons_clicked)

        ## checkboxes
        self._mapping['filter_vanadium'].stateChanged.connect(
            self.filter_standard_checked)
        self._mapping['filter_nicr'].stateChanged.connect(
            self.filter_standard_checked)
        self._mapping['filter_empty'].stateChanged.connect(
            self.filter_standard_checked)
        self._mapping['autoload'].stateChanged.connect(self.autoload_checked)

        ## combo box
        self._content.combB_directory.currentIndexChanged.connect(
            self.combo_changed)

        self._content.groupBox_filter_standard.setHidden(1)
        self.standard_treeview.setHidden(1)
        self.progress = None
        self.standard_active = 0  ### 0 = sample, 1 = standard
        self.combo_changed(0)

    #### SIGNALS
    sig_read_all = Signal(bool)
    sig_filters_clicked = Signal()
    sig_checked_clicked = Signal(str)
    sig_progress_canceled = Signal()
    sig_autoload_clicked = Signal(int)
    sig_dataset_changed = Signal(int)
    sig_standard_filters_clicked = Signal()

    def autoload_checked(self, state):
        self.sig_autoload_clicked.emit(state)

    def check_buttons_clicked(self):
        sender_name = self.sender().objectName()
        self.sig_checked_clicked.emit(sender_name)

    def combo_changed(self, index):
        self._content.groupBox_filter.setHidden(index)
        self._content.pB_check_last_scan.setHidden(index)
        self._content.pB_check_last_complete_scan.setHidden(index)
        self._content.sB_last_scans.setHidden(index)
        self._content.sB_td_file_to.setHidden(index)
        self._content.sB_td_file_nb.setHidden(index)
        self._content.l_td_file_nb.setHidden(index)
        self._content.pB_td_read_filtered.setHidden(index)
        self._content.cB_autoload.setHidden(index)
        self._content.l_td_file_to.setHidden(index)
        self._content.groupBox_filter_standard.setHidden(1 - index)
        self.standard_treeview.setHidden(1 - index)
        self.cB_auto_standard.setHidden(1 - index)
        self.sample_treeview.setHidden(index)
        if index:
            self.treeview = self.standard_treeview
        else:
            self.treeview = self.sample_treeview
        self.sig_dataset_changed.emit(index)

    def expand(self, alle=False):
        if alle:
            self.treeview.expandAll()
            return
        sender = self.sender()
        if sender.objectName() == 'pB_expand_all':
            self.treeview.expandAll()
        else:
            self.treeview.collapseAll()

    def filter_scans_checked(self):
        self.sig_filters_clicked.emit()

    def filter_standard_checked(self):
        self.sig_standard_filters_clicked.emit()

    def get_filters(self):
        """
        Returning chosen filters which should be applied to the list of scans
        """
        state_dict = self.get_state()
        freetext = state_dict['filter_free_text']
        filters = {
            'det_rot': state_dict['filter_det_rot'],
            'sample_rot': state_dict['filter_sample_rot'],
            ' scan': state_dict['filter_scans'],
            ## space is important to not get cscans
            'cscan': state_dict['filter_cscans'],
            freetext: state_dict['filter_free'],
        }
        if filters[' scan'] and filters['cscan']:
            filters['scan'] = True
            filters.pop(' scan')
            filters.pop('cscan')
        return filters

    def get_nb_scans_to_check(self):
        return self._content.sB_last_scans.value()

    def get_selected_indexes(self):
        return self.treeview.selectedIndexes()

    def get_standard_filters(self):
        state_dict = self.get_state()
        filters = {
            'vanadium': state_dict['filter_vanadium'],
            'nicr': state_dict['filter_nicr'],
            'empty': state_dict['filter_empty'],
        }
        return filters

    def get_start_end_filenumbers(self):
        start = self._mapping['file_nb'].value()
        end = self._mapping['file_to'].value()
        return [start, end]

    def hide_file(self, child, scanindex, hidden=True):
        self.treeview.setRowHidden(child.row(), scanindex, hidden)

    def hide_scan(self, row, hidden=True):
        self.treeview.setRowHidden(row, self.treeview.rootIndex(), hidden)

    def hide_tof(self, hidden=True):
        self.standard_treeview.setColumnHidden(7, hidden)
        self.standard_treeview.setColumnHidden(8, hidden)
        self.sample_treeview.setColumnHidden(7, hidden)
        self.sample_treeview.setColumnHidden(8, hidden)

    def is_file_hidden(self, child, scanindex):
        return self.treeview.isRowHidden(child.row(), scanindex)

    def is_scan_hidden(self, row):
        return self.treeview.isRowHidden(row, self.treeview.rootIndex())

    def open_progress_dialog(self, numofsteps):
        self.progress = QProgressDialog(
            "Loading {} files...".format(numofsteps),
            "Abort Loading",
            0,
            numofsteps)
        self.progress.setWindowModality(Qt.WindowModal)
        self.progress.setMinimumDuration(200)
        self.progress.open(self.progress_canceled)

    def progress_canceled(self):
        self.sig_progress_canceled.emit()

    def read_all_clicked(self):
        filtered = False
        if self.sender().objectName() == 'pB_td_read_filtered':
            filtered = True
        self.sig_read_all.emit(filtered)

    def set_first_column_spanned(self, scanrange):
        for i in scanrange:
            self.treeview.setFirstColumnSpanned(i, self.treeview.rootIndex(),
                                                True)

    def set_progress(self, step):
        self.progress.setValue(step)

    def set_start_end_filenumbers(self, alldatafiles):
        """
        called by read all, set the filenumber of the first and last file
        """
        start = int(alldatafiles[0].split('_')[-2][:-2])
        end = int(alldatafiles[-1].split('_')[-2][:-2])
        self.set_single_state(self._mapping['file_nb'], start)
        self.set_single_state(self._mapping['file_to'], end)

    def set_tree_model(self, model, standard=False):
        if standard:
            self.standard_treeview.setModel(model)
        else:
            self.sample_treeview.setModel(model)
