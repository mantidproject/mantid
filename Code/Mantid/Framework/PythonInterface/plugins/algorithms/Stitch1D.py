"""*WIKI* 

Stitches single histogram [[MatrixWorkspace|Matrix Workspaces]] together outputing a stitched Matrix Workspace. This algorithm is a wrapper over [[Stitch1DMD]].

*WIKI*"""
import mantid.simpleapi as api

from mantid.api import *
from mantid.kernel import *
import numpy as np

class Stitch1D(PythonAlgorithm):

    def category(self):
        return "Reflectometry\\ISIS;PythonAlgorithms"

    def name(self):
	    return "Stitch1D"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("LHSWorkspace", "", Direction.Input), "Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("RHSWorkspace", "", Direction.Input), "Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output stitched workspace")
        
        overlap_validator = FloatMandatoryValidator() 

        self.declareProperty(name="StartOverlap", defaultValue=-1.0, validator=overlap_validator, doc="Overlap in Q.");
        self.declareProperty(name="EndOverlap", defaultValue=-1.0, validator=overlap_validator, doc="End overlap in Q.");
        self.declareProperty(name="Qmin", defaultValue=-1.0, doc="Minimum Q of output workspace. This is optional.")
        self.declareProperty(name="Qmax", defaultValue=-1.0, doc="Maximum Q of output workspace. This is optional.")
        self.declareProperty(name="ScaleRHSWorkspace", defaultValue=True, doc="Scaling either with respect to workspace 1 or workspace 2.")
        self.declareProperty(name="UseManualScaleFactor", defaultValue=False, doc="True to use a provided value for the scale factor.")
        self.declareProperty(name="ManualScaleFactor", defaultValue=1.0, doc="Provided value for the scale factor.")
        self.declareProperty(name="OutScaleFactor", defaultValue=-2.0, direction = Direction.Output, doc="The actual used value for the scaling factor.")
    
        
    def PyExec(self):
        # Translate the workspace inputs
        lhs_workspace = self.getProperty("LHSWorkspace").value
        rhs_workspace = self.getProperty("RHSWorkspace").value
        # Just forward the other properties on.
        startOverlap = self.getProperty('StartOverlap').value
        endOverlap = self.getProperty('EndOverlap').value
        scaleRHSWorkspace = self.getProperty('ScaleRHSWorkspace').value
        useManualScaleFactor = self.getProperty('UseManualScaleFactor').value
        manualScaleFactor = self.getProperty('ManualScaleFactor').value
        outScaleFactor = self.getProperty('OutScaleFactor').value
        qMin = self.getProperty('Qmin').value
        qMax = self.getProperty('Qmax').value
       
        factory = api.WorkspaceFactoryImpl.Instance()
        outws = factory.create("Workspace2D", NVectors=1, XLength=len(x), YLength=len(y))
        outws.setX(0, x)
        outws.setY(0, y)
        outws.setE(0, e)
        outws.getAxis(0).setUnit("MomentumTransfer")
        
        self.setProperty('OutputWorkspace', outws)
        self.setProperty('OutScaleFactor', out_properties[1])
        return None
        

#############################################################################################

AlgorithmFactory.subscribe(Stitch1D())
