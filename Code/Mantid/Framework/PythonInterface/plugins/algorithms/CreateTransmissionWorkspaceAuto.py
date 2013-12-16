import mantid.simpleapi
from mantid.api import *
from mantid.kernel import *

class CreateTransmissionWorkspaceAuto(PythonAlgorithm):
    
    def category(self):
        return "Reflectometry\\ISIS"

    def name(self):
        return "CreateTransmissionWorkspaceAuto"
    
    def PyInit(self):
        
        '''
        
        declareProperty(
          new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>(true)),
          "A comma separated list of first bin boundary, width, last bin boundary. "
              "These parameters are used for stitching together transmission runs. "
              "Values are in wavelength (angstroms). This input is only needed if a SecondTransmission run is provided.");

      declareProperty(
          new PropertyWithValue<double>("StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
          "Start wavelength for stitching transmission runs together");

      declareProperty(
          new PropertyWithValue<double>("EndOverlap", Mantid::EMPTY_DBL(), Direction::Input),
          "End wavelength (angstroms) for stitching transmission runs together");
        
        
      this->initStitchingInputs();
      this->initIndexInputs();
      this->initWavelengthInputs();

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
          "Output Workspace IvsQ.");

        '''
        
        
        input_validator = WorkspaceUnitValidator("TOF")
        self.declareProperty(MatrixWorkspaceProperty("FirstTransmissionRun", "", Direction.Input, validator=input_validator), "Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("SecondTransmissionRun", "", optional=PropertyMode.Optional,  validator=input_validator, direction=Direction.Input), "Second transmission run workspace in TOF")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output transmission workspace in wavelength")
        self.declareProperty(FloatArrayProperty(name="Params", values=[0.1]), doc="Rebinning Parameters. See Rebin for format.")
        
        self.declareProperty(name="StartOverlap", defaultValue=-1.0, doc="Overlap in Q.")
        self.declareProperty(name="EndOverlap", defaultValue=-1.0, doc="End overlap in Q.")
        
        
        

    def PyExec(self):
        firstws = self.getProperty("FirstTransmissionRun").value
        secondws = self.getProperty("SecondTransmissionRun").value
        start_overlap = self.getProperty("StartOverlap").value
        end_overlap = self.getProperty("EndOverlap").value
        params = self.getProperty("Params").value
        
        self.setProperty("OutputWorkspace", firstws)
    
AlgorithmFactory.subscribe(CreateTransmissionWorkspaceAuto)
