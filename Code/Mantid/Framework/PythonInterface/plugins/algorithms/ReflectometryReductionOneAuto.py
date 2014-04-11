"""*WIKI* 

Facade over [[ReflectometryReductionOne]]. Pulls numeric parameters out of the instrument parameters where possible. You can override any of these automatically
applied defaults by providing your own value for the input.

See [[ReflectometryReductionOne]] for more information on the wrapped algorithm.

*WIKI*"""


import sys
from mantid.simpleapi import ReflectometryReductionOne
from mantid.api import *
from mantid.kernel import *

class ReflectometryReductionOneAuto(DataProcessorAlgorithm):
    
    def category(self):
        return "Reflectometry\\ISIS"

    def name(self):
        return "ReflectometryReductionOneAuto"
    
    def PyInit(self):
    
        self.setOptionalMessage("Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections.")
        self.setWikiSummary("Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections. See [[Reflectometry_Guide]]")
   
        input_validator = WorkspaceUnitValidator("TOF")
        self.declareProperty(MatrixWorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Mandatory), "Input run in TOF")
        
        analysis_modes = ["PointDetectorAnalysis", "MultiDetectorAnalysis"]
        analysis_mode_validator = StringListValidator(analysis_modes)
        
        self.declareProperty(IntArrayProperty(name="RegionOfDirectBeam", direction=Direction.Input), doc="Indices of the spectra a pair (lower, upper) that mark the ranges that correspond to the direct beam in multi-detector mode.")

        self.declareProperty(name="AnalysisMode", defaultValue=analysis_modes[0], validator=analysis_mode_validator, direction = Direction.Input, doc="Analysis Mode to Choose")
        
        self.declareProperty(MatrixWorkspaceProperty("FirstTransmissionRun", "", Direction.Input, optional=PropertyMode.Optional), "First transmission run workspace in TOF or Wavelength")
        self.declareProperty(MatrixWorkspaceProperty("SecondTransmissionRun", "", optional=PropertyMode.Optional,  validator=input_validator, direction=Direction.Input), "Second transmission run workspace in TOF")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace in wavelength q")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspaceWavelength", "", Direction.Output), "Output workspace in wavelength")
        
        self.declareProperty(FloatArrayProperty(name="Params", values=[sys.maxint]), doc="Rebinning Parameters. See Rebin for format.")
        self.declareProperty(name="StartOverlap", defaultValue=sys.float_info.max, doc="Overlap in Q.")
        self.declareProperty(name="EndOverlap", defaultValue=sys.float_info.max, doc="End overlap in Q.")
        index_bounds = IntBoundedValidator()
        index_bounds.setLower(0)
        
        self.declareProperty(name="I0MonitorIndex", defaultValue=sys.maxint, validator=index_bounds, doc="I0 monitor index" )
        self.declareProperty(name="ProcessingInstructions", direction=Direction.Input, defaultValue="", doc="Processing commands to select and add spectrum to make a detector workspace. See [[PeformIndexOperations]] for syntax.")
        self.declareProperty(name="WavelengthMin", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Wavelength Min in angstroms")
        self.declareProperty(name="WavelengthMax", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Wavelength Max in angstroms")
        self.declareProperty(name="WavelengthStep", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Wavelength step in angstroms")
        self.declareProperty(name="MonitorBackgroundWavelengthMin", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor wavelength background min in angstroms")
        self.declareProperty(name="MonitorBackgroundWavelengthMax", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor wavelength background max in angstroms")
        self.declareProperty(name="MonitorIntegrationWavelengthMin", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor integral min in angstroms")
        self.declareProperty(name="MonitorIntegrationWavelengthMax", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor integral max in angstroms")
        
        self.declareProperty(name="DetectorComponentName", direction=Direction.Input, defaultValue="", doc="Name of the detector component i.e. point-detector. If these are not specified, the algorithm will attempt lookup using a standard naming convention.")
        self.declareProperty(name="SampleComponentName", direction=Direction.Input, defaultValue="", doc="Name of the sample component i.e. some-surface-holder. If these are not specified, the algorithm will attempt lookup using a standard naming convention." )
        
        self.declareProperty(name="ThetaIn", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Final theta in degrees")
        self.declareProperty(name="ThetaOut", direction=Direction.Output, defaultValue=sys.float_info.max, doc="Calculated final theta in degrees.")
        
        self.declareProperty(name="CorrectDetectorPositions", defaultValue=True, direction=Direction.Input, doc="Correct detector positions using ThetaIn (if given)")
        
        self.declareProperty(name="StrictSpectrumChecking", defaultValue=True, direction=Direction.Input, doc="Strict checking between spectrum numbers in input workspaces and transmission workspaces.")
        
    def value_or_none(self, prop_name):
        property = self.getProperty(prop_name)
        if property.isDefault:
            return None 
        else:
            return property.value
    
        
    def value_to_apply(self, prop_name, instrument, idf_name=""):
        property = self.getProperty(prop_name)
        if property.isDefault:
            return instrument.getNumberParameter(idf_name)[0] 
        else:
            return property.value
        
    def PyExec(self):
        
        in_ws = self.getProperty("InputWorkspace").value
        instrument = in_ws.getInstrument()
        
        first_ws = self.getProperty("FirstTransmissionRun").value
        
        '''
        Get all the inputs.
        '''
        output_workspace_name = self.getPropertyValue("OutputWorkspace")
        
        output_workspace_lam_name = self.getPropertyValue("OutputWorkspaceWavelength")
        
        analysis_mode = self.getPropertyValue("AnalysisMode")
        
        first_ws = self.value_or_none("FirstTransmissionRun")
        
        second_ws = self.value_or_none("SecondTransmissionRun")
        
        start_overlap = self.value_or_none("StartOverlap")
        
        end_overlap = self.value_or_none("EndOverlap")
        
        params = self.value_or_none("Params")
        
        i0_monitor_index = int(self.value_to_apply("I0MonitorIndex", instrument, "I0MonitorIndex"))
        
        processing_commands = str()
        workspace_index_list = list()
        if self.getProperty("ProcessingInstructions").isDefault:
            if analysis_mode == "PointDetectorAnalysis":
                workspace_index_list.append( int(instrument.getNumberParameter("PointDetectorStart")[0]) )
                workspace_index_list.append( int(instrument.getNumberParameter("PointDetectorStop")[0]) )
            else:
                workspace_index_list.append( int(instrument.getNumberParameter("MultiDetectorStart")[0]) )
                workspace_index_list.append( in_ws.getNumberHistograms() - 1)
            processing_commands = ','.join(map(str, workspace_index_list))
        else:
            processing_commands = self.getProperty("ProcessingInstructions").value
        
        wavelength_min = self.value_to_apply("WavelengthMin", instrument, "LambdaMin")
        
        wavelength_max = self.value_to_apply("WavelengthMax", instrument, "LambdaMax")
        
        wavelength_step = self.value_or_none("WavelengthStep")
        
        wavelength_back_min = self.value_to_apply("MonitorBackgroundWavelengthMin", instrument, "MonitorBackgroundMin")
        
        wavelength_back_max = self.value_to_apply("MonitorBackgroundWavelengthMax", instrument, "MonitorBackgroundMax")
        
        wavelength_integration_min = self.value_to_apply("MonitorIntegrationWavelengthMin", instrument, "MonitorIntegralMin")
        
        wavelength_integration_max = self.value_to_apply("MonitorIntegrationWavelengthMax", instrument, "MonitorIntegralMax")
        
        detector_component_name = self.value_or_none("DetectorComponentName")
        
        sample_component_name = self.value_or_none("SampleComponentName")
        
        theta_in = self.value_or_none("ThetaIn")
        
        correct_positions = self.value_or_none("CorrectDetectorPositions")
        
        region_of_direct_beam = self.value_or_none("RegionOfDirectBeam")
        
        strict_spectrum_checking = self.value_or_none('StrictSpectrumChecking')
        
        '''
        Pass the arguments and execute the main algorithm.
        '''
        new_IvsQ1, new_IvsLam1, thetaOut1 = ReflectometryReductionOne(
                                                                      InputWorkspace=in_ws, 
                                                                      AnalysisMode = analysis_mode, 
                                                                      FirstTransmissionRun=first_ws, 
                                                                      SecondTransmissionRun=second_ws, 
                                                                      OutputWorkspace=output_workspace_name, 
                                                                      OutputWorkspaceWavelength=output_workspace_lam_name,
                                                                      StartOverlap=start_overlap, 
                                                                      EndOverlap=end_overlap, 
                                                                      Params=params, 
                                                                      I0MonitorIndex=i0_monitor_index, 
                                                                      ProcessingInstructions=processing_commands, 
                                                                      WavelengthMin=wavelength_min, 
                                                                      WavelengthStep=wavelength_step, 
                                                                      WavelengthMax=wavelength_max,
                                                                      MonitorBackgroundWavelengthMin=wavelength_back_min, 
                                                                      MonitorBackgroundWavelengthMax=wavelength_back_max,
                                                                      MonitorIntegrationWavelengthMin=wavelength_integration_min, 
                                                                      MonitorIntegrationWavelengthMax=wavelength_integration_max,
                                                                      RegionOfDirectBeam=region_of_direct_beam, 
                                                                      DetectorComponentName=detector_component_name,
                                                                      SampleComponentName=sample_component_name, 
                                                                      ThetaIn=theta_in, 
                                                                      CorrectDetectorPositions=correct_positions,
                                                                      StrictSpectrumChecking=strict_spectrum_checking)
                                    
        self.setProperty("OutputWorkspace", new_IvsQ1)
        self.setProperty("OutputWorkspaceWavelength", new_IvsLam1)
        self.setProperty("ThetaOut", thetaOut1)
        
        
    
AlgorithmFactory.subscribe(ReflectometryReductionOneAuto)
