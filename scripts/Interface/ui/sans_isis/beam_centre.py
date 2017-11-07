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

    # ------------------------------------------------------------------------------------------------------------------
    # Actions
    # ------------------------------------------------------------------------------------------------------------------
    def set_centre_positions(self, centre_positions):
        self.update_simple_line_edit_field('lab_pos1_lineEdit', centre_positions['LAB1'])
        self.update_simple_line_edit_field('lab_pos2_lineEdit', centre_positions['LAB2'])
        self.update_simple_line_edit_field('hab_pos1_lineEdit', centre_positions['HAB1'])
        self.update_simple_line_edit_field('hab_pos2_lineEdit', centre_positions['HAB2'])

    def update_simple_line_edit_field(self, line_edit, value):
        if value:
            gui_element = getattr(self, line_edit)
            gui_element.setText(str(value))

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
        self.run_button.setText("Display Mask")
        self.run_button.setEnabled(True)


