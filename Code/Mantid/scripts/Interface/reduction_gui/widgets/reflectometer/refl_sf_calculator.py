from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
import time
import sys
from functools import partial
from reduction_gui.reduction.reflectometer.refl_sf_calculator_data_script import DataSets as REFLDataSets
from reduction_gui.reduction.reflectometer.refl_sf_calculator_data_series import DataSeries
from reduction_gui.settings.application_settings import GeneralSettings
#from base_ref_reduction import BaseRefWidget
from reduction_gui.widgets.base_widget import BaseWidget as BaseRefWidget
import ui.reflectometer.ui_refl_sf_calculator

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

class DataReflSFCalculatorWidget(BaseRefWidget):    
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Data"      
    instrument_name = 'REF_L'
    short_name = 'REFL'
    peak_pixel_range = []
    background_pixel_range = []

    def __init__(self, parent=None, state=None, settings=None, name="REFL", data_proxy=None):      
        super(DataReflSFCalculatorWidget, self).__init__(parent, state, settings, data_proxy=data_proxy) 

        class SummaryFrame(QtGui.QFrame, ui.reflectometer.ui_refl_sf_calculator.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self.short_name = name
        self._settings.instrument_name = name
            
        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(DataSeries(data_class=REFLDataSets))

    def initialize_content(self):
        """
        Make sure the text fields accept only the right format of data
        """

        #hide labels
        self._summary.waiting_label.hide()
        self._summary.data_run_number_processing.hide()
                
        self._summary.data_run_number_edit.setValidator(QtGui.QIntValidator(self._summary.data_run_number_edit))
        self._summary.number_of_attenuator.setValidator(QtGui.QIntValidator(self._summary.number_of_attenuator))
        self._summary.data_peak_from_pixel.setValidator(QtGui.QIntValidator(self._summary.data_peak_from_pixel))
        self._summary.data_peak_to_pixel.setValidator(QtGui.QIntValidator(self._summary.data_peak_to_pixel))
        self._summary.data_background_from_pixel.setValidator(QtGui.QIntValidator(self._summary.data_background_from_pixel))
        self._summary.data_background_to_pixel.setValidator(QtGui.QIntValidator(self._summary.data_background_to_pixel))
        
        #Event connections
        self.connect(self._summary.data_run_number_edit, QtCore.SIGNAL("returnPressed()"), self.data_run_number_validated)
        self.connect(self._summary.add_dataset_btn, QtCore.SIGNAL("clicked()"), self._add_data)
        self.connect(self._summary.data_background_switch, QtCore.SIGNAL("clicked(bool)"), self._data_background_clicked)
        self.connect(self._summary.remove_btn, QtCore.SIGNAL("clicked()"), self._remove_item)

        #Catch edited controls        
        #Incident medium (selection or text changed)
        call_back = partial(self._edit_event, ctrl=self._summary.incident_medium_combobox)
        self.connect(self._summary.incident_medium_combobox, QtCore.SIGNAL("editTextChanged(QString)"), call_back)
        #Number of attenuator value changed
        call_back = partial(self._edit_event, ctrl=self._summary.number_of_attenuator)
        self.connect(self._summary.number_of_attenuator, QtCore.SIGNAL("textChanged(QString)"), call_back)
        #peak selection (from and to) changed
        call_back = partial(self._edit_event, ctrl=self._summary.data_peak_from_pixel)
        self.connect(self._summary.data_peak_from_pixel, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.data_peak_to_pixel)
        self.connect(self._summary.data_peak_to_pixel, QtCore.SIGNAL("textChanged(QString)"), call_back)
        #background flag and from/to textEdit changed
        call_back = partial(self._edit_event, ctrl=self._summary.data_background_switch)
        self.connect(self._summary.data_background_switch, QtCore.SIGNAL("clicked()"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.data_background_from_pixel)
        self.connect(self._summary.data_background_from_pixel, QtCore.SIGNAL("textChanged(QString)"), call_back)
        call_back = partial(self._edit_event, ctrl=self._summary.data_background_to_pixel)
        self.connect(self._summary.data_background_to_pixel, QtCore.SIGNAL("textChanged(QString)"), call_back)
                
    def _ref_instrument_selected(self):
        self.instrument_name = "REF_L"
        self._summary.center_pix_radio.hide()
        self._summary.center_pix_edit.hide()
        self._summary.angle_radio.hide()
        self._summary.angle_edit.hide()
        self._summary.angle_unit_label.hide()
        self._summary.angle_offset_label.show()
        self._summary.angle_offset_edit.show()
        self._summary.angle_offset_pm_label.show()
        self._summary.angle_offset_error_edit.show()
        self._summary.angle_offset_unit_label.show()
        self._summary.det_angle_offset_check.hide()
        self._summary.det_angle_offset_edit.hide()
        self._summary.det_angle_offset_unit_label.hide()
        self._summary.det_angle_check.hide()
        self._summary.det_angle_edit.hide()
        self._summary.det_angle_unit_label.hide()
        self._summary.direct_pixel_check.hide()
        self._summary.direct_pixel_edit.hide()
        self._summary.q_bins_label.hide()
        self._summary.ref_pix_estimate.hide()
        
        # Output directory
        self._summary.outdir_label.hide()
        self._summary.outdir_edit.hide()
        self._summary.outdir_browse_button.hide()

        #TODO: allow log binning
        self._summary.log_scale_chk.hide()
                 
    def _remove_item(self):
        if self._summary.angle_list.count()==0:
            return
        self._summary.angle_list.setEnabled(False)        
        self._summary.remove_btn.setEnabled(False)  
        row = self._summary.angle_list.currentRow()
        if row>=0:
            self._summary.angle_list.takeItem(row)
        self._summary.angle_list.setEnabled(True)        
        self._summary.remove_btn.setEnabled(True)  

    def _edit_event(self, text=None, ctrl=None):
        self._summary.edited_warning_label.show()
        util.set_edited(ctrl,True)

    def _run_number_changed(self):
        self._edit_event(ctrl=self._summary.data_run_number_edit)

    def data_run_number_validated(self):
        self._summary.data_run_number_processing.show()
        print 'inside data_run_number_validated'
        run_number = self._summary.data_run_number_edit.text()
        _file = FileFinder.findRuns("REF_L%d"%int(run_number))
#        S1H = ''
#        S2H = ''
#        S1W = ''
#        S2W = ''
        lambdaRequest = ''
        print _file[0]
#        self.getSlitsValueAndLambda(_file[0], S1H, S2H, S1W, S2W, lambdaRequest)
        _file = '/mnt/hgfs/j35/results/REF_L_70982_event.nxs'
        S1H, S2H, S1W, S2W, lambdaRequest = self.getSlitsValueAndLambda(_file)
        print S1H
        print S2H
        print S1W
        print S2W
        
        self._summary.s1h.setText(S1H)
        self._summary.s2h.setText(S2H)
        self._summary.s1w.setText(S1W)
        self._summary.s2w.setText(S2W)
        self._summary.lambda_request.setText(lambdaRequest)

        
        
        
        
        
        self._summary.data_run_number_processing.hide()
        

    def _add_data(self):
        state = self.get_editing_state()
        in_list = False
        # Check whether it's already in the list
        run_numbers = self._summary.data_run_number_edit.text()
        if (run_numbers == ''):
            return
        list_items = self._summary.angle_list.findItems(run_numbers, QtCore.Qt.MatchFixedString)
        if len(list_items)>0:
            list_items[0].setData(QtCore.Qt.UserRole, state)
            in_list = True
        else:
            item_widget = QtGui.QListWidgetItem(run_numbers, self._summary.angle_list)
            item_widget.setData(QtCore.Qt.UserRole, state)
        
#        # Read logs
#        if not in_list and self.short_name == "REFM":
#            self._read_logs()
        
        self._reset_warnings()
    
    def _data_background_clicked(self, is_checked):
        """
            This is reached when the user clicks the Background switch and will enabled or not
            the widgets that follow that button
        """
        self._summary.data_background_from_pixel.setEnabled(is_checked)
        self._summary.data_background_from_pixel_label.setEnabled(is_checked)
        self._summary.data_background_to_pixel.setEnabled(is_checked)
        self._summary.data_background_to_pixel_label.setEnabled(is_checked)
        self._summary.plot_count_vs_y_bck_btn.setEnabled(is_checked)
        self._edit_event(None, self._summary.data_background_switch)
    
    def _reset_warnings(self):
        self._summary.edited_warning_label.hide()
        util.set_edited(self._summary.data_run_number_edit, False)
        util.set_edited(self._summary.incident_medium_combobox, False)
        util.set_edited(self._summary.number_of_attenuator, False)
        util.set_edited(self._summary.data_peak_from_pixel, False)
        util.set_edited(self._summary.data_peak_to_pixel, False)
        util.set_edited(self._summary.data_background_switch, False)
        util.set_edited(self._summary.data_background_from_pixel, False)
        util.set_edited(self._summary.data_background_to_pixel, False)
    
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state. 
            @param state: data object    
        """
        super(DataReflSFCalculatorWidget, self).set_state(state)
        
#        if len(state.data_sets)>0:
#            self._summary.q_step_edit.setText(str(math.fabs(state.data_sets[0].q_step)))
            
#        self._reset_warnings()

    def set_editing_state(self, state):
        super(DataReflSFCalculatorWidget, self).set_editing_state(state)
                
        self._summary.tof_range_switch.setChecked(state.crop_TOF_range)
        self._tof_range_clicked(state.crop_TOF_range)
                                                
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = self.get_editing_state()
        state = DataSeries(data_class=REFLDataSets)
        state_list = []
        
#        # Common Q binning
#        q_min = float(self._summary.q_min_edit.text())
#        q_step = float(self._summary.q_step_edit.text())
#        if self._summary.log_scale_chk.isChecked():
#            q_step = -q_step
#            
#        # Angle offset
#        if hasattr(m, "angle_offset"):
#            angle_offset = float(self._summary.angle_offset_edit.text())
#            angle_offset_error = float(self._summary.angle_offset_error_edit.text())
#                
#        for i in range(self._summary.angle_list.count()):
#            data = self._summary.angle_list.item(i).data(QtCore.Qt.UserRole).toPyObject()
#            # Over-write Q binning with common binning
#            data.q_min = q_min
#            data.q_step = q_step
#        
#            # Over-write angle offset
#            if hasattr(data, "angle_offset"):
#                data.angle_offset = angle_offset
#                data.angle_offset_error = angle_offset_error
#
#            state_list.append(data)
        state.data_sets = state_list
        
        return state
    
    def get_editing_state(self):
        m = REFLDataSets()
        return m
        
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

        #from TOF and to TOF
        from_tof = float(self._summary.data_from_tof.text())
        to_tof = float(self._summary.data_to_tof.text())
        m.DataTofRange = [from_tof, to_tof]
        m.crop_TOF_range = self._summary.tof_range_switch.isChecked()
    
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
        
        # Scattering angle
        m.theta = float(self._summary.angle_edit.text())
        m.use_center_pixel = False

        return m
    
    def getLambdaValue(self,mt):
        """
        return the lambdaRequest value
        """
        mt_run = mt.getRun()
        _lambda = mt_run.getProperty('LambdaRequest').value
        return _lambda
    
    def getSh(self,mt, top_tag, bottom_tag):
        """
            returns the height and units of the given slits
        """
        mt_run = mt.getRun()
        st = mt_run.getProperty(top_tag).value
        sb = mt_run.getProperty(bottom_tag).value
        sh = float(sb[0]) - float(st[0])
        units = mt_run.getProperty(top_tag).units
        return sh, units
    
    def getS1h(self,mt=None):
        """    
            returns the height and units of the slit #1 
        """
        if mt != None:
            _h, units = self.getSh(mt, 's1t', 's1b')
            return _h, units
        return None, ''
    
    def getS2h(self,mt=None):
        """    
            returns the height and units of the slit #2 
        """
        if mt != None:
            _h, units = self.getSh(mt, 's2t', 's2b') 
            return _h, units
        return None, None

    def getSw(self,mt, left_tag, right_tag):
        """
            returns the width and units of the given slits
        """
        mt_run = mt.getRun()
        sl = mt_run.getProperty(left_tag).value
        sr = mt_run.getProperty(right_tag).value
        sw = float(sl[0]) - float(sr[0])
        units = mt_run.getProperty(left_tag).units
        return sw, units

    def getS1w(self,mt=None):
        """    
            returns the width and units of the slit #1 
        """
        if mt != None:
            _w, units = self.getSw(mt, 's1l', 's1r') 
            return _w, units
        return None, ''
    
    def getS2w(self,mt=None):
        """    
            returns the width and units of the slit #2 
        """
        if mt != None:
            _w, units = self.getSh(mt, 's2l', 's2r') 
            return _w, units
        return None, None

    def getSlitsValueAndLambda(self,file):
        """
        Retrieve the S1H (slit 1 height), 
                     S2H (slit 2 height), 
                     S1W (slit 1 width), 
                     S2W (slit 2 width) and 
                     lambda requested values
        """
        _full_file_name = file
        LoadEventNexus(Filename=_full_file_name,
                       OutputWorkspace='tmpWks',
                       MetaDataOnly='1')
        mt1 = mtd['tmpWks']
        _s1h_value, _s1h_units = self.getS1h(mt=mt1)
        _s2h_value, _s2h_units = self.getS2h(mt=mt1)
        S1H = "%2.4f" %(_s1h_value)
        S2H = "%2.4f" %(_s2h_value)
        
        _s1w_value, _s1w_units = self.getS1w(mt=mt1)
        _s2w_value, _s2w_units = self.getS2w(mt=mt1)
        S1W = "%2.4f" %(_s1w_value)
        S2W = "%2.4f" %(_s2w_value)
        
        _lambda_value = self.getLambdaValue(mt=mt1)
        lambdaRequest = "%2.4" %(_lambda_value)
        
        return S1H, S2H, S1W, S2W, lambdaRequest
        
        
    