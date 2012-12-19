import sys
     
class DataProxy(object):
    """
        Class used to load a data file temporarily to extract header information
    """
    wavelength = None
    wavelength_spread = None
    sample_detector_distance = None
    data = None
    data_ws = ''
    sample_thickness = None
    beam_diameter = None
    python_api = 1
    
    ## Error log
    errors = []
    
    def __init__(self, data_file, workspace_name=None, python_api=1):
        self.errors = []
        self.python_api = python_api
        if python_api==1:
            return self._load_python_api_1(data_file, workspace_name)
        else:
            return self._load_python_api_2(data_file, workspace_name)
            
    def _load_python_api_1(self, data_file, workspace_name):
        from MantidFramework import mtd
        import mantidsimple
        from  reduction.instruments.sans.hfir_load import LoadRun
        from reduction.instruments.sans.sans_reducer import SANSReducer
        import reduction.instruments.sans.hfir_instrument as hfir_instrument
        try:
            if workspace_name is None:
                self.data_ws = "__raw_data_file"
            else:
                self.data_ws = str(workspace_name)
            reducer = SANSReducer()
            reducer.set_instrument(hfir_instrument.HFIRSANS())
            loader = LoadRun(str(data_file))
            loader.execute(reducer, self.data_ws)
            x = mtd[self.data_ws].dataX(0)
            self.wavelength = (x[0]+x[1])/2.0
            self.wavelength_spread = x[1]-x[0]
            self.sample_detector_distance = mtd[self.data_ws].getRun().getProperty("sample_detector_distance").value
            self.sample_thickness = mtd[self.data_ws].getRun().getProperty("sample-thickness").value
            self.beam_diameter = mtd[self.data_ws].getRun().getProperty("beam-diameter").value
            
            mtd.sendLogMessage("Loaded data file: %s" % data_file)
        except:
            mtd.sendLogMessage("DataProxy: Error loading data file:\n%s" % sys.exc_value)
            self.errors.append("Error loading data file:\n%s" % sys.exc_value)

    def _load_python_api_2(self, data_file, workspace_name):
        from mantid.api import AnalysisDataService
        from mantid.kernel import Logger
        import mantid.simpleapi as api
        try:
            if workspace_name is None:
                self.data_ws = "__raw_data_file"
            else:
                self.data_ws = str(workspace_name)
            api.HFIRLoad(Filename=str(data_file), OutputWorkspace=self.data_ws)
            ws = AnalysisDataService.retrieve(self.data_ws)
            x = ws.dataX(0)
            self.wavelength = (x[0]+x[1])/2.0
            self.wavelength_spread = x[1]-x[0]
            self.sample_detector_distance = ws.getRun().getProperty("sample_detector_distance").value
            self.sample_thickness = ws.getRun().getProperty("sample-thickness").value
            self.beam_diameter = ws.getRun().getProperty("beam-diameter").value
            
            Logger.get("hfir_data_proxy").information("Loaded data file: %s" % data_file)
        except:
            Logger.get("hfir_data_proxy").error("Error loading data file:\n%s" % sys.exc_value)
            self.errors.append("Error loading data file:\n%s" % sys.exc_value)
        