"""*WIKI* 

Stitches single histogram [[MatrixWorkspace|Matrix Workspaces]] together outputing a stitched Matrix Workspace. This algorithm is a wrapper over [[Stitch1DMD]].

*WIKI*"""
from mantid.simpleapi import *

from mantid.api import *
from mantid.kernel import *
import numpy as np

class Stitch1D(PythonAlgorithm):

    def category(self):
        return "Reflectometry\\ISIS;PythonAlgorithms"

    def name(self):
	    return "Stitch1D"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspaces", "", Direction.Input), "Input workspaces")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output stitched workspace")
        
        overlap_validator = FloatMandatoryValidator() 

        self.declareProperty(name="StartOverlaps", defaultValue=-1.0, validator=overlap_validator, doc="Overlap in Q.")
        self.declareProperty(name="EndOverlaps", defaultValue=-1.0, validator=overlap_validator, doc="End overlap in Q.")
        self.declareProperty(name="Params", defaultValue="0.1", doc="Rebinning Parameters. See Rebin for format.")
        self.declareProperty(name="ScaleRHSWorkspace", defaultValue=True, doc="Scaling either with respect to workspace 1 or workspace 2.")
        self.declareProperty(name="UseManualScaleFactor", defaultValue=False, doc="True to use a provided value for the scale factor.")
        self.declareProperty(name="ManualScaleFactor", defaultValue=1.0, doc="Provided value for the scale factor.")
        self.declareProperty(name="OutScaleFactor", defaultValue=-2.0, direction = Direction.Output, doc="The actual used value for the scaling factor.")
    
    def has_non_zero_errors(self, ws):
        errors = ws.extractE()
        count = len(errors.nonzero()[0])
        return count > 0
            
    def PyExec(self):
    
        inputWorkspaces = self.getProperty("InputWorkspaces")
        # Just forward the other properties on.
        startOverlaps = self.getProperty('StartOverlaps').value
        endOverlaps = self.getProperty('EndOverlaps').value
        scaleRHSWorkspace = self.getProperty('ScaleRHSWorkspace').value
        useManualScaleFactor = self.getProperty('UseManualScaleFactor').value
        manualScaleFactor = self.getProperty('ManualScaleFactor').value
        outScaleFactor = self.getProperty('OutScaleFactor').value
        params = self.getProperty("Params").value
        
        numberOfWorkspaces = len(inputWorkspaces)
        inputWorkspaces = inputWorkspaces.split(',')
        startOverlaps = startOverlaps.split(',')
        endOverlaps = endOverlaps.split(',')
        if not numberOfWorkspaces > 1:
            raise ValueError("Too few workspaces to stitch")
        if not (len(startOverlaps) == len(endOverlaps)):
            raise ValueError("StartOverlaps and EndOverlaps are different lengths")
        if not (len(startOverlaps) == (numberOfWorkspaces- 1)):
            raise ValueError("Wrong number of StartOverlaps, should be %i" % (numberOfWorkspaces - 1))
        
        self.setProperty('OutputWorkspace', result)
        self.setProperty('OutScaleFactor', scalefactor)
        
        return None
        

#############################################################################################

AlgorithmFactory.subscribe(Stitch1D())
