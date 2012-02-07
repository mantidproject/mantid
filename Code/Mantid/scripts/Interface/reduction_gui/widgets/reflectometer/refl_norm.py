from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
#from reduction_gui.reduction.sans.hfir_options_script import ReductionOptions
from reduction_gui.reduction.reflectometer.refl_norm_script import NormSets
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.reflectometer.ui_norm_refl
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
except:
    pass

class NormReflWidget(BaseWidget):    
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Normalization"      

    def __init__(self, parent=None, state=None, settings=None, data_proxy=None):      
        super(NormReflWidget, self).__init__(parent, state, settings, data_proxy=data_proxy) 

        class SummaryFrame(QtGui.QFrame, ui.reflectometer.ui_norm_refl.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)
        
    def initialize_content(self):

        # Validators
        self._summary.norm_peak_from_pixel.setValidator(QtGui.QIntValidator(self._summary.norm_peak_from_pixel))
        self._summary.norm_peak_to_pixel.setValidator(QtGui.QIntValidator(self._summary.norm_peak_to_pixel))
        self._summary.norm_background_from_pixel.setValidator(QtGui.QIntValidator(self._summary.norm_background_from_pixel))
        self._summary.norm_background_to_pixel.setValidator(QtGui.QIntValidator(self._summary.norm_background_to_pixel))

        # Event connections
        self.connect(self._summary.norm_load_nexus_button, QtCore.SIGNAL("clicked()"), self._norm_load_nexus_clicked)
        self.connect(self._summary.norm_peak_load_roi, QtCore.SIGNAL("clicked()"), self._norm_peak_load_roi_clicked)
        self.connect(self._summary.norm_peak_from_pixel, QtCore.SIGNAL("textChanged(QString)"), self._check_status_of_norm_peak_save_button)
        self.connect(self._summary.norm_peak_to_pixel, QtCore.SIGNAL("textChanged(QString)"), self._check_status_of_norm_peak_save_button)
        self.connect(self._summary.norm_peak_save_roi, QtCore.SIGNAL("clicked()"), self._norm_data_save_roi_clicked)
        
        self.connect(self._summary.norm_background_switch, QtCore.SIGNAL("clicked(bool)"), self._norm_background_clicked)
        self.connect(self._summary.norm_background_load_button, QtCore.SIGNAL("clicked()"), self._norm_background_roi_clicked)
        self.connect(self._summary.norm_background_save_button, QtCore.SIGNAL("clicked()"), self._norm_background_save_roi_clicked)
        self.connect(self._summary.norm_background_from_pixel, QtCore.SIGNAL("textChanged(QString)"), self._check_status_of_norm_back_save_button)
        self.connect(self._summary.norm_background_to_pixel, QtCore.SIGNAL("textChanged(QString)"), self._check_status_of_norm_back_save_button)

    def _norm_load_nexus_clicked(self):
        """
            Load norm NeXus
        """
        pass
    
    def _check_status_of_norm_peak_save_button(self):
        """
            This function checks if the SAVE... button of the normalization peak can be enabled or not
        """
        button_status = False
        from_pixel = self._summary.norm_peak_from_pixel.text()
        to_pixel = self._summary.norm_peak_to_pixel.text()
        if from_pixel != '' and to_pixel != '':
                button_status = True
        self._summary.norm_peak_save_roi.setEnabled(button_status)
        self._check_for_missing_fields()
 
    def _check_status_of_norm_back_save_button(self):
        """
            This function will check if the background save ROI button can be enabled or not
        """
        is_checked = self._summary.norm_background_switch.isChecked()

        button_status = False
        if is_checked is True:
            from_pixel = self._summary.norm_background_from_pixel.text()
            to_pixel = self._summary.norm_background_to_pixel.text()
            if from_pixel != '' and to_pixel != '':
                button_status = True
            
        self._summary.norm_background_save_button.setEnabled(button_status)
        self._check_for_missing_fields()
        
    def _check_for_missing_fields(self):

        #peak selection
        from_pixel = self._summary.norm_peak_from_pixel.text()
        if from_pixel == '':
            self._summary.norm_peak_from_pixel_missing.setText("*")
        else:
            self._summary.norm_peak_from_pixel_missing.setText(" ")
            
        to_pixel = self._summary.norm_peak_to_pixel.text()
        if to_pixel == '':
            self._summary.norm_peak_to_pixel_missing.setText("*")
        else:
            self._summary.norm_peak_to_pixel_missing.setText(" ")
                
        #background
        is_checked = self._summary.norm_background_switch.isChecked()
        if is_checked:
            from_pixel = self._summary.norm_background_from_pixel.text()
            if from_pixel == '':
                self._summary.norm_back_from_pixel_missing.setText("*")
            else:
                self._summary.norm_back_from_pixel_missing.setText(" ")
                
            to_pixel = self._summary.norm_background_to_pixel.text()
            if to_pixel == '':
                self._summary.norm_back_to_pixel_missing.setText("*")
            else:
                self._summary.norm_back_to_pixel_missing.setText(" ")
            
        else:
            self._summary.norm_back_from_pixel_missing.setText(" ")
            self._summary.norm_back_to_pixel_missing.setText(" ")
                
    def _norm_background_clicked(self, is_checked):
        """
            This is reached when user clicks the normalization background switch
        """
        self._summary.norm_background_from_pixel_label.setEnabled(is_checked)
        self._summary.norm_background_from_pixel.setEnabled(is_checked)
        self._summary.norm_background_to_pixel_label.setEnabled(is_checked)
        self._summary.norm_background_to_pixel.setEnabled(is_checked)
        self._summary.norm_background_load_button.setEnabled(is_checked)
        #self._summary.norm_background_save_button.setEnabled(is_checked)
        self._check_for_missing_fields()
        self._check_status_of_norm_back_save_button()
    
    def _norm_peak_load_roi_clicked(self):
        """
            Reached by the Load peak selection button
        """
        fname = self.data_browse_dialog(data_type="*.txt *.dat", title="Normalization peak selection - Choose a ROI file")
        if fname:
            #retrieved from and to pixels values
            myROI = LoadSNSRoi(filename=fname)
            mode = myROI.getMode()
            pixelRange = myROI.getPixelRange()
                
            if (mode == 'narrow/broad'):                
                from_pixel = pixelRange[0]
                to_pixel = pixelRange[1]
                self._summary.norm_peak_from_pixel.setText(str(from_pixel))
                self._summary.norm_peak_to_pixel.setText(str(to_pixel))
            else:
                QtGui.QMessageBox.warning(self, "Wrong normalization peak ROI file format!",
                                            "                             Please check the ROI file!")           

    def _norm_data_save_roi_clicked(self):
        """
            Reached by the save peak button of the normalization tab
        """
        fname = self.data_browse_dialog(data_type="*.txt *.dat", title="Peak ROI file - select or enter a new ROI file name")
        if fname:
            from_px = self._summary.norm_peak_from_pixel.text()
            to_px = self._summary.norm_peak_to_pixel.text()
            _range = [int(from_px), int(to_px)]
            SaveSNSRoi(filename=fname, pixel_range=_range, mode='narrow/broad')
                
    def _norm_background_save_roi_clicked(self):
        """
            Reached by the save ROI of the background/normalization button
        """
        fname = self.data_browse_dialog(data_type="*.txt *.dat", title="Background ROI file - select or enter a new ROI file name")
        if fname:
            from_px = self._summary.norm_background_from_pixel.text()
            to_px = self._summary.norm_background_to_pixel.text()
            _range = [int(from_px), int(to_px)]
            SaveSNSRoi(filename=fname, pixel_range=_range, mode='narrow/broad')
    
    def _norm_background_roi_clicked(self):
        """
            Reached by the load background selection button
        """
        fname = self.data_browse_dialog(data_type="*.txt *.dat", title="Normalization background selection - Choose a ROI file")
        if fname:
            #retrieved from and to pixels values
            myROI = LoadSNSRoi(filename=fname)
            mode = myROI.getMode()
            pixelRange = myROI.getPixelRange()
                
            if (mode == 'narrow/broad'):                
                from_pixel = pixelRange[0]
                to_pixel = pixelRange[1]
                self._summary.norm_background_from_pixel.setText(str(from_pixel))
                self._summary.norm_background_to_pixel.setText(str(to_pixel))
            else:
                QtGui.QMessageBox.warning(self, "Wrong normalization background ROI file format!",
                                            "                                   Please check the ROI file!")                  
        
    def set_state(self, state):
        """
            Populate the UI element with the data from the given state
            @param state: norm object
        """
        
        #Peak from/to pixels
        self._summary.norm_peak_from_pixel.setText(str(state.NormPeakPixels[0]))
        self._summary.norm_peak_to_pixel.setText(str(state.NormPeakPixels[1]))
        
        #background flag
        self._summary.norm_background_switch.setChecked(state.NormBackgroundFlag)
        
        #Background from/to pixels
        self._summary.norm_background_from_pixel.setText(str(state.NormBackgroundRoi[0]))
        self._summary.norm_background_to_pixel.setText(str(state.NormBackgroundRoi[1]))        

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        
        m = NormSets()

        #Peak selection
        m.NormPeakPixels[0] = str(self._summary.norm_peak_from_pixel.text())
        m.NormPeakPixels[1] = str(self._summary.norm_peak_to_pixel.text())
        
        #background flag
        m.NormBackgroundFlag = self._summary.norm_background_switch.isChecked()
        
        #background from/to pixels
        roi_from = str(self._summary.norm_background_from_pixel.text())
        roi_to = str(self._summary.norm_background_to_pixel.text())
        m.NormBackgroundRoi = [roi_from, roi_to]

        return m
