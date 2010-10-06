"""
    List of user commands. This should eventually be split into two files, one
    general module that all instruments will use, and one module that will work
    only for SANS instruments. The SANS-specific commands will likely only work 
    with the SANSReducer. 
"""
#TODO: pull HFIR-specific commands in a separate module.

import SANSInsts
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
        
    @classmethod
    def clean(cls, reducer_cls=None):
        if reducer_cls==None:
            ReductionSingleton.__instance = SANSReducer()
        else:
            ReductionSingleton.__instance = reducer_cls()
        
    @classmethod
    def run(cls):
        if ReductionSingleton.__instance is not None:
            ReductionSingleton.__instance._reduce()
        ReductionSingleton.clean(ReductionSingleton.__instance.__class__)
        
    def __getattr__(self, attr):
        """ Delegate access to implementation """
        return getattr(self.__instance, attr)

    def __setattr__(self, attr, value):
        """ Delegate access to implementation """
        return setattr(self.__instance, attr, value)

## List of user commands ######################################################
def Clear(reducer_cls=None):
    """
        Clears the Reducer of changes applied by all previous commands
    """
    ReductionSingleton.clean(reducer_cls)

def DataPath(path):
    ReductionSingleton().set_data_path(path)

def DirectBeamCenter(datafile):
    ReductionSingleton().set_beam_finder(SANSReductionSteps.DirectBeamCenter(datafile))

def ScatteringBeamCenter(datafile, beam_radius=3.0):
    ReductionSingleton().set_beam_finder(SANSReductionSteps.ScatteringBeamCenter(datafile, beam_radius=beam_radius))

def SetBeamCenter(x,y):
    ReductionSingleton().set_beam_finder(SANSReductionSteps.BaseBeamFinder(x,y))
    
def Reduce1D():
    return ReductionSingleton().reduce()
        
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
    
def NoNormalization():
    ReductionSingleton().set_normalizer(None)
    
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
    
def AzimuthalAverage(binning="0.01,0.001,0.11", suffix="_Iq", error_weighting=False):
    ReductionSingleton().set_azimuthal_averager(SANSReductionSteps.WeightedAzimuthalAverage(binning=binning,
                                                                                            suffix=suffix,
                                                                                            error_weighting=error_weighting))

def NoTransmission():
    ReductionSingleton().set_transmission(None)
    
def SetTransmission(trans, error):
    ReductionSingleton().set_transmission(SANSReductionSteps.BaseTransmission(trans, error))

def DirectBeamTransmission(sample_file, empty_file, beam_radius=3.0):
    ReductionSingleton().set_transmission(SANSReductionSteps.DirectBeamTransmission(sample_file=sample_file,
                                                                                    empty_file=empty_file,
                                                                                    beam_radius=beam_radius))

def BeamSpreaderTransmission(sample_spreader, direct_spreader,
                             sample_scattering, direct_scattering,
                             spreader_transmission=1.0, spreader_transmission_err=0.0 ):
    ReductionSingleton().set_transmission(SANSReductionSteps.BeamSpreaderTransmission(sample_spreader=sample_spreader, 
                                                                                      direct_spreader=direct_spreader,
                                                                                      sample_scattering=sample_scattering, 
                                                                                      direct_scattering=direct_scattering,
                                                                                      spreader_transmission=spreader_transmission, 
                                                                                      spreader_transmission_err=spreader_transmission_err))
  
def Mask(nx_low=0, nx_high=0, ny_low=0, ny_high=0): 
    ReductionSingleton().set_mask(SANSReductionSteps.Mask(nx_low=nx_low, nx_high=nx_high, ny_low=ny_low, ny_high=ny_high))

def Background(datafile):
    ReductionSingleton().set_background(datafile) 

def NoBackground():
    ReductionSingleton().set_background(None) 

def SaveIqAscii(reducer=None):
    if reducer is None:
        reducer = ReductionSingleton()
    reducer.set_save_Iq(SANSReductionSteps.SaveIqAscii())

def NoSaveIq():
    ReductionSingleton().set_save_Iq(None)
    
def SampleGeometry(shape):
    if not isinstance(ReductionSingleton().geometry_correcter, SANSReductionSteps.SampleGeomCor):
        defGeom = SANSReductionSteps.GetSampleGeom()
        ReductionSingleton().set_geometry_correcter(
            SANSReductionSteps.SampleGeomCor(defGeom))
    
    ReductionSingleton().geometry_correcter.geo.shape = shape
    
def SampleThickness(thickness):
    if not isinstance(ReductionSingleton().geometry_correcter, SANSReductionSteps.SampleGeomCor):
        defGeom = SANSReductionSteps.GetSampleGeom()
        ReductionSingleton().set_geometry_correcter(
            SANSReductionSteps.SampleGeomCor(defGeom))

    ReductionSingleton().geometry_correcter.geo.thickness = thickness
    
def SampleHeight(height):
    if not isinstance(ReductionSingleton().geometry_correcter, SANSReductionSteps.SampleGeomCor):
        defGeom = SANSReductionSteps.GetSampleGeom()
        ReductionSingleton().set_geometry_correcter(
            SANSReductionSteps.SampleGeomCor(defGeom))
        
    ReductionSingleton().geometry_correcter.geo.height = height

def SampleWidth(width):
    if not isinstance(ReductionSingleton().geometry_correcter, SANSReductionSteps.SampleGeomCor):
        defGeom = SANSReductionSteps.GetSampleGeom()
        ReductionSingleton().set_geometry_correcter(
            SANSReductionSteps.SampleGeomCor(defGeom))
        
    ReductionSingleton().geometry_correcter.geo.width = width

    
    