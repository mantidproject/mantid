# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import (QButtonGroup, QFrame)  # noqa
from qtpy.QtGui import (QIntValidator)  # noqa
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.inelastic.dgs_data_corrections_script import DataCorrectionsScript
import reduction_gui.widgets.util as util
try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    from mantid.kernel import Logger
    Logger("DataCorrectionsWidget").information('Using legacy ui importer')
    from mantidplot import load_ui


class DataCorrectionsWidget(BaseWidget):
    """
        Widget that presents data correction options to the user.
    """
    ## Widget name
    name = "Data Corrections"

    _old_backgnd_sub = None
    _old_norm_button = None
    incident_beam_norm_grp = None

    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        super(DataCorrectionsWidget, self).__init__(parent, state, settings, data_type=data_type)

        class DataCorrsFrame(QFrame):
            def __init__(self, parent=None):
                QFrame.__init__(self, parent)
                self.ui = load_ui(__file__, '../../../ui/inelastic/dgs_data_corrections.ui', baseinstance=self)

        self._content = DataCorrsFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()
        self._instrument_name = settings.instrument_name

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(DataCorrectionsScript(self._instrument_name))

    def initialize_content(self):
        # Set some validators
        self._content.monint_low_edit.setValidator(QIntValidator(self._content.monint_low_edit))
        self._content.monint_high_edit.setValidator(QIntValidator(self._content.monint_high_edit))
        self._content.tof_start_edit.setValidator(QIntValidator(self._content.tof_start_edit))
        self._content.tof_end_edit.setValidator(QIntValidator(self._content.tof_end_edit))

        # Make group for incident beam normalisation radio buttons
        self.incident_beam_norm_grp = QButtonGroup()
        self.incident_beam_norm_grp.addButton(self._content.none_rb, 0)
        self.incident_beam_norm_grp.addButton(self._content.current_rb, 1)
        self.incident_beam_norm_grp.addButton(self._content.monitor1_rb, 2)

        self._monitor_intrange_widgets_state(self._content.monitor1_rb.isChecked())
        self._content.monitor1_rb.toggled.connect(self._monitor_intrange_widgets_state)

        self._detvan_intrange_widgets_state(self._content.van_int_cb.isChecked())
        self._content.van_int_cb.toggled.connect(self._detvan_intrange_widgets_state)
        self._content.use_procdetvan_cb.toggled.connect(self._detvan_widgets_opp_state)

        self._save_detvan_widgets_state(self._content.save_procdetvan_cb.isChecked())
        self._content.save_procdetvan_cb.toggled.connect(self._save_detvan_widgets_state)

        # Connections
        self._content.van_input_browse.clicked.connect(self._detvan_browse)
        self._content.save_procdetvan_save.clicked.connect(self._save_procdetvan_save)

    def _monitor_intrange_widgets_state(self, state=False):
        self._content.monint_label.setEnabled(state)
        self._content.monint_low_edit.setEnabled(state)
        self._content.monint_high_edit.setEnabled(state)

    def _detvan_intrange_widgets_state(self, state=False):
        self._content.van_int_range_label.setEnabled(state)
        self._content.van_int_range_low_edit.setEnabled(state)
        self._content.van_int_range_high_edit.setEnabled(state)
        self._content.van_int_range_units_cb.setEnabled(state)

    def _detvan_widgets_opp_state(self, state=False):
        self._content.van_int_cb.setEnabled(not state)
        if self._content.van_int_cb.isChecked():
            self._detvan_intrange_widgets_state(not state)
            self._content.van_int_cb.setChecked(False)
        self._content.save_procdetvan_cb.setEnabled(not state)
        if self._content.save_procdetvan_cb.isChecked():
            self._content.save_procdetvan_cb.setChecked(False)

    def _save_detvan_widgets_state(self, state=False):
        self._content.save_procdetvan_label.setEnabled(state)
        self._content.save_procdetvan_edit.setEnabled(state)
        self._content.save_procdetvan_save.setEnabled(state)

    def _detvan_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.van_input_edit.setText(fname)

    def _save_procdetvan_save(self):
        fname = self.data_save_dialog("*.nxs")
        if fname:
            self._content.save_procdetvan_edit.setText(fname)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: DataCorrectionsScript object
        """
        self._content.filter_bad_pulses_chkbox.setChecked(state.filter_bad_pulses)
        button_index = DataCorrectionsScript.INCIDENT_BEAM_NORM_TYPES.index(state.incident_beam_norm)
        cbutton = self.incident_beam_norm_grp.button(button_index)
        cbutton.setChecked(True)
        self._content.monint_low_edit.setText(str(state.monitor_int_low))
        self._content.monint_high_edit.setText(str(state.monitor_int_high))
        self._content.background_sub_gb.setChecked(state.tib_subtraction)
        self._content.tof_start_edit.setText(str(state.tib_tof_start))
        self._content.tof_end_edit.setText(str(state.tib_tof_end))
        self._content.correct_kikf_cb.setChecked(state.correct_kikf)
        self._content.van_input_edit.setText(state.detector_vanadium)
        self._content.van_int_cb.setChecked(state.detvan_integration)
        self._content.van_int_range_low_edit.setText(str(state.detvan_int_range_low))
        self._content.van_int_range_high_edit.setText(str(state.detvan_int_range_high))
        entry_index = self._content.van_int_range_units_cb.findText(state.detvan_int_range_units)
        self._content.van_int_range_units_cb.setCurrentIndex(entry_index)
        self._content.save_procdetvan_cb.setChecked(state.save_proc_detvan)
        self._content.save_procdetvan_edit.setText(str(state.save_proc_detvan_file))
        self._content.use_procdetvan_cb.setChecked(state.use_proc_detvan)

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        d = DataCorrectionsScript(self._instrument_name)
        d.filter_bad_pulses = self._content.filter_bad_pulses_chkbox.isChecked()
        d.incident_beam_norm = DataCorrectionsScript.INCIDENT_BEAM_NORM_TYPES[self.incident_beam_norm_grp.checkedId()]
        d.monitor_int_low = util._check_and_get_float_line_edit(self._content.monint_low_edit)
        d.monitor_int_high = util._check_and_get_float_line_edit(self._content.monint_high_edit)
        d.tib_subtraction = self._content.background_sub_gb.isChecked()
        d.tib_tof_start = util._check_and_get_float_line_edit(self._content.tof_start_edit)
        d.tib_tof_end = util._check_and_get_float_line_edit(self._content.tof_end_edit)
        d.correct_kikf = self._content.correct_kikf_cb.isChecked()
        d.detector_vanadium = self._content.van_input_edit.text()
        d.detvan_integration = self._content.van_int_cb.isChecked()
        d.detvan_int_range_low = util._check_and_get_float_line_edit(self._content.van_int_range_low_edit)
        d.detvan_int_range_high = util._check_and_get_float_line_edit(self._content.van_int_range_high_edit)
        d.detvan_int_range_units = self._content.van_int_range_units_cb.currentText()
        d.save_proc_detvan = self._content.save_procdetvan_cb.isChecked()
        d.save_proc_detvan_file = self._content.save_procdetvan_edit.text()
        d.use_proc_detvan = self._content.use_procdetvan_cb.isChecked()
        return d

    def live_button_toggled_actions(self,checked):
        if checked:
            self._old_norm_button = self.incident_beam_norm_grp.checkedId()
            self._old_backgnd_sub = self._content.background_sub_gb.isChecked()
            self._content.none_rb.setChecked(True)
            self._content.background_sub_gb.setChecked(False)
        else:
            try:
                self.incident_beam_norm_grp.button(self._old_norm_button).setChecked(True)
                self._content.background_sub_gb.setChecked(self._old_backgnd_sub)
            except:  # This is for if the live button started out checked
                pass
        self._content.incident_beam_norm_gb.setEnabled(not checked)
        self._content.background_sub_gb.setEnabled(not checked)
