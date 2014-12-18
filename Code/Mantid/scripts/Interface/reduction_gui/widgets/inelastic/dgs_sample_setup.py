from PyQt4 import QtGui, uic, QtCore
from functools import partial
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.inelastic.dgs_sample_data_setup_script import SampleSetupScript
import reduction_gui.widgets.util as util
import ui.inelastic.ui_dgs_sample_setup
import os

IS_IN_MANTIDPLOT = False
try:
    import mantidqtpython
    from mantid.kernel import config
    IS_IN_MANTIDPLOT = True
except:
    pass

class SampleSetupWidget(BaseWidget):
    """
        Widget that presents sample setup options to the user.
    """
    ## Widget name
    name = "Sample Setup"

    def __init__(self, parent=None, state=None, settings=None, data_type=None):
        super(SampleSetupWidget, self).__init__(parent, state, settings, data_type=data_type)

        class SamSetFrame(QtGui.QFrame, ui.inelastic.ui_dgs_sample_setup.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = SamSetFrame(self)
        self._instrument_name = settings.instrument_name
        self._facility_name = settings.facility_name
        self._livebuttonwidget = None
        if IS_IN_MANTIDPLOT:
            self._swap_in_mwrunfiles_widget()
        self._layout.addWidget(self._content)
        self.initialize_content()

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(SampleSetupScript(self._instrument_name))

    def initialize_content(self):
        # Constraints
        dv = QtGui.QDoubleValidator(self._content.ei_guess_edit)
        dv.setBottom(0.0)
        self._content.ei_guess_edit.setValidator(dv)
        if "SNS" != self._facility_name:
            util.set_valid(self._content.ei_guess_edit, False)
        self._content.tzero_guess_edit.setValidator(QtGui.QDoubleValidator(self._content.tzero_guess_edit))
        self._content.etr_low_edit.setValidator(QtGui.QDoubleValidator(self._content.etr_low_edit))
        self._content.etr_width_edit.setValidator(QtGui.QDoubleValidator(self._content.etr_width_edit))
        self._content.etr_high_edit.setValidator(QtGui.QDoubleValidator(self._content.etr_high_edit))
        self._content.monitor1_specid_edit.setValidator(QtGui.QIntValidator(self._content.monitor1_specid_edit))
        self._content.monitor2_specid_edit.setValidator(QtGui.QIntValidator(self._content.monitor2_specid_edit))

        # Default states
        self._handle_tzero_guess(self._content.use_ei_guess_chkbox.isChecked())

        # Connections
        self.connect(self._content.sample_browse, QtCore.SIGNAL("clicked()"),
                     self._sample_browse)
        self.connect(self._content.detcal_browse, QtCore.SIGNAL("clicked()"),
                     self._detcal_browse)
        self.connect(self._content.hardmask_browse, QtCore.SIGNAL("clicked()"),
                     self._hardmask_browse)
        self.connect(self._content.grouping_browse, QtCore.SIGNAL("clicked()"),
                     self._grouping_browse)
        self.connect(self._content.use_ei_guess_chkbox, QtCore.SIGNAL("stateChanged(int)"),
                     self._handle_tzero_guess)
        self.connect(self._content.savedir_browse, QtCore.SIGNAL("clicked()"),
                     self._savedir_browse)

        # Validated widgets
        self._connect_validated_lineedit(self._content.sample_edit)
        self._connect_validated_lineedit(self._content.ei_guess_edit)
        self._connect_validated_lineedit(self._content.savedir_edit)

    def _swap_in_mwrunfiles_widget(self):
        labeltext = self._content.sample_label.text()
        self._content.sample_label.hide()
        self._content.sample_edit.hide()
        self._content.sample_browse.hide()
        self._content.horizontalLayout.removeWidget(self._content.sample_label)
        self._content.horizontalLayout.removeWidget(self._content.sample_edit)
        self._content.horizontalLayout.removeWidget(self._content.sample_browse)
        spacer = self._content.horizontalLayout.takeAt(0)
        self._content.sample_edit = mantidqtpython.MantidQt.MantidWidgets.MWRunFiles()
        # Unfortunately, can only use live if default instrument = gui-set instrument
        if self._instrument_name == config.getInstrument().name():
            self._content.sample_edit.setProperty("liveButton","ShowIfCanConnect")
        self._content.sample_edit.setProperty("multipleFiles",True)
        self._content.sample_edit.setProperty("algorithmAndProperty","Load|Filename")
        self._content.sample_edit.setProperty("label",labeltext)
        self._content.sample_edit.setLabelMinWidth(self._content.sample_label.minimumWidth())
        self._content.horizontalLayout.addWidget(self._content.sample_edit)
        self._content.horizontalLayout.addItem(spacer)
        self._content.sample_edit.fileFindingFinished.connect(lambda: self._validate_edit(self._content.sample_edit))
        self._livebuttonwidget = self._content.sample_edit

    def _handle_tzero_guess(self, is_enabled):
        self._content.tzero_guess_label.setEnabled(is_enabled)
        self._content.tzero_guess_edit.setEnabled(is_enabled)
        self._content.tzero_guess_unit_label.setEnabled(is_enabled)

    def _check_and_set_lineedit_content(self, lineedit, content):
        lineedit.setText(content)
        util.set_valid(lineedit, not lineedit.text() == '')

    def _connect_validated_lineedit(self, ui_ctrl):
        call_back = partial(self._validate_edit, ctrl=ui_ctrl)
        self.connect(ui_ctrl, QtCore.SIGNAL("editingFinished()"), call_back)
        self.connect(ui_ctrl, QtCore.SIGNAL("textEdited(QString)"), call_back)
        self.connect(ui_ctrl, QtCore.SIGNAL("textChanged(QString)"), call_back)

    def _validate_edit(self, ctrl=None):
        is_valid = True
        if "isValid" in dir(ctrl): # For mwRunFiles widget
            if not ctrl.isValid():
                is_valid = False
        else:
            if not ctrl.text():
                is_valid = False
        util.set_valid(ctrl, is_valid)

    def _sample_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_edit.setText(fname)

    def _detcal_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_edit.setText(fname)

    def _hardmask_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.hardmask_edit.setText(fname)

    def _grouping_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.grouping_edit.setText(fname)

    def _savedir_browse(self):
        save_dir = QtGui.QFileDialog.getExistingDirectory(self, "Output Directory - Choose a directory",
                                                            os.path.expanduser('~'),
                                                            QtGui.QFileDialog.ShowDirsOnly
                                                            | QtGui.QFileDialog.DontResolveSymlinks)
        if save_dir:
            self._content.savedir_edit.setText(save_dir)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: SampleSetupScript object
        """
        if IS_IN_MANTIDPLOT:
            self._content.sample_edit.setUserInput(state.sample_file)
            self._content.sample_edit.liveButtonSetChecked(state.live_button)
        else:
            self._check_and_set_lineedit_content(self._content.sample_edit,
                                                 state.sample_file)
        self._content.output_ws_edit.setText(state.output_wsname)
        self._content.detcal_edit.setText(state.detcal_file)
        if "SNS" != self._facility_name:
            self._check_and_set_lineedit_content(self._content.ei_guess_edit,
                                                 state.incident_energy_guess)
        self._content.use_ei_guess_chkbox.setChecked(state.use_ei_guess)
        self._content.tzero_guess_edit.setText(str(state.tzero_guess))
        self._content.monitor1_specid_edit.setText(str(state.monitor1_specid))
        self._content.monitor2_specid_edit.setText(str(state.monitor2_specid))
        self._content.et_range_box.setChecked(state.rebin_et)
        self._content.etr_low_edit.setText(state.et_range_low)
        self._content.etr_width_edit.setText(state.et_range_width)
        self._content.etr_high_edit.setText(state.et_range_high)
        self._content.et_is_distribution_cb.setChecked(state.et_is_distribution)
        self._content.hardmask_edit.setText(state.hardmask_file)
        self._content.grouping_edit.setText(state.grouping_file)
        self._content.show_workspaces_cb.setChecked(state.show_workspaces)
        self._content.savedir_edit.setText(state.savedir)

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        s = SampleSetupScript(self._instrument_name)
        s.sample_file = self._content.sample_edit.text()
        if IS_IN_MANTIDPLOT:
            s.live_button = self._content.sample_edit.liveButtonIsChecked()
        s.output_wsname = self._content.output_ws_edit.text()
        s.detcal_file = self._content.detcal_edit.text()
        s.incident_energy_guess = self._content.ei_guess_edit.text()
        s.use_ei_guess = self._content.use_ei_guess_chkbox.isChecked()
        s.tzero_guess = util._check_and_get_float_line_edit(self._content.tzero_guess_edit)
        s.monitor1_specid = int(self._content.monitor1_specid_edit.text())
        s.monitor2_specid = int(self._content.monitor2_specid_edit.text())
        s.rebin_et = self._content.et_range_box.isChecked()
        s.et_range_low = self._content.etr_low_edit.text()
        s.et_range_width = self._content.etr_width_edit.text()
        s.et_range_high = self._content.etr_high_edit.text()
        s.et_is_distribution = self._content.et_is_distribution_cb.isChecked()
        s.hardmask_file = self._content.hardmask_edit.text()
        s.grouping_file = self._content.grouping_edit.text()
        s.show_workspaces = self._content.show_workspaces_cb.isChecked()
        s.savedir = self._content.savedir_edit.text()
        return s

    def live_button_widget(self):
        """
            Returns a reference to the MWRunFiles widget that contains the live button
            (if using interface inside MantidPlot)
        """
        return self._livebuttonwidget

    def live_button_toggled_actions(self,checked):
        if checked:
            self._old_ei_guess_state = self._content.use_ei_guess_chkbox.isChecked()
            self._content.use_ei_guess_chkbox.setChecked(True)
        else:
            try:
                self._content.use_ei_guess_chkbox.setChecked(self._old_ei_guess_state)
            except:  # This is for if the live button started out checked
                pass
        self._content.use_ei_guess_chkbox.setEnabled(not checked)
        self._content.savedir_edit.setEnabled(not checked)
        self._content.savedir_browse.setEnabled(not checked)
