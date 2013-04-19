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
        
        overlap_validator = CompositeValidator()
        overlap_validator.add(FloatBoundedValidator(lower=0.0, upper=1.0))
        overlap_validator.add(FloatMandatoryValidator())    

        self.declareProperty(name="StartOverlap", defaultValue=0.0, validator=overlap_validator, doc="Fraction along axis to start overlap. 0 to 1.");
        self.declareProperty(name="EndOverlap", defaultValue=0.1, validator=overlap_validator, doc="Fraction along axis to end overlap. 0 to 1.");
        self.declareProperty(name="ScaleRHSWorkspace", defaultValue=True, doc="Scaling either with respect to workspace 1 or workspace 2.")
        self.declareProperty(name="UseManualScaleFactor", defaultValue=False, doc="True to use a provided value for the scale factor.")
        self.declareProperty(name="ManualScaleFactor", defaultValue=1.0, doc="Provided value for the scale factor.")
        self.declareProperty(name="OutScaleFactor", defaultValue=-2.0, direction = Direction.Output, doc="The actual used value for the scaling factor.")
    
    def convert_to_mdworkspace(self, ws,name):
        x = ws.readX(0)
        axis = ws.getAxis(0)
        unit =  axis.getUnit()
        mdws = api.CreateMDHistoWorkspace(Dimensionality=1,SignalInput=ws.readY(0), ErrorInput=ws.readE(0), Extents=[x[0],x[len(x)-1]], NumberOfBins=[len(x)], Names=unit.caption(), Units=unit.label(), OutputWorkspace=name)    
        return mdws
        
    def __get_first_non_integrated_dimension(self, ws):
        for i in range(0, ws.getNumDims()):
            dim = ws.getDimension(i)
            if not dim.getNBins() == 1:
                return dim
        raise RuntimeError("No non integrated dimension")
        
    def PyExec(self):
        # Translate the workspace inputs
        lhs_workspace = mtd[self.getPropertyValue("LHSWorkspace")]
        rhs_workspace = mtd[self.getPropertyValue("RHSWorkspace")]
        md_lhs_workspace = self.convert_to_mdworkspace(lhs_workspace, "md_lhs_workspace")
        md_rhs_workspace = self.convert_to_mdworkspace(rhs_workspace, "md_rhs_workspace")
        # Just forward the other properties on.
        StartOverlap = self.getProperty('StartOverlap').value
        EndOverlap = self.getProperty('EndOverlap').value
        ScaleRHSWorkspace = self.getProperty('ScaleRHSWorkspace').value
        UseManualScaleFactor = self.getProperty('UseManualScaleFactor').value
        ManualScaleFactor = self.getProperty('ManualScaleFactor').value
        OutScaleFactor = self.getProperty('OutScaleFactor').value
        # Run the MDVersion of the algorithm
        out_properties = api.Stitch1DMD( OutputWorkspace='out_md_workspace',
                    LHSWorkspace=md_lhs_workspace, 
                    RHSWorkspace=md_rhs_workspace, 
                    StartOverlap=StartOverlap, 
                    EndOverlap=EndOverlap, 
                    ScaleRHSWorkspace=ScaleRHSWorkspace, 
                    UseManualScaleFactor=UseManualScaleFactor, 
                    ManualScaleFactor=ManualScaleFactor,
                    OutScaleFactor=OutScaleFactor)
        
       
        outMDWS = mtd['out_md_workspace']
            
        # Create a Workspace2D from the MDHistoWorkspace output
        y = outMDWS.getSignalArray()
        e = np.sqrt(outMDWS.getErrorSquaredArray())
        dim = self.__get_first_non_integrated_dimension(outMDWS)
        nbins = dim.getNBins()
        q_low = dim.getMinimum()
        q_high = dim.getMaximum()
        step = (q_high - q_low)/nbins
        x = np.arange(q_low, q_high, step) 
        
        factory = api.WorkspaceFactoryImpl.Instance()
        outws = factory.create("Workspace2D", 1, len(x), len(y))
        outws.setX(0, x)
        outws.setY(0, y)
        outws.setE(0, e)
        outws.getAxis(0).setUnit("MomentumTransfer")
        
        api.DeleteWorkspace(Workspace=md_lhs_workspace)
        api.DeleteWorkspace(Workspace=md_rhs_workspace)
        api.DeleteWorkspace(Workspace=outMDWS)
        
        self.setProperty('OutputWorkspace', outws)
        self.setProperty('OutScaleFactor', out_properties[1])
        return None
        

#############################################################################################

AlgorithmFactory.subscribe(Stitch1D())
