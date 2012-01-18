from PyQt4 import QtGui, QtCore
from reduction_gui.reduction.output_script import Output
from reduction_gui.widgets.output import OutputWidget 
import ui.ui_hfir_output
import math

IS_IN_MANTIDPLOT = False
try:
    import _qti
    from MantidFramework import *
    mtd.initialise()
    from mantidsimple import *
    IS_IN_MANTIDPLOT = True
    from reduction.instruments.sans.sns_command_interface import *
except:
    pass

class EQSANSOutputWidget(OutputWidget):    
    """
        Widget that presents the transmission options to the user
    """
    ## Widget name
    name = "Output"      
    
    def __init__(self, parent=None, state=None, settings=None):
        super(EQSANSOutputWidget, self).__init__(parent=parent, state=state, settings=settings) 

    def initialize_content(self):
        """
            Declare the validators and event connections for the 
            widgets loaded through the .ui file.
        """

        # Clear data list
        self._content.output_text_edit.clear()
        
        if IS_IN_MANTIDPLOT:
            self._content.n_q_bins_edit.setValidator(QtGui.QIntValidator(self._content.n_q_bins_edit))
            self._content.n_q_bins_edit.setText(QtCore.QString("100"))
            self.connect(self._content.rebin_button, QtCore.SIGNAL("clicked()"), self._rebin)
            self._content.lin_binning_radio.setChecked(True)
        else:
            self._content.rebin_groupbox.deleteLater()
            self._content.n_q_bins_label.hide()
            self._content.n_q_bins_edit.hide()
            self._content.rebin_button.hide()
            self._content.lin_binning_radio.hide()
            self._content.log_binning_radio.hide()
        
    def _perform_rebin(self, workspace):
        avg = ReductionSingleton().get_azimuthal_averager()
        avg._binning = None
        avg._nbins = self._content.n_q_bins_edit.text().toInt()[0]
        avg._log_binning = self._content.log_binning_radio.isChecked()
        avg.execute(ReductionSingleton(), event_ws)
        #Rebin(InputWorkspace=event_ws, OutputWorkspace=event_ws, Params="2.6,0.05,5.7", PreserveEvents=True)
        #Q1DWeighted(InputWorkspace=event_ws, OutputWorkspace=workspace, OutputBinning=binning)
        #ReplaceSpecialValues(workspace, workspace, NaNValue=0.0, NaNError=0.0, InfinityValue=0.0, InfinityError=0.0)
                
    def _rebin(self):
        if not hasattr(ReductionSingleton(), "output_workspaces"):
            return
        
        ws_list = ReductionSingleton().output_workspaces
        to_rebin = []
        
        def _check_and_append(ws):
            if mtd[ws].getAxis(0).getUnit().name() == "MomentumTransfer":
                if mtd[ws].getRun().hasProperty("event_ws"):
                    event_ws = mtd[ws].getRun().getProperty("event_ws").value
                    if mtd.workspaceExists(event_ws) and event_ws not in to_rebin:
                        to_rebin.append(event_ws)
        
        for item in ws_list:
            if type(item)==list:
                for ws in item:
                    _check_and_append(ws)
            else:
                _check_and_append(item)
                
        for event_ws in to_rebin:
            avg = ReductionSingleton().get_azimuthal_averager()
            avg._binning = None
            avg._nbins = self._content.n_q_bins_edit.text().toInt()[0]
            avg._log_binning = self._content.log_binning_radio.isChecked()
            avg.execute(ReductionSingleton(), event_ws)
        
        
    def set_state(self, state):
        self._content.output_text_edit.setText(QtCore.QString(state.log_text))
            
    def get_state(self):
        """
            Returns an object with the state of the interface
        """
        return Output()
