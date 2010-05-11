import CommonFunctions as common

MARI = common.defaults(
    background_range=(18000.0,19500.0),
    normalization='monitor-monitor 1',
    instrument_pref='MAR',
    white_beam_integr=(20.0,40.0),
    scale_factor=1.8182e8,
    monitor1_integr=(1000,2000),
    white_beam_scale=1000.,
    getei_monitors=(2,3)
  )

MAPS = common.defaults(
    background_range=(12000.0,18000.0),
    normalization='monitor-monitor 1',
    instrument_pref='MAP',
    white_beam_integr=(20.0,300.0),
    scale_factor=1.7016e8,
    monitor1_integr=(1000,2000),
    white_beam_scale=1000.,
    getei_monitors=(41474,41475)
  )

MERLIN = common.defaults(
    background_range=(12000.0,18000.0),
    normalization='monitor-monitor 1',
    instrument_pref='MER',
    white_beam_integr=(20.0,55.0),
    scale_factor=1.7016e8,
    monitor1_integr=(1000,2000),
    white_beam_scale=1000.,
    getei_monitors=(69634,69638)
  )

def loadDefaults(prefix):
    prefix = prefix.lower()
    if prefix == 'mar':
        return MARI
    elif prefix == 'map':
        return MAPS
    elif prefix == 'mer':
        return MERLIN
    else:
        raise RunTimeError("Unknown instrument prefix selected.")

#called r in DIAG
DetectorEfficiencyVariation_Variation = 1.1