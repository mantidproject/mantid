"""*WIKI* 

Performs 1D stitching of Reflectometry 2D MDHistoWorkspaces.

"""
import mantid.simpleapi as api

from mantid.api import *
from mantid.kernel import *
import numpy as np
import math


class Stitch1D(PythonAlgorithm):

    def __get_first_non_integrated_dimension(self, ws):
        for i in range(0, ws.getNumDims()):
            dim = ws.getDimension(i)
            if not dim.getNBins() == 1:
                return dim
        raise RuntimeError("No non integrated dimension")
        
    def __get_first_integrated_dimension(self, ws):
        for i in range(0, ws.getNumDims()):
            dim = ws.getDimension(i)
            if dim.getNBins() == 1:
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
        
    def __trim_out_integrated_dimension(self, ws):
        dim = self.__get_first_non_integrated_dimension(ws)
        nbins = dim.getNBins()
        q_low = dim.getMinimum()
        q_high = dim.getMaximum()
        
        signals = np.empty(nbins)
        errors = np.empty(nbins)
        
        for index in range(0, nbins):
            signals[index] = ws.signalAt(index)
            errors[index] = ws.errorSquaredAt(index)
        errors = np.sqrt(errors)
        
        one_d_workspace = api.CreateMDHistoWorkspace(SignalInput=signals,ErrorInput=errors,Dimensionality=1,Extents=[q_low, q_high],NumberOfBins=[nbins],Names=[dim.getName()],Units=[dim.getUnits()])
        result = api.RenameWorkspace(InputWorkspace=one_d_workspace, OutputWorkspace=ws.name() + "_one_d")
        return result
        
    def __extract_overlap_as_workspace(self, ws, fraction_low, fraction_high):
        dim = self.__get_first_non_integrated_dimension(ws)
        nbins = dim.getNBins()
        bin_low = int(nbins * fraction_low)
        bin_high = int(nbins * fraction_high)
        step = ( dim.getMaximum() - dim.getMinimum() )/ nbins
        q_low = (bin_low * step) + dim.getMinimum()
        q_high = (bin_high * step) + dim.getMinimum()       
        
        bins_iterating_over = range(bin_low, bin_high)
        signals = np.empty(len(bins_iterating_over))
        errors = np.empty(len(bins_iterating_over))
        
        counter = 0
        for index in bins_iterating_over:
                signals[counter] = ws.signalAt(index)
                errors[counter] = ws.errorSquaredAt(index)
                counter += 1
        errors = np.sqrt(errors)
        overlapped = api.CreateMDHistoWorkspace(SignalInput=signals,ErrorInput=errors,Dimensionality=1,Extents=[q_low, q_high],NumberOfBins=[len(bins_iterating_over)],Names=[dim.getName()],Units=[dim.getUnits()])
        result = api.RenameWorkspace(InputWorkspace=overlapped, OutputWorkspace=ws.name() + "_overlapped")
        return result
    
    def __overlay_overlap(self, sum, overlap):
        target_dim = sum.getDimension(0)
        target_q_max = target_dim.getMaximum()
        target_q_min = target_dim.getMinimum()
        target_nbins = target_dim.getNBins()
        target_step = target_nbins / (target_q_max - target_q_min) 
        target_c = -1 * target_step * target_q_min
        
        overlap_dim = overlap.getDimension(0)
        overlap_q_max = overlap_dim.getMaximum()
        overlap_q_min = overlap_dim.getMinimum()
        overlap_nbins = overlap_dim.getNBins()
        overlap_step = (overlap_q_max - overlap_q_min) / overlap_nbins
        overlap_c = overlap_q_min
        for i in range(0, overlap_nbins):
            q = (overlap_step * i) + overlap_c
            target_index = int((target_step * q) + target_c)
            sum.setSignalAt(target_index, overlap.signalAt(i))
            sum.setErrorSquaredAt(target_index, overlap.errorSquaredAt(i))
	   
       
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
        self.declareProperty(name="OutScaleFactor", defaultValue=-2.0, direction = Direction.Output, doc="The actual used value for the scaling factor.");

    def PyExec(self):
    
        workspace1 = mtd[self.getPropertyValue("Workspace1")]
        workspace2 = mtd[self.getPropertyValue("Workspace2")]
        self.__check_individual_Workspace(workspace1)
        self.__check_individual_Workspace(workspace2)
        self.__check_both_Workspaces(workspace1, workspace2)
        start_overlap = float(self.getPropertyValue("StartOverlap"))
        end_overlap = float(self.getPropertyValue("EndOverlap"))     
        b_manual_scale_factor = self.getProperty("UseManualScaleFactor").value
        b_scale_workspace1 = self.getProperty("ScaleWorkspace1").value
  
        if start_overlap >= end_overlap:
            raise RuntimeError("StartOverlap must be < EndOverlap")
        
        ws1_flattened = self.__trim_out_integrated_dimension(workspace1)
        ws2_flattened = self.__trim_out_integrated_dimension(workspace2)
        
        ws1_overlap = self.__integrate_over(ws1_flattened, start_overlap, end_overlap)
        ws2_overlap = self.__integrate_over(ws2_flattened, start_overlap, end_overlap)
        scale_factor = None
        scaled_workspace_1 = None
        scaled_workspace_2 = None
        if b_manual_scale_factor == True:
            scale_factor = self.getProperty("ManualScaleFactor").value
            if b_scale_workspace1 == True:
                scaled_workspace_1 = ws1_flattened * scale_factor
                scaled_workspace_2 = ws2_flattened * 1
            else:
                scaled_workspace_1 = ws1_flattened * 1
                scaled_workspace_2 = ws2_flattened * scale_factor
        else:
            if b_scale_workspace1 == True:
                scale_factor = (ws2_overlap / ws1_overlap)
            else:
                scale_factor = (ws1_overlap / ws2_overlap)
            scaled_workspace_1 = ws1_flattened * 1
            scaled_workspace_2 = ws2_flattened * 1
        self.setProperty("OutScaleFactor", scale_factor)
        
        #use the start and end positions to 'sum' over the appropriate region in the input workspaces
        workspace1_overlap = self.__extract_overlap_as_workspace(scaled_workspace_1, start_overlap, end_overlap)
        workspace2_overlap = self.__extract_overlap_as_workspace(scaled_workspace_2, start_overlap, end_overlap)
        weighted_mean_overlap = api.WeightedMeanMD(LHSWorkspace=workspace1_overlap,RHSWorkspace=workspace2_overlap)
        
        mtd.remove('sum')
        sum = ws1_flattened + ws2_flattened
        self.__overlay_overlap(sum, weighted_mean_overlap)
        self.setProperty("OutputWorkspace", sum)
        
        #Clean up
        mtd.remove(ws1_flattened.name())
        mtd.remove(ws2_flattened.name())
        mtd.remove(workspace1_overlap.name())
        mtd.remove(workspace2_overlap.name())
        mtd.remove(weighted_mean_overlap.name())
        mtd.remove(sum.name())
        return None

registerAlgorithm(Stitch1D())