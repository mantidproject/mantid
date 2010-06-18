"""
    Test module for HFIR SANS reduction
    
    WARNING: this is not meant to be production code... 
"""
# Mantid imports
from mantidsimple import *

# Python import
import os

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
    
    def __init__(self):
        """
            Initialization
            TODO: most of those values should be read from the data file
        """
        ## Number of detector pixels in X
        self.nx_pixels = 192
        ## Number of detector pixels in Y
        self.ny_pixels = 192
    
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
        
class SANSReductionMethod:
    """
        Defines the reduction steps and parameters. An object from this class
        should completely define the reduction to be applied to raw data.
    """
    
    ## Normalization options
    NORMALIZATION_NONE = None
    NORMALIZATION_TIME = 1
    NORMALIZATION_MONITOR = 0

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
        self.normalization = SANSReductionMethod.NORMALIZATION_MONITOR
        
        
        # Mask parameters #####################################################
        # TODO: to allow for instruments with multiple PSDs, we might want to 
        # change this to a Mask class that takes care of the instrument-specific junk. 
        self.mask_nx_high = 5
        self.mask_nx_low  = 5
        self.mask_ny_high = 3
        self.mask_ny_low  = 3
        self.mask_user_defined = []
        

        # Sensitivity correction parameters ###################################
        
        ## Flood data for sensitivity correction
        self.sensitivity_flood_filepath = None
        ## Dark current file for sensitivity correction
        self.sensitivity_dark_filepath = None
        ## Mask pixels with out-of-range sensitivity
        self.sensitivity_mask_high_low = True
        ## Highest allowed sensitivity, above that pixels will be masked 
        self.sensitivity_high = 1.5
        ## Lowest allowed sensitivity, below that pixels will be masked
        self.sensitivity_low = 0.5
        

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
        
        # Apply corrections to sample data        
        self._apply_corrections(self.reduced_ws)
        
        # Subtract background if applicable
        if self.method.background_filepath is not None:
            bck_ws = _extract_workspace_name(self.method.background_filepath)
            LoadSpice2D(self.method.background_filepath, bck_ws)
        
            # Apply correction to background
            self._apply_corrections(bck_ws)
            
            # Subtract corrected background
            Divide(self.reduced_ws, bck_ws, self.reduced_ws)
            
        # Transform to I(q)
        
        
        
    def _apply_corrections(self, ws):
        """
            Apply the data corrections to a given workspace. The input workspace 
            will be modified.
            @param ws: workspace to apply corrections to
            
            TODO: since the input files will be the same, read them only once
        """
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
            dark_ws = _extract_workspace_name(self.method.dark_current_filepath)
            LoadSpice2D(self.method.dark_current_filepath, dark_ws)
        
            # Normalize the dark current data to counting time
            if self.method.dark_normalization == SANSReductionMethod.NORMALIZATION_TIME:
                darktimer_ws = dark_ws+"_timer"
                CropWorkspace(dark_ws, darktimer_ws,
                              StartWorkspaceIndex = str(SANSReductionMethod.NORMALIZATION_TIME), 
                              EndWorkspaceIndex   = str(SANSReductionMethod.NORMALIZATION_TIME))        
                
                Multiply(dark_ws, timer_ws, dark_ws)
                Divide(dark_ws, darktimer_ws, dark_ws)      
        
            # Perform subtraction
            Minus(ws, dark_ws, ws)
            
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
        if self.method.sensitivity_dark_filepath is not None:
            sensdark_ws = _extract_workspace_name(self.method.sensitivity_dark_filepath)
            LoadSpice2D(self.method.sensitivity_dark_filepath, sensdark_ws)
            
            # Normalize the dark current data to counting time
            sensdarktimer_ws = sensdark_ws+"_timer"
            CropWorkspace(sensdark_ws, sensdarktimer_ws,
                          StartWorkspaceIndex = str(SANSReductionMethod.NORMALIZATION_TIME), 
                          EndWorkspaceIndex   = str(SANSReductionMethod.NORMALIZATION_TIME))  
            
            floodtimer_ws = flood_ws+"_timer"     
            CropWorkspace(flood_ws, floodtimer_ws,
                          StartWorkspaceIndex = str(SANSReductionMethod.NORMALIZATION_TIME), 
                          EndWorkspaceIndex   = str(SANSReductionMethod.NORMALIZATION_TIME))  
            
            Multiply(sensdark_ws, floodtimer_ws, sensdark_ws)
            Divide(sensdark_ws, sensdarktimer_ws, sensdark_ws)      
            Minus(flood_ws, sensdark_ws, flood_ws)
        
        # Divide by detector efficiency, if provided
        
        # Correct for solid angle effect at large angle (Note: SA_Corr_2DSAS)
        
        
        
        
        
        # Apply transmission correction #######################################
        
        
        
    def _extract_workspace_name(self, suffix=''):
        """
            Returns a default workspace name for a file path.
            @param suffix: string to append to name
        """
        return _extract_workspace_name(self.data_filepath, suffix)


    

if __name__ == "__main__":
    # Data file to reduce
    datafile = "/home/m2d/workspace/mantid/Test/Data/SANS2D/BioSANS_exp61_scan0004_0001.xml"
    # Reduction parameters
    method = SANSReductionMethod()
    # Instrument parameters
    conf = InstrumentConfiguration()

    reduction = SANSReduction(datafile, method, conf)    
    reduction.reduce()
    