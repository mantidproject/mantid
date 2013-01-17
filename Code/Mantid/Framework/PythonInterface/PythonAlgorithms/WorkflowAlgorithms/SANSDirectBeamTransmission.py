"""*WIKI* 
    Compute transmission using the direct beam method
*WIKI*"""
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
import os
import sys
from reduction_workflow.find_data import find_data

class SANSDirectBeamTransmission(PythonAlgorithm):

    def category(self):
        return "Workflow\\SANS;PythonAlgorithms"

    def name(self):
        return "SANSDirectBeamTransmission"
    
    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", 
                                                     direction=Direction.Input))
        self.declareProperty(FileProperty("SampleDataFilename", "",
                                          action=FileAction.Load,                                          
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty(FileProperty("EmptyDataFilename", "",
                                          action=FileAction.Load,                                          
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty("BeamRadius", 3.0, "Beam radius [pixels]")
        self.declareProperty("ThetaDependent", True, 
                             "If true, a theta-dependent correction will be applied")
        self.declareProperty(FileProperty("DarkCurrentFilename", "",
                                          action=FileAction.OptionalLoad,                                          
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty("UseSampleDarkCurrent", False, 
                             "If true, the sample dark current will be used")
        self.declareProperty("BeamCenterX", 0.0, "Beam center position in X")
        self.declareProperty("BeamCenterY", 0.0, "Beam center position in Y")
        self.declareProperty("ReductionProperties", "__sans_reduction_properties", 
                             validator=StringMandatoryValidator(),
                             doc="Property manager name for the reduction")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", 
                                                     direction = Direction.Output))
        self.declareProperty("MeasuredTransmission", 0.0, 
                             direction=Direction.Output)
        self.declareProperty("MeasuredError", 0.0, 
                             direction=Direction.Output)
        self.declareProperty("OutputMessage", "", 
                             direction=Direction.Output, doc = "Output message")
        
    def PyExec(self):
        sample_file = self.getPropertyValue("SampleDataFilename")
        empty_file = self.getPropertyValue("EmptyDataFilename")
        
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)
        property_list = [p.name for p in property_manager.getProperties()]
        
        # Build the name we are going to give the transmission workspace
        sample_basename = os.path.basename(sample_file)
        empty_basename = os.path.basename(empty_file)
        entry_name = "Transmission%s%s" % (sample_basename, empty_basename)
        trans_ws_name = "__transmission_fit_%s" % sample_basename
        trans_ws = None
        
        if entry_name in property_list:
            trans_ws_name = property_manager.getProperty(entry_name)
            if AnalysisDataService.doesExist(trans_ws_name):
                trans_ws = AnalysisDataService.retrieve(trans_ws_name)
        
        if trans_ws is None:
            # Load data files
            sample_mon_ws, empty_mon_ws, first_det, output_str, monitor_det_ID= self._load_monitors(property_manager)
            self._calculate_transmission(sample_mon_ws, empty_mon_ws, first_det, trans_ws_name, monitor_det_ID)
            trans_ws = AnalysisDataService.retrieve(trans_ws_name)
            
        # 2- Apply correction (Note: Apply2DTransCorr)
        input_ws = self.getProperty("InputWorkspace").value
        workspace = self.getPropertyValue("InputWorkspace")
        api.CloneWorkspace(InputWorkspace=input_ws, OutputWorkspace='__'+workspace)
        
        self._apply_transmission('__'+workspace, trans_ws_name)

        trans = trans_ws.dataY(0)[0]
        error = trans_ws.dataE(0)[0]
        
        if len(trans_ws.dataY(0))==1:
            self.setProperty("MeasuredTransmission", trans)
            self.setProperty("MeasuredError", error)
            output_str = "%s   T = %6.2g += %6.2g\n" % (output_str, trans, error)
        output_msg = "Transmission correction applied [%s]\n%s\n" % (trans_ws_name, output_str)
        
        output_ws = AnalysisDataService.retrieve('__'+workspace)
        self.setProperty("OutputWorkspace", output_ws)
        self.setPropertyValue("OutputMessage", output_msg)
        
    def _load_monitors(self, property_manager):
        from TransmissionUtils import load_monitors
        return load_monitors(self, property_manager)
    
    def _calculate_transmission(self, sample_mon_ws, empty_mon_ws, first_det, trans_output_workspace, monitor_det_ID):
        """
            Compute zero-angle transmission
        """
        try:
            if monitor_det_ID is not None:
                api.CalculateTransmission(DirectRunWorkspace=empty_mon_ws, SampleRunWorkspace=sample_mon_ws, 
                                      OutputWorkspace=trans_output_workspace,
                                      IncidentBeamMonitor=str(monitor_det_ID), 
                                      TransmissionMonitor=str(first_det),
                                      OutputUnfittedData=True)
            else:
                api.CalculateTransmission(DirectRunWorkspace=empty_mon_ws, SampleRunWorkspace=sample_mon_ws, 
                                      OutputWorkspace=trans_output_workspace,
                                      TransmissionMonitor=str(first_det),
                                      OutputUnfittedData=True)
        except:
            raise RuntimeError, "Couldn't compute transmission. Is the beam center in the right place?\n\n%s" % sys.exc_value
        
        
        for ws in [empty_mon_ws, sample_mon_ws]:
            if AnalysisDataService.doesExist(ws):
                AnalysisDataService.remove(ws)       
                
    def _apply_transmission(self, workspace, trans_workspace):
        """
            Apply transmission correction
            @param workspace: workspace to apply correction to
            @param trans_workspace: workspace name for of the transmission
        """
        # Make sure the binning is compatible
        api.RebinToWorkspace(WorkspaceToRebin=trans_workspace,
                             WorkspaceToMatch=workspace,
                             OutputWorkspace=trans_workspace+'_rebin',
                             PreserveEvents=False)
        # Apply angle-dependent transmission correction using the zero-angle transmission
        theta_dependent = self.getProperty("ThetaDependent").value
        
        api.ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                        TransmissionWorkspace=trans_workspace+'_rebin', 
                                        OutputWorkspace=workspace,
                                        ThetaDependent=theta_dependent)          

        if AnalysisDataService.doesExist(trans_workspace+'_rebin'):
            AnalysisDataService.remove(trans_workspace+'_rebin')          
       
    def _subtract_dark_current(self, workspace_name, property_manager):
        """
            Subtract the dark current
            @param workspace_name: name of the workspace to subtract from
            @param property_manager: property manager object 
        """
        # Subtract dark current
        use_sample_dc = self.getProperty("UseSampleDarkCurrent").value
        dark_current_data = self.getPropertyValue("DarkCurrentFilename")
        property_manager_name = self.getProperty("ReductionProperties").value
        # Get instrument to use with FileFinder
        instrument = ''
        if property_manager.existsProperty("InstrumentName"):
            instrument = property_manager.getProperty("InstrumentName").value
        
        dark_current_property = "DefaultDarkCurrentAlgorithm"
        def _dark(workspace, dark_current_property):
            if property_manager.existsProperty(dark_current_property):
                p=property_manager.getProperty(dark_current_property)
                # Dark current subtraction for sample data
                alg=Algorithm.fromString(p.valueAsStr)
                alg.setProperty("InputWorkspace", workspace)
                alg.setProperty("OutputWorkspace", workspace)
                alg.setProperty("Filename", dark_current_data)
                if alg.existsProperty("PersistentCorrection"):
                    alg.setProperty("PersistentCorrection", False)
                if alg.existsProperty("ReductionProperties"):
                    alg.setProperty("ReductionProperties", property_manager_name)
                alg.execute()
                msg = "Dark current subtracted"
                if alg.existsProperty("OutputMessage"):
                    msg += alg.getProperty("OutputMessage").value
                return msg

        if use_sample_dc is True:
            _dark(workspace_name, "DarkCurrentAlgorithm")
        elif len(dark_current_data.strip())>0:
            _dark(workspace_name, "DefaultDarkCurrentAlgorithm")        

#############################################################################################

registerAlgorithm(SANSDirectBeamTransmission)
