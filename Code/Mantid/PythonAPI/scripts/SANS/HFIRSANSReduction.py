"""
    Test module for HFIR SANS reduction
    
    WARNING: this is not meant to be production code... 
"""
# Mantid imports
from mantidsimple import *

# Python import
import os, sys

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
        self.sensitivity_dark_filepath = None
        ## Highest allowed sensitivity, above that pixels will be masked 
        self.sensitivity_high = None
        ## Lowest allowed sensitivity, below that pixels will be masked
        self.sensitivity_low = None


        # Transmission parameters #############################################
        

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
        
        # Find beam center position
        if self.configuration.beam_center_method is not InstrumentConfiguration.BEAM_CENTER_NONE:
            direct_beam = (self.configuration.beam_center_method is InstrumentConfiguration.BEAM_CENTER_DIRECT_BEAM)
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
            
            #TODO: find the center of the flood data independently
            
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
        # 1- Compute zero-angle transmission correction (Note: CalcTransCoef)
        
        # 2- Apply correction (Note: Apply2DTransCorr)
        # TODO: Need an Algo that will produce a workspace with the following spectra values
        # sec[x][y] = sqrt(1+(pixel_size_x*(x-beam_center_x)/sample_detector_distance)^2
        #                     +(pixel_size_y*(y-beam_center_y)/sample_detector_distance)^2)
        # ws[x][y] = ws[x][y]/transmission^((sec[x][y]+1)/2)
        # err[x][y] = sqrt( ws[x][y] / (transmission^((sec[x][y]+1)/2))^2 
        #                    + ((d_transmission*ws[x][y]*((sec[x][y]+1)/2))/(transmission^((sec[x][y]+1)/2+1)))^2
        
        
        # TODO: set the X bins as wavelength with the correct value
        
        
    def _extract_workspace_name(self, suffix=''):
        """
            Returns a default workspace name for a file path.
            @param suffix: string to append to name
        """
        return _extract_workspace_name(self.data_filepath, suffix)

################### TESTS ##################################################################
#TODO: Place this as tests in another file. Imports within MantidPlot don't behave as within
#      a standard python interpreter so we'll leave these here for now.

# Set directory containg the test data, relative to the Mantid release directory.
TEST_DIR = "../../../Test/Data/SANS2D/"

def _read_IGOR(filepath):
    """
        Read in an HFIR IGOR output file with reduced data
        @param filepath: path of the file to be read
    """
    data = []
    with open(filepath) as f:
        # Skip first header line
        f.readline()
        for line in f:
            toks = line.split()
            try:
                q    = float(toks[0])
                iq   = float(toks[1])
                diq  = float(toks[2])
                diq2 = float(toks[3])
                data.append([q, iq, diq])
            except:
                print "_read_IGOR:", sys.exc_value  
    return data
    
def _read_Mantid(filepath):
    """
        Read in a CVS Mantid file
        @param filepath: path of the file to be read
    """
    data = []
    with open(filepath) as f:
        # Read Q line. Starts with 'A', so remove the first item
        qtoks = f.readline().split()
        qtoks.pop(0)
        
        # Second line is I(q), Starts with 0 to be skipped
        iqtoks = f.readline().split()
        iqtoks.pop(0)
        
        # Third and fourth lines are dummy lines
        f.readline()
        f.readline()
        
        # Fifth line is dI(q). Starts with 0 to be skipped
        diqtoks = f.readline().split()
        diqtoks.pop(0)
        
        for i in range(len(qtoks)-1):
            try:
                q   = float(qtoks[i])
                iq  = float(iqtoks[i])
                diq = float(diqtoks[i])
                data.append([q, iq, diq])
            except:
                print "_read_Mantid:", i, sys.exc_value   
    return data
    
def _verify_result(reduced_file, test_file):
    """
        Compare the data in two reduced data files.
        @param reduced_file: path of the Mantid-reduced file
        @param test_file: path of the IGOR-reduced file
    """
    
    # Read reduced file
    data_mantid = _read_Mantid(reduced_file)
    # Read the test data to compare with
    data_igor = _read_IGOR(test_file)
    
    # Check length
    assert(len(data_mantid)==len(data_igor))
    
    # Utility methods for manipulating the lists
    def _diff_iq(x,y): return x[1]-y[1]
    def _diff_err(x,y): return x[2]-y[2]
    def _add(x,y): return x+y
    
    # Check that I(q) is the same for both data sets
    deltas = map(_diff_iq, data_mantid, data_igor)
    delta  = reduce(_add, deltas)
    assert(delta<1e-5)
    
    # Then compare the errors
    deltas = map(_diff_err, data_mantid, data_igor)
    delta  = reduce(_add, deltas)
    assert(delta<1e-5)
    
    print "Completed tests"
    
def test_default():   
     # Data file to reduce
    datafile = TEST_DIR+"BioSANS_test_data.xml"
    # Reduction parameters
    method = SANSReductionMethod()
    # Instrument parameters
    conf = InstrumentConfiguration()

    reduction = SANSReduction(datafile, method, conf)    
    reduction.reduce()
    
def test_center_by_hand():
    """
        Beam center entered by hand
        Subtract dark current
        Correct for solid angle 
        Correct for detector efficiency, excluding high/low pixels
        No transmission
        No background
    """
    # Data file to reduce
    datafile = TEST_DIR+"BioSANS_test_data.xml"
    
    # Reduction parameters
    method = SANSReductionMethod()
    method.dark_current_filepath = TEST_DIR+"BioSANS_dark_current.xml"
    method.sensitivity_flood_filepath = TEST_DIR+"BioSANS_flood_data.xml"
    method.sensitivity_dark_filepath = TEST_DIR+"BioSANS_dark_current.xml"
    method.sensitivity_high = 1.5
    method.sensitivity_low = 0.5
    
    # Instrument parameters
    conf = InstrumentConfiguration()
    conf.beam_center_method = InstrumentConfiguration.BEAM_CENTER_NONE
    conf.beam_center_x = 16
    conf.beam_center_y = 95

    reduction = SANSReduction(datafile, method, conf)    
    reduction.reduce()
    
    SaveCSV(Filename="mantid_center_by_hand.txt", InputWorkspace="Iq", Separator="\t", LineSeparator="\n")
    
    _verify_result(reduced_file="mantid_center_by_hand.txt", test_file=TEST_DIR+"reduced_center_by_hand.txt")
    
if __name__ == "__main__":
    test_center_by_hand()

    
