import sys
import mantid.simpleapi
from mantid.api import *
from mantid.kernel import *

class CreateTransmissionWorkspaceAuto(PythonAlgorithm):
    
    def category(self):
        return "Reflectometry\\ISIS"

    def name(self):
        return "CreateTransmissionWorkspaceAuto"
    
    def PyInit(self):
        
        
        input_validator = WorkspaceUnitValidator("TOF")
        self.declareProperty(MatrixWorkspaceProperty("FirstTransmissionRun", "", Direction.Input, validator=input_validator), "Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("SecondTransmissionRun", "", optional=PropertyMode.Optional,  validator=input_validator, direction=Direction.Input), "Second transmission run workspace in TOF")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output transmission workspace in wavelength")

        self.declareProperty(FloatArrayProperty(name="Params", values=[sys.maxint]), doc="Rebinning Parameters. See Rebin for format.")
        self.declareProperty(name="StartOverlap", defaultValue=sys.maxint, doc="Overlap in Q.")
        self.declareProperty(name="EndOverlap", defaultValue=sys.maxint, doc="End overlap in Q.")
        index_bounds = IntBoundedValidator()
        index_bounds.setLower(0)
        self.declareProperty(name="I0MonitorIndex", defaultValue=sys.maxint, validator=index_bounds, doc="I0 monitor index" )
        self.declareProperty(IntArrayProperty(name="WorkspaceIndexList", values=[sys.maxint]), doc="Workspace index list")
        self.declareProperty(name="WavelengthMin", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Wavelength Min in angstroms")
        self.declareProperty(name="WavelengthMax", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Wavelength Max in angstroms")
        self.declareProperty(name="WavelengthStep", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Wavelength step in angstroms")
        self.declareProperty(name="MonitorBackgroundWavelengthMin", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor wavelength background min in angstroms")
        self.declareProperty(name="MonitorBackgroundWavelengthMax", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor wavelength background max in angstroms")
        self.declareProperty(name="MonitorIntegrationWavelengthMin", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor integral min in angstroms")
        self.declareProperty(name="MonitorIntegrationWavelengthMax", direction=Direction.Input, defaultValue=sys.float_info.max, doc="Monitor integral max in angstroms")
        

    def PyExec(self):
        firstws = self.getProperty("FirstTransmissionRun").value
        secondws = self.getProperty("SecondTransmissionRun").value
        start_overlap = self.getProperty("StartOverlap").value
        end_overlap = self.getProperty("EndOverlap").value
        params = self.getProperty("Params").value
        
        self.setProperty("OutputWorkspace", firstws)
    
AlgorithmFactory.subscribe(CreateTransmissionWorkspaceAuto)
