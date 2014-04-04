"""*WIKI* 

Facade over [[CreateTransmissionWorkspace]]. Pull numeric parameters out of the instrument parameters where possible. You can override any of these automatically
applied defaults by providing your own value for the input.

See [[CreateTransmissionWorkspace]] for more information on the wrapped algorithm.

*WIKI*"""


import sys
from mantid.simpleapi import CreateTransmissionWorkspace
from mantid.api import *
from mantid.kernel import *

class CreateTransmissionWorkspaceAuto(DataProcessorAlgorithm):
    
    def category(self):
        return "Reflectometry\\ISIS"

    def name(self):
        return "CreateTransmissionWorkspaceAuto"
    
    def PyInit(self):
    
        self.setOptionalMessage("Creates a transmission run workspace in Wavelength from input TOF workspaces.")
        self.setWikiSummary("Creates a transmission run workspace in Wavelength from input TOF workspaces. See [[Reflectometry_Guide]]")
        
        analysis_modes = ["PointDetectorAnalysis", "MultiDetectorAnalysis"]
        analysis_mode_validator = StringListValidator(analysis_modes)
        
        self.declareProperty(name="AnalysisMode", defaultValue=analysis_modes[0], validator=analysis_mode_validator, direction = Direction.Input, doc="Analysis Mode to Choose")
        
        input_validator = WorkspaceUnitValidator("TOF")
        self.declareProperty(MatrixWorkspaceProperty("FirstTransmissionRun", "", Direction.Input, validator=input_validator), "Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("SecondTransmissionRun", "", optional=PropertyMode.Optional,  validator=input_validator, direction=Direction.Input), "Second transmission run workspace in TOF")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output transmission workspace in wavelength")

        self.declareProperty(FloatArrayProperty(name="Params", values=[sys.maxint]), doc="Rebinning Parameters. See Rebin for format.")
        self.declareProperty(name="StartOverlap", defaultValue=sys.float_info.max, doc="Overlap in Q.")
        self.declareProperty(name="EndOverlap", defaultValue=sys.float_info.max, doc="End overlap in Q.")
        index_bounds = IntBoundedValidator()
        index_bounds.setLower(0)
        self.declareProperty(name="I0MonitorIndex", defaultValue=sys.maxint, validator=index_bounds, doc="I0 monitor index" )
        self.declareProperty(name="ProcessingInstructions", direction=Direction.Input, defaultValue="", doc="Processing instructions to extract spectra belonging to detector workspace, see [[PerformIndexOperations]] for syntax.")
        self.declareProperty(name="WavelengthMin", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Wavelength Min in angstroms")
        self.declareProperty(name="WavelengthMax", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Wavelength Max in angstroms")
        self.declareProperty(name="WavelengthStep", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Wavelength step in angstroms")
        self.declareProperty(name="MonitorBackgroundWavelengthMin", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor wavelength background min in angstroms")
        self.declareProperty(name="MonitorBackgroundWavelengthMax", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor wavelength background max in angstroms")
        self.declareProperty(name="MonitorIntegrationWavelengthMin", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor integral min in angstroms")
        self.declareProperty(name="MonitorIntegrationWavelengthMax", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor integral max in angstroms")
        
        
    def value_or_none(self, prop_name, instrument):
        property = self.getProperty(prop_name)
        if property.isDefault:
            return None # TODO: check this
        else:
            return property.value
    
        
    def value_to_apply(self, prop_name, instrument, idf_name=""):
        property = self.getProperty(prop_name)
        if property.isDefault:
            return instrument.getNumberParameter(idf_name)[0] 
        else:
            return property.value
        
    def PyExec(self):
        first_ws = self.getProperty("FirstTransmissionRun").value
        instrument = first_ws.getInstrument()
        
        '''
        Get all the inputs.
        '''
        outputWorkspaceName = self.getPropertyValue("OutputWorkspace")
        
        analysis_mode = self.getPropertyValue("AnalysisMode")
        
        second_ws_property = self.getProperty("SecondTransmissionRun")
        second_ws = None
        if not second_ws_property.isDefault:
            second_ws = second_ws_property.value
        
        start_overlap = self.value_or_none("StartOverlap", instrument)
        
        end_overlap = self.value_or_none("EndOverlap", instrument)
        
        params = self.value_or_none("Params", instrument)
        
        i0_monitor_index = int(self.value_to_apply("I0MonitorIndex", instrument, "I0MonitorIndex"))
        
        processing_commands = str()
        workspace_index_list = list()
        if self.getProperty("ProcessingInstructions").isDefault:
            if analysis_mode == "PointDetectorAnalysis":
                workspace_index_list.append( int(instrument.getNumberParameter("PointDetectorStart")[0]) )
                workspace_index_list.append( int(instrument.getNumberParameter("PointDetectorStop")[0]) )
            else:
                workspace_index_list.append( int(instrument.getNumberParameter("MultiDetectorStart")[0]) )
                workspace_index_list.append( first_ws.getNumberHistograms() - 1)
            processing_commands = ','.join(map(str, workspace_index_list))
        else:
            processing_commands = self.getProperty("ProcessingInstructions").value
        
        wavelength_min = self.value_to_apply("WavelengthMin", instrument, "LambdaMin")
        
        wavelength_max = self.value_to_apply("WavelengthMax", instrument, "LambdaMax")
        
        wavelength_step = self.value_or_none("WavelengthStep", instrument)
        
        wavelength_back_min = self.value_to_apply("MonitorBackgroundWavelengthMin", instrument, "MonitorBackgroundMin")
        
        wavelength_back_max = self.value_to_apply("MonitorBackgroundWavelengthMax", instrument, "MonitorBackgroundMax")
        
        wavelength_integration_min = self.value_to_apply("MonitorIntegrationWavelengthMin", instrument, "MonitorIntegralMin")
        
        wavelength_integration_max = self.value_to_apply("MonitorIntegrationWavelengthMax", instrument, "MonitorIntegralMax")
        
        '''
        Pass the arguments and execute the main algorithm.
        '''
        output_ws = CreateTransmissionWorkspace(FirstTransmissionRun=first_ws, SecondTransmissionRun=second_ws, OutputWorkspace=outputWorkspaceName, 
                                    StartOverlap=start_overlap, EndOverlap=end_overlap, Params=params, I0MonitorIndex=i0_monitor_index,
                                    ProcessingInstructions=processing_commands, WavelengthMin=wavelength_min, WavelengthStep=wavelength_step, WavelengthMax=wavelength_max,
                                    MonitorBackgroundWavelengthMin=wavelength_back_min, MonitorBackgroundWavelengthMax=wavelength_back_max,
                                    MonitorIntegrationWavelengthMin=wavelength_integration_min, MonitorIntegrationWavelengthMax=wavelength_integration_max)
                                    
        
        self.setProperty("OutputWorkspace", output_ws)
    
AlgorithmFactory.subscribe(CreateTransmissionWorkspaceAuto)
