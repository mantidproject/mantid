# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS file selector tab view of DNS reduction GUI.
"""

from mantidqt.utils.qt import load_ui
from qtpy.QtCore import QModelIndex, Qt, Signal, QSignalBlocker
from qtpy.QtWidgets import QProgressDialog
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_view import DNSView
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_treeitem import TreeItemEnum

TREEVIEW_MAX_NUMBER_COLUMNS = 10


class DNSFileSelectorView(DNSView):
    """
    Lets user select DNS data files for further reduction.
    """

    NAME = "Data"

    def __init__(self, parent):
        super().__init__(parent)
        self._ui = load_ui(__file__, "file_selector.ui", baseinstance=self)

        self._sample_treeview = self._ui.DNS_sample_view
        self._sample_treeview.setUniformRowHeights(True)
        self._treeview = self._sample_treeview
        self._standard_treeview = self._ui.DNS_standard_view
        self._standard_treeview.setUniformRowHeights(True)

        self._map = {
            "filter_scans": self._ui.cB_filter_scans,
            "filter_free": self._ui.cB_filter_free,
            "autoload_new": self._ui.cB_autoload_new,
            "filter_free_text": self._ui.lE_filter_free_text,
            "filter_empty": self._ui.cB_filter_empty,
            "filter_cscans": self._ui.cB_filter_cscans,
            "filter_sample_rot": self._ui.cB_filter_sample_rot,
            "filter_nicr": self._ui.cB_filter_nicr,
            "filter_vanadium": self._ui.cB_filter_vanadium,
            "last_scans": self._ui.sB_last_scans,
            "filter_det_rot": self._ui.cB_filter_det_rot,
            "auto_select_standard": self._ui.cB_auto_select_standard,
            "standard_data_selector": self._ui.pB_standard_data,
            "sample_data_selector": self._ui.pB_sample_data,
        }

        self._sample_treeview.setContextMenuPolicy(Qt.CustomContextMenu)
        self._sample_treeview.customContextMenuRequested.connect(self._treeview_clicked)
        self._standard_treeview.setContextMenuPolicy(Qt.CustomContextMenu)
        self._standard_treeview.customContextMenuRequested.connect(self._treeview_clicked)
        self._treeview.expanded.connect(self._expanded)
        self._treeview.collapsed.connect(self._expanded)

        # buttons
        self._attach_button_signal_slots()

        # check boxes
        self._attach_checkbox_signal_slots()

        # hide standard files view
        self._standard_treeview.setHidden(True)

        self.progress = None

    # signals
    sig_filters_clicked = Signal()

    sig_check_all = Signal()
    sig_uncheck_all = Signal()
    sig_check_selected = Signal()
    sig_check_last = Signal(str)
    sig_expanded = Signal()
    sig_progress_canceled = Signal()
    sig_autoload_new_clicked = Signal(int)
    sig_auto_select_standard_clicked = Signal(int)
    sig_standard_filters_clicked = Signal()
    sig_right_click = Signal(QModelIndex)
    sig_sample_data_clicked = Signal()
    sig_standard_data_clicked = Signal()

    # signal reactions
    def _expanded(self, _dummy):
        self.sig_expanded.emit()

    def _treeview_clicked(self, point):
        self.sig_right_click.emit(self._treeview.indexAt(point))

    def _autoload_new_checked(self, state):
        self.sig_autoload_new_clicked.emit(state)

    def _auto_select_standard_clicked(self, state):
        self.sig_auto_select_standard_clicked.emit(state)

    def _check_all(self):
        self.sig_check_all.emit()

    def _uncheck_all(self):
        self.sig_uncheck_all.emit()

    def _check_selected(self):
        self.sig_check_selected.emit()

    def _check_last(self):
        sender_name = self.sender().objectName()
        self.sig_check_last.emit(sender_name)

    def _un_expand_all(self):
        with QSignalBlocker(self._treeview):
            self._treeview.collapseAll()
        self.adjust_treeview_columns_width(TREEVIEW_MAX_NUMBER_COLUMNS)

    # public can be called from presenter
    def expand_all(self):
        with QSignalBlocker(self._treeview):
            self._treeview.expandAll()
        self.adjust_treeview_columns_width(TREEVIEW_MAX_NUMBER_COLUMNS)

    def _filter_scans_checked(self):
        self.sig_filters_clicked.emit()

    def _filter_standard_checked(self):
        self.sig_standard_filters_clicked.emit()

    def _sample_data_clicked(self):
        self.sig_sample_data_clicked.emit()

    def _standard_data_clicked(self):
        self.sig_standard_data_clicked.emit()

    # get states
    def get_filters(self):
        """
        Returning chosen filters which should be applied to the list of scans.
        """
        state_dict = self.get_state()
        free_text = state_dict["filter_free_text"]
        filters = {
            "det_rot": state_dict["filter_det_rot"],
            "sample_rot": state_dict["filter_sample_rot"],
            " scan": state_dict["filter_scans"],
            # space is important not to get cscans
            "cscan": state_dict["filter_cscans"],
            free_text: state_dict["filter_free"],
        }
        if filters[" scan"] and filters["cscan"]:
            filters["scan"] = True
            filters.pop(" scan")
            filters.pop("cscan")
        return filters

    def get_number_scans_to_check(self):
        return self._ui.sB_last_scans.value()

    def get_selected_indexes(self):
        return self._treeview.selectedIndexes()

    def get_standard_filters(self):
        state_dict = self.get_state()
        filters = {
            "vanadium": state_dict["filter_vanadium"],
            "nicr": state_dict["filter_nicr"],
            "empty": state_dict["filter_empty"],
        }
        return filters

    def hide_scan(self, row):
        self._treeview.setRowHidden(row, self._treeview.rootIndex(), True)

    def show_scan(self, row):
        self._treeview.setRowHidden(row, self._treeview.rootIndex(), False)

    def hide_tof(self, hidden=True):
        self._standard_treeview.setColumnHidden(TreeItemEnum.tof_channels.value, hidden)
        self._standard_treeview.setColumnHidden(TreeItemEnum.tof_channel_width.value, hidden)
        self._sample_treeview.setColumnHidden(TreeItemEnum.tof_channels.value, hidden)
        self._sample_treeview.setColumnHidden(TreeItemEnum.tof_channel_width.value, hidden)

    def is_scan_hidden(self, row):
        return self._treeview.isRowHidden(row, self._treeview.rootIndex())

    # progress dialog
    def open_progress_dialog(self, num_of_steps):
        if num_of_steps:
            self.progress = QProgressDialog(f"Loading {num_of_steps} files...", "Abort Loading", 0, num_of_steps)
            self.progress.setAttribute(Qt.WA_DeleteOnClose, True)
            self.progress.setWindowModality(Qt.WindowModal)
            self.progress.setMinimumDuration(200)
            self.progress.open(self._progress_canceled)

    def _progress_canceled(self):
        self.sig_progress_canceled.emit()

    def set_progress(self, iteration, iteration_max):
        self.progress.setValue(iteration)
        if iteration == iteration_max:
            self.progress.close()

    # manipulating view
    def set_first_column_spanned(self, scan_range):
        for i in scan_range:
            self._treeview.setFirstColumnSpanned(i, self._treeview.rootIndex(), True)

    def set_standard_data_tree_model(self, model):
        self._standard_treeview.setModel(model)
        self._ui.groupBox_filter_by_scan.setHidden(True)
        self._ui.pB_check_last_scan.setHidden(True)
        self._ui.pB_check_last_complete_scan.setHidden(True)
        self._ui.sB_last_scans.setHidden(True)
        self._ui.cB_autoload_new.setHidden(True)
        self._ui.groupBox_filter_standard.setHidden(False)
        self._standard_treeview.setHidden(False)
        self._sample_treeview.setHidden(True)
        self.cB_auto_select_standard.setHidden(False)
        self._treeview = self._standard_treeview
        self._ui.pB_standard_data.setDefault(True)
        self._ui.pB_sample_data.setDefault(False)

    def set_sample_data_tree_model(self, model):
        self._sample_treeview.setModel(model)
        self._ui.groupBox_filter_by_scan.setHidden(False)
        self._ui.pB_check_last_scan.setHidden(False)
        self._ui.pB_check_last_complete_scan.setHidden(False)
        self._ui.sB_last_scans.setHidden(False)
        self._ui.cB_autoload_new.setHidden(False)
        self._ui.groupBox_filter_standard.setHidden(True)
        self._standard_treeview.setHidden(True)
        self._sample_treeview.setHidden(False)
        self.cB_auto_select_standard.setHidden(True)
        self._treeview = self._sample_treeview
        self._ui.pB_sample_data.setDefault(True)
        self._ui.pB_standard_data.setDefault(False)

    def adjust_treeview_columns_width(self, num_columns):
        for i in range(num_columns):
            self._treeview.resizeColumnToContents(i)

    def _attach_button_signal_slots(self):
        self._ui.cB_filter_det_rot.stateChanged.connect(self._filter_scans_checked)
        self._ui.cB_filter_sample_rot.stateChanged.connect(self._filter_scans_checked)
        self._ui.cB_filter_scans.stateChanged.connect(self._filter_scans_checked)
        self._ui.cB_filter_cscans.stateChanged.connect(self._filter_scans_checked)
        self._ui.cB_filter_free.stateChanged.connect(self._filter_scans_checked)
        self._ui.lE_filter_free_text.textChanged.connect(self._filter_scans_checked)
        self._ui.pB_expand_all.clicked.connect(self.expand_all)
        self._ui.pB_expand_none.clicked.connect(self._un_expand_all)
        self._ui.pB_check_all.clicked.connect(self._check_all)
        self._ui.pB_check_none.clicked.connect(self._uncheck_all)
        self._ui.pB_check_last_scan.clicked.connect(self._check_last)
        self._ui.pB_check_last_complete_scan.clicked.connect(self._check_last)
        self._ui.pB_check_selected.clicked.connect(self._check_selected)
        self._ui.pB_standard_data.clicked.connect(self._standard_data_clicked)
        self._ui.pB_sample_data.clicked.connect(self._sample_data_clicked)

    def _attach_checkbox_signal_slots(self):
        self._map["filter_vanadium"].stateChanged.connect(self._filter_standard_checked)
        self._map["filter_nicr"].stateChanged.connect(self._filter_standard_checked)
        self._map["filter_empty"].stateChanged.connect(self._filter_standard_checked)
        self._map["autoload_new"].stateChanged.connect(self._autoload_new_checked)
        self._map["auto_select_standard"].stateChanged.connect(self._auto_select_standard_clicked)
