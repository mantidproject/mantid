"""
    Load EQSANS data file and perform TOF corrections, move detector to beam center, etc...
"""
import os
import sys
import pickle
import math
from reduction import ReductionStep
from reduction import extract_workspace_name, find_data
from eqsans_config import EQSANSConfig
from sans_reduction_steps import BaseBeamFinder

# Mantid imports
from mantid.simpleapi import *

class LoadRun(ReductionStep):
    """
        Load a data file, move its detector to the right position according
        to the beam center and normalize the data.
    """
    def __init__(self, datafile=None, keep_events=False):
        super(LoadRun, self).__init__()
        self._data_file = datafile
        
        # TOF range definition
        self._use_config_cutoff = False
        self._low_TOF_cut = 0
        self._high_TOF_cut = 0
        
        # Wavelength step
        self._wavelength_step = None
        
        # TOF flight path correction
        self._correct_for_flight_path = False
        
        # Use mask defined in configuration file
        self._use_config_mask = False
        self._use_config = True
        
        # Workspace on which to apply correction that should be done
        # independently of the pixel. If False, all correction will be 
        # applied directly to the data workspace.
        self._separate_corrections = False
        
        self._sample_det_dist = None
        self._sample_det_offset = 0
        
        # Option to skip TOF correction
        self._skip_tof_correction = False
        
        # Load monitors
        self._load_monitors = False
        
        # Flag to tell us whether we should do the full reduction with events
        self._keep_events = keep_events
   
    def clone(self, data_file=None, keep_events=None):
        if data_file is None:
            data_file = self._data_file
        if keep_events is None:
            keep_events = self._keep_events
        loader = LoadRun(datafile=data_file, keep_events=keep_events)
        loader._use_config_cutoff = self._use_config_cutoff
        loader._low_TOF_cut = self._low_TOF_cut
        loader._high_TOF_cut = self._high_TOF_cut
        loader._correct_for_flight_path = self._correct_for_flight_path
        loader._use_config_mask = self._use_config_mask
        loader._use_config = self._use_config
        loader._wavelength_step = self._wavelength_step
        loader._sample_det_dist = self._sample_det_dist
        loader._sample_det_offset = self._sample_det_offset
        loader._skip_tof_correction = self._skip_tof_correction
        loader._load_monitors = self._load_monitors
        return loader

    def load_monitors(self, load_monitors):
        self._load_monitors = load_monitors
        
    def skip_tof_correction(self, skip):
        self._skip_tof_correction = skip
        
    def set_wavelength_step(self, step):
        if step is not None and type(step) != int and type(step) != float:
            raise RuntimeError, "LoadRun._wavelength_step expects a float: %s" % str(step)
        self._wavelength_step = step
        
    def set_sample_detector_distance(self, distance):
        # Check that the distance given is either None of a float
        if distance is not None and type(distance) != int and type(distance) != float:
            raise RuntimeError, "LoadRun.set_sample_detector_distance expects a float: %s" % str(distance)
        self._sample_det_dist = distance
        
    def set_sample_detector_offset(self, offset):
        # Check that the offset given is either None of a float
        if offset is not None and type(offset) != int and type(offset) != float:
            raise RuntimeError, "LoadRun.set_sample_detector_offset expects a float: %s" % str(offset)
        self._sample_det_offset = offset
        
    def set_flight_path_correction(self, do_correction=False):
        """
            Set the flag to perform the TOF correction to take into
            account the different in flight path at larger angle.
            @param do_correction: if True, correction will be made
        """
        self._correct_for_flight_path = do_correction
        
    def set_TOF_cuts(self, low_cut=0, high_cut=0):
        """
            Set the range of TOF to be cut on each side of the frame.
            @param low_cut: TOF to be cut from the low-TOF end
            @param high_cut: TOF to be cut from the high-TOF end
        """
        self._low_TOF_cut = low_cut
        self._high_TOF_cut = high_cut
        
    def use_config_cuts(self, use_config=False):
        """
            Set the flag to cut the TOF tails on each side of the
            frame according to the cuts found in the configuration file.
            @param use_config: if True, the configuration file will be used
        """
        self._use_config_cutoff = use_config

    def use_config(self, use_config=True):
        """
            Set the flag to use the configuration file or not.
            Only used for test purposes
        """
        self._use_config = use_config

    def use_config_mask(self, use_config=False):
        """
            Set the flag to use the mask defined in the
            configuration file.
            @param use_config: if True, the configuration file will be used
        """
        self._use_config_mask = use_config
        
    def set_beam_center(self, beam_center):
        """
            Sets the beam center to be used when loading the file
            @param beam_center: [pixel_x, pixel_y]
        """
        pass
    
    @classmethod
    def delete_workspaces(cls, workspace):
        """
            Delete all workspaces related to the loading of the given workspace
            @param workspace: workspace to clean
        """
        # Delete the beam hole transmission workspace if it exists
        if mtd[workspace].getRun().hasProperty("transmission_ws"):
            trans_ws = mtd[workspace].getRun().getProperty("transmission_ws").value
            if mtd.doesExist(trans_ws):
                DeleteWorkspace(trans_ws)
        if mtd.doesExist(workspace):
            DeleteWorkspace(workspace)
        
    def execute(self, reducer, workspace):
        output_str = ""      
        # If we don't have a data file, look up the workspace handle
        # Only files that are used for computing data corrections have
        # a path that is passed directly. Data files that are reduced
        # are simply found in reducer._data_files 
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
        use_config_beam = False
        [pixel_ctr_x, pixel_ctr_y] = reducer.get_beam_center()
        if pixel_ctr_x == 0.0 and pixel_ctr_y == 0.0:
            use_config_beam = True            
            
        def _load_data_file(file_name, wks_name):
            # Check whether we are processing an event workspace or whether
            # we need to load a file
            if mtd.doesExist(file_name) \
                and mtd[file_name].getAxis(0).getUnit().name()=="TOF":
                input_ws = file_name
                filepath = None
            else:
                filepath = find_data(file_name, instrument=reducer.instrument.name())
                input_ws = None
                
            results = EQSANSLoad(Filename=filepath,
                           InputWorkspace=input_ws,
                           OutputWorkspace=wks_name,
                           UseConfigBeam=use_config_beam,
                           BeamCenterX=None,
                           BeamCenterY=None,
                           UseConfigTOFCuts=self._use_config_cutoff,
                           LowTOFCut=self._low_TOF_cut,
                           HighTOFCut=self._high_TOF_cut,
                           SkipTOFCorrection=self._skip_tof_correction,
                           WavelengthStep=self._wavelength_step,
                           UseConfigMask=self._use_config_mask,
                           UseConfig=self._use_config,
                           CorrectForFlightPath=self._correct_for_flight_path,
                           SampleDetectorDistance=self._sample_det_dist,
                           SampleDetectorDistanceOffset=self._sample_det_offset,
                           PreserveEvents=self._keep_events,
                           LoadMonitors=self._load_monitors,
                           ReductionProperties=reducer.get_reduction_table_name()
                           )            
            return results[-1] # return output message
        
        # Check whether we have a list of files that need merging
        #   Make sure we process a list of files written as a string
        if type(data_file)==str:
            data_file = find_data(data_file, instrument=reducer.instrument.name(), allow_multiple=True)
        if type(data_file)==list:
            for i in range(len(data_file)):
                output_str += "Loaded %s\n" % data_file[i]
                if i==0:
                    output_str += _load_data_file(data_file[i], workspace)
                else:
                    output_str += _load_data_file(data_file[i], '__tmp_wksp')
                    Plus(LHSWorkspace=workspace,
                         RHSWorkspace='__tmp_wksp',
                         OutputWorkspace=workspace)
            if mtd.doesExist('__tmp_wksp'):
                DeleteWorkspace('__tmp_wksp')
        else:
            output_str += "Loaded %s\n" % data_file
            output_str += _load_data_file(data_file, workspace)
        
        mtd[workspace].getRun().addProperty("event_ws", workspace, True)
        
        if mtd[workspace].getRun().hasProperty("beam_center_x") and \
            mtd[workspace].getRun().hasProperty("beam_center_y"):
            beam_center_x = mtd[workspace].getRun().getProperty("beam_center_x").value
            beam_center_y = mtd[workspace].getRun().getProperty("beam_center_y").value
            if type(reducer._beam_finder) is BaseBeamFinder:
                reducer.set_beam_finder(BaseBeamFinder(beam_center_x, beam_center_y))
                logger.notice("Setting beam center to [%-6.1f, %-6.1f]" % (beam_center_x, beam_center_y))
        
        # Remove the dirty flag if it existed
        reducer.clean(workspace)
        mtd[workspace].getRun().addProperty("loaded_by_eqsans_reduction", 1, True)
        
        return output_str
