"""
    Implementation of reduction steps for SNS EQSANS
    
    TODO:   - Allow use of FindNexus when we are on an analysis computer  
"""
import os
from reduction import ReductionStep
from sans_reduction_steps import BaseTransmission

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
        # The file also has to be pristine
        if mtd[workspace] is not None and not force and reducer.is_clean(workspace):
            return "Data %s is already loaded: delete it first if reloading is intended" % (workspace)

        # Find all the necessary files
        event_file = ""
        pulseid_file = ""
        nxs_file = ""
        
        # Check if we have an event file or a pulseid file.
        is_event_nxs = False
        
        if filepath.find("_neutron_event")>0:
            event_file = filepath
            pulseid_file = filepath.replace("_neutron_event", "_pulseid")
        elif filepath.find("_pulseid")>0:
            pulseid_file = filepath
            event_file = filepath.replace("_pulseid", "_neutron_event")
        else:
            #raise RuntimeError, "SNSReductionSteps.LoadRun couldn't find the event and pulseid files"
            # Doesn't look like event pre-nexus, try event nexus
            is_event_nxs = True
        
        # Mapping file
        mapping_file = reducer.instrument.definition.getStringParameter("TS_mapping_file")[0]
        directory,_ = os.path.split(event_file)
        mapping_file = os.path.join(directory, mapping_file)
        
        if is_event_nxs:
            mantid.sendLogMessage("Loading %s as event Nexus" % (filepath))
            LoadSNSEventNexus(Filename=filepath, OutputWorkspace=workspace+'_evt')
        else:
            mantid.sendLogMessage("Loading %s as event pre-Nexus" % (filepath))
            nxs_file = event_file.replace("_neutron_event.dat", ".nxs")
            LoadEventPreNeXus(EventFilename=event_file, OutputWorkspace=workspace+'_evt', PulseidFilename=pulseid_file, MappingFilename=mapping_file, PadEmptyPixels=1)
            LoadLogsFromSNSNexus(Workspace=workspace+'_evt', Filename=nxs_file)
        
        # Store the sample-detector distance
        reducer.instrument.sample_detector_distance = mtd[workspace+'_evt'].getRun()["detectorZ"].getStatistics().mean
        
        # Move the detector to its correct position
        MoveInstrumentComponent(workspace+'_evt', "detector1", Z=reducer.instrument.sample_detector_distance/1000.0, RelativePosition=0)
        Rebin(workspace+'_evt', workspace, "0,10,20000")
        
        # Apply the TOF offset for this run
        mantid.sendLogMessage("Frame-skipping option: %s" % str(reducer.frame_skipping))
        offset = EQSANSTofOffset(InputWorkspace=workspace, FrameSkipping=reducer.frame_skipping)
        offset_calc = offset["Offset"].value
        ChangeBinOffset(workspace, workspace, offset_calc) 

        ConvertUnits(workspace, workspace, "Wavelength")
        
        mantid.sendLogMessage("Loaded %s: sample-detector distance = %g" %(workspace, reducer.instrument.sample_detector_distance))
        
        # Remove the dirty flag if it existed
        reducer.clean(workspace)
        return "Data file loaded: %s" % (workspace)



class Normalize(ReductionStep):
    """
        Normalize the data to the accelerator current
    """
    def execute(self, reducer, workspace):
        # Flag the workspace as dirty
        reducer.dirty(workspace)
        
        #NormaliseByCurrent(workspace, workspace)
        proton_charge = mantid.getMatrixWorkspace(workspace).getRun()["proton_charge"].getStatistics().mean
        duration = mantid.getMatrixWorkspace(workspace).getRun()["proton_charge"].getStatistics().duration
        frequency = mantid.getMatrixWorkspace(workspace).getRun()["frequency"].getStatistics().mean
        acc_current = 1.0e-12 * proton_charge * duration * frequency
        Scale(InputWorkspace=workspace, OutputWorkspace=workspace, Factor=1.0/acc_current, Operation="Multiply")
        
        return "Data normalized to accelerator current" 
    
    
class Transmission(BaseTransmission):
    """
        Perform the transmission correction for EQ-SANS
    """
    def __init__(self, normalize_to_unity=True):
        super(Transmission, self).__init__()
        self._normalize = normalize_to_unity
    
    def execute(self, reducer, workspace):
        # The transmission calculation only works on the original un-normalized counts
        if not reducer.is_clean(workspace):
            raise RuntimeError, "The Transmission can only be calculated using un-modified data"
        
        beam_center = reducer.get_beam_center()

        # Calculate the transmission as a function of wavelength
        EQSANSTransmission(InputWorkspace=workspace,
                           OutputWorkspace="transmission",
                           XCenter=beam_center[0],
                           YCenter=beam_center[1],
                           NormalizeToUnity = self._normalize)
        
        # Apply the transmission. For EQSANS, we just divide by the 
        # transmission instead of using the angular dependence of the
        # correction.
        reducer.dirty(workspace)
        Divide(workspace, "transmission", workspace)
        
        return "Transmission correction applied"
    
    