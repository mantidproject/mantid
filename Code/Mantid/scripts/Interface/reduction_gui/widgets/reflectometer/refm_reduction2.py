from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
import time
import sys
from functools import partial
from reduction_gui.reduction.reflectometer.refm_data_script import DataSets as REFMDataSets
from reduction_gui.reduction.reflectometer.refl_data_series import DataSeries
from reduction_gui.settings.application_settings import GeneralSettings
from base_ref_reduction import BaseRefWidget
import ui.reflectometer.ui_refm_reduction

IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    from MantidFramework import *
    mtd.initialise(False)
    from mantidsimple import *
    import _qti
    from reduction.instruments.reflectometer import data_manipulation

    IS_IN_MANTIDPLOT = True
except:
    pass

class DataReflWidget(BaseRefWidget):    
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Data"      
    instrument_name = 'REF_M'
    short_name = 'REFM'
    peak_pixel_range = []
    background_pixel_range = []

    def __init__(self, parent=None, state=None, settings=None, name="REFM", data_proxy=None):      
        super(DataReflWidget, self).__init__(parent=parent, state=state, settings=settings, name=name, data_proxy=data_proxy) 

        class SummaryFrame(QtGui.QFrame, ui.reflectometer.ui_refm_reduction.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self.short_name = name
        self._settings.instrument_name = name
            
        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)

        self._detector_distance = 1.0
        self._sangle_parameter = 0.0

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(DataSeries(data_class=REFMDataSets))

    def initialize_content(self):
        super(DataReflWidget, self).initialize_content()
        self._summary.q_step_edit.setValidator(QtGui.QIntValidator(self._summary.q_step_edit))
        self._summary.tof_bin_width_edit.setValidator(QtGui.QDoubleValidator(self._summary.tof_bin_width_edit))

        call_back = partial(self._edit_event, ctrl=self._summary.tof_bin_width_edit)
        self.connect(self._summary.tof_bin_width_edit, QtCore.SIGNAL("textChanged(QString)"), call_back)

        self._ref_instrument_selected()

        self.connect(self._summary.det_angle_edit, QtCore.SIGNAL("textChanged(QString)"), self._update_scattering_angle)
        self.connect(self._summary.det_angle_offset_edit, QtCore.SIGNAL("textChanged(QString)"), self._update_scattering_angle)
        self.connect(self._summary.direct_pixel_edit, QtCore.SIGNAL("textChanged(QString)"), self._update_scattering_angle)
        self.connect(self._summary.center_pix_edit, QtCore.SIGNAL("textChanged(QString)"), self._update_scattering_angle)
                
    def _reset_warnings(self):
        super(DataReflWidget, self)._reset_warnings()
        util.set_edited(self._summary.tof_bin_width_edit, False)

    def _det_angle_offset_chk_changed(self):
        is_checked = self._summary.det_angle_offset_check.isChecked()
        self._summary.det_angle_offset_edit.setEnabled(is_checked)
    
    def _det_angle_chk_changed(self):
        is_checked = self._summary.det_angle_check.isChecked()
        self._summary.det_angle_edit.setEnabled(is_checked)
    
    def _direct_pixel_chk_changed(self):
        is_checked = self._summary.direct_pixel_check.isChecked()
        self._summary.direct_pixel_edit.setEnabled(is_checked)
        
    def _scattering_angle_changed(self):
        self._summary.det_angle_check.setEnabled(True)
        self._det_angle_chk_changed()
        self._summary.det_angle_offset_check.setEnabled(True)
        self._det_angle_offset_chk_changed()
        self._summary.direct_pixel_check.setEnabled(True)
        self._direct_pixel_chk_changed()
        self._summary.det_angle_unit_label.setEnabled(True)   
        self._summary.det_angle_offset_unit_label.setEnabled(True)   
            
    def _data_peak_range_changed(self):
        """
            Update the direct pixel value when the data peak
            range changes
        """
        min_pix = float(self._summary.data_peak_from_pixel.text())
        max_pix = float(self._summary.data_peak_to_pixel.text())
        dir_pix = (max_pix+min_pix)/2.0
        dir_pix_str = "%4.4g" % dir_pix
        self._summary.center_pix_edit.setText(dir_pix_str.strip())
     
    def _update_scattering_angle(self):
        dangle = util._check_and_get_float_line_edit(self._summary.det_angle_edit)
        dangle0 = util._check_and_get_float_line_edit(self._summary.det_angle_offset_edit)
        direct_beam_pix = util._check_and_get_float_line_edit(self._summary.direct_pixel_edit)
        ref_pix = util._check_and_get_float_line_edit(self._summary.center_pix_edit)
        PIXEL_SIZE = 0.0007 # m
        
        delta = (dangle-dangle0)*math.pi/180.0/2.0\
            + ((direct_beam_pix-ref_pix)*PIXEL_SIZE)/ (2.0*self._detector_distance)
        
        scattering_angle = delta*180.0/math.pi
        scattering_angle_str = "%4.3g" % scattering_angle
        self._summary.angle_edit.setText(scattering_angle_str.strip())
     
    def _ref_instrument_selected(self):
        self.instrument_name = "REF_M"
        self._summary.center_pix_edit.show()
        self._summary.center_pix_edit.setEnabled(False)
        self._summary.angle_edit.setEnabled(False)
        self._summary.angle_edit.show()
        self._summary.angle_unit_label.show()
        self._summary.angle_offset_label.hide()
        self._summary.angle_offset_edit.hide()
        self._summary.angle_offset_pm_label.hide()
        self._summary.angle_offset_error_edit.hide()
        self._summary.angle_offset_unit_label.hide()  
        self._summary.q_bins_label.hide()
        self._summary.q_step_edit.hide()
        self._summary.q_step_label.hide()
        self._summary.q_step_unit_label.hide()
        self._summary.q_min_edit.hide()
        self._summary.q_min_label.hide()
        self._summary.q_min_unit_label.hide() 
        #TODO: allow log binning
        self._summary.log_scale_chk.hide()

    def _norm_clicked(self, is_checked):
        """
            This is reached when the user clicks the Normalization switch and will
            turn on/off all the option related to the normalization file
        """
        self._summary.norm_run_number_label.setEnabled(is_checked)
        self._summary.norm_run_number_edit.setEnabled(is_checked)
        self._summary.norm_peak_selection_label.setEnabled(is_checked)
        self._summary.norm_peak_selection_from_label.setEnabled(is_checked)
        self._summary.norm_peak_from_pixel.setEnabled(is_checked)
        self._summary.norm_peak_selection_to_label.setEnabled(is_checked)
        self._summary.norm_peak_to_pixel.setEnabled(is_checked)
        
        self._summary.norm_background_switch.setEnabled(is_checked)
        if (not(is_checked)):
            self._norm_background_clicked(False)
        else:
            NormBackFlag = self._summary.norm_background_switch.isChecked()
            self._norm_background_clicked(NormBackFlag)
        
        self._summary.norm_low_res_range_switch.setEnabled(is_checked)
        if (not(is_checked)):
            self._norm_low_res_clicked(False)
        else:
            LowResFlag = self._summary.norm_low_res_range_switch.isChecked()
            self._norm_low_res_clicked(LowResFlag)
        
        self._edit_event(None, self._summary.norm_switch)


    def _add_data(self):
        super(DataReflWidget, self)._add_data()
        # Read logs
        self._read_logs()
        self._reset_warnings()

    def _read_logs(self):
        if IS_IN_MANTIDPLOT:
            # Showing the waiting message
            self._summary.waiting_label.show()
            self._summary.update()
            QtGui.QApplication.processEvents()
            QtGui.QApplication.hasPendingEvents()
            
            try:
                run_entry = str(self._summary.data_run_number_edit.text()).strip()
                if len(run_entry)==0 or run_entry=="0":
                    return
                
                logs = data_manipulation.get_logs(self.instrument_name, run_entry)
                if not self._summary.direct_pixel_check.isChecked():
                    angle_str = "%-4.3g"%logs["DIRPIX"]
                    self._summary.direct_pixel_edit.setText(angle_str.strip())
                if not self._summary.det_angle_offset_check.isChecked():
                    angle_str = "%-4.4g"%logs["DANGLE0"]
                    self._summary.det_angle_offset_edit.setText(angle_str.strip())
                if not self._summary.det_angle_check.isChecked():
                    angle_str = "%-4.4g"%logs["DANGLE"]
                    self._summary.det_angle_edit.setText(angle_str.strip())
                
                angle_str = "%-4.4g"%logs["SANGLE"]
                self._summary.angle_edit.setText(angle_str.strip())
                                        
                self._sangle_parameter = logs["SANGLE"]
                self._detector_distance = logs["DET_DISTANCE"]
            except:
                # Could not read in the parameters, skip.
                msg = "No data set was found for run %s\n\n" % run_entry
                msg += "Make sure that your data directory was added to the "
                msg += "Mantid search directories."
                QtGui.QMessageBox.warning(self, 
                                          "Unable to find data set", msg)
            
            self._summary.waiting_label.hide()
       
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state. 
            @param state: data object    
        """
        super(DataReflWidget, self).set_state(state)
        
        if len(state.data_sets)>0:
            self._summary.q_step_edit.setText(str(state.data_sets[0].q_bins))
            
        self._reset_warnings()
        
    def set_editing_state(self, state):
        super(DataReflWidget, self).set_editing_state(state)
        
        self._summary.tof_bin_width_edit.setText(str(state.TOFstep))
        
        self._summary.det_angle_check.setChecked(state.set_detector_angle)
        if state.set_detector_angle:
            self._summary.det_angle_edit.setText(str(state.detector_angle).strip())
        self._summary.det_angle_offset_check.setChecked(state.set_detector_angle_offset)
        if state.set_detector_angle_offset:
            self._summary.det_angle_offset_edit.setText(str(state.detector_angle_offset).strip())
        self._summary.direct_pixel_check.setChecked(state.set_direct_pixel)
        if state.set_direct_pixel:
            self._summary.direct_pixel_edit.setText(str(state.direct_pixel).strip())
        self._det_angle_chk_changed()
        self._det_angle_offset_chk_changed()
        self._direct_pixel_chk_changed()
        
        # Q binning
        self._summary.q_step_edit.setText(str(state.q_bins))

        # Scattering angle
        self._scattering_angle_changed()
            
        self._read_logs()
        self._update_scattering_angle()

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = self.get_editing_state()
        state = DataSeries(data_class=REFMDataSets)
        state_list = []
        
        # Common Q binning
        q_min = float(self._summary.q_min_edit.text())
        q_step = float(self._summary.q_step_edit.text())
        q_bins = int(math.ceil(float(self._summary.q_step_edit.text())))
            
        for i in range(self._summary.angle_list.count()):
            data = self._summary.angle_list.item(i).data(QtCore.Qt.UserRole).toPyObject()
            # Over-write Q binning with common binning
            data.q_min = q_min
            data.q_step = q_step
        
            # Over-write angle offset
            if hasattr(data, "q_bins"):
                data.q_bins = q_bins
                data.q_log = self._summary.log_scale_chk.isChecked()

            if hasattr(data, "output_dir"):
                data.output_dir = self._summary.outdir_edit.text()
            
            state_list.append(data)
        state.data_sets = state_list
        
        return state
    
    def get_editing_state(self):
        m = REFMDataSets()
        
        #Peak from/to pixels
        m.DataPeakPixels = [int(self._summary.data_peak_from_pixel.text()),
                            int(self._summary.data_peak_to_pixel.text())] 
        
        m.data_x_range = [int(self._summary.x_min_edit.text()),
                     int(self._summary.x_max_edit.text())]
        m.data_x_range_flag = self._summary.data_low_res_range_switch.isChecked()
        
        m.norm_x_range = [int(self._summary.norm_x_min_edit.text()),
                          int(self._summary.norm_x_max_edit.text())]
        m.norm_x_range_flag = self._summary.norm_low_res_range_switch.isChecked()
        
        #Background flag
        m.DataBackgroundFlag = self._summary.data_background_switch.isChecked()

        #Background from/to pixels
        roi1_from = int(self._summary.data_background_from_pixel1.text())
        roi1_to = int(self._summary.data_background_to_pixel1.text())
        m.DataBackgroundRoi = [roi1_from, roi1_to, 0, 0]

        # From TOF and to TOF
        from_tof = float(self._summary.data_from_tof.text())
        to_tof = float(self._summary.data_to_tof.text())
        m.DataTofRange = [from_tof, to_tof]
        
        m.TOFstep = float(self._summary.tof_bin_width_edit.text())
    
        datafiles = str(self._summary.data_run_number_edit.text()).split(',')
        m.data_files = [int(i) for i in datafiles]
    
        # Normalization flag
        m.NormFlag = self._summary.norm_switch.isChecked()

        # Normalization options
        m.norm_file = int(self._summary.norm_run_number_edit.text())
        m.NormPeakPixels = [int(self._summary.norm_peak_from_pixel.text()),
                            int(self._summary.norm_peak_to_pixel.text())]   
        
        #Background flag
        m.NormBackgroundFlag = self._summary.norm_background_switch.isChecked()

        #Background from/to pixels
        roi1_from = int(self._summary.norm_background_from_pixel1.text())
        roi1_to = int(self._summary.norm_background_to_pixel1.text())
        m.NormBackgroundRoi = [roi1_from, roi1_to]
        
        if hasattr(m, "set_detector_angle"):
            m.set_detector_angle = self._summary.det_angle_check.isChecked()
            m.detector_angle = util._check_and_get_float_line_edit(self._summary.det_angle_edit)
            m.set_detector_angle_offset = self._summary.det_angle_offset_check.isChecked()
            m.detector_angle_offset = util._check_and_get_float_line_edit(self._summary.det_angle_offset_edit)
            m.set_direct_pixel = self._summary.direct_pixel_check.isChecked()
            m.direct_pixel = util._check_and_get_float_line_edit(self._summary.direct_pixel_edit)

        return m