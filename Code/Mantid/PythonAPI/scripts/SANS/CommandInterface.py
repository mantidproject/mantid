"""
    List of user commands. This should eventually be split into two files, one
    general module that all instruments will use, and one module that will work
    only for SANS instruments. The SANS-specific commands will likely only work 
    with the SANSReducer. 
"""
#TODO: When the SANSReduction refactor is completed, change this import to SANSReduction
import HFIRSANSReduction
from SANSReducer import SANSReducer
import SANSReductionSteps

class ReductionSingleton:
    """ Singleton reduction class """

    ## storage for the instance reference
    __instance = None

    def __init__(self):
        """ Create singleton instance """
        # Check whether we already have an instance
        if ReductionSingleton.__instance is None:
            # Create and remember instance
            ReductionSingleton.__instance = SANSReducer()

        # Store instance reference as the only member in the handle
        self.__dict__['_ReductionSingleton__instance'] = ReductionSingleton.__instance
        
    def __getattr__(self, attr):
        """ Delegate access to implementation """
        return getattr(self.__instance, attr)

    def __setattr__(self, attr, value):
        """ Delegate access to implementation """
        return setattr(self.__instance, attr, value)

## List of user commands ######################################################

def DataPath(path):
    ReductionSingleton().set_data_path(path)

def HFIRSANS():
    ReductionSingleton().set_instrument(HFIRSANSReduction.InstrumentConfiguration())

class FindBeamCenter:
    BEAMFINDER_DIRECT_BEAM = SANSReductionSteps.FindBeamCenter.BEAMFINDER_DIRECT_BEAM
    BEAMFINDER_SCATTERING = SANSReductionSteps.FindBeamCenter.BEAMFINDER_SCATTERING

    def __init__(self, datafile, method=BEAMFINDER_DIRECT_BEAM):
        #ReductionSingleton().append_step(SANSReductionSteps.FindBeamCenter(datafile, method))
        ReductionSingleton().set_beam_finder(SANSReductionSteps.FindBeamCenter(datafile, method))
        
def SetBeamCenter(x,y):
    ReductionSingleton().set_beam_center(x,y)
    
def Reduce1D():
    ReductionSingleton().reduce()
        
def AppendDataFile(datafile, workspace=None):
    """
        Append a data file in the list of files to be processed.
        @param datafile: data file to be processed
        @param workspace: optional workspace name for this data file
            [Default will be the name of the file]
    """
    ReductionSingleton().append_data_file(datafile, workspace)
    
def TimeNormalization():
    ReductionSingleton().set_normalizer(SANSReducer.NORMALIZATION_TIME)
    
def MonitorNormalization():
    ReductionSingleton().set_normalizer(SANSReducer.NORMALIZATION_MONITOR)
    
def SensitivityCorrection(flood_data, min_sensitivity=0.5, max_sensitivity=1.5):
    ReductionSingleton().set_sensitivity_correcter(SANSReductionSteps.SensitivityCorrection(flood_data, min_sensitivity, max_sensitivity))
    
def NoSensitivityCorrection():
    ReductionSingleton().set_sensitivity_correcter(None)
    
def DarkCurrent(datafile):
    ReductionSingleton().set_dark_current_subtracter(SANSReductionSteps.SubtractDarkCurrent(datafile))
    
def NoDarkCurrent():
    ReductionSingleton().set_dark_current_subtracter(None)
    
def SolidAngle():
    ReductionSingleton().set_solid_angle_correcter(SANSReductionSteps.SolidAngle())
    
def NoSolidAngle():
    ReductionSingleton().set_solid_angle_correcter(None)
    
def AzimuthalAverage():
    ReductionSingleton().set_azimuthal_averager(SANSReductionSteps.WeightedAzimuthalAverage())

    
    