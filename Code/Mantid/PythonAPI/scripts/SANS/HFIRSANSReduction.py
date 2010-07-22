"""
    Test module for HFIR SANS reduction
    
    WARNING: this is not meant to be production code... 
"""
# Mantid imports
from mantidsimple import *

# Python import
import os, sys, math

## Version number
__version__ = 0.1


def _extract_workspace_name(filepath, suffix=''):
    """
        Returns a default workspace name for a given data file path.
        
        @param filepath: path of the file to generate a workspace name for
        @param suffix: string to append to name
    """
    (head, tail) = os.path.split(filepath)
    basename, extension = os.path.splitext(tail)
    
    #TODO: check whether the workspace name is already in use
    #      and modify it if it is. 

    return basename+suffix

class InstrumentConfiguration:
    """
        Information tied to the instrument and the set of data files to
        be reduced
    """
    
    # Beam center finding methods
    BEAM_CENTER_NONE        = 0
    BEAM_CENTER_DIRECT_BEAM = 1
    BEAM_CENTER_SCATTERING  = 2
    
    def __init__(self):
        """
            Initialization
            TODO: most of those values should be read from the data file
        """
        ## Number of detector pixels in X
        self.nx_pixels = 192
        ## Number of detector pixels in Y
        self.ny_pixels = 192
        ## Number of counters. This is the number of detector channels (spectra) before the
        # data channels start
        self.nMonitors = 2
        ## Pixel size in mm
        self.pixel_size_x = 5.15
        self.pixel_size_y = 5.15
        ## Beam center [either set by hand or find]
        self.beam_center_method = InstrumentConfiguration.BEAM_CENTER_NONE
        self.beam_center_filepath = None
        self.beam_center_beam_radius = 20
        self.beam_center_x = 16
        self.beam_center_y = 95
        ## Sample-to-detector distance in mm
        self.sample_detector_distance = 6000
        ## Detector name
        self.detector_ID = 'detector1'
    
    def get_masked_pixels(self, nx_low, nx_high, ny_low, ny_high):
        """
            Generate a list of masked pixels.
            @param nx_low: number of pixels to mask on the lower-x side of the detector
            @param nx_high: number of pixels to mask on the higher-x side of the detector
            @param ny_low: number of pixels to mask on the lower-y side of the detector
            @param ny_high: number of pixels to mask on the higher-y side of the detector
            
            TODO: hide this in an instrument-specific mask implementation
        """
        masked_x = range(0, nx_low)
        masked_x.extend(range(self.nx_pixels-nx_high, self.nx_pixels))

        masked_y = range(0, ny_low)
        masked_y.extend(range(self.ny_pixels-ny_high, self.ny_pixels))
        
        masked_pts = []
        for y in masked_y:
            masked_pts.extend([ [x,y] for x in masked_x ])
        
        return masked_pts
    
    def get_masked_detectors(self, masked_pixels):
        """
            Returns a list of masked detector channels from the list of pixels.
            This is very instrument-dependent, because it depends on the actual
            mapping of the detector pxiels to Mantid detector IDs.
            TODO: think about a way to clean this up
        """
        return [ 1000000 + p[0]*1000 + p[1] for p in masked_pixels ]
    
    def get_spectra_from_ID(self, ids):
        """
            Returns a list of spectra from a list of detector IDs
            TODO: think about a way to clean this up
        """
        def _get_spec(id):
            iy = (id-1000000)%1000
            ix = int(math.floor(((id-1000000)-iy)/1000))
            return ix+iy*self.ny_pixels
        return map(_get_spec, ids)
    
    def get_monitor_UDET(self, spectraID):
        """
            Returns the detector ID of a monitor spectra
            TODO: Fix this
        """
        return spectraID+1
        
        
