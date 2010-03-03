#! you need to restart the application you are using (e.g. MantidPlot) before changes to this file will take effect!
import CommonFunctions as common

MARI = common.defaults(backgroundRange=(12000, 18000), normalization='monitor-monitor 1', instrument_pref='MAR')
MAPS = common.defaults(backgroundRange=(12000, 18000), normalization='monitor-monitor 1', instrument_pref='MAP')

# we only have support for MARI at the moment, ??STEVES?? change this
def loadDefaults(instPrefix):
  if instPrefix == 'mar' : instPrefix = 'MAR'
  if instPrefix == 'MAR' : return MARI
  else : raise Exception("As only MARI is supported at the moment the first argument must be MAR")

#called r in DIAG
DetectorEfficiencyVariation_Variation = 1.1