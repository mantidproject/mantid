from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod

from PyQt4 import QtGui, QtCore
from six import with_metaclass
import ui_beam_centre
from mantidqtpython import MantidQt
from sans.gui_logic.gui_common import get_detector_from_gui_selection, \
    get_detector_strings_for_gui, get_string_for_gui_from_reduction_mode

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s


class BeamCentre(QtGui.QWidget, ui_beam_centre.Ui_BeamCentre):
    class BeamCentreListener(with_metaclass(ABCMeta, object)):
        """
        Defines the elements which a presenter can listen to for the beam centre finder
        """
        @abstractmethod
        def on_run_clicked(self):
            pass

    def __init__(self, parent=None):
        super(BeamCentre, self).__init__(parent)
        self.setupUi(self)
        self._setup_log_widget()
        self.instrument = None

        # Hook up signal and slots
        self.connect_signals()
        self._beam_centre_tab_listeners = []

        # Attach validators
        self._attach_validators()

        # This feature is currently broken and not strictly needed so I am hiding this part of the GUI.
        self.Q_limits.hide()
        self.Q_from.hide()
        self.q_min_line_edit.hide()
        self.q_max_line_edit.hide()
        self.Q_to.hide()

    def _setup_log_widget(self):
        self.log_widget = MantidQt.MantidWidgets.MessageDisplay(self.groupBox_2)
        self.log_widget.setMinimumSize(QtCore.QSize(491, 371))
        self.log_widget.setObjectName(_fromUtf8("log_widget"))
        self.gridLayout.addWidget(self.log_widget, 0, 1, 4, 1)
        self.log_widget.setSource("CentreFinder")
        self.log_widget.attachLoggingChannel()

    def connect_signals(self):
        self.run_button.clicked.connect(self.on_run_clicked)

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

    def _attach_validators(self):
        # Setup the list of validators
        double_validator = QtGui.QDoubleValidator()
        positive_double_validator = QtGui.QDoubleValidator()
        positive_double_validator.setBottom(0.0)
        positive_integer_validator = QtGui.QIntValidator()
        positive_integer_validator.setBottom(1)

        self.lab_pos_1_line_edit.setValidator(double_validator)
        self.lab_pos_2_line_edit.setValidator(double_validator)
        self.hab_pos_1_line_edit.setValidator(double_validator)
        self.hab_pos_2_line_edit.setValidator(double_validator)

        self.r_min_line_edit.setValidator(positive_double_validator)
        self.r_max_line_edit.setValidator(positive_double_validator)

        self.q_min_line_edit.setValidator(positive_double_validator)
        self.q_max_line_edit.setValidator(positive_double_validator)

        self.max_iterations_line_edit.setValidator(positive_integer_validator)
        self.tolerance_line_edit.setValidator(positive_double_validator)

    def on_update_instrument(self, instrument):
        self.instrument = instrument
        component_list = get_detector_strings_for_gui(self.instrument)
        self.set_component_options(component_list)
        if len(component_list) < 2:
            self.hab_pos_1_line_edit.setEnabled(False)
            self.hab_pos_2_line_edit.setEnabled(False)
            self.update_hab_check_box.setEnabled(False)
        else:
            self.hab_pos_1_line_edit.setEnabled(True)
            self.hab_pos_2_line_edit.setEnabled(True)
            self.update_hab_check_box.setEnabled(True)

    # ------------------------------------------------------------------------------------------------------------------
    # Actions
    # ------------------------------------------------------------------------------------------------------------------
    def set_options(self, options):
        self.r_min = options.r_min
        self.r_max = options.r_max
        self.max_iterations = options.max_iterations
        self.tolerance = options.tolerance
        self.left_right = options.left_right
        self.up_down = options.up_down
        self.verbose = options.verbose
        self.COM = options.COM
        self.q_min = options.q_min
        self.q_max = options.q_max
        self.component = options.component
        self.update_lab = options.update_lab
        self.update_hab = options.update_hab

    def update_simple_line_edit_field(self, line_edit, value):
        gui_element = getattr(self, line_edit)
        gui_element.setText(str(value))

    def get_simple_line_edit_field(self, expected_type, line_edit):
        gui_element = getattr(self, line_edit)
        value_as_string = gui_element.text()
        return expected_type(value_as_string) if value_as_string else None

    def set_run_button_to_processing(self):
        self.run_button.setText("Processing ...")
        self.run_button.setEnabled(False)

    def set_run_button_to_normal(self):
        self.run_button.setText("Run")
        self.run_button.setEnabled(True)

    # ------------------------------------------------------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------------------------------------------------------
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
    def q_max(self):
        return self.get_simple_line_edit_field(line_edit="q_max_line_edit", expected_type=float)

    @q_max.setter
    def q_max(self, value):
        self.update_simple_line_edit_field(line_edit="q_max_line_edit", value=value)

    @property
    def q_min(self):
        return self.get_simple_line_edit_field(line_edit="q_min_line_edit", expected_type=float)

    @q_min.setter
    def q_min(self, value):
        self.update_simple_line_edit_field(line_edit="q_min_line_edit", value=value)

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
        return self.up_down_check_box.isChecked()

    @up_down.setter
    def up_down(self, value):
        self.up_down_check_box.setChecked(value)

    @property
    def verbose(self):
        return self.verbose_check_box.isChecked()

    @verbose.setter
    def verbose(self, value):
        self.verbose_check_box.setChecked(value)

    @property
    def COM(self):
        return self.COM_check_box.isChecked()

    @COM.setter
    def COM(self, value):
        self.COM_check_box.setChecked(value)

    @property
    def lab_pos_1(self):
        return self.get_simple_line_edit_field(line_edit="lab_pos_1_line_edit", expected_type=float)

    @lab_pos_1.setter
    def lab_pos_1(self, value):
        self.update_simple_line_edit_field(line_edit="lab_pos_1_line_edit", value=value)

    @property
    def lab_pos_2(self):
        return self.get_simple_line_edit_field(line_edit="lab_pos_2_line_edit", expected_type=float)

    @lab_pos_2.setter
    def lab_pos_2(self, value):
        self.update_simple_line_edit_field(line_edit="lab_pos_2_line_edit", value=value)

    @property
    def hab_pos_1(self):
        return self.get_simple_line_edit_field(line_edit="hab_pos_1_line_edit", expected_type=float)

    @hab_pos_1.setter
    def hab_pos_1(self, value):
        self.update_simple_line_edit_field(line_edit="hab_pos_1_line_edit", value=value)

    @property
    def hab_pos_2(self):
        return self.get_simple_line_edit_field(line_edit="hab_pos_2_line_edit", expected_type=float)

    @hab_pos_2.setter
    def hab_pos_2(self, value):
        self.update_simple_line_edit_field(line_edit="hab_pos_2_line_edit", value=value)

    @property
    def component(self):
        component_as_string = self.component_combo_box.currentText()
        return get_detector_from_gui_selection(component_as_string)

    @component.setter
    def component(self, value):
        # There are two types of values that can be passed:
        # String: we look for string and we set it
        # Convert the value to the correct GUI string

        # Set the correct selection of reduction modes which are available
        component_list = get_detector_strings_for_gui(self.instrument)
        self.set_component_options(component_list)

        component_as_string = get_string_for_gui_from_reduction_mode(value, self.instrument)
        if component_as_string:
            index = self.reduction_mode_combo_box.findText(component_as_string)
            if index != -1:
                self.component_combo_box.setCurrentIndex(index)

    def set_component_options(self, component_list):
        current_index = self.component_combo_box.currentIndex()
        self.component_combo_box.clear()
        for element in component_list:
            self.component_combo_box.addItem(element)
        if current_index != -1:
            self.component_combo_box.setCurrentIndex(current_index)

    @property
    def update_hab(self):
        return self.update_hab_check_box.isChecked()

    @update_hab.setter
    def update_hab(self, value):
        self.update_hab_check_box.setChecked(value)

    @property
    def update_lab(self):
        return self.update_lab_check_box.isChecked()

    @update_lab.setter
    def update_lab(self, value):
        self.update_lab_check_box.setChecked(value)
