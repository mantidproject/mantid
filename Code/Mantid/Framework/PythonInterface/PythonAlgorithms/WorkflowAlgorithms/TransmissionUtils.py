import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
import os
import sys
from reduction_workflow.find_data import find_data

def load_monitors(self, property_manager):
    """
        Load files necessary to compute transmission.
        Return their names.
    """
    output_str = ""
    property_manager_name = self.getPropertyValue("ReductionProperties")
    # If we need to get a special beam center position, do it now
    beam_center_x = None
    beam_center_y = None
    beam_center_x_input = self.getProperty("BeamCenterX").value
    beam_center_y_input = self.getProperty("BeamCenterY").value
    if beam_center_x_input > 0 and beam_center_y_input > 0:
        beam_center_x = beam_center_x_input
        beam_center_y = beam_center_y_input  
    
    # Get instrument to use with FileFinder
    instrument = ''
    if property_manager.existsProperty("InstrumentName"):
        instrument = property_manager.getProperty("InstrumentName").value

    # Get the data loader
    def _load_data(filename, output_ws):
        if not property_manager.existsProperty("LoadAlgorithm"):
            Logger.get("SANSDirectBeamTransmission").error("SANS reduction not set up properly: missing load algorithm")
            raise RuntimeError, "SANS reduction not set up properly: missing load algorithm"
        p=property_manager.getProperty("LoadAlgorithm")
        alg=Algorithm.fromString(p.valueAsStr)
        alg.setProperty("Filename", filename)
        alg.setProperty("OutputWorkspace", output_ws)
        if beam_center_x is not None and beam_center_y is not None:
            if alg.existsProperty("BeamCenterX"):
                alg.setProperty("BeamCenterX", beam_center_x)
            if alg.existsProperty("BeamCenterY"):
                alg.setProperty("BeamCenterY", beam_center_y)
        if alg.existsProperty("ReductionProperties"):
            alg.setProperty("ReductionProperties", property_manager_name)
        alg.execute()
        msg = ''
        if alg.existsProperty("OutputMessage"):
            msg = alg.getProperty("OutputMessage").value
        return msg

    # Load the data files
    sample_file = self.getPropertyValue("SampleDataFilename")
    empty_file = self.getPropertyValue("EmptyDataFilename")

    sample_ws_name = "__transmission_sample"
    filepath = find_data(sample_file, instrument=instrument)
    l_text = _load_data(sample_file, sample_ws_name)
    output_str += "   Sample: %s\n" % os.path.basename(sample_file)
    output_str += "   %s\n" % l_text
    
    empty_ws_name = "__transmission_empty"
    filepath = find_data(empty_file, instrument=instrument)
    l_text = _load_data(empty_file, empty_ws_name)
    output_str += "   Empty: %s\n" % os.path.basename(empty_file)
    output_str += "   %s\n" % l_text
    
    # Subtract dark current
    use_sample_dc = self.getProperty("UseSampleDarkCurrent").value
    dark_current_data = self.getPropertyValue("DarkCurrentFilename")
    
    partial_out1 = subtract_dark_current(self, sample_ws_name, property_manager)
    partial_out2 = subtract_dark_current(self, empty_ws_name, property_manager)
    partial_out = "\n   Sample: %s\n   Empty: %s" % (partial_out1, partial_out2)
    partial_out.replace('\n', '   \n')
    output_str += partial_out
        
    # Find which pixels to sum up as our "monitor". At this point we have moved the detector
    # so that the beam is at (0,0), so all we need is to sum the area around that point.
    #TODO: in IGOR, the error-weighted average is computed instead of simply summing up the pixels
    sample_ws = AnalysisDataService.retrieve(sample_ws_name)
    beam_radius = self.getProperty("BeamRadius").value
    pixel_size_x = sample_ws.getInstrument().getNumberParameter("x-pixel-size")[0]
    cylXML = '<infinite-cylinder id="transmission_monitor">' + \
               '<centre x="0.0" y="0.0" z="0.0" />' + \
               '<axis x="0.0" y="0.0" z="1.0" />' + \
               '<radius val="%12.10f" />' % (beam_radius*pixel_size_x/1000.0) + \
             '</infinite-cylinder>\n'
             
    # Use the transmission workspaces to find the list of monitor pixels
    # since the beam center may be at a different location
    alg = AlgorithmManager.create("FindDetectorsInShape")
    alg.initialize()
    alg.setChild(True)
    alg.setProperty("Workspace", sample_ws)
    alg.setPropertyValue("ShapeXML", cylXML)
    alg.execute()
    det_list = alg.getProperty("DetectorList").value
    first_det = det_list[0]

    #TODO: check that both workspaces have the same masked spectra
    
    # Get normalization for transmission calculation
    monitor_det_ID = None
    if property_manager.existsProperty("TransmissionNormalisation"):
        if property_manager.getProperty("TransmissionNormalisation").value=="Monitor":
            monitor_det_ID = int(sample_ws.getInstrument().getNumberParameter("default-incident-monitor-spectrum")[0])
        else:
            monitor_det_ID = int(sample_ws.getInstrument().getNumberParameter("default-incident-timer-spectrum")[0])
    elif property_manager.existsProperty("NormaliseAlgorithm"):
        def _normalise(workspace):
            p=property_manager.getProperty("NormaliseAlgorithm")
            alg=Algorithm.fromString(p.valueAsStr)
            alg.setProperty("InputWorkspace", workspace)
            alg.setProperty("OutputWorkspace", workspace)
            if alg.existsProperty("ReductionProperties"):
                alg.setProperty("ReductionProperties", property_manager_name)
            alg.execute()
            msg = ''
            if alg.existsProperty("OutputMessage"):
                msg += alg.getProperty("OutputMessage").value+'\n'
            return msg
        norm_msg = _normalise(empty_ws_name)
        output_str += "   %s\n" % norm_msg.replace('\n', '   \n')
        norm_msg = _normalise(sample_ws_name)
        output_str += "   %s\n" % norm_msg.replace('\n', '   \n')
        
    empty_mon_ws = "__empty_mon"
    sample_mon_ws = "__sample_mon"
    
    det_list = [str(i) for i in det_list]
    det_list = ','.join(det_list)
    api.GroupDetectors(InputWorkspace=empty_ws_name,  OutputWorkspace=empty_mon_ws,  DetectorList=det_list, KeepUngroupedSpectra="1")
    api.GroupDetectors(InputWorkspace=sample_ws_name, OutputWorkspace=sample_mon_ws, DetectorList=det_list, KeepUngroupedSpectra="1")
    api.ConvertToMatrixWorkspace(InputWorkspace=empty_mon_ws, OutputWorkspace=empty_mon_ws)
    api.ConvertToMatrixWorkspace(InputWorkspace=sample_mon_ws, OutputWorkspace=sample_mon_ws)
    
    # Calculate transmission. Use the reduction method's normalization channel (time or beam monitor)
    # as the monitor channel.
    api.RebinToWorkspace(WorkspaceToRebin=empty_mon_ws,
                         WorkspaceToMatch=sample_mon_ws,
                         OutputWorkspace=empty_mon_ws)

    # Clean up
    for ws in [empty_ws_name, sample_ws_name]:
        if AnalysisDataService.doesExist(ws):
            AnalysisDataService.remove(ws)       

    return sample_mon_ws, empty_mon_ws, first_det, output_str, monitor_det_ID
    
def calculate_transmission(self, sample_mon_ws, empty_mon_ws, first_det, trans_output_workspace, monitor_det_ID):
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
            
def apply_transmission(self, workspace, trans_workspace):
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
   
def subtract_dark_current(self, workspace_name, property_manager):
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
            