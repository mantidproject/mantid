from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod

from PyQt4 import QtGui, QtCore
from six import with_metaclass
import ui_diagnostics_page
from mantidqtpython import MantidQt
from sans.gui_logic.gui_common import (load_file, GENERIC_SETTINGS)

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s


class DiagnosticsPage(QtGui.QWidget, ui_diagnostics_page.Ui_DiagnosticsPage):
    class DiagnosticsPageListener(with_metaclass(ABCMeta, object)):
        """
        Defines the elements which a presenter can listen to for the beam centre finder
        """
        @abstractmethod
        def on_browse_clicked(self):
            pass

        @abstractmethod
        def on_det1_horizontal_clicked(self):
            pass

        @abstractmethod
        def on_det1_vertical_clicked(self):
            pass

        @abstractmethod
        def on_det1_time_clicked(self):
            pass

        @abstractmethod
        def on_det2_horizontal_clicked(self):
            pass

        @abstractmethod
        def on_det2_vertical_clicked(self):
            pass

        @abstractmethod
        def on_det2_time_clicked(self):
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
        self.detector_1_horizontal_button.clicked.connect(self.on_det1_horizontal_clicked)
        self.detector_1_vertical_button.clicked.connect(self.on_det1_vertical_clicked)
        self.detector_1_time_button.clicked.connect(self.on_det1_time_clicked)
        self.detector_2_horizontal_button.clicked.connect(self.on_det2_horizontal_clicked)
        self.detector_2_vertical_button.clicked.connect(self.on_det2_vertical_clicked)
        self.detector_2_time_button.clicked.connect(self.on_det2_time_clicked)

    def on_det1_horizontal_clicked(self):
        self._call_diagnostics_page_listeners(lambda listener: listener.on_det1_horizontal_clicked())

    def on_det1_vertical_clicked(self):
        self._call_diagnostics_page_listeners(lambda listener: listener.on_det1_vertical_clicked())

    def on_det1_time_clicked(self):
        self._call_diagnostics_page_listeners(lambda listener: listener.on_det1_time_clicked())

    def on_det2_horizontal_clicked(self):
        self._call_diagnostics_page_listeners(lambda listener: listener.on_det2_horizontal_clicked())

    def on_det2_vertical_clicked(self):
        self._call_diagnostics_page_listeners(lambda listener: listener.on_det2_vertical_clicked())

    def on_det2_time_clicked(self):
        self._call_diagnostics_page_listeners(lambda listener: listener.on_det2_time_clicked())

    def _on_browse_clicked(self):
        """
        Load the user file
        """
        # Load the user file
        load_file(self.run_input_line_edit, "*.*", self.__generic_settings, self.__path_key,
                  self.get_file_path)

        # Notify presenters
        self._call_diagnostics_page_listeners(lambda listener: listener.on_browse_clicked())

    def get_file_path(self):
        return str(self.run_input_line_edit.text())

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
    def det1_horizontal_range(self):
        return self.get_simple_line_edit_field(line_edit="det1_horizontal_range_line_edit", expected_type=str)

    @det1_horizontal_range.setter
    def det1_horizontal_range(self, value):
        self.update_simple_line_edit_field(line_edit="det1_horizontal_range_line_edit", value=value)

    @property
    def det1_vertical_range(self):
        return self.get_simple_line_edit_field(line_edit="det1_vertical_range_line_edit", expected_type=str)

    @det1_vertical_range.setter
    def det1_vertical_range(self, value):
        self.update_simple_line_edit_field(line_edit="det1_vertical_range_line_edit", value=value)

    @property
    def det1_time_range(self):
        return self.get_simple_line_edit_field(line_edit="det1_time_range_line_edit", expected_type=str)

    @det1_time_range.setter
    def det1_time_range(self, value):
        self.update_simple_line_edit_field(line_edit="det1_time_range_line_edit", value=value)

    @property
    def det2_horizontal_range(self):
        return self.get_simple_line_edit_field(line_edit="det2_horizontal_range_line_edit", expected_type=str)

    @det2_horizontal_range.setter
    def det2_horizontal_range(self, value):
        self.update_simple_line_edit_field(line_edit="det2_horizontal_range_line_edit", value=value)

    @property
    def det2_vertical_range(self):
        return self.get_simple_line_edit_field(line_edit="det2_vertical_range_line_edit", expected_type=str)

    @det2_vertical_range.setter
    def det2_vertical_range(self, value):
        self.update_simple_line_edit_field(line_edit="det2_vertical_range_line_edit", value=value)

    @property
    def det2_time_range(self):
        return self.get_simple_line_edit_field(line_edit="det2_time_range_line_edit", expected_type=str)

    @det2_time_range.setter
    def det2_time_range(self, value):
        self.update_simple_line_edit_field(line_edit="det2_time_range_line_edit", value=value)

    @property
    def det1_horizontal_mask(self):
        return self.det1_horizontal_mask_check_box.isChecked()

    @det1_horizontal_mask.setter
    def det1_horizontal_mask(self, value):
        self.det1_horizontal_mask_check_box.setChecked(value)

    @property
    def det1_vertical_mask(self):
        return self.det1_vertical_mask_check_box.isChecked()

    @det1_vertical_mask.setter
    def det1_vertical_mask(self, value):
        self.det1_vertical_mask_check_box.setChecked(value)

    @property
    def det1_time_mask(self):
        return self.det1_time_mask_checkbox.isChecked()

    @det1_time_mask.setter
    def det1_time_mask(self, value):
        self.det1_time_mask_checkbox.setChecked(value)

    @property
    def det2_horizontal_mask(self):
        return self.det2_horizontal_mask_check_box.isChecked()

    @det2_horizontal_mask.setter
    def det2_horizontal_mask(self, value):
        self.det2_horizontal_mask_check_box.setChecked(value)

    @property
    def det2_vertical_mask(self):
        return self.det2_vertical_mask_check_box.isChecked()

    @det2_vertical_mask.setter
    def det2_vertical_mask(self, value):
        self.det2_vertical_mask_check_box.setChecked(value)

    @property
    def det2_time_mask(self):
        return self.det2_time_mask_checkbox.isChecked()

    @det2_time_mask.setter
    def det2_time_mask(self, value):
        self.det2_time_mask_checkbox.setChecked(value)



