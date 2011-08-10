"""
    Implementation of reduction steps for SNS EQSANS    
"""
import os
import sys
import pickle
import math
from reduction import ReductionStep
from sans_reduction_steps import BaseTransmission, BaseBeamFinder, WeightedAzimuthalAverage
from sans_reduction_steps import DirectBeamTransmission as SingleFrameDirectBeamTransmission
from sans_reduction_steps import SaveIqAscii as BaseSaveIqAscii
from sans_reduction_steps import SensitivityCorrection as BaseSensitivityCorrection
from reduction import extract_workspace_name, find_file, find_data
from eqsans_config import EQSANSConfig

# Mantid imports
from MantidFramework import *
from mantidsimple import *
    
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
        
        # TOF flight path correction
        self._correct_for_flight_path = False
        
        # Use mask defined in configuration file
        self._use_config_mask = False
        
        # Workspace on which to apply correction that should be done
        # independently of the pixel. If False, all correction will be 
        # applied directly to the data workspace.
        self._separate_corrections = False
        
        self._sample_det_dist = None
        self._sample_det_offset = 0
        
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
        return loader

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
    
    def _process_existing_event_ws(self, reducer, event_ws, workspace):
        """
            Prepare a histogram workspace out of an event workspace
            @param event_ws: event workspace to process
            @param workspace: histrogram workspace
        """
        # Get wavelength bands
        if mtd[event_ws].getRun().hasProperty("wavelength_min"):
            wl_min = mtd[event_ws].getRun().getProperty("wavelength_min").value
        else:
            return False

        if mtd[event_ws].getRun().hasProperty("wavelength_max_frame2"):
            wl_max = mtd[event_ws].getRun().getProperty("wavelength_max_frame2").value
        elif mtd[event_ws].getRun().hasProperty("wavelength_max"):
            wl_max = mtd[event_ws].getRun().getProperty("wavelength_max").value
        else:
            return False

        Rebin(event_ws, workspace, "%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max), False)
        
        # Add the name of the event workspace as a property in case we need it later (used by the beam stop transmission)
        mantid[workspace].getRun().addProperty_str("event_ws", event_ws, True)
            
        if self._separate_corrections:
            # If we pick up the data file from the workspace, it's because we 
            # are going through the reduction chain, otherwise we are just loading
            # the file to compute a correction
            if self._data_file is None:    
                data_ws = "%s_data" % workspace
                mantid[workspace].getRun().addProperty_str("data_ws", data_ws, True)
        
                CloneWorkspace(workspace, data_ws)
                Divide(workspace, workspace, workspace)
                reducer.clean(data_ws)
                    
        # Remove the dirty flag if it existed
        reducer.clean(workspace)
        mtd[workspace].getRun().addProperty_int("loaded_by_eqsans_reduction", 1, True)
        return True
             
    @classmethod
    def delete_workspaces(cls, workspace):
        """
            Delete all workspaces related to the loading of the given workspace
            @param workspace: workspace to clean
        """
        # Delete the beam hole transmission workspace if it exists
        if mtd[workspace].getRun().hasProperty("transmission_ws"):
            trans_ws = mtd[workspace].getRun().getProperty("transmission_ws").value
            if mtd.workspaceExists(trans_ws):
                mtd.deleteWorkspace(trans_ws)
        if mtd.workspaceExists(workspace):
            mtd.deleteWorkspace(workspace)
        if mtd.workspaceExists(workspace+'_evt'):
            mtd.deleteWorkspace(workspace+'_evt')
        
    def _look_for_loaded_data(self, reducer, file_path):
        # If we are using only events, we can't trust any existing workspace
        if self._keep_events:
            return None, None
        
        if not type(file_path) == list:
            file_path = [file_path] 
        for item in reducer._data_files.keys():
            if reducer._data_files[item] == file_path and \
                mtd.workspaceExists(item):
                # Don't trust existing workspace2Ds
                #if reducer.is_clean(item):
                #    return item, None
                if mtd[item].getRun().hasProperty("event_ws") and \
                    mtd.workspaceExists(mtd[item].getRun().getProperty("event_ws").value):
                    return None, mtd[item].getRun().getProperty("event_ws").value
        return None, None
                
    def _get_source_slit_size(self, reducer, workspace, config=None):
        """
            Determine the source aperture by reading in the slit settings from 
            the log
            @param reducer: Reducer object
            @param workspace: name of workspace being processed
            @param config: EQSANSConfig object 
        """
        
        slit_positions = config.slit_positions
        # If we don't have a config file, assume the standard slit settings
        if config is None:
            slit_positions = [[5.0, 10.0, 10.0, 15.0, 20.0, 20.0, 25.0, 40.0], 
                              [0.0, 10.0, 10.0, 15.0, 15.0, 20.0, 20.0, 40.0], 
                              [0.0, 10.0, 10.0, 15.0, 15.0, 20.0, 20.0, 40.0]]
        
        slit1 = None
        slit2 = None
        slit3 = None
        if mtd[workspace].getRun().hasProperty("vBeamSlit"):
            slit1 = int(mtd[workspace].getRun().getProperty("vBeamSlit").value[0])
        if mtd[workspace].getRun().hasProperty("vBeamSlit2"):
            slit2 = int(mtd[workspace].getRun().getProperty("vBeamSlit2").value[0])
        if mtd[workspace].getRun().hasProperty("vBeamSlit3"):
            slit3 = int(mtd[workspace].getRun().getProperty("vBeamSlit3").value[0])
            
        if slit1 is None and slit2 is None and slit3 is None:
            return "   Could not determine source aperture diameter\n"
            
        slits = [slit1, slit2, slit3]
        
        S1 = 20.0 # slit default size
        source_to_sample = math.fabs(mtd[workspace].getInstrument().getSource().getPos().getZ())*1000.0 
        L1 = None
        
        for i in range(3):
            m=slits[i]-1; 
            if m>=0 and m<6:
                x = slit_positions[i][m];
                y = source_to_sample - reducer.instrument.slit_to_source[i]
                if (L1 is None or x/y<S1/L1):
                    L1=y
                    S1=x
                    
        mtd[workspace].getRun().addProperty_dbl("source-aperture-diameter", S1, 'mm', True) 
        return "   Source aperture diameter = %g mm\n" % S1    
                
    def execute(self, reducer, workspace, force=False):
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
                force = True
            else:
                raise RuntimeError, "SNSReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
        else:
            data_file = self._data_file

        # Check whether that file was already loaded
        # The file also has to be pristine
        if mtd[workspace] is not None and not force and reducer.is_clean(workspace):
            if not mtd[workspace].getRun().hasProperty("loaded_by_eqsans_reduction"):
                raise RuntimeError, "Workspace %s was loaded outside the EQSANS reduction. Delete it before restarting." % workspace
            
            mantid.sendLogMessage("Data %s is already loaded: delete it first if reloading is intended" % (workspace))
            return "Data %s is already loaded: delete it first if reloading is intended\n" % (workspace)
        
        # Configuration files
        config_file = None
        config_files = []
        class _ConfigFile(object):
            def __init__(self, run, path):
                self.run = run
                self.path = path
        
        # Load data
        def _load_data_file(file_name, wks_name):
            filepath = find_data(file_name, instrument=reducer.instrument.name())
    
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
                # Doesn't look like event pre-nexus, try event nexus
                is_event_nxs = True
            
            # Find available configuration files
            data_dir,_ = os.path.split(filepath)
            files = find_file(startswith="eqsans_configuration", data_dir=data_dir)
            for file in files:
                name, ext = os.path.splitext(file)
                # The extension should be a run number
                try:
                    ext = ext.replace('.','')
                    config_files.append(_ConfigFile(int(ext),file))
                except:
                    # Bad extension, which means it's not the file we are looking for
                    pass
                           
            if is_event_nxs:
                mantid.sendLogMessage("Loading %s as event Nexus" % (filepath))
                LoadEventNexus(Filename=filepath, OutputWorkspace=wks_name)
            else:
                # Mapping file
                default_instrument = reducer.instrument.get_default_instrument()

                mapping_file = default_instrument.getStringParameter("TS_mapping_file")[0]
                mapping_file = os.path.join(data_dir, mapping_file)
                
                mantid.sendLogMessage("Loading %s as event pre-Nexus" % (filepath))
                nxs_file = event_file.replace("_neutron_event.dat", "_histo.nxs")
                if not os.path.isfile(nxs_file):
                    nxs_file = event_file.replace("_neutron_event.dat", ".nxs")
                LoadEventPreNeXus(EventFilename=event_file, OutputWorkspace=wks_name, PulseidFilename=pulseid_file, MappingFilename=mapping_file)
                LoadNexusLogs(Workspace=wks_name, Filename=nxs_file)
            
            return "   Loaded %s\n" % wks_name
        
        # If the event workspace exists, don't reload it
        if mtd.workspaceExists(workspace+'_evt'):
            if self._process_existing_event_ws(reducer, workspace+'_evt', workspace):
                mantid.sendLogMessage("INFO: %s already loaded" % workspace+'_evt')
                return "INFO: %s_evt already loaded" % workspace
        
        # Check whether there is an equivalent event workspace
        eq_histo_ws, eq_event_ws = self._look_for_loaded_data(reducer, data_file)
        if eq_histo_ws is not None:
            # The data is loaded and clean. We don't know what is going to happen to it so clone it
            CloneWorkspace(eq_histo_ws, OutputWorkspace=workspace)
            mantid.sendLogMessage("INFO: %s already loaded as %s" % (workspace, eq_histo_ws))
            return "INFO: %s already loaded as %s" % (workspace, eq_histo_ws)
        elif eq_event_ws is not None:
            # The data is available as an event workspace. We just need to process it.
            if self._process_existing_event_ws(reducer, eq_event_ws, workspace):
                mantid.sendLogMessage("INFO: %s already loaded as %s" % (workspace+'_evt', eq_event_ws))
                return "INFO: %s already loaded as %s" % (workspace+'_evt', eq_event_ws)
        
        # Check whether we have a list of files that need merging
        if type(data_file)==list:
            for i in range(len(data_file)):
                if i==0:
                    _load_data_file(data_file[i], workspace+'_evt')
                else:
                    _load_data_file(data_file[i], '__tmp_wksp')
                    Plus(LHSWorkspace=workspace+'_evt',
                         RHSWorkspace='__tmp_wksp',
                         OutputWorkspace=workspace+'_evt')
            if mtd.workspaceExists('__tmp_wksp'):
                mtd.deleteWorkspace('__tmp_wksp')
        else:
            _load_data_file(data_file, workspace+'_evt')
        
        
        # Store the sample-detector distance.
        sdd = mtd[workspace+'_evt'].getRun()["detectorZ"].getStatistics().mean

        if self._sample_det_dist is not None:
            sdd = self._sample_det_dist            
        elif not self._sample_det_offset == 0:
            sdd += self._sample_det_offset

        mtd[workspace+'_evt'].getRun().addProperty_dbl("sample_detector_distance", sdd, 'mm', True)
        
        # Move the detector to its correct position
        MoveInstrumentComponent(workspace+'_evt', "detector1", Z=sdd/1000.0, RelativePosition=0)

        # Choose and process configuration file
        if len(config_files)>0:
            if mtd[workspace+'_evt'].getRun().hasProperty("run_number"):
                run_prop = mtd[workspace+'_evt'].getRun().getProperty("run_number")
                try:
                    run_as_int = int(run_prop.value)
                    def _compare(item, compare_with):
                        # If both configurations have their run number below our run,
                        # use the configuration with the highest run number
                        if item.run<=run_as_int and compare_with.run<=run_as_int:
                            if item.run>compare_with.run:
                                return item
                            else:
                                return compare_with
                        # If both configurations have their run number above our run,
                        # use the configuration with the lowest run number
                        # If one is above and one is below, use the lowest run number too
                        else:
                            if item.run<compare_with.run:
                                return item
                            else:
                                return compare_with                         
                    config_file = reduce(_compare, config_files).path
                except:
                    # Could not read in the run number
                    pass
            else:
                mantid.sendLogMessage("Could not find run number file for %s" % workspace)
            
        # Process the configuration file
        low_TOF_cut = self._low_TOF_cut
        high_TOF_cut = self._high_TOF_cut
        # Width of the prompt pulse
        prompt_pulse_width = 20
        
        # Read in the configuration file
        conf = None
        if config_file is not None:
            mantid.sendLogMessage("Using configuration file: %s\n" % config_file)
            output_str +=  "   Using configuration file: %s\n" % config_file
            conf = EQSANSConfig(config_file)
            mtd[workspace+'_evt'].getRun().addProperty_dbl("low_tof_cut", conf.low_TOF_cut, "microsecond", True)
            mtd[workspace+'_evt'].getRun().addProperty_dbl("high_tof_cut", conf.high_TOF_cut, "microsecond", True)
            
            if conf.prompt_pulse_width is not None and conf.prompt_pulse_width>0:
                mtd[workspace+'_evt'].getRun().addProperty_dbl("prompt_pulse_width", conf.prompt_pulse_width, "microsecond", True)  
                prompt_pulse_width = conf.prompt_pulse_width
                
            if self._use_config_cutoff:
                low_TOF_cut = conf.low_TOF_cut
                high_TOF_cut = conf.high_TOF_cut
                
            # Store mask information
            if self._use_config_mask:
                mtd[workspace+'_evt'].getRun().addProperty_str("rectangular_masks", pickle.dumps(conf.rectangular_masks), "pixels", True)
                output_str +=  "   Using mask information found in configuration file\n"
                
            if type(reducer._beam_finder) is BaseBeamFinder:
                if reducer.get_beam_center() == [0.0,0.0]:
                    reducer.set_beam_finder(BaseBeamFinder(conf.center_x, conf.center_y))   
                    output_str += "   Beam center set from config file: %-6.1f, %-6.1f\n" % (conf.center_x, conf.center_y)                             
        else:
            mantid.sendLogMessage("Could not find configuration file for %s" % workspace)
            output_str += "   Could not find configuration file for %s\n" % workspace

        # Get source aperture radius
        output_str += self._get_source_slit_size(reducer, workspace+'_evt', conf)

        # Move detector array to correct position
        [pixel_ctr_x, pixel_ctr_y] = reducer.get_beam_center()
        if pixel_ctr_x is not None and pixel_ctr_y is not None:
            
            # Check that the center of the detector really is at (0,0)
            nx_pixels = int(mtd[workspace+'_evt'].getInstrument().getNumberParameter("number-of-x-pixels")[0])
            ny_pixels = int(mtd[workspace+'_evt'].getInstrument().getNumberParameter("number-of-y-pixels")[0])
            
            pixel_first = mtd[workspace+'_evt'].getInstrument().getDetector(0).getPos()
            pixel_last_x = mtd[workspace+'_evt'].getInstrument().getDetector(reducer.instrument.get_detector_from_pixel([[nx_pixels-1,0]])[0]).getPos()
            pixel_last_y = mtd[workspace+'_evt'].getInstrument().getDetector(reducer.instrument.get_detector_from_pixel([[0,ny_pixels-1]])[0]).getPos()
            x_offset = (pixel_first.getX()+pixel_last_x.getX())/2.0
            y_offset = (pixel_first.getY()+pixel_last_y.getY())/2.0
            
            [beam_ctr_x, beam_ctr_y] = reducer.instrument.get_coordinate_from_pixel(pixel_ctr_x, pixel_ctr_y)
            [default_pixel_x, default_pixel_y] = reducer.instrument.get_default_beam_center()
            [default_x, default_y] = reducer.instrument.get_coordinate_from_pixel(default_pixel_x, default_pixel_y)
            MoveInstrumentComponent(workspace+'_evt', "detector1", 
                                    X = default_x-x_offset-beam_ctr_x,
                                    Y = default_y-y_offset-beam_ctr_y,
                                    RelativePosition="1")
            mtd[workspace+'_evt'].getRun().addProperty_dbl("beam_center_x", pixel_ctr_x, 'pixel', True)            
            mtd[workspace+'_evt'].getRun().addProperty_dbl("beam_center_y", pixel_ctr_y, 'pixel', True)   
            mantid.sendLogMessage("Beam center: %-6.1f, %-6.1f" % (pixel_ctr_x, pixel_ctr_y))                             
        else:
            # Don't move the detector and use the default beam center
            [default_pixel_x, default_pixel_y] = reducer.instrument.get_default_beam_center()
            mantid[workspace+'_evt'].getRun().addProperty_dbl("beam_center_x", default_pixel_x, 'pixel', True)            
            mantid[workspace+'_evt'].getRun().addProperty_dbl("beam_center_y", default_pixel_y, 'pixel', True)            
            if type(reducer._beam_finder) is BaseBeamFinder:
                reducer.set_beam_finder(BaseBeamFinder(default_pixel_x, default_pixel_y))
                mantid.sendLogMessage("No beam finding method: setting to default [%-6.1f, %-6.1f]" % (default_pixel_x, 
                                                                                                       default_pixel_y))
            
            mantid.sendLogMessage("Beam center isn't defined: skipping beam center alignment for %s" % workspace+'_evt')

        # Modify TOF
        output_str += "   Discarding low %6.1f and high %6.1f microsec\n" % (low_TOF_cut, high_TOF_cut)
        if self._correct_for_flight_path:
            output_str += "   Correcting TOF for flight path\n"
        a = EQSANSTofStructure(InputWorkspace=workspace+'_evt', 
                               LowTOFCut=low_TOF_cut, HighTOFCut=high_TOF_cut,
                               FlightPathCorrection=self._correct_for_flight_path)
        offset = float(a.getPropertyValue("TofOffset"))
        wl_min = float(a.getPropertyValue("WavelengthMin"))
        wl_max = float(a.getPropertyValue("WavelengthMax"))
        wl_combined_max = wl_max
        frame_skipping = a.getProperty("FrameSkipping").value
        mtd[workspace+'_evt'].getRun().addProperty_dbl("wavelength_min", wl_min, "Angstrom", True)
        mtd[workspace+'_evt'].getRun().addProperty_dbl("wavelength_max", wl_max, "Angstrom", True)
        mtd[workspace+'_evt'].getRun().addProperty_int("is_frame_skipping", int(frame_skipping), True)
        output_str += "   Wavelength range: %6.1f - %-6.1f Angstrom  [Frame skipping = %s]" % (wl_min, wl_max, str(frame_skipping))
        if frame_skipping:
            wl_min2 = float(a.getPropertyValue("WavelengthMinFrame2"))
            wl_max2 = float(a.getPropertyValue("WavelengthMaxFrame2"))
            mtd[workspace+'_evt'].getRun().addProperty_dbl("wavelength_min_frame2", wl_min2, "Angstrom", True)
            mtd[workspace+'_evt'].getRun().addProperty_dbl("wavelength_max_frame2", wl_max2, "Angstrom", True)
            wl_combined_max = wl_max2
            output_str += "   Second frame: %6.1f - %-6.1f Angstrom" % (wl_min2, wl_max2)
        
        # Remove prompt pulses
        #RemovePromptPulse(workspace+'_evt', workspace+'_evt', Width=prompt_pulse_width)
        
        # Convert TOF to wavelength
        source_to_sample = math.fabs(mtd[workspace+'_evt'].getInstrument().getSource().getPos().getZ())*1000.0
        conversion_factor = 3.9560346 / (sdd+source_to_sample);
        ScaleX(workspace+'_evt', workspace+'_evt', conversion_factor)
        mtd[workspace+'_evt'].getAxis(0).setUnit("Wavelength")
        
        # Rebin so all the wavelength bins are aligned
        # Keep events
        if self._keep_events:
            mantid.sendLogMessage("Reducing with events only")
            Rebin(workspace+'_evt', workspace+'_evt', "%4.2f,%4.2f,%4.2f" % (wl_min, 0.1, wl_combined_max), True)
            #TODO: remove the need to rename workspace once we know for sure that we will never need to
            # go to histograms at this point in the reduction.
            RenameWorkspace(workspace+'_evt', workspace)
            # Add the name of the event workspace as a property in case we need it later (used by the beam stop transmission)
            mantid[workspace].getRun().addProperty_str("event_ws", workspace, True)
        else:
            Rebin(workspace+'_evt', workspace, "%4.2f,%4.2f,%4.2f" % (wl_min, 0.1, wl_combined_max), False)
            # Add the name of the event workspace as a property in case we need it later (used by the beam stop transmission)
            mantid[workspace].getRun().addProperty_str("event_ws", workspace+'_evt', True)
        
        mantid.sendLogMessage("Loaded %s: sample-detector distance = %g [frame-skipping: %s]" %(workspace, sdd, str(frame_skipping)))
                    
        if self._separate_corrections:
            # If we pick up the data file from the workspace, it's because we 
            # are going through the reduction chain, otherwise we are just loading
            # the file to compute a correction
            if self._data_file is None:    
                data_ws = "%s_data" % workspace
                mantid[workspace].getRun().addProperty_str("data_ws", data_ws, True)
        
                CloneWorkspace(workspace, data_ws)
                Divide(workspace, workspace, workspace)
                reducer.clean(data_ws)
                    
        # Remove the dirty flag if it existed
        reducer.clean(workspace)
        mtd[workspace].getRun().addProperty_int("loaded_by_eqsans_reduction", 1, True)
        
        return "Data file loaded: %s\n%s" % (workspace, output_str)



class Normalize(ReductionStep):
    """
        Normalize the data to the accelerator current
    """
    def __init__(self, normalize_to_beam=True):
        super(Normalize, self).__init__()
        self._normalize_to_beam = normalize_to_beam
        
    def get_normalization_spectrum(self):
        return -1
    
    def execute(self, reducer, workspace):
        # Flag the workspace as dirty
        reducer.dirty(workspace)
        
        flux_data_path = None
        if self._normalize_to_beam:
            # Find available beam flux file
            # First, check whether we have access to the SNS mount, if
            # not we will look in the data directory
            
            flux_files = find_file(filename="bl6_flux_at_sample", data_dir=reducer._data_path)
            if len(flux_files)>0:
                flux_data_path = flux_files[0]
                mantid.sendLogMessage("Using beam flux file: %s" % flux_data_path)
            else:
                mantid.sendLogMessage("Could not find beam flux file!")
                
            if flux_data_path is not None:
                beam_flux_ws = "__beam_flux"
                LoadAscii(flux_data_path, beam_flux_ws, Separator="Tab", Unit="Wavelength")
                ConvertToHistogram(beam_flux_ws, beam_flux_ws)
                RebinToWorkspace(beam_flux_ws, workspace, beam_flux_ws)
                NormaliseToUnity(beam_flux_ws, beam_flux_ws)
                Divide(workspace, beam_flux_ws, workspace)
                mtd[workspace].getRun().addProperty_str("beam_flux_ws", beam_flux_ws, True)
            else:
                flux_data_path = "Could not find beam flux file!"
            
        #NormaliseByCurrent(workspace, workspace)
        proton_charge = mantid.getMatrixWorkspace(workspace).getRun()["proton_charge"].getStatistics().mean
        duration = mantid.getMatrixWorkspace(workspace).getRun()["proton_charge"].getStatistics().duration
        frequency = mantid.getMatrixWorkspace(workspace).getRun()["frequency"].getStatistics().mean
        acc_current = 1.0e-12 * proton_charge * duration * frequency
        Scale(InputWorkspace=workspace, OutputWorkspace=workspace, Factor=1.0/acc_current, Operation="Multiply")
        
        return "Data [%s] normalized to accelerator current\n  Beam flux file: %s" % (workspace, flux_data_path) 
    
    
class BeamStopTransmission(BaseTransmission):
    """
        Perform the transmission correction for EQ-SANS using the beam stop hole
    """
    def __init__(self, normalize_to_unity=False, theta_dependent=False):
        super(BeamStopTransmission, self).__init__()
        self._normalize = normalize_to_unity
        self._theta_dependent = theta_dependent
        self._transmission_ws = None
    
    def execute(self, reducer, workspace):
        # Keep track of workspaces to delete when we clean up
        ws_for_deletion = []
        if self._transmission_ws is not None and mtd.workspaceExists(self._transmission_ws):
            # We have everything we need to apply the transmission correction
            pass
        elif mtd[workspace].getRun().hasProperty("transmission_ws"):
            trans_prop = mtd[workspace].getRun().getProperty("transmission_ws")
            if mtd.workspaceExists(trans_prop.value):
                self._transmission_ws = trans_prop.value
            else:
                raise RuntimeError, "The transmission workspace %s is no longer available" % trans_prop.value
        else:
            raw_ws = workspace
            if mtd[workspace].getRun().hasProperty("event_ws"):
                raw_ws_prop = mtd[workspace].getRun().getProperty("event_ws")
                if mtd.workspaceExists(raw_ws_prop.value):
                    if mtd[workspace].getRun().hasProperty("wavelength_min"):
                        wl_min = mtd[workspace].getRun().getProperty("wavelength_min").value
                    else:
                        raise RuntimeError, "Beam-hole transmission correction could not get minimum wavelength"
                    if mtd[workspace].getRun().hasProperty("wavelength_max_frame2"):
                        wl_max = mtd[workspace].getRun().getProperty("wavelength_max_frame2").value
                    elif mtd[workspace].getRun().hasProperty("wavelength_max"):
                        wl_max = mtd[workspace].getRun().getProperty("wavelength_max").value
                    else:
                        raise RuntimeError, "Beam-hole transmission correction could not get maximum wavelength"
                    raw_ws = '__'+raw_ws_prop.value+'_histo'
                    # Need to convert to workspace until somebody fixes Integration. Ticket #3277
                    Rebin(raw_ws_prop.value, raw_ws, "%4.2f,%4.2f,%4.2f" % (wl_min, 0.1, wl_max), False)
                    ws_for_deletion.append(raw_ws)
                else:
                    raise RuntimeError, "The event workspace %s is no longer available" % raw_ws_prop.value

            # The transmission calculation only works on the original un-normalized counts
            if not reducer.is_clean(raw_ws):
                raise RuntimeError, "The transmission can only be calculated using un-modified data"

            if mtd[workspace].getRun().hasProperty("beam_center_x"):
                beam_center_x = mtd[workspace].getRun().getProperty("beam_center_x").value
            else:
                raise RuntimeError, "Transmission correction algorithm could not get beam center x position"
            if mtd[workspace].getRun().hasProperty("beam_center_y"):
                beam_center_y = mtd[workspace].getRun().getProperty("beam_center_y").value
            else:
                raise RuntimeError, "Transmission correction algorithm could not get beam center y position"
    
            if self._transmission_ws is None:
                self._transmission_ws = "beam_hole_transmission_"+workspace
    
            # Calculate the transmission as a function of wavelength
            EQSANSTransmission(InputWorkspace=raw_ws,
                               OutputWorkspace=self._transmission_ws,
                               XCenter=beam_center_x,
                               YCenter=beam_center_y,
                               NormalizeToUnity = self._normalize)
            
            mantid[workspace].getRun().addProperty_str("transmission_ws", self._transmission_ws, True)

        # Apply the transmission. For EQSANS, we just divide by the 
        # transmission instead of using the angular dependence of the
        # correction.
        reducer.dirty(workspace)

        output_str = "Beam hole transmission correction applied"
        # Get the beam spectrum, if available
        transmission_ws = self._transmission_ws
        if mtd[workspace].getRun().hasProperty("beam_flux_ws"):
            beam_flux_ws_name = mtd[workspace].getRun().getProperty("beam_flux_ws").value
            if mtd.workspaceExists(beam_flux_ws_name):
                beam_flux_ws = mtd[beam_flux_ws_name]
                transmission_ws = "__transmission_tmp"
                ws_for_deletion.append(transmission_ws)
                Divide(self._transmission_ws, beam_flux_ws, transmission_ws)
                output_str += "\n  Transmission corrected for beam spectrum"
            else:
                output_str += "\n  Transmission was NOT corrected for beam spectrum: inconsistent meta-data!"
        else:
            output_str += "\n  Transmission was NOT corrected for beam spectrum: check your normalization option!"
        
        if self._theta_dependent:
            # To apply the transmission correction using the theta-dependent algorithm
            # we should get the beam spectrum out of the measured transmission
            # We should then re-apply it when performing normalization
            ApplyTransmissionCorrection(workspace, workspace, transmission_ws)
        else:
            Divide(workspace, transmission_ws, workspace)
        #ReplaceSpecialValues(workspace, workspace, NaNValue=0.0,NaNError=0.0)
        
        # Clean up 
        for ws in ws_for_deletion:
            if mtd.workspaceExists(ws):
                mtd.deleteWorkspace(ws)
                
        return output_str
    
    
class SubtractDarkCurrent(ReductionStep):
    """
        Subtract the dark current from the input workspace.
        Works only if the proton charge time series is available from DASlogs.
    """
    def __init__(self, dark_current_file):
        super(SubtractDarkCurrent, self).__init__()
        self._dark_current_file = dark_current_file
        self._dark_current_ws = None
        
    def execute(self, reducer, workspace):
        """
            Subtract the dark current from the input workspace.
            If no timer workspace is provided, the counting time will be extracted
            from the input workspace.
            
            @param reducer: Reducer object for which this step is executed
            @param workspace: input workspace
        """
        # Sanity check
        if self._dark_current_file is None:
            raise RuntimeError, "SubtractDarkCurrent called with no defined dark current file"

        # Check whether the dark current was already loaded, otherwise load it
        # Load dark current, which will be used repeatedly
        if self._dark_current_ws is None:
            filepath = find_data(self._dark_current_file, instrument=reducer.instrument.name())
            self._dark_current_ws = "__dc_"+extract_workspace_name(filepath)
            reducer._data_loader.clone(data_file=filepath).execute(reducer, self._dark_current_ws)
        # Normalize the dark current data to counting time
        dark_duration = mtd[self._dark_current_ws].getRun()["proton_charge"].getStatistics().duration
        duration = mtd[workspace].getRun()["proton_charge"].getStatistics().duration
        scaling_factor = duration/dark_duration
    
        # Scale the stored dark current by the counting time
        scaled_dark_ws = "__scaled_dark_current"
        RebinToWorkspace(WorkspaceToRebin=self._dark_current_ws, WorkspaceToMatch=workspace, OutputWorkspace=scaled_dark_ws)
        Scale(InputWorkspace=scaled_dark_ws, OutputWorkspace=scaled_dark_ws, Factor=scaling_factor, Operation="Multiply")
        
        # Perform subtraction
        Minus(workspace, scaled_dark_ws, workspace)  

        # Clean up 
        if mtd.workspaceExists(scaled_dark_ws):
            mtd.deleteWorkspace(scaled_dark_ws)
        
        return "Dark current subtracted [%s]" % extract_workspace_name(self._dark_current_file)
    
class AzimuthalAverageByFrame(WeightedAzimuthalAverage):
    """
        ReductionStep class that performs azimuthal averaging
        and transforms the 2D reduced data set into I(Q).
        Done for each frame independently.
    """
    def __init__(self, binning=None, suffix="_Iq", error_weighting=False, n_bins=100, n_subpix=1, log_binning=False, scale=False):
        super(AzimuthalAverageByFrame, self).__init__(binning, suffix, error_weighting, n_bins, n_subpix, log_binning)
        self._is_frame_skipping = False
        self._scale = scale
        self._tolerance = 2.0
        self._compute_resolution = False
        self._sample_aperture_radius = 5.0
        
    def compute_resolution(self, sample_aperture_diameter=10.0):
        """
            Sets the flag to compute the Q resolution
            @param sample_aperture_diameter: diameter of the sample aperture, in mm
        """
        self._compute_resolution = True
        self._sample_aperture_radius = sample_aperture_diameter/2.0
        
    def get_output_workspace(self, workspace):
        if not self._is_frame_skipping:
            return workspace+self._suffix
        return [workspace+'_frame1'+self._suffix, workspace+'_frame2'+self._suffix]
        
    def execute(self, reducer, workspace):
        # We will need the pixel dimensions to compute the Q resolution        
        pixel_size_x = mtd[workspace].getInstrument().getNumberParameter("x-pixel-size")[0]
        pixel_size_y = mtd[workspace].getInstrument().getNumberParameter("y-pixel-size")[0]
        
        # Get the source aperture radius
        source_aperture_radius = 10.0
        if mtd[workspace].getRun().hasProperty("source-aperture-diameter"):
            source_aperture_radius = mtd[workspace].getRun().getProperty("source-aperture-diameter").value/2.0

        if mtd[workspace].getRun().hasProperty("is_frame_skipping") \
            and mtd[workspace].getRun().getProperty("is_frame_skipping").value==0:
            self._is_frame_skipping = False
            output_str = super(AzimuthalAverageByFrame, self).execute(reducer, workspace)
            if self._compute_resolution:
                EQSANSResolution(InputWorkspace=workspace+self._suffix, 
                                  ReducedWorkspace=workspace, OutputBinning=self._binning,
                                  PixelSizeX=pixel_size_x, PixelSizeY=pixel_size_y,
                                  SourceApertureRadius=source_aperture_radius,
                                  SampleApertureRadius=self._sample_aperture_radius)
            return output_str
        
        self._is_frame_skipping = True
        
        # Execution computes the binning if it's initially set to None,
        # so we keep track of it to make sure it's properly passed to both frames
        binning = self._binning
        
        # Second frame
        wl_min = None
        wl_max = None
        if mtd[workspace].getRun().hasProperty("wavelength_min_frame2"):
            wl_min = mtd[workspace].getRun().getProperty("wavelength_min_frame2").value
        if mtd[workspace].getRun().hasProperty("wavelength_max_frame2"):
            wl_max = mtd[workspace].getRun().getProperty("wavelength_max_frame2").value
        if wl_min is None and wl_max is None:
            raise RuntimeError, "Could not get the wavelength band for frame 2"
        CropWorkspace(workspace, workspace+'_frame2', XMin=wl_min, XMax=wl_max)
        
        output_str = "Performed radial averaging: frame 2 = [%6.1f, %-6.1f], " % (wl_min, wl_max)
        
        super(AzimuthalAverageByFrame, self).execute(reducer, workspace+'_frame2')
        if self._compute_resolution:
            EQSANSResolution(InputWorkspace=workspace+'_frame2'+self._suffix, 
                              ReducedWorkspace=workspace, OutputBinning=self._binning,
                              MinWavelength=wl_min, MaxWavelength=wl_max,
                              PixelSizeX=pixel_size_x, PixelSizeY=pixel_size_y,
                              SourceApertureRadius=source_aperture_radius,
                              SampleApertureRadius=self._sample_aperture_radius)                                                                        
            
        # Reset binning
        self._binning = binning

        # First frame
        wl_min = None
        wl_max = None
        if mtd[workspace].getRun().hasProperty("wavelength_min"):
            wl_min = mtd[workspace].getRun().getProperty("wavelength_min").value
        if mtd[workspace].getRun().hasProperty("wavelength_max"):
            wl_max = mtd[workspace].getRun().getProperty("wavelength_max").value
        if wl_min is None and wl_max is None:
            raise RuntimeError, "Could not get the wavelength band for frame 1"
        CropWorkspace(workspace, workspace+'_frame1', XMin=wl_min, XMax=wl_max)
        
        output_str += "frame 1 = [%6.1f, %-6.1f]" % (wl_min, wl_max)
        
        super(AzimuthalAverageByFrame, self).execute(reducer, workspace+'_frame1')
        
        # Workspace operations do not keep Dx, so scale frame 1 before putting
        # in the Q resolution
        # Scale frame 1 to frame 2
        if self._scale:
            q_min = min(mtd[workspace+'_frame1'+self._suffix].dataX(0))
            q_max = max(mtd[workspace+'_frame2'+self._suffix].dataX(0))
            rebin_pars = "%s,0.001,%s" % (q_min, q_max)
            Rebin(InputWorkspace=workspace+'_frame1'+self._suffix, OutputWorkspace="__frame1_rebinned", Params=rebin_pars)
            Rebin(InputWorkspace=workspace+'_frame2'+self._suffix, OutputWorkspace="__frame2_rebinned", Params=rebin_pars)
            iq_f1 = mtd["__frame1_rebinned"].dataY(0)
            iq_f2 = mtd["__frame2_rebinned"].dataY(0)
            
            scale_f1 = 0.0
            scale_f2 = 0.0
            scale_factor = 1.0
            for i in range(len(iq_f1)):
                if iq_f1[i]>0 and iq_f2[i]>0:
                    scale_f1 += iq_f1[i]
                    scale_f2 += iq_f2[i]
            if scale_f1>0 and scale_f2>0:
                scale_factor = scale_f2/scale_f1
            
                scale_f1 = 0.0
                scale_f2 = 0.0
                scale_factor = 1.0
                for i in range(len(iq_f1)):
                    if iq_f1[i]>0 and iq_f2[i]>0 \
                    and scale_factor*iq_f1[i]/iq_f2[i]<self._tolerance \
                    and iq_f2[i]/(scale_factor*iq_f1[i])<self._tolerance:
                        scale_f1 += iq_f1[i]
                        scale_f2 += iq_f2[i]
                if scale_f1>0 and scale_f2>0:
                    scale_factor = scale_f2/scale_f1
            
            Scale(InputWorkspace=workspace+'_frame1'+self._suffix, OutputWorkspace=workspace+'_frame1'+self._suffix, Factor=scale_factor, Operation="Multiply")
            
        if self._compute_resolution:
            EQSANSResolution(InputWorkspace=workspace+'_frame1'+self._suffix, 
                             ReducedWorkspace=workspace, OutputBinning=self._binning,
                             MinWavelength=wl_min, MaxWavelength=wl_max,
                             PixelSizeX=pixel_size_x, PixelSizeY=pixel_size_y,
                             SourceApertureRadius=source_aperture_radius,
                             SampleApertureRadius=self._sample_aperture_radius)       
                    
        # Add output workspaces to the list of important output workspaces
        for item in self.get_output_workspace(workspace):
            if item in reducer.output_workspaces:
                reducer.output_workspaces.remove(item)
        reducer.output_workspaces.append(self.get_output_workspace(workspace))                

        # Clean up 
        for ws in ["__frame1_rebinned", "__frame2_rebinned", workspace+'_frame1', workspace+'_frame2']:
            if mtd.workspaceExists(ws):
                mtd.deleteWorkspace(ws)
        
        return output_str
        
    def get_data(self, workspace):
        if not self._is_frame_skipping:
            return super(AzimuthalAverageByFrame, self).get_data(workspace)
        
        class DataSet(object):
            x=[]
            y=[]
            dy=[]
        
        d = DataSet()
        d.x = mtd[self.get_output_workspace(workspace)[0]].dataX(0)[1:]
        d.y = mtd[self.get_output_workspace(workspace)[0]].dataY(0)
        d.dx = mtd[self.get_output_workspace(workspace)[0]].dataE(0)
        return d
        
class DirectBeamTransmission(SingleFrameDirectBeamTransmission):
    """
        Calculate transmission using the direct beam method
    """
    def __init__(self, sample_file, empty_file, beam_radius=3.0, theta_dependent=True, dark_current=None, 
                 use_sample_dc=False, combine_frames=True):
        super(DirectBeamTransmission, self).__init__(sample_file, empty_file, beam_radius, theta_dependent, 
                                                     dark_current, use_sample_dc)
        ## Whether of not we need to combine the two frames in frame-skipping mode when performing the fit
        self._combine_frames = combine_frames
        
    def set_combine_frames(self, combine_frames=True):
        self._combine_frames = combine_frames
        
    def get_transmission(self):
        #mtd.sendLogMessage("TOF SANS doesn't have a single zero-angle transmission. DirectBeamTransmission.get_transmission() should not be called.")
        return [0, 0]
        
    def execute(self, reducer, workspace):
        if self._combine_frames or \
            (mtd[workspace].getRun().hasProperty("is_frame_skipping") \
             and mtd[workspace].getRun().getProperty("is_frame_skipping").value==0):
            return super(DirectBeamTransmission, self).execute(reducer, workspace)
    
        output_str = ""
        if self._transmission_ws is None:
            self._transmission_ws = "transmission_fit_"+workspace
            # Load data files
            sample_mon_ws, empty_mon_ws, first_det, output_str = self._load_monitors(reducer, workspace)
            
            def _crop_and_compute(wl_min_prop, wl_max_prop, suffix):
                # Get the wavelength band from the run properties
                if mtd[workspace].getRun().hasProperty(wl_min_prop):
                    wl_min = mtd[workspace].getRun().getProperty(wl_min_prop).value
                else:
                    raise RuntimeError, "DirectBeamTransmission could not retrieve the %s property" % wl_min_prop
                
                if mtd[workspace].getRun().hasProperty(wl_max_prop):
                    wl_max = mtd[workspace].getRun().getProperty(wl_max_prop).value
                else:
                    raise RuntimeError, "DirectBeamTransmission could not retrieve the %s property" % wl_max_prop
                
                CropWorkspace(workspace, workspace+suffix, XMin=wl_min, XMax=wl_max)
                CropWorkspace(sample_mon_ws, sample_mon_ws+suffix, XMin=wl_min, XMax=wl_max)
                CropWorkspace(empty_mon_ws, empty_mon_ws+suffix, XMin=wl_min, XMax=wl_max)
                self._calculate_transmission(sample_mon_ws+suffix, empty_mon_ws+suffix, first_det, self._transmission_ws+suffix)
                RebinToWorkspace(self._transmission_ws+suffix, workspace, OutputWorkspace=self._transmission_ws+suffix)
                RebinToWorkspace(self._transmission_ws+suffix+'_unfitted', workspace, OutputWorkspace=self._transmission_ws+suffix+'_unfitted')
                return self._transmission_ws+suffix
                
            # First frame
            trans_frame_1 = _crop_and_compute("wavelength_min", "wavelength_max", "_frame1")
            
            # Second frame
            trans_frame_2 = _crop_and_compute("wavelength_min_frame2", "wavelength_max_frame2", "_frame2")
            
            Plus(trans_frame_1, trans_frame_2, self._transmission_ws)
            Plus(trans_frame_1+'_unfitted', trans_frame_2+'_unfitted', self._transmission_ws+'_unfitted')

            # Clean up            
            for ws in [trans_frame_1, trans_frame_2, 
                       trans_frame_1+'_unfitted', trans_frame_2+'_unfitted',
                       sample_mon_ws, empty_mon_ws]:
                if mtd.workspaceExists(ws):
                    mtd.deleteWorkspace(ws)
            
        # Add output workspace to the list of important output workspaces
        reducer.output_workspaces.append([self._transmission_ws, self._transmission_ws+'_unfitted'])
            
        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        if self._theta_dependent:
            ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                        TransmissionWorkspace=self._transmission_ws, 
                                        OutputWorkspace=workspace)          
        else:
            Divide(workspace, self._transmission_ws, workspace)  
        
        return "Transmission correction applied for both frame independently [%s]\n%s\n" % (self._transmission_ws, output_str)
    
