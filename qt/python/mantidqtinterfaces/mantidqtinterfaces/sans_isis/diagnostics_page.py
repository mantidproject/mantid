# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod
from qtpy import QtCore, QtWidgets

from mantidqt.utils.qt import load_ui
from mantidqtinterfaces.sans_isis.gui_logic.gui_common import load_file, GENERIC_SETTINGS

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:

    def _fromUtf8(s):
        return s


Ui_DiagnosticsPage, _ = load_ui(__file__, "diagnostics_page.ui")


class DiagnosticsPage(QtWidgets.QWidget, Ui_DiagnosticsPage):
    class DiagnosticsPageListener(metaclass=ABCMeta):
        """
        Defines the elements which a presenter can listen to for the beam centre finder
        """

        @abstractmethod
        def on_browse_clicked(self):
            pass

        @abstractmethod
        def on_horizontal_clicked(self):
            pass

        @abstractmethod
        def on_vertical_clicked(self):
            pass

        @abstractmethod
        def on_time_clicked(self):
            pass

    def __init__(self, parent=None):
        super(DiagnosticsPage, self).__init__(parent)
        self.setupUi(self)

        # Hook up signal and slots
        self._diagnostics_page_listeners = []
        self.connect_signals()

        # Attach validators
        self._attach_validators()

        # Q Settings
        self.__generic_settings = GENERIC_SETTINGS
        self.__path_key = "sans_path"
        self.detector_combo_box.currentIndexChanged.connect(self.combo_box_changed)

    def update_simple_line_edit_field(self, line_edit, value):
        gui_element = getattr(self, line_edit)
        gui_element.setText(str(value))

    def get_simple_line_edit_field(self, expected_type, line_edit):
        gui_element = getattr(self, line_edit)
        value_as_string = gui_element.text()
        return expected_type(value_as_string) if value_as_string else None

    def _attach_validators(self):
        pass

    def add_listener(self, listener):
        if not isinstance(listener, DiagnosticsPage.DiagnosticsPageListener):
            raise ValueError("The listener ist not of type DiagnosticTabListener but rather {}".format(type(listener)))
        self._diagnostics_page_listeners.append(listener)

    def clear_listeners(self):
        self._diagnostics_page_listeners = []

    def _call_diagnostics_page_listeners(self, target):
        for listener in self._diagnostics_page_listeners:
            target(listener)

    def connect_signals(self):
        self.browse_button.clicked.connect(self._on_browse_clicked)
        self.horizontal_button.clicked.connect(self.on_horizontal_clicked)
        self.vertical_button.clicked.connect(self.on_vertical_clicked)
        self.time_button.clicked.connect(self.on_time_clicked)

    def on_horizontal_clicked(self):
        self._call_diagnostics_page_listeners(lambda listener: listener.on_horizontal_clicked())

    def on_vertical_clicked(self):
        self._call_diagnostics_page_listeners(lambda listener: listener.on_vertical_clicked())

    def on_time_clicked(self):
        self._call_diagnostics_page_listeners(lambda listener: listener.on_time_clicked())

    def _on_browse_clicked(self):
        load_file(self.run_input_line_edit, "*.*", self.__generic_settings, self.__path_key, self.get_file_path)

        # Notify presenters
        self._call_diagnostics_page_listeners(lambda listener: listener.on_browse_clicked())

    def get_file_path(self):
        return str(self.run_input_line_edit.text())

    def set_detectors(self, detector_list):
        self.detector_combo_box.clear()
        for element in detector_list:
            self.detector_combo_box.addItem(element)

    def combo_box_changed(self, index):
        current_detector_name = self.detector_combo_box.currentText()
        self.detector_group_box.setTitle(current_detector_name)

    # ------------------------------------------------------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def run_input(self):
        return self.get_simple_line_edit_field(line_edit="run_input_line_edit", expected_type=str)

    @run_input.setter
    def run_input(self, value):
        self.update_simple_line_edit_field(line_edit="run_input_line_edit", value=value)

    @property
    def period(self):
        return self.get_simple_line_edit_field(line_edit="period_line_edit", expected_type=str)

    @period.setter
    def period(self, value):
        self.update_simple_line_edit_field(line_edit="period_line_edit", value=value)

    @property
    def horizontal_range(self):
        return self.get_simple_line_edit_field(line_edit="horizontal_range_line_edit", expected_type=str)

    @horizontal_range.setter
    def horizontal_range(self, value):
        self.update_simple_line_edit_field(line_edit="horizontal_range_line_edit", value=value)

    @property
    def vertical_range(self):
        return self.get_simple_line_edit_field(line_edit="vertical_range_line_edit", expected_type=str)

    @vertical_range.setter
    def vertical_range(self, value):
        self.update_simple_line_edit_field(line_edit="vertical_range_line_edit", value=value)

    @property
    def time_range(self):
        return self.get_simple_line_edit_field(line_edit="time_range_line_edit", expected_type=str)

    @time_range.setter
    def time_range(self, value):
        self.update_simple_line_edit_field(line_edit="time_range_line_edit", value=value)

    @property
    def horizontal_mask(self):
        return self.horizontal_mask_check_box.isChecked()

    @horizontal_mask.setter
    def horizontal_mask(self, value):
        self.horizontal_mask_check_box.setChecked(value)

    @property
    def vertical_mask(self):
        return self.vertical_mask_check_box.isChecked()

    @vertical_mask.setter
    def vertical_mask(self, value):
        self.vertical_mask_check_box.setChecked(value)

    @property
    def time_mask(self):
        return self.time_mask_checkbox.isChecked()

    @time_mask.setter
    def time_mask(self, value):
        self.time_mask_checkbox.setChecked(value)

    @property
    def user_file_name(self):
        return self.user_file_name_label.text()

    @user_file_name.setter
    def user_file_name(self, value):
        self.user_file_name_label.setText(value)

    @property
    def detector(self):
        return self.detector_combo_box.currentText()

    @detector.setter
    def detector(self, value):
        self.detector_combo_box.currentText(value)

    # ------------------------------------------------------------------------------------------------------------------
    # Activate buttons
    # ------------------------------------------------------------------------------------------------------------------
    def enable_integrals(self):
        self.horizontal_button.setEnabled(True)
        self.vertical_button.setEnabled(True)
        self.time_button.setEnabled(True)

    def disable_integrals(self):
        self.horizontal_button.setEnabled(False)
        self.vertical_button.setEnabled(False)
        self.time_button.setEnabled(False)
