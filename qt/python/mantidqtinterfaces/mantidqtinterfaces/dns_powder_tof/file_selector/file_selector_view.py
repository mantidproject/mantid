# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS File selector View - Tab of DNS Reduction GUI.
"""

from mantidqt.utils.qt import load_ui
from qtpy.QtCore import QModelIndex, Qt, Signal
from qtpy.QtWidgets import QProgressDialog
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view import DNSView


class DNSFileSelectorView(DNSView):
    """
    Lets user select DNS data files for further reduction.
    """
    NAME = 'Data'

    def __init__(self, parent):
        super().__init__(parent)
        self._content = load_ui(__file__,
                                'file_selector.ui',
                                baseinstance=self)

        self._sample_treeview = self._content.DNS_sample_view
        self._sample_treeview.setUniformRowHeights(True)
        self._treeview = self._sample_treeview
        self._standard_treeview = self._content.DNS_standard_view
        self._standard_treeview.setUniformRowHeights(True)

        self._map = {
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

        self._treeview.setContextMenuPolicy(Qt.CustomContextMenu)
        self._treeview.customContextMenuRequested.connect(
            self._treeview_clicked)
        self._standard_treeview.setContextMenuPolicy(Qt.CustomContextMenu)
        self._standard_treeview.customContextMenuRequested.connect(
            self._treeview_clicked)

        # buttons
        self._content.pB_td_read_all.clicked.connect(self._read_all_clicked)
        self._content.pB_td_read_filtered.clicked.connect(
            self._read_filtered_clicked)
        self._content.cB_filter_det_rot.stateChanged.connect(
            self._filter_scans_checked)
        self._content.cB_filter_sample_rot.stateChanged.connect(
            self._filter_scans_checked)
        self._content.cB_filter_scans.stateChanged.connect(
            self._filter_scans_checked)
        self._content.cB_filter_cscans.stateChanged.connect(
            self._filter_scans_checked)
        self._content.cB_filter_free.stateChanged.connect(
            self._filter_scans_checked)
        self._content.lE_filter_free_text.textChanged.connect(
            self._filter_scans_checked)
        self._content.pB_expand_all.clicked.connect(self.expand_all)
        self._content.pB_expand_none.clicked.connect(self._un_expand_all)
        self._content.pB_check_all.clicked.connect(self._check_all)
        self._content.pB_check_none.clicked.connect(self._uncheck_all)
        self._content.pB_check_last_scan.clicked.connect(self._check_last)
        self._content.pB_check_last_complete_scan.clicked.connect(
            self._check_last)
        self._content.pB_check_selected.clicked.connect(self._check_selected)

        # Checkboxes
        self._map['filter_vanadium'].stateChanged.connect(
            self._filter_standard_checked)
        self._map['filter_nicr'].stateChanged.connect(
            self._filter_standard_checked)
        self._map['filter_empty'].stateChanged.connect(
            self._filter_standard_checked)
        self._map['autoload'].stateChanged.connect(self._autoload_checked)

        # combo box
        self._content.combB_directory.currentIndexChanged.connect(
            self.combo_changed)

        self._content.groupBox_filter_standard.setHidden(1)
        self._standard_treeview.setHidden(1)
        self.progress = None
        self.combo_changed(0)

    # signals
    sig_read_all = Signal()
    sig_read_filtered = Signal()
    sig_filters_clicked = Signal()

    sig_check_all = Signal()
    sig_uncheck_all = Signal()
    sig_check_selected = Signal()
    sig_check_last = Signal(str)

    sig_progress_canceled = Signal()
    sig_autoload_clicked = Signal(int)
    sig_dataset_changed = Signal(int)
    sig_standard_filters_clicked = Signal()
    sig_right_click = Signal(QModelIndex)

    # signal reactions
    def _treeview_clicked(self, point):
        self.sig_right_click.emit(self._treeview.indexAt(point))

    def _autoload_checked(self, state):
        self.sig_autoload_clicked.emit(state)

    def _check_all(self):
        self.sig_check_all.emit()

    def _uncheck_all(self):
        self.sig_uncheck_all.emit()

    def _check_selected(self):
        self.sig_check_selected.emit()

    def _check_last(self):
        sender_name = self.sender().objectName()
        self.sig_check_last.emit(sender_name)

    def combo_changed(self, index):
        self._content.groupBox_filter.setHidden(index)
        self._content.pB_check_last_scan.setHidden(index)
        self._content.pB_check_last_complete_scan.setHidden(index)
        self._content.sB_last_scans.setHidden(index)
        self._content.cB_autoload.setHidden(index)
        self._content.groupBox_filter_standard.setHidden(1 - index)
        self._standard_treeview.setHidden(1 - index)
        self.cB_auto_standard.setHidden(1 - index)
        self._sample_treeview.setHidden(index)
        if index:
            self._treeview = self._standard_treeview
        else:
            self._treeview = self._sample_treeview
        self.sig_dataset_changed.emit(index)

    def _un_expand_all(self):
        self._treeview.collapseAll()

    # public can be called from presenter
    def expand_all(self):
        self._treeview.expandAll()

    def _filter_scans_checked(self):
        self.sig_filters_clicked.emit()

    def _filter_standard_checked(self):
        self.sig_standard_filters_clicked.emit()

    def _read_all_clicked(self):
        self.sig_read_all.emit()

    def _read_filtered_clicked(self):
        self.sig_read_filtered.emit()

    # get states
    def get_filters(self):
        """
        Returning chosen filters which should be applied to the list of scans.
        """
        state_dict = self.get_state()
        free_text = state_dict['filter_free_text']
        filters = {
            'det_rot': state_dict['filter_det_rot'],
            'sample_rot': state_dict['filter_sample_rot'],
            ' scan': state_dict['filter_scans'],
            # space is important to not get cscans
            'cscan': state_dict['filter_cscans'],
            free_text: state_dict['filter_free'],
        }
        if filters[' scan'] and filters['cscan']:
            filters['scan'] = True
            filters.pop(' scan')
            filters.pop('cscan')
        return filters

    def get_nb_scans_to_check(self):
        return self._content.sB_last_scans.value()

    def get_selected_indexes(self):
        return self._treeview.selectedIndexes()

    def get_standard_filters(self):
        state_dict = self.get_state()
        filters = {
            'vanadium': state_dict['filter_vanadium'],
            'nicr': state_dict['filter_nicr'],
            'empty': state_dict['filter_empty'],
        }
        return filters

    def get_start_end_file_numbers(self):
        start = self._map['file_nb'].value()
        end = self._map['file_to'].value()
        return [start, end]

    def hide_scan(self, row, hidden=True):
        self._treeview.setRowHidden(row, self._treeview.rootIndex(), hidden)

    def show_scan(self, row):
        self._treeview.setRowHidden(row, self._treeview.rootIndex(), False)

    def hide_tof(self, hidden=True):
        self._standard_treeview.setColumnHidden(7, hidden)
        self._standard_treeview.setColumnHidden(8, hidden)
        self._sample_treeview.setColumnHidden(7, hidden)
        self._sample_treeview.setColumnHidden(8, hidden)

    def is_scan_hidden(self, row):
        return self._treeview.isRowHidden(row, self._treeview.rootIndex())

    # progress dialog
    def open_progress_dialog(self, num_of_steps):
        if num_of_steps:
            self.progress = QProgressDialog(
                f"Loading {num_of_steps} files...", "Abort Loading", 0,
                num_of_steps)
            self.progress.setWindowModality(Qt.WindowModal)
            self.progress.setMinimumDuration(200)
            self.progress.open(self._progress_canceled)

    def _progress_canceled(self):
        self.sig_progress_canceled.emit()

    def set_progress(self, step):
        self.progress.setValue(step)

    # manipulating view
    def set_first_column_spanned(self, scan_range):
        for i in scan_range:
            self._treeview.setFirstColumnSpanned(i, self._treeview.rootIndex(),
                                                 True)

    def set_start_end_file_numbers_from_arguments(self, start, end):
        self.set_single_state(self._map['file_nb'], start)
        self.set_single_state(self._map['file_to'], end)

    def set_tree_model(self, model, standard=False):
        if standard:
            self._standard_treeview.setModel(model)
        else:
            self._sample_treeview.setModel(model)

    def adjust_treeview_columns_width(self, num_columns):
        for i in range(num_columns):
            self._treeview.resizeColumnToContents(i)
