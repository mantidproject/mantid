from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
#from reduction_gui.reduction.sans.hfir_options_script import ReductionOptions
from reduction_gui.reduction.reflectometer.refl_data_script import DataSets
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.reflectometer.ui_data_refl
from LoadSNSRoi import LoadSNSRoi
from SaveSNSRoi import SaveSNSRoi

IS_IN_MANTIDPLOT = False
try:
    import _qti
    from MantidFramework import *
    mtd.initialise(False)
    from mantidsimple import *
    IS_IN_MANTIDPLOT = True
    from reduction import extract_workspace_name
#    from reduction.instruments.reflectometer.LoadSNSRoi import LoadSNSRoi
#    from reduction.instruments.reflectometer.SaveSNSRoi import SaveSNSRoi
except:
    pass

class DataReflWidget(BaseWidget):    
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Sample"      
    GeneralSettings.instrument_name = 'REF_L'
    peak_pixel_range = []
    background_pixel_range = []

    def __init__(self, parent=None, state=None, settings=None, name="REFL", data_proxy=None):      
        super(DataReflWidget, self).__init__(parent, state, settings, data_proxy=data_proxy) 

        class SummaryFrame(QtGui.QFrame, ui.reflectometer.ui_data_refl.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(DataSets())

    def initialize_content(self):
        
        # Validators
        self._summary.data_peak_from_pixel.setValidator(QtGui.QIntValidator(self._summary.data_peak_from_pixel))
        self._summary.data_peak_to_pixel.setValidator(QtGui.QIntValidator(self._summary.data_peak_to_pixel))
        self._summary.data_background_from_pixel1.setValidator(QtGui.QIntValidator(self._summary.data_background_from_pixel1))
        self._summary.data_background_to_pixel1.setValidator(QtGui.QIntValidator(self._summary.data_background_to_pixel1))
        self._summary.data_background_from_pixel2.setValidator(QtGui.QIntValidator(self._summary.data_background_from_pixel2))
        self._summary.data_background_to_pixel2.setValidator(QtGui.QIntValidator(self._summary.data_background_to_pixel2))
        self._summary.data_from_tof.setValidator(QtGui.QDoubleValidator(self._summary.data_from_tof))
        self._summary.data_to_tof.setValidator(QtGui.QDoubleValidator(self._summary.data_to_tof))

        # Event connections
        self.connect(self._summary.data_load_nexus_button, QtCore.SIGNAL("clicked()"), self._data_load_nexus_clicked)
        self.connect(self._summary.data_peak_narrow_switch, QtCore.SIGNAL("clicked(bool)"), self._data_peak_switch_clicked)
        self.connect(self._summary.data_peak_broad_switch, QtCore.SIGNAL("clicked(bool)"), self._data_peak_switch_clicked)
        self.connect(self._summary.data_peak_discrete_switch, QtCore.SIGNAL("clicked(bool)"), self._data_peak_switch_clicked_discrete)
        self.connect(self._summary.data_peak_load_roi, QtCore.SIGNAL("clicked()"), self._data_peak_load_roi_clicked)
        self.connect(self._summary.data_peak_save_roi, QtCore.SIGNAL("clicked()"), self._data_peak_save_roi_clicked)
        self.connect(self._summary.data_peak_from_pixel, QtCore.SIGNAL("textChanged(QString)"), self._check_status_of_data_peak_save_button)
        self.connect(self._summary.data_peak_to_pixel, QtCore.SIGNAL("textChanged(QString)"), self._check_status_of_data_peak_save_button)

        self.connect(self._summary.data_background_switch, QtCore.SIGNAL("clicked(bool)"), self._data_background_clicked)
        self.connect(self._summary.data_background_load_button, QtCore.SIGNAL("clicked()"), self._data_background_load_roi_clicked)
        self.connect(self._summary.data_background_from_pixel1, QtCore.SIGNAL("textChanged(QString)"), self._check_status_of_data_background_save_button)
        self.connect(self._summary.data_background_to_pixel1, QtCore.SIGNAL("textChanged(QString)"), self._check_status_of_data_background_save_button)
        self.connect(self._summary.data_background_from_pixel2, QtCore.SIGNAL("textChanged(QString)"), self._check_status_of_data_background_save_button)
        self.connect(self._summary.data_background_to_pixel2, QtCore.SIGNAL("textChanged(QString)"), self._check_status_of_data_background_save_button)
        self.connect(self._summary.data_background_save_button, QtCore.SIGNAL("clicked()"), self._data_background_save_roi_clicked)

    def _data_load_nexus_clicked(self):
        """
            Load data NeXus
        """

    def _data_background_clicked(self, is_checked):
        """
            This is reached when the user clicks the Background switch and will enabled or not
            the widgets that follow that button
        """
        self._summary.data_background_from_pixel1.setEnabled(is_checked)
        self._summary.data_background_from_pixel1_label.setEnabled(is_checked)
        self._summary.data_background_to_pixel1.setEnabled(is_checked)
        self._summary.data_background_to_pixel1_label.setEnabled(is_checked)
        self._summary.data_background_from_pixel2.setEnabled(is_checked)
        self._summary.data_background_from_pixel2_label.setEnabled(is_checked)
        self._summary.data_background_to_pixel2.setEnabled(is_checked)
        self._summary.data_background_to_pixel2_label.setEnabled(is_checked)
        self._summary.data_background_load_button.setEnabled(is_checked)
        self._summary.data_background_save_button.setEnabled(is_checked)
        self._check_status_of_data_background_save_button()
        
    def _data_peak_switch_clicked(self, is_checked):
        """
           This is reached when the user clicks the Peak selection Narrow or Broad switch 
        """
        if (is_checked):
            self._summary.data_peak_from_pixel_label.setEnabled(True)
            self._summary.data_peak_from_pixel.setEnabled(True)
            self._summary.data_peak_to_pixel_label.setEnabled(True)
            self._summary.data_peak_to_pixel.setEnabled(True)
            self._summary.data_peak_nbr_selection_label.setEnabled(False)
            self._summary.data_peak_nbr_selection_value.setEnabled(False)
            self._check_status_of_data_peak_save_button()
         
    def _data_peak_switch_clicked_discrete(self, is_checked):
        """
           This is reached when the user clicks the Peak selection Discrete switch 
        """
        if (is_checked):
            self._summary.data_peak_from_pixel_label.setEnabled(False)
            self._summary.data_peak_from_pixel.setEnabled(False)
            self._summary.data_peak_to_pixel_label.setEnabled(False)
            self._summary.data_peak_to_pixel.setEnabled(False)
            self._summary.data_peak_nbr_selection_label.setEnabled(True)
            self._summary.data_peak_nbr_selection_value.setEnabled(True)
            self._check_status_of_data_peak_save_button()

    def _data_peak_load_roi_clicked(self):
        """
            Reached by the Load peak selection button
        """
        fname = self.data_browse_dialog(data_type="*.txt *.dat", title="Data peak selection - Choose a ROI file")
        if fname:
            #retrieved from and to pixels values
            myROI = LoadSNSRoi(filename=fname)
            mode = myROI.getMode()
            pixelRange = myROI.getPixelRange()
            self.peak_pixel_range = pixelRange
            
            if (mode == 'narrow/broad'):
                if self._summary.data_peak_discrete_switch.isChecked():
                   QtGui.QMessageBox.warning(self, "Incompatibility of Formats!",
                                                      "Selection type and ROI file loaded do not match !")
                else:
                    from_pixel = pixelRange[0]
                    to_pixel = pixelRange[1]
                    self._summary.data_peak_from_pixel.setText(str(from_pixel))
                    self._summary.data_peak_to_pixel.setText(str(to_pixel))
                    self._summary.data_peak_nbr_selection_value.setText("N/A")
            else: #file loaded is a discrete ROI-------
                if self._summary.data_peak_discrete_switch.isChecked():
                    _txt = str(len(pixelRange)) + ' -> ' + myROI.retrieveFormatedDiscretePixelRange()
                    self._summary.data_peak_nbr_selection_value.setText(_txt)
                else:
                    self._summary.data_peak_nbr_selection_value.setText("N/A")
                    QtGui.QMessageBox.warning(self, "Incompatibility of Formats!",
                                                  "Selection type and ROI file loaded do not match !")
            self._check_status_of_data_peak_save_button()

    def _check_status_of_data_peak_save_button(self):
        """
            This function checks if the SAVE... button of the data peak can be enabled or not
        """
        button_status = False
        if self._summary.data_peak_discrete_switch.isChecked(): #discrete mode
            #pixel_range = self._summary.data_peak_nbr_selection_value.text()
            #if pixel_range != 'N/A':
            button_status = False
        else:
            from_pixel = self._summary.data_peak_from_pixel.text()
            to_pixel = self._summary.data_peak_to_pixel.text()
            if from_pixel != '' and to_pixel != '':
                button_status = True
        self._summary.data_peak_save_roi.setEnabled(button_status)
        self._check_for_missing_fields()

    def _check_for_missing_fields(self):

        #peak selection
        if self._summary.data_peak_discrete_switch.isChecked(): #discrete
            self._summary.data_peak_from_pixel_missing.setText(" ")
            self._summary.data_peak_to_pixel_missing.setText(" ")
            range = self._summary.data_peak_nbr_selection_value.text()
            if range == 'N/A':
                self._summary.data_peak_discrete_selection_missing.setText("*")
            else:
                self._summary.data_peak_discrete_selection_missing.setText(" ")
            
        else: #broad/Narrow
            self._summary.data_peak_discrete_selection_missing.setText(" ")
            from_pixel = self._summary.data_peak_from_pixel.text()
            if from_pixel == '':
                self._summary.data_peak_from_pixel_missing.setText("*")
            else:
                self._summary.data_peak_from_pixel_missing.setText(" ")
                
            to_pixel = self._summary.data_peak_to_pixel.text()
            if to_pixel == '':
                self._summary.data_peak_to_pixel_missing.setText("*")
            else:
                self._summary.data_peak_to_pixel_missing.setText(" ")
                
        #background
        is_checked = self._summary.data_background_switch.isChecked()
        if is_checked:
            from_pixel1 = self._summary.data_background_from_pixel1.text()
            if from_pixel1 == '':
                self._summary.data_background_from_pixel_missing.setText("*")
            else:
                self._summary.data_background_from_pixel_missing.setText(" ")
                
            to_pixel1 = self._summary.data_background_to_pixel1.text()
            if to_pixel1 == '':
                self._summary.data_background_to_pixel_missing.setText("*")
            else:
                self._summary.data_background_to_pixel_missing.setText(" ")
            
        else:
            self._summary.data_background_from_pixel_missing.setText(" ")
            self._summary.data_background_to_pixel_missing.setText(" ")
                

    def _data_peak_save_roi_clicked(self):
        """
            Reached by the save peak selection button
        """
        fname = self.data_browse_dialog(data_type="*.dat *.txt", title="Data peak ROI file - select or enter a new ROI file name")
        if fname:
            #get selection type
            pixel_range = self.peak_pixel_range
            if self._summary.data_peak_discrete_switch.isChecked(): #discrete mode
                SaveSNSRoi(filename=fname, pixel_range=pixel_range, mode='discrete')
            else: #Narrow/Broad
                from_pixel = int(self._summary.data_peak_from_pixel.text())
                to_pixel = int(self._summary.data_peak_to_pixel.text())
                pixel_range = [from_pixel, to_pixel]
                SaveSNSRoi(filename=fname, pixel_range=pixel_range, mode='narrow/broad')
                
    def _check_status_of_data_background_save_button(self):
        """
            This function will check if the background save ROI button can be enabled or not
        """
        is_checked = self._summary.data_background_switch.isChecked()

        button_status = False
        if is_checked is True:
            from_pixel = self._summary.data_background_from_pixel1.text()
            to_pixel = self._summary.data_background_to_pixel1.text()
            if from_pixel != '' and to_pixel != '':
                button_status = True
            
            from_pixel = self._summary.data_background_from_pixel2.text()
            to_pixel = self._summary.data_background_to_pixel2.text()
            if (from_pixel == '' and to_pixel != '') or (from_pixel != '' and to_pixel == ''):
                button_status = False
                
        self._summary.data_background_save_button.setEnabled(button_status)
        self._check_for_missing_fields()
                 
    def _data_background_load_roi_clicked(self):
        """
            Reached by the load background selection button
        """
        fname = self.data_browse_dialog(data_type="*.txt *.dat", title="Data background selection - Choose a ROI file")
        if fname:
            #retrieved from and to pixels values
            myROI = LoadSNSRoi(filename=fname)
            mode = myROI.getMode()
            pixelRange = myROI.getPixelRange()
            
            self.background_pixel_range = pixelRange
                
            if (mode == 'narrow/broad'):                
                from_pixel = pixelRange[0]
                to_pixel = pixelRange[1]
                self._summary.data_background_from_pixel1.setText(str(from_pixel))
                self._summary.data_background_to_pixel1.setText(str(to_pixel))
            else:
                _pixel_list = myROI.getPixelRange()
                if len(_pixel_list) == 2:
                    roi1_from = str(_pixel_list[0][0])
                    roi1_to = str(_pixel_list[0][1])
                    roi2_from = str(_pixel_list[1][0])
                    roi2_to = str(_pixel_list[1][1])
                    self._summary.data_background_from_pixel1.setText(str(roi1_from))
                    self._summary.data_background_to_pixel1.setText(str(roi1_to))
                    self._summary.data_background_from_pixel2.setText(str(roi2_from))
                    self._summary.data_background_to_pixel2.setText(str(roi2_to))
                else:
                    QtGui.QMessageBox.warning(self, "Wrong data background ROI file format!",
                                             "                         Please check the ROI file!")                  
        self._check_status_of_data_background_save_button()
     
    def get_data_peak_selection(self):
        """
            This function retrieves the from/to pixels of the data peak selection
        """ 
        mode = 'narrow/broad'
        from_pixel = self._summary.data_peak_from_pixel.text()
        to_pixel = self._summary.data_peak_to_pixel.text()
        return [from_pixel, to_pixel]

    def get_data_back_selection(self):
        """
            This function retrives the from/to pixels of roi#1/#2 of data background selection
        """
        roi1_from = self._summary.data_background_from_pixel1.text()
        roi1_to = self._summary.data_background_to_pixel1.text()
        range1 = [int(roi1_from), int(roi1_to)]
        
        roi2_from = self._summary.data_background_from_pixel2.text()
        if (roi2_from != ''):
            roi2_to = self._summary.data_background_to_pixel2.text()
            range2 = [int(roi2_from), int(roi2_to)]
            _range = [range1, range2]
        else:
            _range = [range1]
        return _range
                    
    def _data_background_save_roi_clicked(self):
        """
            Reached by the data background save button
        """
        fname = self.data_browse_dialog(data_type="*.dat *.txt", title="Background ROI file - select or enter a new ROI file name")
        if fname:
             pixel_range = self.get_data_back_selection()
             SaveSNSRoi(filename=fname, pixel_range=pixel_range, mode='discrete')

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state. 
            @param state: data object
        """

        #Peak Selection
        if state.DataPeakSelectionType == 'narrow':
            self._summary.data_peak_narrow_switch.setChecked(True)
        else:
            if state.DataPeakSelectionType == 'broad':
                self._summary.data_peak_broad_switch.setChecked(True)
            else:
                self._summary.data_peak_discrete_switch.setChecked(True)

        #Peak from/to pixels
        self._summary.data_peak_from_pixel.setText(str(state.DataPeakPixels[0]))
        self._summary.data_peak_to_pixel.setText(str(state.DataPeakPixels[1]))
        
        #Discrete selection string
        self._summary.data_peak_nbr_selection_value.setText(state.DataPeakDiscreteSelection)
        
        #Background flag
        self._summary.data_background_switch.setChecked(state.DataBackgroundFlag)

        #Background from/to pixels
        self._summary.data_background_from_pixel1.setText(str(state.DataBackgroundRoi[0]))
        self._summary.data_background_to_pixel1.setText(str(state.DataBackgroundRoi[1]))
        self._summary.data_background_from_pixel2.setText(str(state.DataBackgroundRoi[2]))
        self._summary.data_background_to_pixel2.setText(str(state.DataBackgroundRoi[3]))
        
        #from TOF and to TOF
        self._summary.data_from_tof.setText(str(state.DataTofRange[0]))
        self._summary.data_to_tof.setText(str(state.DataTofRange[1]))

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = DataSets()
        
        #Peak Selection
        if self._summary.data_peak_discrete_switch.isChecked():
            m.DataPeakSelectionType = 'discrete'
        if self._summary.data_peak_broad_switch.isChecked():
            m.DataPeakSelectionType = 'broad'
        if self._summary.data_peak_narrow_switch.isChecked():
            m.DataPeakSelectionType = 'narrow'

        #Peak from/to pixels
        m.DataPeakPixels[0] = str(self._summary.data_peak_from_pixel.text())
        m.DataPeakPixels[1] = str(self._summary.data_peak_to_pixel.text())    
        
        #Discrete selection string
        m.DataPeakDiscreteSelection = self._summary.data_peak_nbr_selection_value.text()    

        #Background flag
        m.DataBackgroundFlag = self._summary.data_background_switch.isChecked()

        #Background from/to pixels
        roi1_from = str(self._summary.data_background_from_pixel1.text())
        roi1_to = str(self._summary.data_background_to_pixel1.text())
        roi2_from = str(self._summary.data_background_from_pixel2.text())
        roi2_to = str(self._summary.data_background_to_pixel2.text())
        m.DataBackgroundRoi = [roi1_from, roi1_to, roi2_from, roi2_to]

        #from TOF and to TOF
        from_tof = str(self._summary.data_from_tof.text())
        to_tof = str(self._summary.data_to_tof.text())
        m.DataTofRange = [from_tof, to_tof]

        return m
