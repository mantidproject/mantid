"""
    Implementation of reduction steps for SNS EQSANS
    
    TODO:   - Allow use of FindNexus when we are on an analysis computer  
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
                
    def execute(self, reducer, workspace, force=False):      
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

        # Check whether that file was already loaded
        if mtd[workspace] is not None and not force:
            return "Data %s is already loaded: delete it first if reloading is intended" % (workspace)

        # Find all the necessary files
        event_file = ""
        pulseid_file = ""
        nxs_file = ""
        
        # Check if we have an event file or a pulseid file.
        if filepath.find("_neutron_event")>0:
            event_file = filepath
            pulseid_file = filepath.replace("_neutron_event", "_pulseid")
        elif filepath.find("_pulseid")>0:
            pulseid_file = filepath
            event_file = filepath.replace("_pulseid", "_neutron_event")
        else:
            raise RuntimeError, "SNSReductionSteps.LoadRun couldn't find the event and pulseid files"
        nxs_file = event_file.replace("_neutron_event.dat", ".nxs")
        
        # Mapping file
        mapping_file = reducer.instrument.definition.getStringParameter("TS_mapping_file")[0]
        directory,_ = os.path.split(event_file)
        mapping_file = os.path.join(directory, mapping_file)
        
        LoadEventPreNeXus(EventFilename=event_file, OutputWorkspace=workspace+'_evt', PulseidFilename=pulseid_file, MappingFilename=mapping_file, PadEmptyPixels=1)
        LoadLogsFromSNSNexus(Workspace=workspace+'_evt', Filename=nxs_file)
        
        # Store the sample-detector distance
        reducer.instrument.sample_detector_distance = mtd[workspace+'_evt'].getRun()["detectorZ"].getStatistics().mean
        
        # Move the detector to its correct position
        MoveInstrumentComponent(workspace+'_evt', "detector1", Z=reducer.instrument.sample_detector_distance/1000.0, RelativePosition=0)
        Rebin(workspace+'_evt', workspace, "0,10,20000")
        
        # Apply the TOF offset for this run
        offset = EQSANSTofOffset(InputWorkspace=workspace)
        offset_calc = offset["Offset"].value
        ChangeBinOffset(workspace, workspace, offset_calc) 

        ConvertUnits(workspace, workspace, "Wavelength")
        
        mantid.sendLogMessage("Loaded %s: sample-detector distance = %g" %(workspace, reducer.instrument.sample_detector_distance))
        
        return "Data file loaded: %s" % (workspace)

