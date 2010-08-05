"""
    Implementation of reduction steps for SANS
"""

from Reducer import ReductionStep
from Reducer import extract_workspace_name

# Mantid imports
from mantidsimple import *
    
    
class BaseBeamFinder(ReductionStep):
    """
        Base beam finder. Holds the position of the beam center
        as well as the algorithm for finding it.
    """
    def __init__(self, beam_center_x=0.0, beam_center_y=0.0):
        super(BaseBeamFinder, self).__init__()
        self._beam_center_x = beam_center_x
        self._beam_center_y = beam_center_y
        
    def get_beam_center(self):
        return [self._beam_center_x, self._beam_center_y]
    
    def execute(self, reducer, workspace=None):
        pass
        
    def _find_beam(self, direct_beam, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        # Load the file to extract the beam center from, and process it.
        filepath = reducer._full_file_path(self._datafile)
        LoadSpice2D(filepath, "beam_center")
        
        beam_center = FindCenterOfMassPosition("beam_center",
                                               Output = None,
                                               NPixelX=reducer.instrument.nx_pixels,
                                               NPixelY=reducer.instrument.ny_pixels,
                                               DirectBeam = direct_beam,
                                               BeamRadius = self._beam_radius)
        ctr_str = beam_center.getPropertyValue("CenterOfMass")
        ctr = ctr_str.split(',')
        
        self._beam_center_x = float(ctr[0])
        self._beam_center_y = float(ctr[1])


class ScatteringBeamCenter(BaseBeamFinder):
    """
        Find the beam center using the scattering data
    """  
    def __init__(self, datafile, beam_radius=3):
        super(ScatteringBeamCenter, self).__init__()
        ## Location of the data file used to find the beam center
        self._datafile = datafile
        self._beam_radius = beam_radius
        
    def execute(self, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        super(ScatteringBeamCenter, self)._find_beam(False, reducer, workspace)

class DirectBeamCenter(BaseBeamFinder):
    """
        Find the beam center using the direct beam
    """  
    def __init__(self, datafile, beam_radius=3):
        super(DirectBeamCenter, self).__init__()
        ## Location of the data file used to find the beam center
        self._datafile = datafile
        self._beam_radius = beam_radius
        
    def execute(self, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        super(DirectBeamCenter, self)._find_beam(True, reducer, workspace)

class BaseTransmission(ReductionStep):
    """
        Base transmission. Holds the transmission value
        as well as the algorithm for calculating it.
    """
    def __init__(self, trans=0.0, error=0.0):
        super(BaseTransmission, self).__init__()
        self._trans = trans
        self._error = error
        
    def get_transmission(self):
        return [self._trans, self._error]
    
    def execute(self, reducer, workspace):
        ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                    TransmissionValue=self._trans,
                                    TransmissionError=self._error, 
                                    OutputWorkspace=workspace) 
        
  
class BeamSpreaderTransmission(BaseTransmission):
    """
        Calculate transmission using the beam-spreader method
    """
    def __init__(self, sample_spreader, direct_spreader,
                       sample_scattering, direct_scattering,
                       spreader_transmission=1.0, spreader_transmission_err=0.0): 
        super(BeamSpreaderTransmission, self).__init__()
        self._sample_spreader = sample_spreader
        self._direct_spreader = direct_spreader
        self._sample_scattering = sample_scattering
        self._direct_scattering = direct_scattering
        self._spreader_transmission = spreader_transmission
        self._spreader_transmission_err = spreader_transmission_err
        ## Transmission workspace (output of transmission calculation)
        self._transmission_ws = None
        
    def execute(self, reducer, workspace=None):
        """
            Calculate transmission and apply correction
            @param reducer: Reducer object for which this step is executed
            @param workspace: workspace to apply correction to
        """
        if self._transmission_ws is None:
            # 1- Compute zero-angle transmission correction (Note: CalcTransCoef)
            self._transmission_ws = "transmission_fit"
            
            sample_spreader_ws = "_trans_sample_spreader"
            filepath = reducer._full_file_path(self._sample_spreader)
            LoadSpice2D(filepath, sample_spreader_ws)
            
            direct_spreader_ws = "_trans_direct_spreader"
            filepath = reducer._full_file_path(self._direct_spreader)
            LoadSpice2D(filepath, direct_spreader_ws)
            
            sample_scatt_ws = "_trans_sample_scatt"
            filepath = reducer._full_file_path(self._sample_scattering)
            LoadSpice2D(filepath, sample_scatt_ws)
            
            direct_scatt_ws = "_trans_direct_scatt"
            filepath = reducer._full_file_path(self._direct_scattering)
            LoadSpice2D(filepath, direct_scatt_ws)
            
            # Subtract dark current
            if reducer._dark_current_subtracter is not None:
                reducer._dark_current_subtracter.execute(reducer, sample_spreader_ws)
                reducer._dark_current_subtracter.execute(reducer, direct_spreader_ws)
                reducer._dark_current_subtracter.execute(reducer, sample_scatt_ws)
                reducer._dark_current_subtracter.execute(reducer, direct_scatt_ws)
            
            # Get normalization for transmission calculation
            norm_spectrum = reducer.NORMALIZATION_TIME
            if reducer._normalizer is not None:
                norm_spectrum = reducer._normalizer.get_normalization_spectrum()
            
            # Calculate transmission. Use the reduction method's normalization channel (time or beam monitor)
            # as the monitor channel.
            CalculateTransmissionBeamSpreader(SampleSpreaderRunWorkspace=sample_spreader_ws, 
                                              DirectSpreaderRunWorkspace=direct_spreader_ws,
                                              SampleScatterRunWorkspace=sample_scatt_ws, 
                                              DirectScatterRunWorkspace=direct_scatt_ws, 
                                              OutputWorkspace=self._transmission_ws,
                                              SpreaderTransmissionValue=str(self._spreader_transmission), 
                                              SpreaderTransmissionError=str(self._spreader_transmission_err))

        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                    TransmissionWorkspace=self._transmission_ws, 
                                    OutputWorkspace=workspace)                
            
            
class DirectBeamTransmission(BaseTransmission):
    """
        Calculate transmission using the direct beam method
    """
    def __init__(self, sample_file, empty_file, beam_radius=3.0):
        super(DirectBeamTransmission, self).__init__()
        ## Location of the data files used to calculate transmission
        self._sample_file = sample_file
        self._empty_file = empty_file
        ## Radius of the beam
        self._beam_radius = beam_radius
        ## Transmission workspace (output of transmission calculation)
        self._transmission_ws = None
        
    def execute(self, reducer, workspace=None):
        """
            Calculate transmission and apply correction
            @param reducer: Reducer object for which this step is executed
            @param workspace: workspace to apply correction to
        """
        if self._transmission_ws is None:
            # 1- Compute zero-angle transmission correction (Note: CalcTransCoef)
            self._transmission_ws = "transmission_fit"

            sample_ws = "_transmission_sample"
            filepath = reducer._full_file_path(self._sample_file)
            LoadSpice2D(filepath, sample_ws)
            
            empty_ws = "_transmission_empty"
            filepath = reducer._full_file_path(self._empty_file)
            LoadSpice2D(filepath, empty_ws)
            
            # Subtract dark current
            if reducer._dark_current_subtracter is not None:
                reducer._dark_current_subtracter.execute(reducer, sample_ws)
                reducer._dark_current_subtracter.execute(reducer, empty_ws)
            
            # Find which pixels to sum up as our "monitor". At this point we have moved the detector
            # so that the beam is at (0,0), so all we need is to sum the area around that point.
            #TODO: in IGOR, the error-weighted average is computed instead of simply summing up the pixels
            cylXML = '<infinite-cylinder id="transmission_monitor">' + \
                       '<centre x="0.0" y="0.0" z="0.0" />' + \
                       '<axis x="0.0" y="0.0" z="1.0" />' + \
                       '<radius val="%12.10f" />' % (self._beam_radius*reducer.instrument.pixel_size_x/1000.0) + \
                     '</infinite-cylinder>\n'
                     
            det_finder = FindDetectorsInShape(Workspace=workspace, ShapeXML=cylXML)
            det_list = det_finder.getPropertyValue("DetectorList")
            
            first_det = int(det_list.split(',')[0])
            
            GroupDetectors(InputWorkspace=empty_ws,  OutputWorkspace="empty_mon",  DetectorList=det_list, KeepUngroupedSpectra="1")
            GroupDetectors(InputWorkspace=sample_ws, OutputWorkspace="sample_mon", DetectorList=det_list, KeepUngroupedSpectra="1")
            
            #TODO: check that both workspaces have the same masked spectra
            
            # Get normalization for transmission calculation
            norm_spectrum = reducer.NORMALIZATION_TIME
            if reducer._normalizer is not None:
                norm_spectrum = reducer._normalizer.get_normalization_spectrum()
            
            # Calculate transmission. Use the reduction method's normalization channel (time or beam monitor)
            # as the monitor channel.
            CalculateTransmission(DirectRunWorkspace="empty_mon", SampleRunWorkspace="sample_mon", 
                                  OutputWorkspace=self._transmission_ws,
                                  IncidentBeamMonitor=str(norm_spectrum), 
                                  TransmissionMonitor=str(first_det))
            
        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                    TransmissionWorkspace=self._transmission_ws, 
                                    OutputWorkspace=workspace)            
            

