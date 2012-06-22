"""*WIKI* 

Performs 1D stitching of Reflectometry 2D MDHistoWorkspaces.

"""

from mantid.api import *
from mantid.kernel import *
import numpy as np


class Stitch1D(PythonAlgorithm):

    def __get_first_non_integrated_dimension(self, ws):
        for i in range(0, ws.getNumDims()):
            dim = ws.getDimension(i)
            if not dim.getNBins() == 1:
                return dim
        raise RuntimeError("No integrated dimension")

    def __check_individual_Workspace(self, ws):
        if not ws.getNumDims() == 2:
            raise RuntimeError( ws.name() + " must have 2 dimensions" )
        dim1 = ws.getDimension(0)
        dim2 = ws.getDimension(1)
        if not bool(dim1.getNBins() == 1) ^ bool(dim2.getNBins() == 1):
            raise RuntimeError(ws.name() + " must have one integrated and one unintegrated dimension")
        return None
    
    def __check_both_Workspaces(self, ws1, ws2):
        for i in range(0, 2):
            ws_1_dim = ws1.getDimension(i)
            ws_2_dim = ws2.getDimension(i)
            if not ws_1_dim.getNBins() == ws_2_dim.getNBins():
                raise RuntimeError(ws1.name() + " and " + ws2.name() + " do not have the same number of bins.")
            if not ws_1_dim.getName() == ws_2_dim.getName():
                raise RuntimeError("Dimension names do not match up.")
        ws1_integrated_dim = self.__get_first_non_integrated_dimension(ws1)
        ws2_integrated_dim = self.__get_first_non_integrated_dimension(ws2)
        if not ws1_integrated_dim.getMaximum() == ws2_integrated_dim.getMaximum():
            raise RuntimeError("Max values in the two non-integrated dimensions of the combining workspaces are not equal")
        if not ws1_integrated_dim.getMinimum() == ws2_integrated_dim.getMinimum(): 
            raise RuntimeError("Min values in the two non-integrated dimensions of the combining workspaces are not equal")
            
    def __integrate_over(self, ws, fraction_low, fraction_high):
        dim = self.__get_first_non_integrated_dimension(ws)
        nbins = dim.getNBins()
        # Find the corresponding bin indexes. Truncate to get the exact index.
        bin_low = int(nbins * fraction_low)
        bin_high = int(nbins * fraction_high)
        sum_signal = 0.0
        for index in range(bin_low, bin_high):
            sum_signal += ws.signalAt(index)
        return sum_signal

    def category(self):
        return "PythonAlgorithms"
    
    def name(self):
        return "Stitch1D"
    
    def PyInit(self):
        self.declareProperty(IMDHistoWorkspaceProperty("Workspace1", "", Direction.Input), "Input MD Histo Workspace")
        self.declareProperty(IMDHistoWorkspaceProperty("Workspace2", "", Direction.Input), "Input MD Histo Workspace")
        self.declareProperty(IMDHistoWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Stitched Ouput Workspace")
        overlap_validator = CompositeValidator();
        overlap_validator.add(FloatBoundedValidator(lower=0.0, upper=1.0))
        overlap_validator.add(FloatMandatoryValidator())    
        
        self.declareProperty(name="StartOverlap", defaultValue=0.0, validator=overlap_validator, doc="Fraction along axis to start overlap. 0 to 1.");
        self.declareProperty(name="EndOverlap", defaultValue=0.1, validator=overlap_validator, doc="Fraction along axis to end overlap. 0 to 1.");
        self.declareProperty(name="ExpectGroupWorkspaces", defaultValue=False, doc="True if the input workspaces expected to be group workspaces.")
        self.declareProperty(name="GroupWorkspaceIndex", defaultValue=0, doc="Index of the workspace in the group workspaces")
        self.declareProperty(name="ScaleWorkspace1", defaultValue=True, doc="Scaling either with respect to workspace 1 or workspace 2.")
        self.declareProperty(name="UseManualScaleFactor", defaultValue=False, doc="True to use a provided value for the scale factor.")
        self.declareProperty(name="ManualScaleFactor", defaultValue=1.0, doc="Provided value for the scale factor.")
        self.declareProperty(name="AppliedScaleFactor", defaultValue=-1.0, direction = Direction.Output, doc="The actual used value for the scaling factor.");
        

    
    def PyExec(self):
        from mantid.simpleapi import MultiplyMD, DivideMD
        workspace1 = mtd[self.getPropertyValue("Workspace1")]
        workspace2 = mtd[self.getPropertyValue("Workspace2")]
        self.__check_individual_Workspace(workspace1)
        self.__check_individual_Workspace(workspace2)
        self.__check_both_Workspaces(workspace1, workspace2)
        start_overlap = float(self.getPropertyValue("StartOverlap"))
        end_overlap = float(self.getPropertyValue("EndOverlap"))     
        
        b_scale_workspace1 = bool(self.getProperty("ScaleWorkspace1"))
  
        if start_overlap >= end_overlap:
            raise RuntimeError("StartOverlap must be < EndOverlap")
        
        ws1_overlap = self.__integrate_over(workspace1, start_overlap, end_overlap)
        ws2_overlap = self.__integrate_over(workspace2, start_overlap, end_overlap)
        scale_factor = None
        if b_scale_workspace1 == True:
            scale_factor = (ws2_overlap / ws1_overlap)
            x = workspace2 * scale_factor
        else:
            scale_factor = (ws1_overlap / ws2_overlap)
            x = workspace1 * scale_factor
        self.setProperty("AppliedScaleFactor", scale_factor)
        #use the start and end positions to 'sum' over the appopriate region in the input workspaces
        
        self.setProperty("OutputWorkspace", workspace1)

registerAlgorithm(Stitch1D())