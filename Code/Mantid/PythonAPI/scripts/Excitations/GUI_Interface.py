badSpectra=[]
DetectorMask = ''#+|MASK_WORKSPACE|
# MG : 20/05/2010 - A strange bug with signal delivery order seems to be affecting Mantid.  Temporary fix is to delay workspace deletion
remove_workspaces = []
if DetectorMask != '' :
  remove_workspaces.append('_ETrans_loading_bad_detector_WS')
  FDOL = FindDetectorsOutsideLimits(InputWorkspace=DetectorMask,OutputWorkspace=remove_workspaces[0],HighThreshold=10,LowThreshold=-1,OutputFile='')
  badSpectra = FDOL.getPropertyValue('BadSpectraNums')

mono_sample('|PREFIX|', '|GUI_SET_RAWFILE_LIST|', |GUI_SET_E_GUESS|, |GUI_SET_BIN_BOUNDS|, |GUI_SET_WBV|, getEi=|GUI_SET_E|, back='('+|TOF_LOW|+','+|TOF_HIGH|+')', norma=|GUI_SET_NORM|, det_map=|GUI_SET_MAP_FILE|, det_mask=badSpectra, output_name='|GUI_SET_OUTWS|')

for w in remove_workspaces:
  if mtd.workspaceExists(w):
    mtd.deleteWorkspace(w)