class SubtractDarkCurrent(ReductionStep):
    """
        ReductionStep class that subtracts the dark current from the data.
        The loaded dark current is stored by the ReductionStep object so that the
        subtraction can be applied to multiple data sets without reloading it.
    """
    def __init__(self, dark_current_file, timer_ws=None):
        """
            @param timer_ws: if provided, will be used to scale the dark current
        """
        super(SubtractDarkCurrent, self).__init__()
        self._timer_ws = timer_ws
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
            filepath = reducer._full_file_path(self._dark_current_file)
            self._dark_current_ws = extract_workspace_name(filepath)
            LoadSpice2D(filepath, self._dark_current_ws)
            
            # Normalize the dark current data to counting time
            darktimer_ws = self._dark_current_ws+"_timer"
            CropWorkspace(self._dark_current_ws, darktimer_ws,
                          StartWorkspaceIndex = str(reducer.NORMALIZATION_TIME), 
                          EndWorkspaceIndex   = str(reducer.NORMALIZATION_TIME))        
            
            Divide(self._dark_current_ws, darktimer_ws, self._dark_current_ws)      
    
        # If no timer workspace was provided, get the counting time from the data
        timer_ws = self._timer_ws
        if timer_ws is None:
            timer_ws = "tmp_timer"     
            CropWorkspace(workspace, timer_ws,
                          StartWorkspaceIndex = str(reducer.NORMALIZATION_TIME), 
                          EndWorkspaceIndex   = str(reducer.NORMALIZATION_TIME))  
            
        # Scale the stored dark current by the counting time
        scaled_dark_ws = "scaled_dark_current"
        Multiply(self._dark_current_ws, timer_ws, scaled_dark_ws)
        
        # Set time and monitor channels to zero, so that we subtract only detectors
        mtd[scaled_dark_ws].dataY(reducer.NORMALIZATION_TIME)[0] = 0
        mtd[scaled_dark_ws].dataE(reducer.NORMALIZATION_TIME)[0] = 0
        mtd[scaled_dark_ws].dataY(reducer.NORMALIZATION_MONITOR)[0] = 0
        mtd[scaled_dark_ws].dataE(reducer.NORMALIZATION_MONITOR)[0] = 0
        
        # Perform subtraction
        Minus(workspace, scaled_dark_ws, workspace)  
        
        
          
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
                self._data_file = reducer._data_files[workspace]
            else:
                raise RuntimeError, "SANSReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
        
        # Load data
        filepath = reducer._full_file_path(self._data_file)
        loader = LoadSpice2D(filepath, workspace)
        reducer.instrument.sample_detector_distance = float(loader.getPropertyValue("SampleDetectorDistance"))
        mantid.sendLogMessage("Loaded %s: sample-detector distance = %g" %(workspace, reducer.instrument.sample_detector_distance))
        
        # Move detector array to correct position
        MoveInstrumentComponent(workspace, reducer.instrument.detector_ID, 
                                X = -(reducer.get_beam_center()[0]-reducer.instrument.nx_pixels/2.0+0.5) * reducer.instrument.pixel_size_x/1000.0, 
                                Y = -(reducer.get_beam_center()[1]-reducer.instrument.ny_pixels/2.0+0.5) * reducer.instrument.pixel_size_y/1000.0, 
                                Z = reducer.instrument.sample_detector_distance/1000.0,
                                RelativePosition="1")
        
