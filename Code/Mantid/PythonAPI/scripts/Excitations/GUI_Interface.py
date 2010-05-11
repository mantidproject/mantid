badSpectra=[]
DetectorMask = ''#+|MASK_WORKSPACE|
if DetectorMask != '' :
  FDOL = FindDetectorsOutsideLimits(InputWorkspace=DetectorMask,OutputWorkspace='_ETrans_loading_bad_detector_WS',HighThreshold=10,LowThreshold=-1,OutputFile='')
  badSpectra = FDOL.getPropertyValue('BadSpectraNums')
  mantid.deleteWorkspace('_ETrans_loading_bad_detector_WS')

#mtd.sendLogMessage("--Executing Python function: mono_sample('MAR', '|GUI_SET_RAWFILE_LIST|', |GUI_SET_E_GUESS|, |GUI_SET_BIN_BOUNDS|, |GUI_SET_WBV|, getEi=|GUI_SET_E|, back='('+|TOF_LOW|+','+|TOF_HIGH|+')', norma=|GUI_SET_NORM|, det_map=|GUI_SET_MAP_FILE|, det_mask=badSpectra, nameInOut='|GUI_SET_OUTWS|')--")
mono_sample('|PREFIX|', '|GUI_SET_RAWFILE_LIST|', |GUI_SET_E_GUESS|, |GUI_SET_BIN_BOUNDS|, |GUI_SET_WBV|, getEi=|GUI_SET_E|, back='('+|TOF_LOW|+','+|TOF_HIGH|+')', norma=|GUI_SET_NORM|, det_map=|GUI_SET_MAP_FILE|, det_mask=badSpectra, output_name='|GUI_SET_OUTWS|')
