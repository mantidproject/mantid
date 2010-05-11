"""Excitations analysis module
"""
from mantidsimple import *
import CommonFunctions as common
import math

# these are the defaults for different instruments. Need to be moved to parameter file
import ExcitDefaults

def mono_sample(inst_prefix, run_nums, Ei, d_rebin, wbrf, getEi=True, back='', norma='', det_map='', det_mask='', output_name = 'mono_sample_temporyWS') :
  """
  Calculate Ei and detector offsets for a mono-chromatic run.
  """
  run_nums = common.listToString(run_nums).split(',')
  result_ws, det_eff_file = common.loadRun(inst_prefix,run_nums[0], output_name)
  if result_ws.isGroup():
    raise RunTimeError("Workspace groups are not supported here")

  instrument = result_ws.getInstrument()
  # The final workspace will take the X-values and instrument from the first workspace and so we don't need to rerun ChangeBinOffset(), MoveInstrumentComponent(), etc.
  common.sumWorkspaces(result_ws, inst_prefix, run_nums)
  if det_eff_file != '':
    LoadDetectorInfo(result_ws, det_eff_file)

  Ei, mon1_peak = calculateEi(result_ws, Ei, getEi,instrument)
  bin_offset = -mon1_peak
  
  if back != 'noback':
    TOFLow = instrument.getNumberParameter("bkgd-range-min")[0]
    TOFHigh = instrument.getNumberParameter("bkgd-range-max")[0]
    # Remove the count rate seen in the regions of the histograms defined as the background regions, if the user defined a region
    ConvertToDistribution(result_ws)                                             
    FlatBackground(result_ws, result_ws, TOFLow + bin_offset, TOFHigh + bin_offset, '', 'Mean')
    ConvertFromDistribution(result_ws)

  # deals with normalize to monitor (e.g.  norm = 'monitor-monitor 1'), current (if norm = 'protons (uAh)'), etc.
  NormaliseTo(norma, result_ws, bin_offset, instrument)

  ConvertUnits(result_ws, result_ws, 'DeltaE', 'Direct', Ei, AlignBins=0)

  if d_rebin != '':
    Rebin(result_ws, result_ws, common.listToString(d_rebin))
 
  if det_eff_file != '':
    DetectorEfficiencyCor(result_ws, result_ws, Ei)
 
  if det_mask != '' :
    MaskDetectors(Workspace=result_ws, SpectraList=common.listToString(det_mask))
  
  if det_map != '':
    GroupDetectors(result_ws, result_ws, det_map, KeepUngroupedSpectra=0)
  ConvertToDistribution(result_ws)
  
  NormaliseToWhiteBeam(wbrf, result_ws, det_map, det_mask, norma, inst_prefix, instrument)

  # Overall scale factor
  scale_factor = instrument.getNumberParameter("scale-factor")[0]
  result_ws *= scale_factor
  
  return result_ws

def calculateEi(input_ws, E_guess, getEi, instrument):
  """
  Calculate incident energy of neutrons
  """
  getEi = str(getEi).lower()
  if getEi == 'fixei' or getEi == 'false':
    fixei = True
  else:
    fixei = False
  
  monitor1_spec = int(instrument.getNumberParameter("ei-mon1-spec")[0])
  monitor2_spec = int(instrument.getNumberParameter("ei-mon2-spec")[0])
  
  # Get incident energy
  alg = GetEi(input_ws, monitor1_spec, monitor2_spec, E_guess,FixEi=fixei,AdjustBins=True)
  mon1_peak = float(alg.getPropertyValue("FirstMonitorPeak"))
  ei = float(input_ws.getSampleDetails().getLogData("Ei").value())

  return ei, mon1_peak


def NormaliseTo(scheme, data_ws, offset, instrument):

  if scheme == 'monitor-monitor 1':
    min = instrument.getNumberParameter("norm-mon1-min")[0]
    max = instrument.getNumberParameter("norm-mon1-max")[0]
    min += offset
    max += offset
    
    mon_spec = int(instrument.getNumberParameter("norm-mon1-spec")[0])
    NormaliseToMonitor( InputWorkspace=data_ws, OutputWorkspace=data_ws, MonitorSpectrum=mon_spec, IntegrationRangeMin=min, IntegrationRangeMax=max,IncludePartialBins=True)
  elif scheme == 'monitor-monitor 2':
    PEAKWIDTH = 4.0/100
    # the origin of time was set to the location of the peak so to get it's area integrate around the origin, PEAKWIDTH fraction of total time of flight
    min = (-PEAKWIDTH/2)*20000
    max = (PEAKWIDTH/2)*20000
    mon_spec = int(instrument.getNumberParameter("norm-mon2-spec")[0])
    NormaliseToMonitor( InputWorkspace=data_ws, OutputWorkspace=data_ws, MonitorSpectrum=mon_spec, IntegrationRangeMin=min, IntegrationRangeMax=max,IncludePartialBins=True)
  elif scheme == 'protons (uAh)':
    NormaliseByCurrent(InputWorkspace=data_ws, OutputWorkspace=data_ws)
  elif scheme == 'no normalization':
    return
  else:
    raise RunTimeError('Normalisation scheme ' + reference + ' not found. It must be one of monitor, current, peak or none')

def NormaliseToWhiteBeam(WBRun, mono_ws, mapFile, detMask, scheme, prefix, instrument):
  wbnorm_name = "_ETrans_norm_tempory_WS"
  wbnorm_ws = common.LoadNexRaw(common.getFileName(prefix, WBRun), wbnorm_name)[0]
  
  NormaliseTo(scheme, wbnorm_ws, 0., instrument)
  ConvertUnits(wbnorm_ws, wbnorm_ws, "Energy", AlignBins=0)
  # This both integrates the workspace into one bin spectra and sets up common bin boundaries for all spectra
  low = instrument.getNumberParameter("wb-integr-min")[0]
  hi = instrument.getNumberParameter("wb-integr-max")[0]
  delta = 2.0*(hi - low)
  Rebin(wbnorm_ws, wbnorm_ws, [low, delta, hi])

  if detMask != '' :
    MaskDetectors(wbnorm_ws, SpectraList=detMask)
  
  if mapFile!= "" :
    GroupDetectors(wbnorm_ws, wbnorm_ws, mapFile, KeepUngroupedSpectra=0)

  # White beam scale factor
  wb_scale_factor = instrument.getNumberParameter("wb-scale-factor")[0]
  wbnorm_ws *= wb_scale_factor
  mono_ws /= wbnorm_ws
  
  mtd.deleteWorkspace(wbnorm_name)
  return

# a workspace name that is very long and unlikely to have been created by the user before this script is run, it would be replaced  
tempWS = '_ETrans_loading_tempory_WS'
