"""*WIKI* 

Stitches single histogram [[MatrixWorkspace|Matrix Workspaces]] together outputting a stitched Matrix Workspace. Either the right-hand-side or left-hand-side workspace can be chosen to be scaled. Users
must provide a Param step (single value), but the binning start and end are calculated from the input workspaces if not provided. Likewise, StartOverlap and EndOverlap are optional. If the StartOverlap or EndOverlap
are not provided, then these are taken to be the region of x-axis intersection.

The workspaces must be histogrammed. Use [[ConvertToHistogram]] on workspaces prior to passing them to this algorithm.
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
        
        histogram_validator = HistogramValidator()
        
        self.declareProperty(MatrixWorkspaceProperty("LHSWorkspace", "", Direction.Input, validator=histogram_validator), "Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("RHSWorkspace", "", Direction.Input, validator=histogram_validator), "Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output stitched workspace")
        
        self.declareProperty(name="StartOverlap", defaultValue=-1.0, doc="Overlap x-value in units of x-axis. Optional.")
        self.declareProperty(name="EndOverlap", defaultValue=-1.0, doc="End overlap x-value in units of x-value. Optional.")
        self.declareProperty(FloatArrayProperty(name="Params", values=[0.1]), doc="Rebinning Parameters. See Rebin for format. If only a single value is provided, start and end are taken from input workspaces.")
        self.declareProperty(name="ScaleRHSWorkspace", defaultValue=True, doc="Scaling either with respect to workspace 1 or workspace 2.")
        self.declareProperty(name="UseManualScaleFactor", defaultValue=False, doc="True to use a provided value for the scale factor.")
        self.declareProperty(name="ManualScaleFactor", defaultValue=1.0, doc="Provided value for the scale factor.")
        self.declareProperty(name="OutScaleFactor", defaultValue=-2.0, direction = Direction.Output, doc="The actual used value for the scaling factor.")
    
    def has_non_zero_errors(self, ws):
        errors = ws.extractE()
        count = len(errors.nonzero()[0])
        return count > 0
    
    
    def __run_algorithm(self, name, **kwargs):
        """Run a named algorithm and return the
        algorithm handle
    
        Parameters:
            name - The name of the algorithm
            kwargs - A dictionary of property name:value pairs
        """
        alg = AlgorithmManager.createUnmanaged(name)
        alg.initialize()
        alg.setChild(True)
        
        if 'OutputWorkspace' in alg:
            alg.setPropertyValue("OutputWorkspace","UNUSED_NAME_FOR_CHILD")
        
        alg.setRethrows(True)
        for key, value in kwargs.iteritems():
            alg.setProperty(key, value)
        
        alg.execute()
        return alg
    
    def __to_single_value_ws(self, value):
        alg = self.__run_algorithm("CreateSingleValuedWorkspace", DataValue=value)
        ws = alg.getProperty("OutputWorkspace").value
        return ws
        
    
    def __find_indexes_start_end(self, startOverlap, endOverlap, workspace):
        a1=workspace.binIndexOf(startOverlap)
        a2=workspace.binIndexOf(endOverlap)
        if a1 == a2:
            raise RuntimeError("The Params you have provided for binning yield a workspace in which start and end overlap appear in the same bin. Make binning finer via input Params.")
        return a1, a2
    
    def __calculate_x_intersection(self):
        lhs_ws = self.getProperty("LHSWorkspace").value
        rhs_ws = self.getProperty("RHSWorkspace").value
        lhs_x = lhs_ws.readX(0)
        rhs_x = rhs_ws.readX(0)
        return rhs_x[0], lhs_x[-1]
    
    def __get_start_overlap(self, range_tolerance):
        start_overlap_property = self.getProperty('StartOverlap')
        start_overlap = start_overlap_property.value - range_tolerance
        if start_overlap_property.isDefault:
            min, max = self.__calculate_x_intersection()
            start_overlap = min
            logger.information("StartOverlap calculated to be: %0.4f" % start_overlap)
        return start_overlap
        
    def __get_end_overlap(self, range_tolerance):
        end_overlap_property = self.getProperty('EndOverlap')
        end_overlap = end_overlap_property.value + range_tolerance
        if end_overlap_property.isDefault:
            min, max = self.__calculate_x_intersection()
            end_overlap = max
            logger.information("EndOverlap calculated to be: %0.4f" % end_overlap)
        return end_overlap
    
    '''
    Fetch and create rebin parameters.
    If a single step is provided, then the min and max values are taken from the input workspaces.
    '''
    def __create_rebin_parameters(self):
        params = None
        user_params = self.getProperty("Params").value
        if user_params.size >= 3:
            params = user_params
        else:
            lhs_ws = self.getProperty("LHSWorkspace").value
            rhs_ws = self.getProperty("RHSWorkspace").value
            params = list()
            params.append(np.min(lhs_ws.readX(0)))
            params.append(user_params[0])
            params.append(np.max(rhs_ws.readX(0)))
        return params
            
    def PyExec(self):
        # Just forward the other properties on.
        range_tolerance = 1e-9
        
        startOverlap = self.__get_start_overlap(range_tolerance)
        endOverlap = self.__get_end_overlap(range_tolerance)
        scaleRHSWorkspace = self.getProperty('ScaleRHSWorkspace').value
        useManualScaleFactor = self.getProperty('UseManualScaleFactor').value
        manualScaleFactor = self.getProperty('ManualScaleFactor').value
        outScaleFactor = self.getProperty('OutScaleFactor').value
        
        params = self.__create_rebin_parameters()
        print params
        alg = self.__run_algorithm("Rebin", InputWorkspace=self.getProperty("LHSWorkspace").value, Params=params)
        lhs_rebinned = alg.getProperty("OutputWorkspace").value
        alg = self.__run_algorithm("Rebin", InputWorkspace=self.getProperty("RHSWorkspace").value, Params=params)
        rhs_rebinned = alg.getProperty("OutputWorkspace").value
        
        xRange = lhs_rebinned.readX(0)
        minX = xRange[0]
        maxX = xRange[-1]
        if(round(startOverlap, 9) < round(minX, 9)):
            raise RuntimeError("Stitch1D StartOverlap is outside the X range after rebinning. StartOverlap: %0.9f, X min: %0.9f" % (startOverlap, minX))
        if(round(endOverlap, 9) > round(maxX, 9)):
            raise RuntimeError("Stitch1D EndOverlap is outside the X range after rebinning. EndOverlap: %0.9f, X max: %0.9f" % (endOverlap, maxX))
        
        if(startOverlap > endOverlap):
            raise RuntimeError("Stitch1D cannot have a StartOverlap > EndOverlap")
    
        a1, a2 = self.__find_indexes_start_end(startOverlap, endOverlap, lhs_rebinned)
        
        if not useManualScaleFactor:
            
            alg = self.__run_algorithm("Integration", InputWorkspace=lhs_rebinned, RangeLower=startOverlap, RangeUpper=endOverlap)
            lhsOverlapIntegrated = alg.getProperty("OutputWorkspace").value
            alg = self.__run_algorithm("Integration", InputWorkspace=rhs_rebinned, RangeLower=startOverlap, RangeUpper=endOverlap)
            rhsOverlapIntegrated = alg.getProperty("OutputWorkspace").value
            
            y1=lhsOverlapIntegrated.readY(0)
            y2=rhsOverlapIntegrated.readY(0)
            if scaleRHSWorkspace:
                alg = self.__run_algorithm("Divide", LHSWorkspace=lhsOverlapIntegrated, RHSWorkspace=rhsOverlapIntegrated)
                ratio = alg.getProperty("OutputWorkspace").value
                alg = self.__run_algorithm("Multiply", LHSWorkspace=rhs_rebinned, RHSWorkspace=ratio)
                rhs_rebinned = alg.getProperty("OutputWorkspace").value
                scalefactor = y1[0]/y2[0]
            else: 
                
                alg = self.__run_algorithm("Divide", RHSWorkspace=lhsOverlapIntegrated, LHSWorkspace=rhsOverlapIntegrated)
                ratio = alg.getProperty("OutputWorkspace").value
                alg = self.__run_algorithm("Multiply", LHSWorkspace=lhs_rebinned, RHSWorkspace=ratio)
                lhs_rebinned = alg.getProperty("OutputWorkspace").value
                scalefactor = y2[0]/y1[0]   
        else:
            manualScaleFactorWS = self.__to_single_value_ws(manualScaleFactor)
            if scaleRHSWorkspace:
                alg = self.__run_algorithm("Multiply", LHSWorkspace=rhs_rebinned, RHSWorkspace=manualScaleFactorWS)
                rhs_rebinned = alg.getProperty("OutputWorkspace").value
            else:
                alg = self.__run_algorithm("Multiply", LHSWorkspace=lhs_rebinned, RHSWorkspace=manualScaleFactorWS)
                lhs_rebinned = alg.getProperty("OutputWorkspace").value
            scalefactor = manualScaleFactor
        
        # Mask out everything BUT the overlap region as a new workspace.
        alg = self.__run_algorithm("MultiplyRange", InputWorkspace=lhs_rebinned, StartBin=0,EndBin=a1,Factor=0)
        overlap1 = alg.getProperty("OutputWorkspace").value
        alg = self.__run_algorithm("MultiplyRange", InputWorkspace=overlap1,StartBin=a2,Factor=0)
        overlap1 = alg.getProperty("OutputWorkspace").value
        # Mask out everything BUT the overlap region as a new workspace.
        alg = self.__run_algorithm("MultiplyRange", InputWorkspace=rhs_rebinned,StartBin=0,EndBin=a1,Factor=0)
        overlap2 = alg.getProperty("OutputWorkspace").value
        alg = self.__run_algorithm("MultiplyRange", InputWorkspace=overlap2,StartBin=a2,Factor=0)
        overlap2 = alg.getProperty("OutputWorkspace").value
    
        # Mask out everything AFTER the start of the overlap region
        alg = self.__run_algorithm("MultiplyRange", InputWorkspace=lhs_rebinned, StartBin=a1+1, Factor=0)
        lhs_rebinned = alg.getProperty("OutputWorkspace").value
        
        # Mask out everything BEFORE the end of the overlap region
        alg = self.__run_algorithm("MultiplyRange",InputWorkspace=rhs_rebinned,StartBin=0,EndBin=a2-1,Factor=0)
        rhs_rebinned = alg.getProperty("OutputWorkspace").value
        
        # Calculate a weighted mean for the overlap region
        overlapave = None
        if self.has_non_zero_errors(overlap1) and self.has_non_zero_errors(overlap2):
            alg = self.__run_algorithm("WeightedMean", InputWorkspace1=overlap1,InputWorkspace2=overlap2)
            overlapave = alg.getProperty("OutputWorkspace").value
        else:
            self.log().information("Using un-weighted mean for Stitch1D overlap mean")
            # Calculate the mean.
            alg = self.__run_algorithm("Plus", LHSWorkspace=overlap1, RHSWorkspace=overlap2)
            sum = alg.getProperty("OutputWorkspace").value
            denominator = self.__to_single_value_ws(2.0)
            alg = self.__run_algorithm("Divide", LHSWorkspace=sum, RHSWorkspace=denominator)
            
            overlapave = alg.getProperty("OutputWorkspace").value
            
        # Add the Three masked workspaces together to create a complete x-range
        alg = self.__run_algorithm("Plus", LHSWorkspace=lhs_rebinned, RHSWorkspace=overlapave)
        result = alg.getProperty("OutputWorkspace").value
        alg = self.__run_algorithm("Plus", LHSWorkspace=rhs_rebinned, RHSWorkspace=result)
        result = alg.getProperty("OutputWorkspace").value
        #RenameWorkspace(InputWorkspace=result, OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
        
        self.setProperty('OutputWorkspace', result)
        self.setProperty('OutScaleFactor', scalefactor)
        
        return None
        

#############################################################################################

AlgorithmFactory.subscribe(Stitch1D())
