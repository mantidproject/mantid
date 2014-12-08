from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import os
import sys
from reduction_gui.reduction.sans.hfir_detector_script import Detector
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.sans.ui_hfir_detector

IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    from mantid.api import AnalysisDataService
    IS_IN_MANTIDPLOT = True
except:
    pass

class DetectorWidget(BaseWidget):
    """
        Widget that presents the detector options to the user
    """
    _method_box = None

    ## Widget name
    name = "Detector"

    def __init__(self, parent=None, state=None, settings=None, show_transmission=True, data_type=None,
                 data_proxy=None, use_sample_dc=False, options_callback=None):
        super(DetectorWidget, self).__init__(parent, state, settings, data_type, data_proxy=data_proxy)

        class DetFrame(QtGui.QFrame, ui.sans.ui_hfir_detector.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = DetFrame(self)
        self._layout.addWidget(self._content)

        # Option to use the sample dark current instead of choosing it separately
        self._use_sample_dc = use_sample_dc
        self.patch_ws = "__patch"
        self.options_callback = options_callback

        self.initialize_content()

        if state is not None:
            self.set_state(state)
        else:
            m = Detector()
            self.set_state(m)

    def initialize_content(self):
        """
            Declare the validators and event connections for the
            widgets loaded through the .ui file.
        """
        # Validators
        self._content.x_pos_edit.setValidator(QtGui.QDoubleValidator(self._content.x_pos_edit))
        self._content.y_pos_edit.setValidator(QtGui.QDoubleValidator(self._content.y_pos_edit))
        self._content.beam_radius_edit.setValidator(QtGui.QDoubleValidator(self._content.beam_radius_edit))
        self._content.min_sensitivity_edit.setValidator(QtGui.QDoubleValidator(self._content.min_sensitivity_edit))
        self._content.max_sensitivity_edit.setValidator(QtGui.QDoubleValidator(self._content.max_sensitivity_edit))

        self.connect(self._content.data_file_browse_button_2, QtCore.SIGNAL("clicked()"), self._flood_beam_finder_browse)

        self.connect(self._content.sensitivity_chk, QtCore.SIGNAL("clicked(bool)"), self._sensitivity_clicked)
        self.connect(self._content.sensitivity_browse_button, QtCore.SIGNAL("clicked()"), self._sensitivity_browse)
        self.connect(self._content.sensitivity_dark_browse_button, QtCore.SIGNAL("clicked()"), self._sensitivity_dark_browse)

        self.connect(self._content.use_beam_finder_checkbox, QtCore.SIGNAL("clicked(bool)"), self._use_beam_finder_changed)
        self.connect(self._content.scattering_data, QtCore.SIGNAL("clicked()"), self._center_method_changed)
        self.connect(self._content.direct_beam, QtCore.SIGNAL("clicked()"), self._center_method_changed)

        self.connect(self._content.use_sample_center_checkbox, QtCore.SIGNAL("clicked(bool)"), self._use_sample_center_changed)
        self.connect(self._content.scattering_data_2, QtCore.SIGNAL("clicked()"), self._flood_center_method_changed)
        self.connect(self._content.direct_beam_2, QtCore.SIGNAL("clicked()"), self._flood_center_method_changed)
        self.connect(self._content.use_beam_finder_checkbox_2, QtCore.SIGNAL("clicked(bool)"), self._flood_use_beam_finder_changed)

        self.connect(self._content.data_file_browse_button, QtCore.SIGNAL("clicked()"), self._beam_finder_browse)

        self._use_beam_finder_changed(self._content.use_beam_finder_checkbox.isChecked())
        self._content.use_sample_center_checkbox.setChecked(True)
        self._sensitivity_clicked(self._content.sensitivity_chk.isChecked())
        self._use_sample_center_changed(self._content.use_sample_center_checkbox.isChecked())


        self.connect(self._content.sensitivity_plot_button, QtCore.SIGNAL("clicked()"), self._sensitivity_plot_clicked)
        self.connect(self._content.data_file_plot_button, QtCore.SIGNAL("clicked()"), self._data_file_plot_clicked)
        self.connect(self._content.sensitivity_dark_plot_button, QtCore.SIGNAL("clicked()"), self._sensitivity_dark_plot_clicked)
        self.connect(self._content.data_file_plot_button_2, QtCore.SIGNAL("clicked()"), self._data_file_plot2_clicked)

        # Patch sensitivity
        self.connect(self._content.patch_sensitivity_check, QtCore.SIGNAL("clicked()"), self._patch_checked)
        self.connect(self._content.draw_patch_button, QtCore.SIGNAL("clicked()"), self._draw_patch)
        self.connect(self._content.create_sensitivity_button, QtCore.SIGNAL("clicked()"), self._create_sensitivity)
        self._patch_checked()

        if not self._in_mantidplot:
            self._content.sensitivity_plot_button.hide()
            self._content.sensitivity_dark_plot_button.hide()
            self._content.data_file_plot_button.hide()
            self._content.data_file_plot_button_2.hide()

        if self._use_sample_dc:
            self._content.sensitivity_dark_layout.deleteLater()
            self._content.sensitivity_dark_file_label.deleteLater()
            self._content.sensitivity_dark_file_edit.deleteLater()
            self._content.sensitivity_dark_browse_button.deleteLater()
            self._content.sensitivity_dark_plot_button.deleteLater()

        if not self._settings.debug and not self._settings.advanced:
            self._content.flood_center_grpbox.hide()
            self._content.direct_beam.setChecked(True)
            self._content.direct_beam.hide()
            self._content.scattering_data.hide()
            self._content.beam_radius_edit.hide()
            self._content.beam_radius_label.hide()
            self._content.min_sensitivity_edit.hide()
            self._content.max_sensitivity_edit.hide()
            self._content.sensitivity_range_label.hide()
            self._content.sensitivity_min_label.hide()
            self._content.sensitivity_max_label.hide()

        if self.options_callback is None or not self._settings.debug:
            # Patch sensitivity
            self._content.patch_sensitivity_check.hide()
            self._content.draw_patch_button.hide()
            self._content.create_sensitivity_button.hide()

    def _patch_checked(self):
        enabled = self._content.patch_sensitivity_check.isChecked()
        self._content.draw_patch_button.setEnabled(enabled)
        self._content.create_sensitivity_button.setEnabled(enabled)

    def _draw_patch(self):
        if IS_IN_MANTIDPLOT:
            self.show_instrument(self._content.sensitivity_file_edit.text,
              workspace=self.patch_ws, tab=2, reload=True, data_proxy=None)

    def _create_sensitivity(self):
        if IS_IN_MANTIDPLOT and self.options_callback is not None:
            # Get patch information
            patch_ws = ""
            if AnalysisDataService.doesExist(self.patch_ws):
                patch_ws = self.patch_ws

            try:
                reduction_table_ws = self.options_callback()
                patch_output = AnalysisDataService.doesExist(patch_ws)
                filename = self._content.sensitivity_file_edit.text()
                script  = "ComputeSensitivity(Filename='%s',\n" % filename
                script += "                   ReductionProperties='%s',\n" % reduction_table_ws
                script += "                   OutputWorkspace='sensitivity',\n"
                script += "                   PatchWorkspace='%s')\n" % patch_ws
                mantidplot.runPythonScript(script, True)
            except:
                print "Could not compute sensitivity"
                print sys.exc_value

    def _sensitivity_plot_clicked(self):
        self.show_instrument(file_name=self._content.sensitivity_file_edit.text)

    def _data_file_plot_clicked(self):
        self.show_instrument(file_name=self._content.beam_data_file_edit.text)

    def _sensitivity_dark_plot_clicked(self):
        self.show_instrument(file_name=self._content.sensitivity_dark_file_edit.text)

    def _data_file_plot2_clicked(self):
        self.show_instrument(file_name=self._content.beam_data_file_edit_2.text)

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state.
            @param state: Transmission object
        """
        popup_warning = ""
        # Beam finder
        if state.x_position is not None and state.y_position is not None:
            self._content.x_pos_edit.setText(str("%6.4f" % state.x_position))
            self._content.y_pos_edit.setText(str("%6.4f" % state.y_position))
        self._content.use_beam_finder_checkbox.setChecked(state.use_finder)
        self._content.beam_data_file_edit.setText(state.beam_file)
        self._content.beam_radius_edit.setText(str(state.beam_radius))

        if not self._settings.debug and not self._settings.advanced and not state.use_direct_beam:
            # Warn the user if we're about to set an option that is not seen!
            popup_warning += "The reduction you are loading has advanced options for finding the beam center.\n"
            popup_warning += "Use the advanced interface to see all option. Those options will now be skipped."
            state.use_direct_beam = True

        self._content.direct_beam.setChecked(state.use_direct_beam)
        self._content.scattering_data.setChecked(not state.use_direct_beam)
        self._content.beam_radius_edit.setEnabled(not state.use_direct_beam)

        self._use_beam_finder_changed(state.use_finder)

        # Sensitivity
        self._content.sensitivity_file_edit.setText(state.sensitivity_data)
        self._content.sensitivity_chk.setChecked(state.sensitivity_corr)
        self._sensitivity_clicked(state.sensitivity_corr)
        self._content.min_sensitivity_edit.setText(str(state.min_sensitivity))
        self._content.max_sensitivity_edit.setText(str(state.max_sensitivity))
        if not self._use_sample_dc:
            self._content.sensitivity_dark_file_edit.setText(state.sensitivity_dark)

        if not self._settings.debug and not self._settings.advanced and not state.use_sample_beam_center:
            # Warn the user if we're about to set an option that is not seen!
            popup_warning += "The reduction you are loading has advanced options for the flood field beam center.\n"
            popup_warning += "Use the advanced interface to see all option. Those options will now be skipped."
            state.use_sample_beam_center = True

        self._content.use_sample_center_checkbox.setChecked(state.use_sample_beam_center)
        self._content.x_pos_edit_2.setText(str("%6.4f" % state.flood_x_position))
        self._content.y_pos_edit_2.setText(str("%6.4f" % state.flood_y_position))
        self._content.use_beam_finder_checkbox_2.setChecked(state.flood_use_finder)
        self._content.beam_data_file_edit_2.setText(state.flood_beam_file)
        self._content.beam_radius_edit_2.setText(str(state.flood_beam_radius))

        self._content.direct_beam_2.setChecked(state.flood_use_direct_beam)
        self._content.scattering_data_2.setChecked(not state.flood_use_direct_beam)
        self._content.beam_radius_edit_2.setEnabled(not state.flood_use_direct_beam)

        self._sensitivity_clicked(self._content.sensitivity_chk.isChecked())
        self._use_sample_center_changed(self._content.use_sample_center_checkbox.isChecked())

        if len(popup_warning)>0:
            QtGui.QMessageBox.warning(self, "Turn ON advanced interface", popup_warning)

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = Detector()

        # Mask
        m.x_position = util._check_and_get_float_line_edit(self._content.x_pos_edit)
        m.y_position = util._check_and_get_float_line_edit(self._content.y_pos_edit)
        m.beam_radius = util._check_and_get_float_line_edit(self._content.beam_radius_edit)
        m.use_finder = self._content.use_beam_finder_checkbox.isChecked()
        m.beam_file = unicode(self._content.beam_data_file_edit.text())
        m.use_direct_beam = self._content.direct_beam.isChecked()

        # Sensitivity
        m.sensitivity_corr = self._content.sensitivity_chk.isChecked()
        m.sensitivity_data = unicode(self._content.sensitivity_file_edit.text())
        m.min_sensitivity = util._check_and_get_float_line_edit(self._content.min_sensitivity_edit)
        m.max_sensitivity = util._check_and_get_float_line_edit(self._content.max_sensitivity_edit)
        if not self._use_sample_dc:
            m.sensitivity_dark = unicode(self._content.sensitivity_dark_file_edit.text())
        m.use_sample_dark = self._use_sample_dc

        m.use_sample_beam_center = self._content.use_sample_center_checkbox.isChecked()
        m.flood_x_position = util._check_and_get_float_line_edit(self._content.x_pos_edit_2)
        m.flood_y_position = util._check_and_get_float_line_edit(self._content.y_pos_edit_2)
        m.flood_beam_radius = util._check_and_get_float_line_edit(self._content.beam_radius_edit_2)
        m.flood_use_finder = self._content.use_beam_finder_checkbox_2.isChecked()
        m.flood_beam_file = unicode(self._content.beam_data_file_edit_2.text())
        m.flood_use_direct_beam = self._content.direct_beam_2.isChecked()

        self._settings.emit_key_value("FLOOD_FIELD", str(self._content.sensitivity_file_edit.text()))
        return m

    def _use_sample_center_changed(self, is_checked):
        self._content.use_beam_finder_checkbox_2.setEnabled(not is_checked)
        self._flood_use_beam_finder_changed(self._content.use_beam_finder_checkbox_2.isChecked(), not is_checked)


    def _flood_use_beam_finder_changed(self, is_checked, is_flood_ctr=True):
        # Center by hand
        self._content.x_pos_edit_2.setEnabled(not is_checked and is_flood_ctr)
        self._content.y_pos_edit_2.setEnabled(not is_checked and is_flood_ctr)
        self._content.x_pos_label_2.setEnabled(not is_checked and is_flood_ctr)
        self._content.y_pos_label_2.setEnabled(not is_checked and is_flood_ctr)

        # Center computed
        self._content.direct_beam_2.setEnabled(is_checked and is_flood_ctr)
        self._content.scattering_data_2.setEnabled(is_checked and is_flood_ctr)
        self._content.data_file_label_2.setEnabled(is_checked and is_flood_ctr)
        self._content.beam_radius_label_2.setEnabled(is_checked and is_flood_ctr)
        self._content.beam_data_file_edit_2.setEnabled(is_checked and is_flood_ctr)
        self._content.data_file_browse_button_2.setEnabled(is_checked and is_flood_ctr)
        self._content.data_file_plot_button_2.setEnabled(is_checked and is_flood_ctr)
        self._flood_center_method_changed(is_checked and is_flood_ctr)



    def _center_method_changed(self, finder_checked=True):
        is_direct_beam = self._content.direct_beam.isChecked()
        self._content.beam_radius_edit.setEnabled(not is_direct_beam and finder_checked)

    def _flood_center_method_changed(self, finder_checked=True):
        is_direct_beam = self._content.direct_beam_2.isChecked()
        self._content.beam_radius_edit_2.setEnabled(not is_direct_beam and finder_checked)

    def _beam_finder_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.beam_data_file_edit.setText(fname)

    def _flood_beam_finder_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.beam_data_file_edit_2.setText(fname)

    def _use_beam_finder_changed(self, is_checked):
        """
            Call-back method for when the user toggles between using a
            beam finder or setting the beam center by hand
        """
        # Center by hand
        self._content.x_pos_edit.setEnabled(not is_checked)
        self._content.y_pos_edit.setEnabled(not is_checked)
        #self._content.x_pos_label.setEnabled(not is_checked)
        #self._content.y_pos_label.setEnabled(not is_checked)

        # Center computed
        self._content.direct_beam.setEnabled(is_checked)
        self._content.scattering_data.setEnabled(is_checked)
        self._content.data_file_label.setEnabled(is_checked)
        self._content.beam_radius_label.setEnabled(is_checked)
        self._content.beam_data_file_edit.setEnabled(is_checked)
        self._content.data_file_browse_button.setEnabled(is_checked)
        self._content.data_file_plot_button.setEnabled(is_checked)
        self._center_method_changed(is_checked)

    def _sensitivity_clicked(self, is_checked):
        self._content.sensitivity_file_edit.setEnabled(is_checked)
        self._content.sensitivity_browse_button.setEnabled(is_checked)
        self._content.sensitivity_plot_button.setEnabled(is_checked)

        self._content.min_sensitivity_edit.setEnabled(is_checked)
        self._content.max_sensitivity_edit.setEnabled(is_checked)
        self._content.sensitivity_file_label.setEnabled(is_checked)
        self._content.sensitivity_range_label.setEnabled(is_checked)
        self._content.sensitivity_min_label.setEnabled(is_checked)
        self._content.sensitivity_max_label.setEnabled(is_checked)
        self._content.flood_center_grpbox.setEnabled(is_checked)

        if not self._use_sample_dc:
            self._content.sensitivity_dark_file_edit.setEnabled(is_checked)
            self._content.sensitivity_dark_browse_button.setEnabled(is_checked)
            self._content.sensitivity_dark_plot_button.setEnabled(is_checked)
            self._content.sensitivity_dark_file_label.setEnabled(is_checked)

        self._content.patch_sensitivity_check.setEnabled(is_checked)
        if is_checked:
            self._patch_checked()
        else:
            self._content.draw_patch_button.setEnabled(False)
            self._content.create_sensitivity_button.setEnabled(False)


    def _sensitivity_browse(self):
        fname = self.data_browse_dialog(data_type="Sensitivity files *.xml *.nxs (*.xml *.nxs)")
        if fname:
            self._content.sensitivity_file_edit.setText(fname)

    def _sensitivity_dark_browse(self):
        fname = self.data_browse_dialog()
        if fname:
            self._content.sensitivity_dark_file_edit.setText(fname)

