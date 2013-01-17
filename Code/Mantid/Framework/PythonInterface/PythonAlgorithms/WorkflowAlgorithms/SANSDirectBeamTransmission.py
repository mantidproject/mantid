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
        import TransmissionUtils
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
            sample_mon_ws, empty_mon_ws, first_det, output_str, monitor_det_ID = TransmissionUtils.load_monitors(self, property_manager)
            TransmissionUtils.calculate_transmission(self, sample_mon_ws, empty_mon_ws, first_det, trans_ws_name, monitor_det_ID)
            trans_ws = AnalysisDataService.retrieve(trans_ws_name)
            
        # 2- Apply correction (Note: Apply2DTransCorr)
        input_ws = self.getProperty("InputWorkspace").value
        workspace = self.getPropertyValue("InputWorkspace")
        api.CloneWorkspace(InputWorkspace=input_ws, OutputWorkspace='__'+workspace)
        
        TransmissionUtils.apply_transmission(self, '__'+workspace, trans_ws_name)

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
                
#############################################################################################

registerAlgorithm(SANSDirectBeamTransmission)