class Normalize(ReductionStep):
    """
        Normalize the data to time or monitor
    """
    def __init__(self, normalization_spectrum=0):
        super(Normalize, self).__init__()
        self._normalization_spectrum = normalization_spectrum
        
    def get_normalization_spectrum(self):
        return self._normalization_spectrum
        
    def execute(self, reducer, workspace):
        # Get counting time or monitor
        norm_ws = workspace+"_normalization"
        CropWorkspace(workspace, norm_ws,
                      StartWorkspaceIndex = str(self._normalization_spectrum), 
                      EndWorkspaceIndex   = str(self._normalization_spectrum))      

        Divide(workspace, norm_ws, workspace)
        
        # HFIR-specific: If we count for monitor we need to multiply by 1e8
        if self._normalization_spectrum == reducer.NORMALIZATION_MONITOR:         
            Scale(workspace, workspace, 1.0e8, 'Multiply')
            
class WeightedAzimuthalAverage(ReductionStep):
    """
        ReductionStep class that performs azimuthal averaging
        and transforms the 2D reduced data set into I(Q).
    """
    def __init__(self, binning="0.01,0.001,0.11", suffix="_Iq", error_weighting=False):
        super(WeightedAzimuthalAverage, self).__init__()
        self._binning = binning
        self._suffix = suffix
        self._error_weighting = error_weighting
        
    def execute(self, reducer, workspace):
        output_ws = workspace+str(self._suffix)    
        Q1DWeighted(workspace, output_ws, self._binning, 
                    PixelSizeX=reducer.instrument.pixel_size_x,
                    PixelSizeY=reducer.instrument.pixel_size_y, ErrorWeighting=self._error_weighting)  
        
    def get_output_workspace(self, workspace):
        return workspace+str(self._suffix)
            
