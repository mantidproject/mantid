"""
    Implementation of reduction steps for SNS EQSANS
"""
import os
import math
from reduction import ReductionStep

# Mantid imports
from mantidsimple import *

class LoadRun(ReductionStep):
    """
        Load a data file, move its detector to the right position according
        to the beam center and normalize the data.
    """
    def __init__(self, datafile=None):
        super(LoadRun, self).__init__()
        self._data_file = datafile
                
    def execute(self, reducer, workspace):      
        # If we don't have a data file, look up the workspace handle
        if self._data_file is None:
            if workspace in reducer._data_files:
                data_file = reducer._data_files[workspace]
            elif workspace in reducer._extra_files:
                data_file = reducer._extra_files[workspace]
            else:
                raise RuntimeError, "SNSReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
        else:
            data_file = self._data_file
        
        # Load data
        filepath = reducer._full_file_path(data_file)
        LoadSNSNexus(filepath, workspace)
        CropWorkspace(workspace, "normalization", StartWorkspaceIndex=0,EndWorkspaceIndex=0)
        
        # Until LoadSNSNexus is fixed, we need to move the Source to its proper position
        MoveInstrumentComponent(workspace, "Source", Z=-14, RelativePosition=0)

        Divide(workspace, "normalization", workspace)

        # The first spectrum is the monitor, but LoadSNSNexus doesn't flag it as such...
        MaskDetectors(workspace, SpectraList="1")

        ConvertUnits(workspace, workspace, "Wavelength")

        # Pick a pixel and get its Z coordinante. Store the sample-detector distance
        reducer.instrument.sample_detector_distance = mtd[workspace].getInstrument()[100].getPos().getZ()*1000.0
        
        mantid.sendLogMessage("Loaded %s: sample-detector distance = %g" %(workspace, reducer.instrument.sample_detector_distance))
        
        return "Data file loaded: %s" % (workspace)

