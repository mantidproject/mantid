"""*WIKI* 
Subtract background from an I(Q) distribution.
*WIKI*"""

from mantid.api import *
from mantid.kernel import Direction, FloatBoundedValidator 
import mantid.simpleapi 
import os

class SANSSubtract(PythonAlgorithm):
    """ 
        I(Q) subtraction
    """
    def category(self):
        """ 
            Return category
        """
        return "PythonAlgorithms;SANS"
    
    def name(self):
        """ 
            Return name
        """
        return "SANSSubtract"
    
    def PyInit(self):
        """ 
            Declare properties
        """
        self.declareProperty('DataDistribution', '', direction = Direction.Input, 
                             doc='Name of the input workspace or file path')
        self.declareProperty('Background', '', direction = Direction.Input, 
                             doc='Name of the background workspace or file path')
        self.declareProperty("ScaleFactor", 1., FloatBoundedValidator(),
                             doc="Scaling factor [Default: 1]")
        self.declareProperty("Constant", 0., FloatBoundedValidator(),
                             doc="Additive constant [Default:0]")
        self.declareProperty(FileProperty("OutputDirectory","", FileAction.OptionalDirectory), 
                             doc="Directory to write the output files in")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             doc="Workspace containing data from detectors")
        return
    
    def PyExec(self):
        """ 
            Main execution body
        """
        data_str = self.getProperty("DataDistribution").value
        background_str = self.getProperty("Background").value
        scale = self.getProperty("ScaleFactor").value
        constant = self.getProperty("Constant").value
        output_dir = self.getPropertyValue("OutputDirectory")
        
        # Load data or get it from the ADS
        if AnalysisDataService.doesExist(data_str):
            data = AnalysisDataService.retrieve(data_str)
            data_ws_name = data_str
        else:    
            data_ws_name = os.path.basename(data_str)
            load_alg = mantid.api.AlgorithmManager.createUnmanaged('Load')        
            load_alg.setChild(True)
            load_alg.initialize()
            load_alg.setProperty("Filename", data_str)
            load_alg.setProperty("OutputWorkspace", data_ws_name)
            load_alg.execute()
            data = load_alg.getProperty("OutputWorkspace").value
            
        # Load background or get it from the ADS
        if AnalysisDataService.doesExist(background_str):
            background = AnalysisDataService.retrieve(background_str)
            back_ws_name = background_str
        else:    
            load_alg = mantid.api.AlgorithmManager.createUnmanaged('Load')        
            load_alg.setChild(True)
            load_alg.initialize()
            load_alg.setProperty("Filename", background_str)
            load_alg.setProperty("OutputWorkspace", os.path.basename(background_str))
            load_alg.execute()
            background = load_alg.getProperty("OutputWorkspace").value
                     
        # Keep track of dQ
        dq = data.readDx(0)
        
        # Output = data - scale * background + constant
        op = mantid.api.AlgorithmManager.createUnmanaged('Scale') 
        op.initialize()
        op.setChild(True)
        op.setProperty("InputWorkspace", background)
        op.setProperty("OutputWorkspace", '__scaled_bck')
        op.setProperty("Factor", scale)
        op.setProperty("Operation", "Multiply")
        op.execute()
        scaled_bck = op.getProperty("OutputWorkspace").value
        
        op = mantid.api.AlgorithmManager.createUnmanaged('Minus') 
        op.initialize()
        op.setChild(True)
        op.setProperty("LHSWorkspace", data)
        op.setProperty("RHSWorkspace", scaled_bck)
        op.setProperty("OutputWorkspace", '__bck_substracted')
        op.execute()
        bck_subtr = op.getProperty("OutputWorkspace").value
        
        op = mantid.api.AlgorithmManager.createUnmanaged('Scale') 
        op.initialize()
        op.setChild(True)
        op.setProperty("InputWorkspace", bck_subtr)
        op.setProperty("OutputWorkspace", '__corrected_output')
        op.setProperty("Factor", constant)
        op.setProperty("Operation", "Add")
        op.execute()
        output = op.getProperty("OutputWorkspace").value
        
        # Put back dQ
        dq_scaled = output.dataDx(0)
        for i in range(len(dq)):
            dq_scaled[i] = dq[i]
        
        self.setProperty("OutputWorkspace", output)
        
        # Save the output to disk as needed
        if len(output_dir)>0:
            root_name, ext = os.path.splitext(data_ws_name)
            op = mantid.api.AlgorithmManager.createUnmanaged('SaveCanSAS1D') 
            op.initialize()
            op.setChild(True)
            op.setProperty("InputWorkspace", output)
            op.setProperty("Filename", os.path.join(output_dir, root_name+'_corr.xml'))
            op.setProperty("RadiationSource", "Spallation Neutron Source")
            op.execute()
            
            op = mantid.api.AlgorithmManager.createUnmanaged("SaveAscii")
            op.initialize()
            op.setChild(True)
            op.setProperty("Filename", os.path.join(output_dir, root_name+'_corr.txt'))
            op.setProperty("InputWorkspace", output)
            op.setProperty("Separator", "Tab")
            op.setProperty("CommentIndicator", "# ")
            op.setProperty("WriteXError", True)
            op.execute()
        
        return 
    
AlgorithmFactory.subscribe(SANSSubtract)