class SANSReductionMethod:
    """
        Defines the reduction steps and parameters. An object from this class
        should completely define the reduction to be applied to raw data.
    """
    
    ## Normalization options
    NORMALIZATION_NONE = None
    NORMALIZATION_TIME = 1
    NORMALIZATION_MONITOR = 0
    
    ## Transmission options
    TRANSMISSION_NONE = None
    TRANSMISSION_BY_HAND = 1
    TRANSMISSION_DIRECT_BEAM = 2
    TRANSMISSION_BEAM_SPREADER = 3

    def __init__(self):
        """
            Initialization
        """
        ## Dark current
        self.dark_current_filepath = None
        ## Normalization detector for dark current (should always be time)
        self.dark_normalization = SANSReductionMethod.NORMALIZATION_TIME
        
        ## Background data
        self.background_filepath = None
        
        
        ## Normalization counter
        self.normalization = SANSReductionMethod.NORMALIZATION_TIME
        
        
        # Mask parameters #####################################################
        # TODO: to allow for instruments with multiple PSDs, we might want to 
        # change this to a Mask class that takes care of the instrument-specific junk. 
        self.mask_nx_high = 0
        self.mask_nx_low  = 0
        self.mask_ny_high = 0
        self.mask_ny_low  = 0
        self.mask_user_defined = []
        

        # Sensitivity correction parameters ###################################
        
        ## Flood data for sensitivity correction
        self.sensitivity_flood_filepath = None
        ## Dark current file for sensitivity correction
        self.sensitivity_use_dark_current = False
        ## Highest allowed sensitivity, above that pixels will be masked 
        self.sensitivity_high = None
        ## Lowest allowed sensitivity, below that pixels will be masked
        self.sensitivity_low = None


        # Transmission parameters #############################################
        ## Number of pixels around the center that define the transmission monitor
        self.transmission_method = SANSReductionMethod.TRANSMISSION_NONE
        self.transmission_radius = 3.0
        self.transmission_sample_filepath = None
        self.transmission_empty_filepath = None
        self.transmission_use_dark_current = False
        

