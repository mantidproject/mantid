from MantidFramework import *

class ScaleX(PythonAlgorithm):

    def PyInit(self):
        self.declareWorkspaceProperty("InputWorkspace", "",
            Direction=Direction.Input,
            Description="Input workspace.")
        self.declareWorkspaceProperty("OutputWorkspace", "", 
            Direction=Direction.Output,
            Description="Output workspace.")
        self.declareProperty("Factor", 1.0,
            Validator=BoundedValidator(Lower=0.0),
            Description="Value to multiply the X data by.")
        
    def PyExec(self):
        inputWS = self.getProperty("InputWorkspace")
        factor = self.getProperty("Factor")
        
        # Create output workspace to be a copy of the input one
        nvectors = inputWS.getNumberHistograms()
        ysize = inputWS.getNumberBins()
        xsize = len(inputWS.dataX(0))
        outputWS = WorkspaceFactory.createMatrixWorkspaceFromCopy(inputWS)
                
        for spec in range(0, nvectors):
            for bin in range(0, ysize):
                outputWS.dataY(spec)[bin] = inputWS.dataY(spec)[bin]
                outputWS.dataE(spec)[bin] = inputWS.dataE(spec)[bin]            
            for bin in range(0, xsize):
                outputWS.dataX(spec)[bin] = factor * inputWS.dataX(spec)[bin]
        
        self.setProperty("OutputWorkspace", outputWS)

# Register the Python Algorithm with the Mantid Framework
mantid.registerPyAlgorithm(ScaleX())