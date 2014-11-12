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
        
        self.declareProperty(ITableWorkspaceProperty('Projection', '', direction=Direction.Input, optional = PropertyMode.Optional), doc='Projection')
        
        self.declareProperty(FloatArrayProperty(name='P1Bin', values=[]),
                             doc='Projection 1 binning')
        
        self.declareProperty(FloatArrayProperty(name='P2Bin', values=[]),
                             doc='Projection 2 binning')
        
        self.declareProperty(FloatArrayProperty(name='P3Bin', values=[]),
                             doc='Projection 3 binning')

        self.declareProperty(IMDWorkspaceProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Output cut workspace')
        
    def PyExec(self):
        to_cut = self.getProperty("InputWorkspace").value
        
        coord_system = to_cut.getSpecialCoordinateSystem()
        if not coord_system == SpecialCoordinateSystem.HKL:
            raise ValueError("Input Workspace must be in reciprocal lattice dimensions (HKL)")
        
        
        projection = self.getProperty("Projection").value
        if isinstance(projection, ITableWorkspace):
            column_names = set(projection.getColumnNames())
            logger.warning(str(column_names))
            if not column_names == set(['u', 'v']):
                if not column_names == set(['u', 'v', 'offsets']):
                    if not column_names == set(['u', 'v', 'w', 'offsets']):
                        raise ValueError("Projection table schema is wrong")
            if projection.rowCount() != 3:
                raise ValueError("Projection table expects 3 rows")
        
        
        clone_alg = AlgorithmManager.Instance().create("CloneMDWorkspace")
        clone_alg.setChild(True)
        clone_alg.initialize()
        clone_alg.setProperty("InputWorkspace", to_cut)
        clone_alg.setPropertyValue("OutputWorkspace", "cloned")
        clone_alg.execute()
        cloned = clone_alg.getProperty("OutputWorkspace").value
        self.setProperty("OutputWorkspace", cloned)
        
AlgorithmFactory.subscribe(CutMD)
