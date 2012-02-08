from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
#from reduction_gui.reduction.sans.hfir_options_script import ReductionOptions
from reduction_gui.reduction.reflectometer.refl_parameters_script import ParametersSets
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.reflectometer.ui_parameters_refl

IS_IN_MANTIDPLOT = False
try:
    import _qti
    from MantidFramework import *
    mtd.initialise(False)
    from mantidsimple import *
    IS_IN_MANTIDPLOT = True
    from reduction import extract_workspace_name
except:
    pass

class ParametersReflWidget(BaseWidget):    
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Parameters"      
    
    def __init__(self, parent=None, state=None, settings=None, data_proxy=None):      
        super(ParametersReflWidget, self).__init__(parent, state, settings, data_proxy=data_proxy) 

        class SummaryFrame(QtGui.QFrame, ui.reflectometer.ui_parameters_refl.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
                
        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)
        
    def initialize_content(self):

        # Validators
        self._summary.parameters_q_range_from_value.setValidator(QtGui.QDoubleValidator(self._summary.parameters_q_range_from_value))
        self._summary.parameters_q_range_to_value.setValidator(QtGui.QDoubleValidator(self._summary.parameters_q_range_to_value))
        self._summary.parameters_q_range_nbr_bins_value.setValidator(QtGui.QIntValidator(self._summary.parameters_q_range_nbr_bins_value))
        self._summary.parameters_q_range_bin_size_value.setValidator(QtGui.QDoubleValidator(self._summary.parameters_q_range_bin_size_value))

        # Event connections
        self.connect(self._summary.parameters_q_range_automatic_switch, QtCore.SIGNAL("clicked(bool)"), self._q_range_automatic_clicked)
        self.connect(self._summary.parameters_q_range_manual_switch, QtCore.SIGNAL("clicked(bool)"), self._q_range_manual_clicked)
        self.connect(self._summary.parameters_number_of_q_bins_switch, QtCore.SIGNAL("clicked(bool)"), self._number_of_q_bins_clicked)
        self.connect(self._summary.parameters_q_bin_size_switch, QtCore.SIGNAL("clicked(bool)"), self._q_bin_size_clicked)
        
        self.connect(self._summary.parameters_q_range_linear_switch, QtCore.SIGNAL("clicked(bool)"), self._q_range_linear_clicked)
        self.connect(self._summary.parameters_q_range_log_switch, QtCore.SIGNAL("clicked(bool)"), self._q_range_log_clicked)
        self.connect(self._summary.parameters_output_directory_browse_button, QtCore.SIGNAL("clicked()"), self._browse_output_directory_clicked)

        self.connect(self._summary.parameters_q_range_from_value, QtCore.SIGNAL("textChanged(QString)"), self._text_field_changed_event)
        self.connect(self._summary.parameters_q_range_to_value, QtCore.SIGNAL("textChanged(QString)"), self._text_field_changed_event)
        self.connect(self._summary.parameters_q_range_nbr_bins_value, QtCore.SIGNAL("textChanged(QString)"), self._text_field_changed_event)
        self.connect(self._summary.parameters_q_range_bin_size_value, QtCore.SIGNAL("textChanged(QString)"), self._text_field_changed_event)

    def _text_field_changed_event(self):
        """
            This is reached by the various text field when user change value
        """
        self._check_for_missing_fields()
        self._recalculate_nbr_q_or_bin_size_labels()

    def _calculate_nbr_bins(self, bLinear, from_bin, to_bin, bin_size):
        """
            This function will return the number of bins according to the type of binning (linear/log) and
            the from_bin and to_bin values
        """
        
        from_bin = float(from_bin)
        to_bin = float(to_bin)
        bin_size = float(bin_size)
        
        if bLinear:
            _delta = abs(to_bin - from_bin)
            _nbr_bins = _delta / bin_size
        else:
            _nbr_bins=0
            _x1=0
            while _x1 < to_bin:
                _x1 = (1.+bin_size)*from_bin
                from_bin=_x1
                _nbr_bins+=1
        
        #if _nbr_bins is not an integer, takes the next value
        return int(math.ceil(_nbr_bins))
    
    def _calculate_bin_size(self, bLinear, from_bin, to_bin, nbr_bins):
        """
            This will calculate the bin size according to the type of binning (linear/log) and
            the from_bin and to_bin values
        """
        from_bin = float(from_bin)
        to_bin = float(to_bin)
        nbr_bins = float(nbr_bins)
        
        if bLinear:
            _bin_size = (to_bin - from_bin) / nbr_bins
        else:
            _bin_size = pow(to_bin/from_bin,1./nbr_bins)-1
        return round(_bin_size,4)

    def _recalculate_nbr_q_or_bin_size_labels(self):
        """
            This recalculates the bin size if the user enters the number of Q bins
            and the nbr of bins if the user select the bin size
        """
        
        from_bin = self._summary.parameters_q_range_from_value.text()
        to_bin = self._summary.parameters_q_range_to_value.text()
        
        if (from_bin == '' or to_bin == ''):
            if self._summary.parameters_q_bin_size_switch.isChecked(): #Q bin size is selected
                self._summary.parameters_q_range_bin_size_number_of_q_label.setText("N/A")
            else:
                self._summary.parameters_q_range_nbr_bins_bin_size_value.setText("N/A")
        else:    
            if self._summary.parameters_q_range_linear_switch.isChecked():
                bLinear = True
            else:
                bLinear = False
        
            if self._summary.parameters_q_bin_size_switch.isChecked(): #Q bin size is selected
                bin_size = self._summary.parameters_q_range_bin_size_value.text()
                if (bin_size != ''): #only if there is a bin size
                    if float(bin_size) != 0:
                        nbr_bins = self._calculate_nbr_bins(bLinear, from_bin, to_bin, bin_size)
                    else:
                        nbr_bins = 'N/A'
                else:
                    nbr_bins = 'N/A'
                self._summary.parameters_q_range_bin_size_number_of_q_label.setText(str(nbr_bins))

            else:
                nbr_bins = self._summary.parameters_q_range_nbr_bins_value.text()
                if (nbr_bins != ''): #only if there is a number of bins
                    if (int(nbr_bins) != 0):
                        bin_size = self._calculate_bin_size(bLinear, from_bin, to_bin, nbr_bins)
                    else:
                        bin_size = 'N/A'
                else:
                    bin_size = 'N/A'
                self._summary.parameters_q_range_nbr_bins_bin_size_value.setText(str(bin_size))
            
    def _check_for_missing_fields(self):
        """
            This just checks if any of the mandatory field is missing and if it's empty
            shows a red start on its right side to show the user that a value should be 
            entered here
        """

        #auto/manual mode
        if self._summary.parameters_q_range_manual_switch.isChecked():
            from_value = self._summary.parameters_q_range_from_value.text()
            if (from_value == ''):
                self._summary.parameters_q_range_from_value_missing.setText("*")
            else:
                self._summary.parameters_q_range_from_value_missing.setText(" ")
            
            to_value = self._summary.parameters_q_range_to_value.text()
            if (to_value == ''):
                self._summary.parameters_q_range_to_value_missing.setText("*")
            else:
                self._summary.parameters_q_range_to_value_missing.setText(" ")
            
            if self._summary.parameters_number_of_q_bins_switch.isChecked():    
                nbr_q_value = self._summary.parameters_q_range_nbr_bins_value.text()
                if (nbr_q_value == ''):
                    self._summary.parameters_q_range_nbr_bins_value_missing.setText("*")
                else:
                    self._summary.parameters_q_range_nbr_bins_value_missing.setText(" ")
                self._summary.parameters_q_range_bin_size_value_missing.setText(" ")
                    
            else:
                q_bin_value = self._summary.parameters_q_range_bin_size_value.text()
                if (q_bin_value == ''):
                    self._summary.parameters_q_range_bin_size_value_missing.setText("*")
                else:
                    self._summary.parameters_q_range_bin_size_value_missing.setText(" ")
                self._summary.parameters_q_range_nbr_bins_value_missing.setText(" ")

        else:
            self._summary.parameters_q_range_from_value_missing.setText(" ")
            self._summary.parameters_q_range_to_value_missing.setText(" ")
            self._summary.parameters_q_range_nbr_bins_value_missing.setText(" ")
            self._summary.parameters_q_range_bin_size_value_missing.setText(" ")
        
    def _browse_output_directory_clicked(self):
        """
            Reached by the browse output folder button
        """
        output_dir = QtGui.QFileDialog.getExistingDirectory(self, "Output Directory - Choose a directory",
                                                            os.path.expanduser('~'), 
                                                            QtGui.QFileDialog.ShowDirsOnly
                                                            | QtGui.QFileDialog.DontResolveSymlinks)
        if output_dir:
            self._summary.parameters_output_directory_value.setText(output_dir)   

    def _number_of_q_bins_clicked(self, is_clicked):
        """
            Reached by the Number of Q bins switch
        """
        self._summary.parameters_q_bin_size_switch.setChecked(False)
        self._number_of_q_bins_vs_q_bin_size()

    def _q_bin_size_clicked(self, is_clicked):
        """
            Reached by the Q bin size switch
        """
        self._summary.parameters_number_of_q_bins_switch.setChecked(False)
        self._number_of_q_bins_vs_q_bin_size()

    def _q_range_automatic_clicked(self, is_clicked):
        """
            Reached by the Q range automatic switch
        """
        self._q_range_activate_manual_mode(False)
        self._summary.parameters_q_range_manual_switch.setChecked(False)
        self._summary.parameters_q_range_automatic_switch.setChecked(True)
        self._number_of_q_bins_vs_q_bin_size()
        
    def _q_range_manual_clicked(self, is_clicked):
        """
            Reached by the Q range manual switch
        """
        self._q_range_activate_manual_mode(True)
        self._summary.parameters_q_range_automatic_switch.setChecked(False)
        self._summary.parameters_q_range_manual_switch.setChecked(True)
        self._number_of_q_bins_vs_q_bin_size()
        
    def _q_range_activate_manual_mode(self, is_activated):
        """
            Reached by the Q range automatic and manual switches
        """
        #disable or not the widgets that rely on the manual mode
        self._summary.parameters_q_range_from_label.setEnabled(is_activated)
        self._summary.parameters_q_range_from_value.setEnabled(is_activated)
        self._summary.parameters_q_range_from_unit_label.setEnabled(is_activated)
        self._summary.parameters_q_range_to_label.setEnabled(is_activated)
        self._summary.parameters_q_range_to_value.setEnabled(is_activated)
        self._summary.parameters_q_range_to_unit_label.setEnabled(is_activated)
        self._summary.parameters_number_of_q_bins_switch.setEnabled(is_activated)
        self._summary.parameters_q_bin_size_switch.setEnabled(is_activated)        
        self._summary.parameters_q_range_linear_switch.setEnabled(is_activated)
        self._summary.parameters_q_range_log_switch.setEnabled(is_activated)
        self._number_of_q_bins_vs_q_bin_size()
        self._check_for_missing_fields()
        
    def _number_of_q_bins_vs_q_bin_size(self):
        """
            Reached when the user switch between Automatic/linear mode or
            'Number of Q bins' and 'Q bin size'
        """
        
        if self._summary.parameters_q_range_manual_switch.isChecked():

            if self._summary.parameters_q_bin_size_switch.isChecked():
                q_bin_size_status = True
                number_q_bin_status = False
            else:
                q_bin_size_status = False
                number_q_bin_status = True
        
        else:
            q_bin_size_status = False
            number_q_bin_status = False

        #Q bins size widgets
        self._summary.parameters_q_range_bin_size_value.setEnabled(q_bin_size_status)
        self._summary.parameters_q_range_bin_size_unit.setEnabled(q_bin_size_status)
        self._summary.parameters_q_range_bin_size_number_of_q_label.setEnabled(q_bin_size_status)
        
        #Number of Q bins widgets
        self._summary.parameters_q_range_nbr_bins_value.setEnabled(number_q_bin_status)
        self._summary.parameters_q_range_nbr_bins_bin_size_label.setEnabled(number_q_bin_status)
        self._summary.parameters_q_range_nbr_bins_bin_size_value.setEnabled(number_q_bin_status)
        self._summary.parameters_q_range_nbr_bins_bin_size_unit.setEnabled(number_q_bin_status)

        self._check_for_missing_fields()
    
    def _q_range_linear_clicked(self, is_clicked):
        """
            Reached by the Q range linear switch
        """
        self._summary.parameters_q_range_linear_switch.setChecked(True)
        self._summary.parameters_q_range_log_switch.setChecked(False)
        self._recalculate_nbr_q_or_bin_size_labels()

    def _q_range_log_clicked(self, is_clicked):
        """
            Reached by the Q range log switch
        """
        self._summary.parameters_q_range_linear_switch.setChecked(False)
        self._summary.parameters_q_range_log_switch.setChecked(True)
        self._recalculate_nbr_q_or_bin_size_labels()
        
    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state. 
            @param state: data object
        """   
        
        #Automatic or Manual mode selected
        self._summary.parameters_q_range_automatic_switch.setChecked(state.AutomaticQRangeFlag)
        self._summary.parameters_q_range_manual_switch.setChecked(not state.AutomaticQRangeFlag)
        
        #from/to Q values
        self._summary.parameters_q_range_from_value.setText(state.Qrange[0])
        self._summary.parameters_q_range_to_value.setText(state.Qrange[1])
        
        #Number of Q bins and Q bin size flags
        self._summary.parameters_number_of_q_bins_switch.setChecked(state.NbrOfQBinsFlag)
        self._summary.parameters_q_bin_size_switch.setChecked(not state.NbrOfQBinsFlag)
        
        #nbr of Q bins and bin size
        self._summary.parameters_q_range_nbr_bins_value.setText(state.NbrOfBins_BinSize[0])
        self._summary.parameters_q_range_nbr_bins_bin_size_value.setText(state.NbrOfBins_BinSize[1])
        
        self._summary.parameters_q_range_bin_size_value.setText(state.BinSize_NbrOfBins[0])
        self._summary.parameters_q_range_bin_size_number_of_q_label.setText(state.BinSize_NbrOfBins[1])
        
        #linear/log binning
        self._summary.parameters_q_range_linear_switch.setChecked(state.LinearBinningFlag)
        self._summary.parameters_q_range_log_switch.setChecked(not state.LinearBinningFlag)
        
        #Output directory
        self._summary.parameters_output_directory_value.setText(state.OutputFolder)
        
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = ParametersSets()
        
        #Automatic or Manual mode selected
        m.AutomaticQRangeFlag = self._summary.parameters_q_range_automatic_switch.isChecked()
  
        #from/to Q values
        from_q = str(self._summary.parameters_q_range_from_value.text())
        to_q = str(self._summary.parameters_q_range_to_value.text())
        m.Qrange = [from_q, to_q]
        
        #Number of Q bins flag and fields
        m.NbrOfQBinsFlag = self._summary.parameters_number_of_q_bins_switch.isChecked()
        nbr_bins = str(self._summary.parameters_q_range_nbr_bins_value.text())
        bin_size = str(self._summary.parameters_q_range_nbr_bins_bin_size_value.text())
        m.NbrOfBins_BinSize = [nbr_bins, bin_size]
  
        #Q bin size
        bin_size = str(self._summary.parameters_q_range_bin_size_value.text())
        nbr_bins = str(self._summary.parameters_q_range_bin_size_number_of_q_label.text())
        m.BinSize_NbrOfBins = [bin_size, nbr_bins]

        #Linear/log binning
        m.LinearBinningFlag = self._summary.parameters_q_range_linear_switch.isChecked()
        
        #output directory
        m.OutputFolder = str(self._summary.parameters_output_directory_value.text())
        
        return m
  