from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod

from PyQt4 import QtGui
from six import with_metaclass
import ui_beam_centre

class BeamCentre(QtGui.QWidget, ui_beam_centre.Ui_BeamCentre):
    class BeamCentreListener(with_metaclass(ABCMeta, object)):
        """
        Defines the elements which a presenter can listen to for the beam centre finder
        """
        @abstractmethod
        def on_run_clicked(self):
            pass

        @abstractmethod
        def on_clear_log_clicked(self):
            pass

        @abstractmethod
        def on_update_rows(self):
            pass

        @abstractmethod
        def on_row_changed(self):
            pass

    def __init__(self, parent=None):
        super(BeamCentre, self).__init__(parent)
        self.setupUi(self)

        # Hook up signal and slots
        self.connect_signals()
        self._beam_centre_tab_listeners = []

        self.scale = 1000

    def connect_signals(self):
        self.run_button.clicked.connect(self.on_run_clicked)
        self.clear_log_button.clicked.connect(self.on_clear_log_clicked)
        self.select_row_combo_box.currentIndexChanged.connect(self.on_row_changed)
        self.select_row_push_button.clicked.connect(self.on_update_rows)

    def add_listener(self, listener):
        if not isinstance(listener, BeamCentre.BeamCentreListener):
            raise ValueError("The listener ist not of type BeamCentreListener but rather {}".format(type(listener)))
        self._beam_centre_tab_listeners.append(listener)

    def clear_listeners(self):
        self._beam_centre_tab_listeners = []

    def _call_beam_centre_tab_listeners(self, target):
        for listener in self._beam_centre_tab_listeners:
            target(listener)

    def on_run_clicked(self):
        self._call_beam_centre_tab_listeners(lambda listener: listener.on_run_clicked())

    def on_clear_log_clicked(self):
        self._call_beam_centre_tab_listeners(lambda listener: listener.on_clear_log_clicked())

    def on_row_changed(self):
        self._call_beam_centre_tab_listeners(lambda listener: listener.on_row_changed())

    def on_update_rows(self):
        self._call_beam_centre_tab_listeners(lambda listener: listener.on_update_rows())

    @property
    def r_min(self):
        return self.get_simple_line_edit_field(line_edit="r_min_line_edit", expected_type=float)

    @r_min.setter
    def r_min(self, value):
        self.update_simple_line_edit_field(line_edit="r_min_line_edit", value=value)

    @property
    def r_max(self):
        return self.get_simple_line_edit_field(line_edit="r_max_line_edit", expected_type=float)

    @r_max.setter
    def r_max(self, value):
        self.update_simple_line_edit_field(line_edit="r_max_line_edit", value=value)

    @property
    def max_iterations(self):
        return self.get_simple_line_edit_field(line_edit="max_iterations_line_edit", expected_type=int)

    @max_iterations.setter
    def max_iterations(self, value):
        self.update_simple_line_edit_field(line_edit="max_iterations_line_edit", value=value)

    @property
    def tolerance(self):
        return self.get_simple_line_edit_field(line_edit="tolerance_line_edit", expected_type=float)

    @tolerance.setter
    def tolerance(self, value):
        self.update_simple_line_edit_field(line_edit="tolerance_line_edit", value=value)

    @property
    def left_right(self):
        return self.left_right_check_box.isChecked()

    @left_right.setter
    def left_right(self, value):
        self.left_right_check_box.setChecked(value)

    @property
    def up_down(self):
        return self.left_right_check_box.isChecked()

    @up_down.setter
    def up_down(self, value):
        self.up_down_check_box.setChecked(value)

    @property
    def lab_pos_1(self):
        return self.get_simple_line_edit_field(line_edit="lab_pos_1_line_edit", expected_type=float)/self.scale

    @lab_pos_1.setter
    def lab_pos_1(self, value):
        self.update_simple_line_edit_field(line_edit="lab_pos_1_line_edit", value=value*self.scale)

    @property
    def lab_pos_2(self):
        return self.get_simple_line_edit_field(line_edit="lab_pos_2_line_edit", expected_type=float)/self.scale

    @lab_pos_2.setter
    def lab_pos_2(self, value):
        self.update_simple_line_edit_field(line_edit="lab_pos_2_line_edit", value=value*self.scale)

    @property
    def hab_pos_1(self):
        return self.get_simple_line_edit_field(line_edit="hab_pos_1_line_edit", expected_type=float)/self.scale

    @hab_pos_1.setter
    def hab_pos_1(self, value):
        self.update_simple_line_edit_field(line_edit="hab_pos_1_line_edit", value=value*self.scale)

    @property
    def hab_pos_2(self):
        return self.get_simple_line_edit_field(line_edit="hab_pos_2_line_edit", expected_type=float)/self.scale

    @hab_pos_2.setter
    def hab_pos_2(self, value):
        self.update_simple_line_edit_field(line_edit="hab_pos_2_line_edit", value=value*self.scale)

    # ------------------------------------------------------------------------------------------------------------------
    # Actions
    # ------------------------------------------------------------------------------------------------------------------
    def set_centre_positions(self, centre_positions):
        self.lab_pos_1 = centre_positions['LAB1']
        self.lab_pos_2 = centre_positions['LAB2']
        self.hab_pos_1 = centre_positions['HAB1']
        self.hab_pos_2 = centre_positions['HAB2']

    def set_options(self, options):
        self.r_min = options.r_min
        self.r_max = options.r_max
        self.max_iterations = options.max_iterations
        self.tolerance = options.tolerance
        self.left_right = options.left_right
        self.up_down = options.up_down
        self.lab_pos_1 = options.lab_pos_1
        self.lab_pos_2 = options.lab_pos_2
        self.hab_pos_1 = options.hab_pos_1
        self.hab_pos_2 = options.hab_pos_2

    def update_simple_line_edit_field(self, line_edit, value):
        if value:
            gui_element = getattr(self, line_edit)
            gui_element.setText(str(value))

    def get_simple_line_edit_field(self, expected_type, line_edit):
        gui_element = getattr(self, line_edit)
        value_as_string = gui_element.text()
        return expected_type(value_as_string) if value_as_string else None

    def get_current_row(self):
        value = self.select_row_combo_box.currentText()
        if not value:
            value = -1
        return int(value)

    def set_row(self, index):
        found_index = self.select_row_combo_box.findText(str(index))
        if found_index and found_index != -1:
            self.select_row_combo_box.setCurrentIndex(found_index)

    def update_rows(self, indices):
        self.select_row_combo_box.blockSignals(True)
        self.select_row_combo_box.clear()
        for index in indices:
            self.select_row_combo_box.addItem(str(index))
        self.select_row_combo_box.blockSignals(False)

    def set_run_button_to_processing(self):
        self.run_button.setText("Processing ...")
        self.run_button.setEnabled(False)

    def set_run_button_to_normal(self):
        self.run_button.setText("Run")
        self.run_button.setEnabled(True)