class SANSReduction:
    
    def __init__(self, filepath, method, configuration, workspace=None):
        """
            @param filepath: path of the file to reduce
            @param method: SANSReductionMethod object
            @param configuration: InstrumentConfiguration object
            @param workspace: name of output workspace for raw data
        """
        ## Path of file to reduce
        self.data_filepath = filepath
        
        ## Name of output workspace for loaded data
        self.workspace = workspace
        if self.workspace == None:
            self.workspace = self._extract_workspace_name()
            
        ## Reduced workspace
        self.reduced_ws = self.workspace+"_reduced"
        
        ## Reduction method
        self.method = method
        
        ## Configuration
        self.configuration = configuration
        
        # Internal data members
        self._dark_ws = None
        
        
        
    def reduce(self):
        """
            Perform HFIR SANS reduction:
            
            - Subtract dark current
            - Normalize data
            - Apply 2D pixel mask
            - Apply pixel sensitivity correction
            - Apply transmission correction
            - Correct and subtract background data
            - Perform radial average and transform to I(q)
        """
        # Load data
        LoadSpice2D(self.data_filepath, self.workspace)
        
        # Make a copy that will be our reduced data
        CloneWorkspace(self.workspace, self.reduced_ws)
        
        # Find beam center position. If no beam finding option was provided, use the current value
        if self.configuration.beam_center_method is not InstrumentConfiguration.BEAM_CENTER_NONE:
            # Check whether the direct beam or scattering pattern option was selected
            direct_beam = (self.configuration.beam_center_method is InstrumentConfiguration.BEAM_CENTER_DIRECT_BEAM)
            # Load the file to extract the beam center from, and process it.
            LoadSpice2D(self.configuration.beam_center_filepath, "beam_center")
            beam_center = FindCenterOfMassPosition("beam_center",
                                                   Output = None,
                                                   NPixelX=self.configuration.nx_pixels,
                                                   NPixelY=self.configuration.ny_pixels,
                                                   DirectBeam = direct_beam,
                                                   BeamRadius = self.configuration.beam_center_beam_radius)
            ctr_str = beam_center.getPropertyValue("CenterOfMass")
            ctr = ctr_str.split(',')
            self.configuration.beam_center_x = float(ctr[0])
            self.configuration.beam_center_y = float(ctr[1])
    
        # Apply corrections to sample data        
        self._apply_corrections(self.reduced_ws)
        
        # Subtract background if applicable
        if self.method.background_filepath is not None:
            bck_ws = _extract_workspace_name(self.method.background_filepath)
            LoadSpice2D(self.method.background_filepath, bck_ws)
        
            # Apply correction to background
            self._apply_corrections(bck_ws)
            
            # Subtract corrected background
            Minus(self.reduced_ws, bck_ws, self.reduced_ws)
            
        # Transform to I(q)
        Q1DWeighted(self.reduced_ws, "Iq", "0.01,0.001,0.11", 
                    PixelSizeX=self.configuration.pixel_size_x,
                    PixelSizeY=self.configuration.pixel_size_y, ErrorWeighting=True)        
    
    def _apply_corrections(self, ws):
        """
            Apply the data corrections to a given workspace. The input workspace 
            will be modified.
            @param ws: workspace to apply corrections to
            
            TODO: since the input files will be the same, read them only once
        """
        # Move detector array to correct position
        MoveInstrumentComponent(ws, self.configuration.detector_ID, 
                                X = -(self.configuration.beam_center_x-self.configuration.nx_pixels/2.0+0.5) * self.configuration.pixel_size_x/1000.0, 
                                Y = -(self.configuration.beam_center_y-self.configuration.ny_pixels/2.0+0.5) * self.configuration.pixel_size_y/1000.0, 
                                Z = self.configuration.sample_detector_distance/1000.0,
                                RelativePosition="1")
        # Get counting time
        timer_ws = self.workspace+"_timer"
        CropWorkspace(self.workspace, timer_ws,
                      StartWorkspaceIndex = str(SANSReductionMethod.NORMALIZATION_TIME), 
                      EndWorkspaceIndex   = str(SANSReductionMethod.NORMALIZATION_TIME))        

        # Get monitor counts
        monitor_ws = self.workspace+"_monitor"
        CropWorkspace(self.workspace, monitor_ws,
                      StartWorkspaceIndex = str(SANSReductionMethod.NORMALIZATION_MONITOR), 
                      EndWorkspaceIndex   = str(SANSReductionMethod.NORMALIZATION_MONITOR))


        # Subtract dark current ###############################################
        if self.method.dark_current_filepath is not None:
            self._subtract_dark_current(ws, ws, timer_ws)
            
        # Normalize data ######################################################
        if self.method.normalization == SANSReductionMethod.NORMALIZATION_MONITOR:
            # Normalize by monitor counts
            Divide(ws, monitor_ws, ws)
            
            # Result needs to be multiplied by 1e8
            Scale(ws, ws, 1.0e8, 'Multiply')
            
        elif self.method.normalization == SANSReductionMethod.NORMALIZATION_TIME:
            # Normalize by counting time
            Divide(ws, timer_ws, ws)
        
        # Apply mask  #########################################################
        
        # Get a list of detector pixels to mask
        masked_pixels = self.configuration.get_masked_pixels(self.method.mask_nx_low,
                                                             self.method.mask_nx_high,
                                                             self.method.mask_ny_low,
                                                             self.method.mask_ny_high)
        # Extend that list by a user defined list as needed
        masked_pixels.extend(self.method.mask_user_defined)
        
        # Transform the list of pixels into a list of Mantid detector IDs
        masked_detectors = self.configuration.get_masked_detectors(masked_pixels)
        
        # Mask the pixels by passing the list of IDs
        MaskDetectors(ws, None, masked_detectors)
        
        
        # Apply sensitivity correction ########################################
        
        # Set up the detector efficiency by reading in file and using 
        # "Method" parameters (Note: SASDetEffMaskMenu)
        
        # Load Flood data
        if self.method.sensitivity_flood_filepath is not None:
            flood_ws = _extract_workspace_name(self.method.sensitivity_flood_filepath)
            LoadSpice2D(self.method.sensitivity_flood_filepath, flood_ws)

            # Subtract dark current
            if self.method.sensitivity_use_dark_current:
                self._subtract_dark_current(flood_ws, flood_ws)
            
            # Move the flood data detector before applying the solid angle correction
            MoveInstrumentComponent(flood_ws, self.configuration.detector_ID, 
                                   X = -(self.configuration.beam_center_x-self.configuration.nx_pixels/2.0+0.5) * self.configuration.pixel_size_x/1000.0, 
                                   Y = -(self.configuration.beam_center_y-self.configuration.ny_pixels/2.0+0.5) * self.configuration.pixel_size_y/1000.0, 
                                   Z = self.configuration.sample_detector_distance/1000.0,
                                   RelativePosition="1")
            # Correct flood data for solid angle effects (Note: SA_Corr_2DSAS)
            SolidAngleCorrection(flood_ws, flood_ws)
        
            # Create efficiency profile: 
            # Divide each pixel by average signal, and mask high and low pixels.
            CalculateEfficiency(flood_ws, flood_ws, self.method.sensitivity_low, self.method.sensitivity_high)
            
            # Divide by detector efficiency
            Divide(self.reduced_ws, flood_ws, self.reduced_ws)
            
            # Copy over the efficiency's masked pixels to the reduced workspace
            masked_detectors = GetMaskedDetectors(flood_ws)
            MaskDetectors(self.reduced_ws, None, masked_detectors.getPropertyValue("DetectorList"))
            
        # Correct data for solid angle effects (Note: SA_Corr_2DSAS)
        SolidAngleCorrection(ws, ws)
          
        # Apply transmission correction #######################################
        if self.method.transmission_method==SANSReductionMethod.TRANSMISSION_DIRECT_BEAM:
            # 1- Compute zero-angle transmission correction (Note: CalcTransCoef)
            self._transmission_correction()
                    
            # 2- Apply correction (Note: Apply2DTransCorr)
            #Apply angle-dependent transmission correction using the zero-angle transmission
            ApplyTransmissionCorrection(InputWorkspace=self.reduced_ws, 
                                        TransmissionWorkspace="transmission_fit", 
                                        OutputWorkspace=self.reduced_ws)
            
        
        
    def _transmission_correction(self):
        """
            Compute the transmission using the attenuated direct beam method.
            
            Pixels around the beam center will be summed to create a transmission "monitor".
            
            Depending on the reduction method settings, the time or monitor channel
            will be used for normalization.             
        """
        sample_ws = _extract_workspace_name(self.method.transmission_sample_filepath)
        LoadSpice2D(self.method.transmission_sample_filepath, sample_ws)
        
        empty_ws = _extract_workspace_name(self.method.transmission_empty_filepath)
        LoadSpice2D(self.method.transmission_empty_filepath, empty_ws)
        
        if self.method.transmission_use_dark_current:
            self._subtract_dark_current(sample_ws, sample_ws)
            self._subtract_dark_current(empty_ws, empty_ws)
        
        # Find which pixels to sum up as our "monitor". At this point we have moved the detector
        # so that the beam is at (0,0), so all we need is to sum the area around that point.
        #TODO: in IGOR, the error-weighted average is computed instead of simply summing up the pixels
        cylXML = '<infinite-cylinder id="transmission_monitor">' + \
                   '<centre x="0.0" y="0.0" z="0.0" />' + \
                   '<axis x="0.0" y="0.0" z="1.0" />' + \
                   '<radius val="%12.10f" />' % (self.method.transmission_radius*self.configuration.pixel_size_x/1000.0) + \
                 '</infinite-cylinder>\n'
                 
        det_finder = FindDetectorsInShape(Workspace=self.reduced_ws, ShapeXML=cylXML)
        det_list = det_finder.getPropertyValue("DetectorList")
        
        first_det = int(det_list.split(',')[0])
        print "DETLIST", len(det_list.split(',')), det_list 
        
        GroupDetectors(InputWorkspace=empty_ws,  OutputWorkspace="empty_mon",  DetectorList=det_list, KeepUngroupedSpectra="1")
        GroupDetectors(InputWorkspace=sample_ws, OutputWorkspace="sample_mon", DetectorList=det_list, KeepUngroupedSpectra="1")
        
        #TODO: check that both workspaces have the same masked spectra
        
        # Calculate transmission. Use the reduction method's normalization channel (time or beam monitor)
        # as the monitor channel.
        CalculateTransmission(DirectRunWorkspace="empty_mon", SampleRunWorkspace="sample_mon", 
                              OutputWorkspace="transmission_fit",
                              IncidentBeamMonitor=str(self.method.normalization), 
                              TransmissionMonitor=str(first_det))
        
        
    def _subtract_dark_current(self, input_ws, output_ws, timer_ws=None):
        """
            Subtract the dark current from the input workspace.
            If no timer workspace is provided, the counting time will be extracted
            from the input workspace.
            
            @param input_ws: input workspace
            @param output_ws: output workspace
            @param timer_ws: if provided, will be used to scale the dark current
        """
        # Sanity check
        if self.method.dark_current_filepath is None:
            raise RuntimeError, "SANSReduction._subtract_dark_current: method called with no defined dark current"

        # Check whether the dark current was already loaded, otherwise load it
        # Load dark current, which will be used repeatedly
        if self._dark_ws is None:
            self._dark_ws = _extract_workspace_name(self.method.dark_current_filepath)
            LoadSpice2D(self.method.dark_current_filepath, self._dark_ws)
        
            # Normalize the dark current data to counting time
            if self.method.dark_normalization == SANSReductionMethod.NORMALIZATION_TIME:
                darktimer_ws = self._dark_ws+"_timer"
                CropWorkspace(self._dark_ws, darktimer_ws,
                              StartWorkspaceIndex = str(SANSReductionMethod.NORMALIZATION_TIME), 
                              EndWorkspaceIndex   = str(SANSReductionMethod.NORMALIZATION_TIME))        
                
                Divide(self._dark_ws, darktimer_ws, self._dark_ws)      
    
        # If no timer workspace was provided, get the counting time from the data
        if timer_ws is None:
            timer_ws = "tmp_timer"     
            CropWorkspace(input_ws, timer_ws,
                          StartWorkspaceIndex = str(SANSReductionMethod.NORMALIZATION_TIME), 
                          EndWorkspaceIndex   = str(SANSReductionMethod.NORMALIZATION_TIME))  
        # Scale the stored dark current by the counting time
        Multiply(self._dark_ws, timer_ws, "scaled_dark_current")
        # Perform subtraction
        Minus(input_ws, "scaled_dark_current", output_ws)
            
        
        
    def _extract_workspace_name(self, suffix=''):
        """
            Returns a default workspace name for a file path.
            @param suffix: string to append to name
        """
        return _extract_workspace_name(self.data_filepath, suffix)

