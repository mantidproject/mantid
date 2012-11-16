"""*WIKI* 
    Normalise the data to timer or a spectrum, typically a monitor, 
    with in the workspace. By default the Normalisation is done with 
    respect to the Instrument's incident monitor
*WIKI*"""
from mantid.api import *
from mantid.kernel import *

class HFIRSANSNormalise(PythonAlgorithm):

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", 
                                                     direction = Direction.Input))
        
        self.declareProperty("NormalisationType", "Monitor", 
                             StringListValidator(["Monitor", "Timer"]), 
                             doc="Type of Normalisation to use")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", 
                                                     direction = Direction.Output))

        self.declareProperty("OutputMessage", "", direction=Direction.Output)
        
    def PyExec(self):
        input_ws = self.getPropertyValue("InputWorkspace")
        output_ws = self.getPropertyValue("OutputWorkspace")
        Normalisation = self.getPropertyValue("NormalisationType")
        
        # Get the monitor or timer
        ws = AnalysisDataService.retrieve(input_ws)
        norm_count = ws.getRun().getProperty(Normalisation.lower()).value
        print norm_count
        
        if Normalisation=="Monitor":
            factor = 1.0e8/norm_count
        else:
            factor = 1.0/norm_count
    
        alg = AlgorithmManager.create("Scale")
        alg.initialize()
        alg.setPropertyValue("InputWorkspace", input_ws)
        alg.setPropertyValue("OutputWorkspace", output_ws) 
        alg.setProperty("Factor", factor)
        alg.setProperty("Operation", "Multiply")
        
        self.setPropertyValue("OutputWorkspace", output_ws)
        self.setProperty("OutputMessage", 
                         "Normalisation by %s: %6.2g" % (Normalisation, norm_count))

#############################################################################################

registerAlgorithm(HFIRSANSNormalise)
