"""
    Command interface for ISIS SANS instruments.
    
    As commands in the common CommandInterface module are found to be ISIS-specific, 
    they should be moved here.
"""
import os

import SANSInsts
from CommandInterface import *
import ISISReducer

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
    
