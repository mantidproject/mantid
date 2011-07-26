from MantidFramework import *
from mantidsimple import *

class NormaliseByThickness(PythonAlgorithm):
    """
        Normalise detector counts by the sample thickness
    """
    
    def category(self):
        return "Workflow\\SANS"

    def name(self):
        return "NormaliseByThickness"

    def PyInit(self):
        # Input workspace
        self.declareProperty("InputWorkspace", "", Direction=Direction.Input, Description="Workspace to be normalised")
        self.declareProperty("OutputWorkspace", "", Direction=Direction.Input, Description="Name of the workspace that will contain the transmission histogram")
        self.declareProperty("SampleThickness", 0.0, Direction=Direction.Input, Description="Optional sample thickness value. If not provided the sample-thickness run property will be used.")
        self.declareProperty("OutputMessage", "", Direction=Direction.Output)

    def PyExec(self):
        input_ws_name = self.getProperty("InputWorkspace")
        if mtd[input_ws_name].getAxis(0).getUnit().name() != "Wavelength":
            raise RuntimeError, "NormaliseByThickness expects an input workspace with units of Wavelength"        
        
        # Determine whether we should use the input thickess or try to read it from the run properties
        thickness = self.getProperty("SampleThickness")
        if thickness <= 0:
            if mtd[input_ws_name].getRun().hasProperty("sample-thickness"):
                thickness = mtd[input_ws_name].getRun().getProperty("sample-thickness").value
                if thickness <= 0:
                    raise RuntimeError, "NormaliseByThickness could not get the sample thickness"
            else:
                raise RuntimeError, "NormaliseByThickness could not get the sample thickness"

        output_ws_name = self.getProperty("OutputWorkspace")                
        self.executeSubAlg(Scale, input_ws_name, OutputWorkspace=output_ws_name, Factor=1.0/thickness, Operation="Multiply")
        
        self.setProperty("OutputMessage", "Normalised by thickness [%g cm]" % thickness)

mtd.registerPyAlgorithm(NormaliseByThickness())
