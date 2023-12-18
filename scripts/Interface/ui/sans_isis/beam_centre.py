# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod
from qtpy import QtGui, QtCore, QtWidgets
from mantidqt.utils.qt import load_ui
from mantidqt.widgets import messagedisplay
from sans.gui_logic.gui_common import get_detector_strings_for_gui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:

    def _fromUtf8(s):
        return s


Ui_BeamCentre, _ = load_ui(__file__, "beam_centre.ui")


class BeamCentre(QtWidgets.QWidget, Ui_BeamCentre):
    class BeamCentreListener(metaclass=ABCMeta):
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
        self.log_widget = messagedisplay.MessageDisplay(parent=self.groupBox_2)
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

        self.rear_pos_1_line_edit.setValidator(double_validator)
        self.rear_pos_2_line_edit.setValidator(double_validator)
        self.front_pos_1_line_edit.setValidator(double_validator)
        self.front_pos_2_line_edit.setValidator(double_validator)

        self.r_min_line_edit.setValidator(positive_double_validator)
        self.r_max_line_edit.setValidator(positive_double_validator)

        self.q_min_line_edit.setValidator(positive_double_validator)
        self.q_max_line_edit.setValidator(positive_double_validator)

        self.max_iterations_line_edit.setValidator(positive_integer_validator)
        self.tolerance_line_edit.setValidator(positive_double_validator)

    def on_update_instrument(self, instrument):
        self.instrument = instrument
        component_list = get_detector_strings_for_gui(self.instrument)
        if len(component_list) < 2:
            self.front_pos_1_line_edit.setEnabled(False)
            self.front_pos_2_line_edit.setEnabled(False)
            self.update_front_radio.setEnabled(False)
        else:
            self.front_pos_1_line_edit.setEnabled(True)
            self.front_pos_2_line_edit.setEnabled(True)
            self.update_front_radio.setEnabled(True)

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
        self.update_rear = options.update_rear
        self.update_front = options.update_front

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

    def enable_update_front(self, enabled):
        self.update_front_radio.setChecked(enabled)

    def enable_update_rear(self, enabled):
        self.update_rear_radio.setChecked(enabled)

    def set_position_unit(self, unit: str):
        self.rear_centre_label.setText(f"Centre Position - Rear ({unit})")
        self.front_centre_label.setText(f"Centre Position - Front ({unit})")

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
    def rear_pos_1(self):
        return self.get_simple_line_edit_field(line_edit="rear_pos_1_line_edit", expected_type=float)

    @rear_pos_1.setter
    def rear_pos_1(self, value):
        self.update_simple_line_edit_field(line_edit="rear_pos_1_line_edit", value=value)

    @property
    def rear_pos_2(self):
        return self.get_simple_line_edit_field(line_edit="rear_pos_2_line_edit", expected_type=float)

    @rear_pos_2.setter
    def rear_pos_2(self, value):
        self.update_simple_line_edit_field(line_edit="rear_pos_2_line_edit", value=value)

    @property
    def front_pos_1(self):
        return self.get_simple_line_edit_field(line_edit="front_pos_1_line_edit", expected_type=float)

    @front_pos_1.setter
    def front_pos_1(self, value):
        self.update_simple_line_edit_field(line_edit="front_pos_1_line_edit", value=value)

    @property
    def front_pos_2(self):
        return self.get_simple_line_edit_field(line_edit="front_pos_2_line_edit", expected_type=float)

    @front_pos_2.setter
    def front_pos_2(self, value):
        self.update_simple_line_edit_field(line_edit="front_pos_2_line_edit", value=value)

    @property
    def update_front(self):
        return self.update_front_radio.isChecked()

    @update_front.setter
    def update_front(self, value):
        self.update_front_radio.setChecked(value)

    @property
    def update_rear(self):
        return self.update_rear_radio.isChecked()

    @update_rear.setter
    def update_rear(self, value):
        self.update_rear_radio.setChecked(value)
