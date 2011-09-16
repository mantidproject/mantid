from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
#from reduction_gui.reduction.sans.hfir_options_script import ReductionOptions
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.reflectometer.ui_parameters_refl

IS_IN_MANTIDPLOT = False
try:
    import qti
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
        self._summary.parameters_q_range_bin_size_value.setValidator(QtGui.QIntValidator(self._summary.parameters_q_range_bin_size_value))

        # Event connections
        self.connect(self._summary.parameters_q_range_automatic_switch, QtCore.SIGNAL("clicked(bool)"), self._q_range_automatic_clicked)
        self.connect(self._summary.parameters_q_range_manual_switch, QtCore.SIGNAL("clicked(bool)"), self._q_range_manual_clicked)
        self.connect(self._summary.parameters_number_of_q_bins_switch, QtCore.SIGNAL("clicked(bool)"), self._number_of_q_bins_clicked)
        self.connect(self._summary.parameters_q_bin_size_switch, QtCore.SIGNAL("clicked(bool)"), self._q_bin_size_clicked)
        
        self.connect(self._summary.parameters_q_range_linear_switch, QtCore.SIGNAL("clicked(bool)"), self._q_range_linear_clicked)
        self.connect(self._summary.parameters_q_range_log_switch, QtCore.SIGNAL("clicked(bool)"), self._q_range_log_clicked)
        self.connect(self._summary.parameters_output_directory_browse_button, QtCore.SIGNAL("clicked()"), self._browse_output_directory_clicked)

        self.connect(self._summary.parameters_q_range_from_value, QtCore.SIGNAL("textChanged(QString)"), self._check_for_missing_fields)
        self.connect(self._summary.parameters_q_range_to_value, QtCore.SIGNAL("textChanged(QString)"), self._check_for_missing_fields)
        self.connect(self._summary.parameters_q_range_nbr_bins_value, QtCore.SIGNAL("textChanged(QString)"), self._check_for_missing_fields)
        self.connect(self._summary.parameters_q_range_bin_size_value, QtCore.SIGNAL("textChanged(QString)"), self._check_for_missing_fields)

    def _check_for_missing_fields(self):

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
        
    def _q_range_log_clicked(self, is_clicked):
        """
            Reached by the Q range log switch
        """
        self._summary.parameters_q_range_linear_switch.setChecked(False)
        self._summary.parameters_q_range_log_switch.setChecked(True)
        
  