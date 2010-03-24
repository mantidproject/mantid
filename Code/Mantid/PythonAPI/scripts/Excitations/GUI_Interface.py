# Used by MantidPlot if the script in is in Code\Mantid\PythonAPI\scripts\Excitations make a
# copy of the script if you want to change it or risk the MantidPlot interface not working
###########################################
# applies unit conversion, detector convcy and grouping correction to a
# raw file
###########################################
#from ConversionLib import *
#from DetectorTestLib import *

#this commented out code will replace the code immediately below it when the interface been diagnose and the interface is implemented
#if ??? :
#  runs = '|GUI_SET_RAWFILE_LIST|'
#badSpectra = diagnose(instrum='MAR',wbrf='11060',wbrf2='11060',runs='15537',tiny=1e-10,huge=1e10,median_lo=0.1,median_hi=3.0,sv_sig=3.3,bmedians=5.0,zero='False', out_asc='',maskFile='')
badSpectra=[]
DetectorMask = ''#+|MASK_WORKSPACE|
if DetectorMask != '' :
  FDOL = FindDetectorsOutsideLimits(InputWorkspace=DetectorMask,OutputWorkspace='_ETrans_loading_bad_detector_WS',HighThreshold=10,LowThreshold=-1,OutputFile='')
  badSpectra = FDOL.getPropertyValue('BadSpectraNums')
  mantid.deleteWorkspace('_ETrans_loading_bad_detector_WS')

mtd.sendLogMessage("--Executing Python function: mono_sample('MAR', '|GUI_SET_RAWFILE_LIST|', |GUI_SET_E_GUESS|, |GUI_SET_BIN_BOUNDS|, |GUI_SET_WBV|, getEi=|GUI_SET_E|, back='('+|TOF_LOW|+','+|TOF_HIGH|+')', norma=|GUI_SET_NORM|, det_map=|GUI_SET_MAP_FILE|, det_mask=badSpectra, nameInOut='|GUI_SET_OUTWS|')--")
mono_sample('MAR', '|GUI_SET_RAWFILE_LIST|', |GUI_SET_E_GUESS|, |GUI_SET_BIN_BOUNDS|, |GUI_SET_WBV|, getEi=|GUI_SET_E|, back='('+|TOF_LOW|+','+|TOF_HIGH|+')', norma=|GUI_SET_NORM|, det_map=|GUI_SET_MAP_FILE|, det_mask=badSpectra, nameInOut='|GUI_SET_OUTWS|')
