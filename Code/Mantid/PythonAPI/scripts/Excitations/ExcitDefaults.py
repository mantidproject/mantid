#! you need to restart the application you are using (e.g. MantidPlot) before changes to this file will take effect!
import CommonFunctions as common

MARI = common.defaults(
  background_range=(18000.0,19500.0),
  normalization='monitor-monitor 1',
  instrument_pref='MAR',
  white_beam_integr=(20.0,40.0),
  scale_factor=1.8182e8,
  monitor1_integr=(1000,2000),
  white_beam_scale=1000.
  )

MAPS = common.defaults(
  background_range=(12000.0, 18000.0),
  normalization='monitor-monitor 1',
  instrument_pref='MAP')

# we only have support for MARI at the moment, ??STEVES?? change this
def loadDefaults(instPrefix):
  if instPrefix == 'mar' : instPrefix = 'MAR'
  if instPrefix == 'MAR' : return MARI
  else : raise Exception("As only MARI is supported at the moment the first argument must be MAR")

#called r in DIAG
DetectorEfficiencyVariation_Variation = 1.1