class SaveIqAscii(BaseSaveIqAscii):
    """
        Save the reduced data to ASCII files
    """
    def __init__(self):
        super(SaveIqAscii, self).__init__()
        
    def execute(self, reducer, workspace):
        log_text = super(SaveIqAscii, self).execute(reducer, workspace)
        
        # Determine which directory to use
        output_dir = reducer._data_path
        if reducer._output_path is not None:
            if os.path.isdir(reducer._output_path):
                output_dir = reducer._output_path
            else:
                raise RuntimeError, "SaveIqAscii could not save in the following directory: %s" % reducer._output_path
            
        if reducer._two_dim_calculator is not None:
            if not hasattr(reducer._two_dim_calculator, "get_output_workspace"):
                
                for output_ws in [workspace+'_Iqxy', workspace+'_frame1_Iqxy', workspace+'_frame2_Iqxy']:
                    if mtd.workspaceExists(output_ws):
                        filename = os.path.join(output_dir, output_ws+'.dat')
                        SaveNISTDAT(InputWorkspace=output_ws, Filename=filename)
                        
                        if len(log_text)>0:
                            log_text += '\n'
                        log_text += "I(Qx,Qy) for %s saved in %s" % (output_ws, filename)
            
        return log_text
            
class SensitivityCorrection(BaseSensitivityCorrection):
    """
        Perform sensitivity correction as a function of wavelength
    """
    def __init__(self, flood_data, min_sensitivity=0.5, max_sensitivity=1.5, dark_current=None, 
                 beam_center=None, use_sample_dc=False):
        super(SensitivityCorrection, self).__init__(flood_data=flood_data, 
                                                    min_sensitivity=min_sensitivity, max_sensitivity=max_sensitivity, 
                                                    dark_current=dark_current, beam_center=beam_center, use_sample_dc=use_sample_dc)
    
    def execute(self, reducer, workspace):
        # Perform standard sensitivity correction
        # If the sensitivity correction workspace exists, just apply it. Otherwise create it.      
        output_str = "   Using data set: %s" % extract_workspace_name(self._flood_data)
        if self._efficiency_ws is None:
            self._compute_efficiency(reducer, workspace)
            
        # Modify for wavelength dependency of the efficiency of the detector tubes
        EQSANSSensitivityCorrection(InputWorkspace=workspace, EfficiencyWorkspace=self._efficiency_ws,
                                    Factor=0.95661, Error=0.005, OutputWorkspace=workspace,
                                    OutputEfficiencyWorkspace="__wl_efficiency")
        
        # Copy over the efficiency's masked pixels to the reduced workspace
        masked_detectors = GetMaskedDetectors(self._efficiency_ws)
        MaskDetectors(workspace, None, masked_detectors.getPropertyValue("DetectorList"))        
        
        return "Wavelength-dependent sensitivity correction applied\n%s\n" % output_str
        