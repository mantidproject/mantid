"""*WIKI* 

Workflow algorithm used to perform live reduction of EQSANS data.

EQSANSLiveSetup has to be run first.

*WIKI*"""

import os
from MantidFramework import *
from mantidsimple import *

class EQSANSLiveReduce(PythonAlgorithm):
    """
        Workflow algorithm used to perform live reduction of EQSANS data
    """
    
    def category(self):
        return "Workflow\\SANS;PythonAlgorithms"

    def name(self):
        return "EQSANSLiveReduce"

    def PyInit(self):
        self.declareWorkspaceProperty("InputWorkspace", "",
                                      Direction=Direction.Input,
                                      Description="Workspace to be reduced")
        self.declareWorkspaceProperty("OutputWorkspace", "",
                                      Direction=Direction.Output,
                                      Description="Output workspace containing the reduced data")
        self.declareProperty("ReductionProcess", True,
                             Description="If true, both the reduction and the post-processing will be run")
        self.declareProperty("PostProcess", False,
                             Description="If true, I(q) will be computed from the input workspace")
        self.declareFileProperty("LogDataFile", "",
                                 FileAction.OptionalLoad, [".nxs"],
                                 Description="For testing: optional file containing the sample logs")

    def PyExec(self):
        workspace = self.getPropertyValue("InputWorkspace")
        output_workspace = self.getPropertyValue("OutputWorkspace")
        reduction_process = self.getProperty("ReductionProcess")
        post_process = self.getProperty("PostProcess")
        
        # Reduce the data
        if reduction_process:
            self._reduction_process(workspace)

        # Post-processing        
        if post_process:
            self._post_process(workspace)
            RenameWorkspace(InputWorkspace=workspace+'_Iq', OutputWorkspace=output_workspace)
            self._setWorkspaceProperty("OutputWorkspace", mtd[output_workspace]._getHeldObject())
        elif reduction_process:
            if workspace != output_workspace:
                RenameWorkspace(InputWorkspace=workspace, OutputWorkspace=output_workspace)
            self._setWorkspaceProperty("OutputWorkspace", mtd[output_workspace]._getHeldObject())
        else:
            mtd.sendErrorMessage("EQSANSLiveReduce has both the ReductionProcess and PostProcess properties set to FALSE")
    
    def _reduction_process(self, workspace):
        """
            Perform the main reduction process
            @param workspace: workspace to reduce
        """
        file_path = self.getProperty("LogDataFile")
        if os.path.isfile(file_path):
            LoadNexusLogs(Workspace=workspace, Filename=file_path,
                          OverwriteLogs=True)
        else:
            mtd.sendLogMessage("EQSANSLiveReduce could not find the sample log data file: skipping")
        
        # EQSANS reduction script
        import reduction.instruments.sans.sns_command_interface as cmd 
        cmd.AppendDataFile([workspace])
        cmd.Reduce1D()
        
    def _post_process(self, workspace):
        """
            Post processing. Compute I(q) on the final reduced workspace
            @param workspace: reduced workspace
        """
        import reduction.instruments.sans.sns_command_interface as cmd 
        from reduction.instruments.sans.sns_reduction_steps import AzimuthalAverageByFrame
        averager = AzimuthalAverageByFrame()
        averager.execute(cmd.ReductionSingleton(), workspace)
        #self.setPropertyValue("OutputWorkspace", workspace+'_Iq')
                
mtd.registerPyAlgorithm(EQSANSLiveReduce())
