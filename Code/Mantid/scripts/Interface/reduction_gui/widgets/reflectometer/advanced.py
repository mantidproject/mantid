from PyQt4 import QtGui, uic, QtCore
import reduction_gui.widgets.util as util
import math
import os
#from reduction_gui.reduction.sans.hfir_options_script import ReductionOptions
from reduction_gui.reduction.reflectometer.refl_advanced_script import AdvancedSets
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
import ui.reflectometer.ui_advanced

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

class AdvancedWidget(BaseWidget):    
    """
        Widget that present instrument details to the user
    """
    ## Widget name
    name = "Advanced"      
    
    def __init__(self, parent=None, state=None, settings=None, data_proxy=None):      
        super(AdvancedWidget, self).__init__(parent, state, settings, data_proxy=data_proxy) 

        class SummaryFrame(QtGui.QFrame, ui.reflectometer.ui_advanced.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)
#                
        self._summary = SummaryFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._summary)

        if state is not None:
            self.set_state(state)
        else:
            self.set_state(AdvancedSets())

    def initialize_content(self):

        # Validators
        self._summary.advanced_auto_cleanup_percentage_value.setValidator(QtGui.QDoubleValidator(self._summary.advanced_auto_cleanup_percentage_value))

        # Event connections
        self.connect(self._summary.advanced_auto_cleanup_switch, QtCore.SIGNAL("clicked(bool)"), self._advanced_auto_cleanup_clicked)
        self.connect(self._summary.advanced_overwrite_data_geometry_switch, QtCore.SIGNAL("clicked(bool)"), self._advanced_data_geometry_clicked)            
        self.connect(self._summary.advanced_overwrite_norm_geometry_switch, QtCore.SIGNAL("clicked(bool)"), self._advanced_norm_geometry_clicked)            

        self.connect(self._summary.advanced_overwrite_data_button, QtCore.SIGNAL("clicked()"), self._data_geometry_clicked)
        self.connect(self._summary.advanced_overwrite_norm_button, QtCore.SIGNAL("clicked()"), self._norm_geometry_clicked)

    def _advanced_auto_cleanup_clicked(self, is_clicked):
        """
            Reached by auto_cleanup switch
        """
        self._summary.advanced_auto_cleanup_percentage_label.setEnabled(is_clicked)
        self._summary.advanced_auto_cleanup_percentage_value.setEnabled(is_clicked)
        self._summary.advanced_auto_cleanup_percentage_label2.setEnabled(is_clicked)

    def _advanced_data_geometry_clicked(self, is_clicked):
        """
            Reached by the Overwrite data geometry
        """
        self._summary.advanced_overwrite_data_button.setEnabled(is_clicked)
        self._summary.advanced_overwrite_data_label.setEnabled(is_clicked)
        
    def _advanced_norm_geometry_clicked(self, is_clicked):
        """
            Reached by the Overwrite normalization geometry
        """
        self._summary.advanced_overwrite_norm_button.setEnabled(is_clicked)
        self._summary.advanced_overwrite_norm_label.setEnabled(is_clicked)

    def _data_geometry_clicked(self):
        """
            Reached by Overwrite data geometry browse button
        """
        fname = self.data_browse_dialog(data_type="*.nxs", title="Data geometry file - Choose a geometry file")
        if fname:
            self._summary.advanced_overwrite_data_label.setText(fname)      

    def _norm_geometry_clicked(self):
        """
            Reached by Overwrite norm geometry browse button
        """
        fname = self.data_browse_dialog(data_type="*.nxs", title="Normalization geometry file - Choose a geometry file")
        if fname:
            self._summary.advanced_overwrite_norm_label.setText(fname)      

    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        m = AdvancedSets()
        
        #general
        m.FilteringDataFlag = self._summary.advanced_filtering_data.isChecked()
        m.DtOverTFlag = self._summary.advanced_dt_d.isChecked()
        m.AutoCleanupFlag = self._summary.advanced_auto_cleanup_switch.isChecked()
        m.PercentageQToRemove = str(self._summary.advanced_auto_cleanup_percentage_value.text())

        #Geometry
        m.OverwriteDataGeometryFlag = self._summary.advanced_overwrite_data_geometry_switch.isChecked()
        m.DataGeometryFileName = self._summary.advanced_overwrite_data_label.text()
        m.OverwriteNormGeometryFlag = self._summary.advanced_overwrite_norm_geometry_switch.isChecked()
        m.NormGeometryFileName = self._summary.advanced_overwrite_norm_label.text()
        
        #Intermediate files
        m.DataCombinedBckFlag = self._summary.data_combined_back_tof.isChecked()
        m.DataCombinedSpecularFlag = self._summary.data_combined_specular_tof.isChecked()
        m.DataCombinedSubtractedFlag = self._summary.data_combined_subtracted_tof.isChecked()
        m.NormCombinedBckFlag = self._summary.normalization_combined_back_tof.isChecked()
        m.NormCombinedSpecularFlag = self._summary.normalization_combined_specular_tof.isChecked()
        m.NormCombinedSubtractedFlag = self._summary.normalization_combined_subtracted_tof.isChecked()
        m.RvsTOFFlag = self._summary.r_vs_tof.isChecked()
        m.RvsTOFCombinedFlag = self._summary.r_vs_tof_combined.isChecked()
        m.RvsQFlag = self._summary.r_vs_q.isChecked()
        m.RvsQRebinningFlag = self._summary.r_vs_q_after_rebinning.isChecked()

        return m

    def set_state(self, state):
        """
            Populate the UI elements with the data from the given state. 
            @param state: data object           
        """

        #general
        self._summary.advanced_filtering_data.setChecked(state.FilteringDataFlag)
        self._summary.advanced_dt_d.setChecked(state.DtOverTFlag)
        self._summary.advanced_auto_cleanup_switch.setChecked(state.AutoCleanupFlag)
        self._summary.advanced_auto_cleanup_percentage_value.setText(state.PercentageQToRemove)
        
        #geometry
        self._summary.advanced_overwrite_data_geometry_switch.setChecked(state.OverwriteDataGeometryFlag)
        self._summary.advanced_overwrite_data_label.setText(state.DataGeometryFileName)
        self._summary.advanced_overwrite_norm_geometry_switch.setChecked(state.OverwriteNormGeometryFlag)
        self._summary.advanced_overwrite_norm_label.setText(state.NormGeometryFileName)

        #Intermediate files
        self._summary.data_combined_back_tof.setChecked(state.DataCombinedBckFlag)
        self._summary.data_combined_specular_tof.setChecked(state.DataCombinedSpecularFlag)
        self._summary.data_combined_subtracted_tof.setChecked(state.DataCombinedSubtractedFlag)
        self._summary.normalization_combined_back_tof.setChecked(state.NormCombinedBckFlag)
        self._summary.normalization_combined_specular_tof.setChecked(state.NormCombinedSpecularFlag)
        self._summary.normalization_combined_subtracted_tof.setChecked(state.NormCombinedSubtractedFlag)
        self._summary.r_vs_tof.setChecked(state.RvsTOFFlag)
        self._summary.r_vs_tof_combined.setChecked(state.RvsTOFCombinedFlag)
        self._summary.r_vs_q.setChecked(state.RvsQFlag)
        self._summary.r_vs_q_after_rebinning.setChecked(state.RvsQRebinningFlag)
