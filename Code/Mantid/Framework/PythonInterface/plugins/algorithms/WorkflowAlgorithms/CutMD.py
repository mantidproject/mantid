from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import os.path


class CutMD(DataProcessorAlgorithm):
    
    def category(self):
        return 'MDAlgorithms'

    def summary(self):
        return 'Slices multidimensional workspaces using input projection information'

    def PyInit(self):
        self.declareProperty(IMDEventWorkspaceProperty('InputWorkspace', '', direction=Direction.Input),
                             doc='MDWorkspace to slice')

        self.declareProperty(IMDWorkspaceProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Output cut workspace')
        
    def PyExec(self):
        to_cut = self.getProperty("InputWorkspace").value
        
        coord_system = to_cut.getSpecialCoordinateSystem()
        if not coord_system == SpecialCoordinateSystem.HKL:
            raise ValueError("Input Workspace must be in reciprocal lattice dimensions (HKL)")
        
        self.setProperty("OutputWorkspace", to_cut)
        
AlgorithmFactory.subscribe(CutMD)
