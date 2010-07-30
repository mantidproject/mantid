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
    
    def execute(self, reducer, workspace):
        pass
        
class FindBeamCenter(BaseBeamFinder):
    """
        Find the beam center with one of two methods
    """
    BEAMFINDER_DIRECT_BEAM = 1
    BEAMFINDER_SCATTERING = 2
   
    def __init__(self, datafile, method=BEAMFINDER_DIRECT_BEAM, beam_radius=3):
        super(FindBeamCenter, self).__init__()
        ## Location of the data file used to find the beam center
        self._datafile = datafile
        ## Method used to find the beam center
        self._method = method
        self._beam_radius = beam_radius
        
    def execute(self, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        # Check whether the direct beam or scattering pattern option was selected
        direct_beam = (self._method is FindBeamCenter.BEAMFINDER_DIRECT_BEAM)
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
        
        reducer.set_beam_center(float(ctr[0]), float(ctr[1]))

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
    def __init__(self, datafile):
        super(LoadRun, self).__init__()
        self._data_file = datafile
        
    def execute(self, reducer, workspace):       
        # Load data
        filepath = reducer._full_file_path(self._data_file)
        LoadSpice2D(filepath, workspace)
        
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
    def execute(self, reducer, workspace):
        Q1DWeighted(workspace, "Iq", "0.01,0.001,0.11", 
                    PixelSizeX=reducer.instrument.pixel_size_x,
                    PixelSizeY=reducer.instrument.pixel_size_y, ErrorWeighting=True)  
            
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
    

