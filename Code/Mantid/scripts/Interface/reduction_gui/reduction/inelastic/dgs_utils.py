IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    from mantid.kernel import ConfigService
    import mantid.simpleapi as api
    IS_IN_MANTIDPLOT = True
except:
    pass

import os

class InstrumentParameters(object):
    instrument_name = None
    _instrument = None
    
    def __init__(self, inst_name):
        if self.instrument_name is None:
            self._load_param_file(inst_name)
        else:
            if self.instrument_name != inst_name:
                self._load_param_file(inst_name)
                
    def _load_param_file(self, inst_name):
        InstrumentParameters.instrument_name = inst_name
        ws_name = "__emptyInst_" + inst_name
        if IS_IN_MANTIDPLOT:
            idf_loc = ConfigService.Instance().getInstrumentDirectory()
            idf_pattern = os.path.join(idf_loc, "%s_Definition*.xml") % inst_name
            import glob
            idf_files = glob.glob(idf_pattern)
            output_ws = api.LoadEmptyInstrument(Filename=idf_files[0], 
                                         OutputWorkspace=ws_name)
            InstrumentParameters._instrument = output_ws.getInstrument()

    def _self_check(self):
        if self._instrument is None:
            raise ValueError("Instrument was not loaded, cannot retrieve parameters.")

    def get_parameter(self, parname):
        default = -1
        try:
            self._self_check()
        except ValueError:
            return default
        values = self._instrument.getNumberParameter(parname)
        try:
            return values[0]
        except IndexError:
            return default
        
    def get_bool_param(self, parname):
        default = False
        try:
            self._self_check()
        except ValueError:
            return default
        values = self._instrument.getNumberParameter(parname)
        try:
            return bool(values[0])
        except IndexError:
            return default
        