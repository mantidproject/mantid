"""
    Load EQSANS data file and perform TOF corrections, move detector to beam center, etc...
"""
import os
import sys
import pickle
import math
from reduction import ReductionStep
from reduction import extract_workspace_name, find_file, find_data
from eqsans_config import EQSANSConfig
from sans_reduction_steps import BaseBeamFinder

# Mantid imports
from mantidsimple import *

class LoadRun(ReductionStep):
    """
        Load a data file, move its detector to the right position according
        to the beam center and normalize the data.
    """
    def __init__(self, datafile=None, keep_events=False, is_new=False):
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
        self._use_config = True
        
        # Workspace on which to apply correction that should be done
        # independently of the pixel. If False, all correction will be 
        # applied directly to the data workspace.
        self._separate_corrections = False
        
        self._sample_det_dist = None
        self._sample_det_offset = 0
        
        # Flag to tell us whether we should do the full reduction with events
        self._keep_events = keep_events
        self._is_new = is_new
   
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
        loader._is_new = self._is_new
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
            if mtd.workspaceExists(trans_ws):
                mtd.deleteWorkspace(trans_ws)
        if mtd.workspaceExists(workspace):
            mtd.deleteWorkspace(workspace)
        if mtd.workspaceExists(workspace+'_evt'):
            mtd.deleteWorkspace(workspace+'_evt')
        
    def _get_source_slit_size(self, reducer, workspace, config=None):
        """
            Determine the source aperture by reading in the slit settings from 
            the log
            @param reducer: Reducer object
            @param workspace: name of workspace being processed
            @param config: EQSANSConfig object 
        """
        if config is None:
            return "   Could not find source aperture\n"
        
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
        if self._keep_events and self._is_new:
            return self.new_execute(reducer, workspace, force)
        else:
            return self.old_execute(reducer, workspace, force)
                
    def new_execute(self, reducer, workspace, force=False):
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

        # Load data
        use_config_beam = False
        [pixel_ctr_x, pixel_ctr_y] = reducer.get_beam_center()
        if pixel_ctr_x == 0.0 and pixel_ctr_y == 0.0:
            use_config_beam = True            
            
        def _load_data_file(file_name, wks_name):
            filepath = find_data(file_name, instrument=reducer.instrument.name())
            l = EQSANSLoad(Filename=filepath, OutputWorkspace=wks_name,
                       UseConfigBeam=use_config_beam,
                       BeamCenterX=pixel_ctr_x,
                       BeamCenterY=pixel_ctr_y,
                       UseConfigTOFCuts=self._use_config_cutoff,
                       UseConfigMask=self._use_config_mask,
                       UseConfig=self._use_config,
                       CorrectForFlightPath=self._correct_for_flight_path,
                       SampleDetectorDistance=self._sample_det_dist,
                       SampleDetectorDistanceOffset=self._sample_det_offset
                       )            
            return l.getPropertyValue("OutputMessage")
        
        # Check whether we have a list of files that need merging
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
            if mtd.workspaceExists('__tmp_wksp'):
                mtd.deleteWorkspace('__tmp_wksp')
        else:
            output_str += "Loaded %s\n" % data_file
            output_str += _load_data_file(data_file, workspace)
        
        mantid[workspace].getRun().addProperty_str("event_ws", workspace, True)
        
        if mtd[workspace].getRun().hasProperty("beam_center_x") and \
            mtd[workspace].getRun().hasProperty("beam_center_y"):
            beam_center_x = mtd[workspace].getRun().getProperty("beam_center_x").value
            beam_center_y = mtd[workspace].getRun().getProperty("beam_center_y").value
            if type(reducer._beam_finder) is BaseBeamFinder:
                reducer.set_beam_finder(BaseBeamFinder(beam_center_x, beam_center_y))
                mantid.sendLogMessage("No beam finding method: setting to default [%-6.1f, %-6.1f]" % (beam_center_x, beam_center_y))
        
        # Remove the dirty flag if it existed
        reducer.clean(workspace)
        mtd[workspace].getRun().addProperty_int("loaded_by_eqsans_reduction", 1, True)
        
        return output_str
        
    def old_execute(self, reducer, workspace, force=False):
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
            if self._use_config:
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
        output_str += "   Detector position: %-6.3f\n" % (sdd/1000.0)

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
                mantid.sendLogMessage("Could not find run number for %s" % workspace)
            
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
            
            if conf.prompt_pulse_width is not None and conf.prompt_pulse_width>0:
                mtd[workspace+'_evt'].getRun().addProperty_dbl("prompt_pulse_width", conf.prompt_pulse_width, "microsecond", True)  
                prompt_pulse_width = conf.prompt_pulse_width
                
            if self._use_config_cutoff:
                mtd[workspace+'_evt'].getRun().addProperty_dbl("low_tof_cut", conf.low_TOF_cut, "microsecond", True)
                mtd[workspace+'_evt'].getRun().addProperty_dbl("high_tof_cut", conf.high_TOF_cut, "microsecond", True)
                low_TOF_cut = conf.low_TOF_cut
                high_TOF_cut = conf.high_TOF_cut
                
            # Store mask information
            if self._use_config_mask:
                mtd[workspace+'_evt'].getRun().addProperty_str("rectangular_masks", pickle.dumps(conf.rectangular_masks), "pixels", True)
                output_str +=  "   Using mask information found in configuration file\n"
                
            if type(reducer._beam_finder) is BaseBeamFinder:
                if reducer.get_beam_center() == [0.0,0.0]:
                    reducer.set_beam_finder(BaseBeamFinder(conf.center_x, conf.center_y))   
                    output_str += "   Beam center set from config file: %-6.2f, %-6.2f\n" % (conf.center_x, conf.center_y)          
                    
            # Modify moderator position
            if conf.moderator_position is not None:
                sample_to_mod = -conf.moderator_position/1000.0
                MoveInstrumentComponent(workspace+'_evt', "moderator", 
                                        Z=sample_to_mod,
                                        RelativePosition="0")
                mtd[workspace+'_evt'].getRun().addProperty_dbl("moderator_position", sample_to_mod, "mm", True)  
                output_str +=  "   Moderator position: %-6.3f m\n" % sample_to_mod
        elif self._use_config:
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
            MoveInstrumentComponent(workspace+'_evt', "detector1", 
                                    X = -x_offset-beam_ctr_x,
                                    Y = -y_offset-beam_ctr_y,
                                    RelativePosition="1")
            mtd[workspace+'_evt'].getRun().addProperty_dbl("beam_center_x", pixel_ctr_x, 'pixel', True)            
            mtd[workspace+'_evt'].getRun().addProperty_dbl("beam_center_y", pixel_ctr_y, 'pixel', True)   
            mantid.sendLogMessage("Beam center: %-6.1f, %-6.1f" % (pixel_ctr_x, pixel_ctr_y))    
            output_str += "   Beam center offset: %g, %g m\n" % (x_offset, y_offset)
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
            output_str += "   Flight path correction applied\n"
        else:
            output_str += "   Flight path correction NOT applied\n"
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
        output_str += "   Wavelength range: %6.1f - %-6.1f " % (wl_min, wl_max)
        if frame_skipping:
            wl_min2 = float(a.getPropertyValue("WavelengthMinFrame2"))
            wl_max2 = float(a.getPropertyValue("WavelengthMaxFrame2"))
            mtd[workspace+'_evt'].getRun().addProperty_dbl("wavelength_min_frame2", wl_min2, "Angstrom", True)
            mtd[workspace+'_evt'].getRun().addProperty_dbl("wavelength_max_frame2", wl_max2, "Angstrom", True)
            wl_combined_max = wl_max2
            output_str += "and %6.1f - %-6.1f Angstrom\n" % (wl_min2, wl_max2)
        else:
            output_str += "Angstrom\n"
        
        # Remove prompt pulses
        #RemovePromptPulse(workspace+'_evt', workspace+'_evt', Width=prompt_pulse_width)
        
        # Convert TOF to wavelength
        source_to_sample = math.fabs(mtd[workspace+'_evt'].getInstrument().getSource().getPos().getZ())*1000.0
        conversion_factor = 3.9560346 / (sdd+source_to_sample);
        output_str += "   TOF to wavelength conversion factor: %g\n" % conversion_factor
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
            if self._data_file is None:    
                mantid[workspace].getRun().addProperty_int("is_separate_corrections", 1, True)
                
                # Create workspace of pixel weights
                nhist = mtd[workspace].getNumberHistograms()
                xvec = mtd[workspace].readX(0)
                yvec = nhist*[1]
                evec = nhist*[0]
                ws_pix = "__pixel_adj_"+workspace
                CreateWorkspace(OutputWorkspace=ws_pix,
                                DataX=[min(xvec), max(xvec)], DataY=yvec, DataE=evec, NSpec=nhist)
                mantid[workspace].getRun().addProperty_str("pixel_adj", ws_pix, True)
                
                # Create workspace of wavelength weights
                ws_wl = "__wl_adj_"+workspace
                yvec = (len(xvec)-1)*[1]
                evec = (len(xvec)-1)*[0]
                CreateWorkspace(OutputWorkspace=ws_wl, UnitX="Wavelength",
                                DataX=xvec, DataY=yvec, DataE=evec, NSpec=1)
                mantid[workspace].getRun().addProperty_str("wl_adj", ws_wl, True)
                    
        # Remove the dirty flag if it existed
        reducer.clean(workspace)
        mtd[workspace].getRun().addProperty_int("loaded_by_eqsans_reduction", 1, True)
        
        return "Data file loaded: %s\n%s" % (workspace, output_str)
