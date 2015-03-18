#pylint: disable=invalid-name
from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import os
from reduction_gui.reduction.sans.eqsans_data_script import DataSets
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.sans.ui_eqsans_sample_data

class DataSetsWidget(BaseWidget):
    """
        Widget that presents the transmission options to the user
    """
    _method_box = None

    ## Widget name
    name = "Data"

    def __init__(self, parent=None, state=None, settings=None, data_type=None, data_proxy=None):
        super(DataSetsWidget, self).__init__(parent, state, settings, data_type, data_proxy=data_proxy)

        class DataFrame(QtGui.QFrame, ui.sans.ui_eqsans_sample_data.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = DataFrame(self)
        self._layout.addWidget(self._content)
        self.initialize_content()

        if state is not None:
            self.set_state(state)
        else:
            m = DataSets()
            self.set_state(m)

    def initialize_content(self):
        """
            Declare the validators and event connections for the
            widgets loaded through the .ui file.
        """
        # Sample data

        # Validators
        self._content.transmission_edit.setValidator(QtGui.QDoubleValidator(self._content.transmission_edit))
        self._content.dtransmission_edit.setValidator(QtGui.QDoubleValidator(self._content.dtransmission_edit))
        self._content.beam_radius_edit.setValidator(QtGui.QDoubleValidator(self._content.beam_radius_edit))
        self._content.sample_thickness_edit.setValidator(QtGui.QDoubleValidator(self._content.sample_thickness_edit))

        # Connections
        self.connect(self._content.data_file_browse_button, QtCore.SIGNAL("clicked()"), self._data_file_browse)
        self.connect(self._content.calculate_radio, QtCore.SIGNAL("clicked()"), self._calculate_clicked)
        self.connect(self._content.fix_trans_radio, QtCore.SIGNAL("clicked()"), self._calculate_clicked)

        self.connect(self._content.empty_button, QtCore.SIGNAL("clicked()"), self._empty_browse)
        self.connect(self._content.sample_button, QtCore.SIGNAL("clicked()"), self._sample_browse)

        self.connect(self._content.data_file_plot_button, QtCore.SIGNAL("clicked()"), self._data_file_plot)
        self.connect(self._content.empty_plot_button, QtCore.SIGNAL("clicked()"), self._empty_plot)
        self.connect(self._content.sample_plot_button, QtCore.SIGNAL("clicked()"), self._sample_plot)

        # Calculate/Fix radio button
        g1 = QtGui.QButtonGroup(self)
        g1.addButton(self._content.calculate_radio)
        g1.addButton(self._content.fix_trans_radio)
        g1.setExclusive(True)

        if not self._settings.debug:
            self._content.fix_transmission_layout.deleteLater()
            self._content.calculate_radio.hide()
            self._content.fix_trans_radio.hide()
            self._content.plus_minus_label.hide()
            self._content.transmission_edit.hide()
            self._content.dtransmission_edit.hide()

        if not self._in_mantidplot:
            self._content.data_file_plot_button.hide()
            self._content.empty_plot_button.hide()
            self._content.sample_plot_button.hide()

        # Background ##########
        # Validators
        self._content.bck_transmission_edit.setValidator(QtGui.QDoubleValidator(self._content.bck_transmission_edit))
        self._content.bck_dtransmission_edit.setValidator(QtGui.QDoubleValidator(self._content.bck_dtransmission_edit))
        self._content.bck_beam_radius_edit.setValidator(QtGui.QDoubleValidator(self._content.beam_radius_edit))
        #self._content.bck_thickness_edit.setValidator(QtGui.QDoubleValidator(self._content.bck_thickness_edit))

        # Connections
        self.connect(self._content.background_chk, QtCore.SIGNAL("clicked(bool)"), self._background_clicked)
        self.connect(self._content.background_browse, QtCore.SIGNAL("clicked()"), self._background_browse)
        self.connect(self._content.bck_calculate_radio, QtCore.SIGNAL("clicked()"), self._bck_calculate_clicked)
        self.connect(self._content.bck_fix_trans_radio, QtCore.SIGNAL("clicked()"), self._bck_calculate_clicked)

        self.connect(self._content.bck_empty_button, QtCore.SIGNAL("clicked()"), self._bck_empty_browse)
        self.connect(self._content.bck_sample_button, QtCore.SIGNAL("clicked()"), self._bck_sample_browse)

        self.connect(self._content.background_plot_button, QtCore.SIGNAL("clicked()"), self._background_plot_clicked)
        self.connect(self._content.bck_empty_plot_button, QtCore.SIGNAL("clicked()"), self._bck_empty_plot)
        self.connect(self._content.bck_sample_plot_button, QtCore.SIGNAL("clicked()"), self._bck_sample_plot)

        # Calculate/Fix radio button
        g2 = QtGui.QButtonGroup(self)
        g2.addButton(self._content.bck_calculate_radio)
        g2.addButton(self._content.bck_fix_trans_radio)
        g2.setExclusive(True)

        if not self._settings.debug:
            self._content.bck_fix_transmission_layout.deleteLater()
            self._content.bck_calculate_radio.hide()
            self._content.bck_fix_trans_radio.hide()
            self._content.bck_plus_minus_label.hide()
            self._content.bck_transmission_edit.hide()
            self._content.bck_dtransmission_edit.hide()

            if not self._settings.advanced:
                self._content.theta_dep_chk.hide()
                self._content.bck_theta_dep_chk.hide()
                self._content.sample_thickness_label.hide()
                self._content.sample_thickness_edit.hide()
                #self._content.bck_thickness_label.hide()
                #self._content.bck_thickness_edit.hide()

        if not self._in_mantidplot:
            self._content.background_plot_button.hide()
            self._content.bck_empty_plot_button.hide()
            self._content.bck_sample_plot_button.hide()

    def _background_plot_clicked(self):
        self.show_instrument(file_name=self._content.background_edit.text)

    def _empty_plot(self):
        self.show_instrument(file_name=self._content.empty_edit.text)

    def _sample_plot(self):
        self.show_instrument(file_name=self._content.sample_edit.text)

    def _bck_empty_plot(self):
        self.show_instrument(file_name=self._content.bck_empty_edit.text)

    def _bck_sample_plot(self):
        self.show_instrument(file_name=self._content.bck_sample_edit.text)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        popup_warning = ""

        self._content.transmission_edit.setText(str("%6.4f" % state.transmission))
        self._content.dtransmission_edit.setText(str("%6.4f" % state.transmission_spread))
        self._content.sample_thickness_edit.setText(str("%6.4f" % state.sample_thickness))

        self._content.beam_radius_edit.setText(str(state.calculation_method.beam_radius))
        self._content.sample_edit.setText(state.calculation_method.sample_file)
        self._content.empty_edit.setText(state.calculation_method.direct_beam)

        self._content.calculate_radio.setChecked(state.calculate_transmission)
        self._content.fix_trans_radio.setChecked(not state.calculate_transmission)
        if not self._settings.debug and not state.calculate_transmission:
            # If we fix the transmission while not in debug mode, warn the user that
            # things will look weird.
            popup_warning = "Turn on debug mode to see all options:\n\n"
            popup_warning += "The sample transmission for the loaded reduction was set in debug mode to %-6.1g\n" % state.transmission

        self._content.theta_dep_chk.setChecked(state.theta_dependent)
        self._content.fit_together_check.setChecked(state.combine_transmission_frames)
        self._calculate_clicked()

        # Data file
        #   Check whether we are updating the data file
        data_files = self._get_data_files()
        current_file = ''
        if len(data_files)>0:
            current_file = data_files[0].strip()
        self._content.separate_jobs_check.setChecked(state.separate_jobs)

        self._content.data_file_edit.setText(str(';'.join(state.data_files)))
        if len(state.data_files)>0:
            self._settings.last_file = state.data_files[0]
            self._settings.last_data_ws = ''

            # Store the location of the loaded file
            if len(state.data_files[0])>0:
                (folder, file_name) = os.path.split(state.data_files[0])
                self._settings.data_path = folder
                if current_file != state.data_files[0].strip():
                    self.get_data_info()
                else:
                    self._emit_experiment_parameters()

        # Background
        bck_file = str(self._content.background_edit.text()).strip()
        self._content.background_chk.setChecked(state.background.background_corr)
        self._content.background_edit.setText(state.background.background_file)
        if state.background.background_file.strip() != bck_file:
            self.get_data_info()
        self._background_clicked(state.background.background_corr)

        self._content.bck_transmission_edit.setText(str("%6.4f" % state.background.bck_transmission))
        self._content.bck_dtransmission_edit.setText(str("%6.4f" % state.background.bck_transmission_spread))
        #self._content.bck_thickness_edit.setText(QtCore.QString("%6.4f" % state.background.sample_thickness))

        self._content.bck_beam_radius_edit.setText(str(state.background.trans_calculation_method.beam_radius))
        self._content.bck_sample_edit.setText(state.background.trans_calculation_method.sample_file)
        self._content.bck_empty_edit.setText(state.background.trans_calculation_method.direct_beam)

        self._content.bck_calculate_radio.setChecked(state.background.calculate_transmission)
        self._content.bck_fix_trans_radio.setChecked(not state.background.calculate_transmission)
        if not self._settings.debug and not state.background.calculate_transmission:
            # If we fix the transmission while not in debug mode, warn the user that
            # things will look weird.
            if len(popup_warning)==0:
                popup_warning = "Turn on debug mode to see all options:\n\n"
            popup_warning += "The background transmission for the loaded reduction was set in debug mode to %-6.1g\n" % state.background.bck_transmission

        self._content.bck_theta_dep_chk.setChecked(state.background.theta_dependent)
        self._content.bck_fit_together_check.setChecked(state.background.combine_transmission_frames)
        self._bck_calculate_clicked(state.background.calculate_transmission)

        if len(popup_warning)>0:
            QtGui.QMessageBox.warning(self, "Turn ON debug mode", popup_warning)

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = DataSets()

        m.transmission = util._check_and_get_float_line_edit(self._content.transmission_edit)
        m.transmission_spread = util._check_and_get_float_line_edit(self._content.dtransmission_edit)
        m.sample_thickness = util._check_and_get_float_line_edit(self._content.sample_thickness_edit)

        m.calculate_transmission = self._content.calculate_radio.isChecked()
        m.theta_dependent = self._content.theta_dep_chk.isChecked()
        m.combine_transmission_frames = self._content.fit_together_check.isChecked()

        d = m.calculation_method
        d.beam_radius = util._check_and_get_float_line_edit(self._content.beam_radius_edit)
        d.sample_file = unicode(self._content.sample_edit.text())
        d.direct_beam = unicode(self._content.empty_edit.text())

        # Data file
        m.data_files = self._get_data_files()
        m.separate_jobs = self._content.separate_jobs_check.isChecked()

        # Background
        b = m.background
        b.background_corr = self._content.background_chk.isChecked()
        b.background_file = str(self._content.background_edit.text())

        b.bck_transmission_enabled = True
        b.bck_transmission = util._check_and_get_float_line_edit(self._content.bck_transmission_edit)
        b.bck_transmission_spread = util._check_and_get_float_line_edit(self._content.bck_dtransmission_edit)
        #b.sample_thickness = util._check_and_get_float_line_edit(self._content.bck_thickness_edit)
        b.calculate_transmission = self._content.bck_calculate_radio.isChecked()
        b.theta_dependent = self._content.bck_theta_dep_chk.isChecked()
        b.combine_transmission_frames = self._content.bck_fit_together_check.isChecked()

        d = b.trans_calculation_method
        d.beam_radius = util._check_and_get_float_line_edit(self._content.bck_beam_radius_edit)
        d.sample_file = unicode(self._content.bck_sample_edit.text())
        d.direct_beam = unicode(self._content.bck_empty_edit.text())

        self._settings.emit_key_value("TRANS_SAMPLE", str(self._content.sample_edit.text()))
        self._settings.emit_key_value("TRANS_DIRECT", str(self._content.empty_edit.text()))
        self._settings.emit_key_value("TRANS_BCK", str(self._content.bck_sample_edit.text()))
        self._settings.emit_key_value("TRANS_DIRECT", str(self._content.bck_empty_edit.text()))
        return m

    def _background_clicked(self, is_checked):
        self._content.background_edit.setEnabled(is_checked)
        #self._content.bck_thickness_edit.setEnabled(is_checked)
        #self._content.bck_thickness_label.setEnabled(is_checked)
        self._content.background_browse.setEnabled(is_checked)
        self._content.background_plot_button.setEnabled(is_checked)
        self._content.bck_calculate_radio.setEnabled(is_checked)
        self._content.bck_theta_dep_chk.setEnabled(is_checked)
        self._content.bck_plus_minus_label.setEnabled(is_checked)
        self._content.transmission_grpbox.setEnabled(is_checked)

        self._bck_calculate_clicked(self._content.bck_calculate_radio.isChecked())

    def _background_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            bck_file = str(self._content.background_edit.text()).strip()
            self._content.background_edit.setText(fname)
            if str(fname).strip() != bck_file:
                #self.get_data_info()
                pass

    def _data_file_browse(self):
        #   Check whether we are updating the data file
        data_files = self._get_data_files()
        current_file = ''
        if len(data_files)>0:
            current_file = data_files[0].strip()

        fname = self.data_browse_dialog(multi=True)
        if fname and len(fname)>0:
            self._content.data_file_edit.setText(';'.join(fname))
            self._settings.last_file = fname[0]
            self._settings.last_data_ws = ''
            if current_file != str(fname[0]).strip():
                self.get_data_info()

    def _sample_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sample_edit.setText(fname)

    def _empty_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.empty_edit.setText(fname)

    def _bck_sample_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.bck_sample_edit.setText(fname)

    def _bck_empty_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.bck_empty_edit.setText(fname)

    def _data_file_plot(self):
        data_files = self._get_data_files()
        if len(data_files)>0:
            self.show_instrument(data_files[0])

    def _calculate_clicked(self):
        is_checked = self._content.calculate_radio.isChecked()

        self._content.transmission_edit.setEnabled(not is_checked)
        self._content.dtransmission_edit.setEnabled(not is_checked)

        self._content.fit_together_check.setEnabled(is_checked)

        self._content.sample_label.setEnabled(is_checked)
        self._content.sample_edit.setEnabled(is_checked)
        self._content.sample_button.setEnabled(is_checked)
        self._content.sample_plot_button.setEnabled(is_checked)

        self._content.empty_label.setEnabled(is_checked)
        self._content.empty_edit.setEnabled(is_checked)
        self._content.empty_button.setEnabled(is_checked)
        self._content.empty_plot_button.setEnabled(is_checked)

        self._content.beam_radius_label.setEnabled(is_checked)
        self._content.beam_radius_edit.setEnabled(is_checked)

    def _bck_calculate_clicked(self, enabled=True):
        is_checked = self._content.bck_calculate_radio.isChecked() and enabled
        self._content.bck_transmission_edit.setEnabled(not is_checked and self._content.background_chk.isChecked())
        self._content.bck_dtransmission_edit.setEnabled(not is_checked and self._content.background_chk.isChecked())

        self._content.bck_fit_together_check.setEnabled(is_checked)

        self._content.bck_sample_label.setEnabled(is_checked)
        self._content.bck_sample_edit.setEnabled(is_checked)
        self._content.bck_sample_button.setEnabled(is_checked)
        self._content.bck_sample_plot_button.setEnabled(is_checked)

        self._content.bck_empty_label.setEnabled(is_checked)
        self._content.bck_empty_edit.setEnabled(is_checked)
        self._content.bck_empty_button.setEnabled(is_checked)
        self._content.bck_empty_plot_button.setEnabled(is_checked)

        self._content.bck_beam_radius_label.setEnabled(is_checked)
        self._content.bck_beam_radius_edit.setEnabled(is_checked)


    def _get_data_files(self):
        """
            Utility method to return the list of data files in the sample data file edit box.
        """
        flist_str = str(self._content.data_file_edit.text())
        flist_str = flist_str.replace(',', ';')
        return flist_str.split(';')

    def _emit_experiment_parameters(self):
        pass

    def get_data_info(self):
        """
            Retrieve information from the data file and update the display
        """
        if self._data_proxy is None:
            return

        data_files = self._get_data_files()
        if len(data_files)<1:
            return
        fname = data_files[0]
        if len(str(fname).strip())>0:
            dataproxy = self._data_proxy(fname)
            if len(dataproxy.errors)>0:
                #QtGui.QMessageBox.warning(self, "Error", dataproxy.errors[0])
                return

            self._settings.last_data_ws = dataproxy.data_ws
            if dataproxy.sample_detector_distance is not None:
                self._settings.emit_key_value("sample_detector_distance", str(dataproxy.sample_detector_distance))
            # Keep for later
            #if dataproxy.sample_thickness is not None:
            #    self._settings.emit_key_value("sample_thickness", QtCore.QString(str(dataproxy.sample_thickness)))
            if dataproxy.beam_diameter is not None:
                self._settings.emit_key_value("beam_diameter", str(dataproxy.beam_diameter))

            self._emit_experiment_parameters()