class SolidAngle(ReductionStep):
    """
        ReductionStep class that performs the solid angle correction.
    """
    def execute(self, reducer, workspace):
        SolidAngleCorrection(workspace, workspace)
            
class SensitivityCorrection(ReductionStep):
    """
        Compute the sensitivity correction and apply it to the input workspace.
        
        The ReductionStep object stores the sensitivity, so that the object
        be re-used on multiple data sets and the sensitivity will not be
        recalculated.
    """
    def __init__(self, flood_data, min_sensitivity=0.5, max_sensitivity=1.5):
        super(SensitivityCorrection, self).__init__()
        self._flood_data = flood_data
        self._efficiency_ws = None
        self._min_sensitivity = min_sensitivity
        self._max_sensitivity = max_sensitivity
        
    def execute(self, reducer, workspace):
        # If the sensitivity correction workspace exists, just apply it.
        # Otherwise create it.      
        if self._efficiency_ws is None:
            # Load the flood data
            filepath = reducer._full_file_path(self._flood_data)
            flood_ws = extract_workspace_name(filepath)
            
            LoadRun(self._flood_data).execute(reducer, flood_ws)

            # Subtract dark current
            if reducer._dark_current_subtracter is not None:
                reducer._dark_current_subtracter.execute(reducer, flood_ws)
            
            # Correct flood data for solid angle effects (Note: SA_Corr_2DSAS)
            if reducer._solid_angle_correcter is not None:
                reducer._solid_angle_correcter.execute(reducer, flood_ws)
        
            # Create efficiency profile: 
            # Divide each pixel by average signal, and mask high and low pixels.
            CalculateEfficiency(flood_ws, flood_ws, self._min_sensitivity, self._max_sensitivity)
            self._efficiency_ws = flood_ws
            
        # Divide by detector efficiency
        Divide(workspace, self._efficiency_ws, workspace)
        
        # Copy over the efficiency's masked pixels to the reduced workspace
        masked_detectors = GetMaskedDetectors(self._efficiency_ws)
        MaskDetectors(workspace, None, masked_detectors.getPropertyValue("DetectorList"))        
    

class Mask(ReductionStep):
    """
        Apply mask to workspace
    """
    def __init__(self, nx_low=0, nx_high=0, ny_low=0, ny_high=0):
        super(Mask, self).__init__()
        self._nx_low = nx_low
        self._nx_high = nx_high
        self._ny_low = ny_low
        self._ny_high = ny_high
        
    def execute(self, reducer, workspace):
        # Get a list of detector pixels to mask
        masked_pixels = reducer.instrument.get_masked_pixels(self._nx_low,
                                                             self._nx_high,
                                                             self._ny_low,
                                                             self._ny_high)
        
        # Transform the list of pixels into a list of Mantid detector IDs
        masked_detectors = reducer.instrument.get_masked_detectors(masked_pixels)
        
        # Mask the pixels by passing the list of IDs
        MaskDetectors(workspace, None, masked_detectors)

class SaveIqAscii(ReductionStep):
    def __init__(self):
        super(SaveIqAscii, self).__init__()
        
    def execute(self, reducer, workspace):
        if reducer._azimuthal_averager is not None:
            output_ws = reducer._azimuthal_averager.get_output_workspace(workspace)
            SaveAscii(Filename=output_ws+'.txt', Workspace=output_ws)
            
            
class SubtractBackground(ReductionStep):
    """
        Subtracts background from a sample data workspace.
        The processed background workspace is stored for later use.
    """
    def __init__(self, background_file):
        super(SubtractBackground, self).__init__()
        self._background_file = background_file
        self._background_ws = None
        
    def execute(self, reducer, workspace):
        if self._background_ws is None:
            self._background_ws = extract_workspace_name(self._background_file)
            for item in reducer._2D_steps():
                item.execute(reducer, self._background_ws)
        
        Minus(workspace, self._background_ws, workspace)
        
    