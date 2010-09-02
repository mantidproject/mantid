"""
    Command interface for ISIS SANS instruments.
    
    As commands in the common CommandInterface module are found to be ISIS-specific, 
    they should be moved here.
"""
import os

import SANSInsts
from CommandInterface import *
import ISISReducer

_NOPRINT_ = False
_VERBOSE_ = False

def SetNoPrintMode(quiet = True):
    global _NOPRINT_
    _NOPRINT_ = quiet

def SetVerboseMode(state):
    global _VERBOSE_
    _VERBOSE_ = state

# Print a message and log it if the 
def _printMessage(msg, log = True):
    if log == True and _VERBOSE_ == True:
        mantid.sendLogMessage('::SANS::' + msg)
    if _NOPRINT_ == True: 
        return
    print msg
    
def _issueWarning(msg):
    """
        Issues a Mantid message
        @param msg: message to be issued
    """
    mantid.sendLogMessage('::SANS::Warning: ' + msg)
                
def UserPath(path):
    ReductionSingleton().set_user_path(path)
        
def SANS2D():
    Clear(ISISReducer.ISISReducer)
    instrument = SANSInsts.SANS2D()
    
    #TODO: this should probably be part of __init__. Leave it for now
    # so we don't create problems with the current code.
    instrument.TRANS_WAV1_FULL = instrument.TRANS_WAV1 = 2.0
    instrument.TRANS_WAV2_FULL = instrument.TRANS_WAV2 = 14.0
    instrument.lowAngDetSet = True
    
    ReductionSingleton().set_instrument(instrument)

def LOQ():
    instrument = SANSInsts.LOQ()
    
    #TODO: refactor this away
    instrument.TRANS_WAV1_FULL = instrument.TRANS_WAV1 = 2.2
    instrument.TRANS_WAV2_FULL = instrument.TRANS_WAV2 = 10.0
    instrument.lowAngDetSet = True
    
    ReductionSingleton().set_instrument(instrument)
    
def Detector(det_name):
    ReductionSingleton().instrument.setDetector(det_name)
    
def MaskFile(file_name):
    ReductionSingleton().read_mask_file(file_name)
    
def SetMonitorSpectrum(specNum, interp=False):
    ReductionSingleton().set_monitor_spectrum(specNum, interp)

def SuggestMonitorSpectrum(specNum, interp=False):  
    ReductionSingleton().suggest_monitor_spectrum(specNum, interp)
    
def SetTransSpectrum(specNum, interp=False):
    ReductionSingleton().set_trans_spectrum(specNum, interp)
      
def SetPhiLimit(phimin,phimax, phimirror=True):
    ReductionSingleton().set_phi_limit(phimin, phimax, phimirror)
    
def SetSampleOffset(value):
    ReductionSingleton().instrument.set_sample_offset(value)
    
def Gravity(flag):
    ReductionSingleton().set_gravity(flag)
    
def TransFit(mode,lambdamin=None,lambdamax=None):
    ReductionSingleton().set_trans_fit(lambda_min=lambdamin, 
                                       lambda_max=lambdamax, 
                                       fit_method=mode)
    
def AssignCan(can_run, reload = True, period = -1):
    ReductionSingleton().set_background(can_run, reload = reload, period = period)
    
def SampleGeometry(geom_id):
    ReductionSingleton().set_sample_geometry(geom_id)
    
def SampleThickness(thickness):
    ReductionSingleton().set_sample_thickness(thickness)
    
def SampleHeight(height):
    ReductionSingleton().set_sample_height(height)

def SampleWidth(width):
    ReductionSingleton().set_sample_width(width)
    
def TransmissionSample(sample, direct, reload = True, period = -1):
    ReductionSingleton().set_trans_sample(sample, direct, reload = True, period = -1)
    
def TransmissionCan(can, direct, reload = True, period = -1):
    _printMessage('TransmissionCan("' + can + '","' + direct + '")')
    ReductionSingleton().set_trans_can(can, direct, reload = True, period = -1)
    
def AssignSample(sample_run, reload = True, period = -1):
    _printMessage('AssignSample("' + sample_run + '")')
    ReductionSingleton().append_data_file(sample_run)
    ReductionSingleton().load_set_options(reload, period)
    
def AppendDataFile(datafile, workspace=None):
    AssignSample(datafile)
    