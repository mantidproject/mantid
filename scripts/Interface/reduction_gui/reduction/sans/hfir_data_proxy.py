#pylint: disable=invalid-name
import sys
 # Check whether Mantid is available
try:
    from mantid.api import AnalysisDataService
    from mantid.kernel import Logger
    import mantid.simpleapi as api
    HAS_MANTID = True
except:
    HAS_MANTID = False

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

    ## Error log
    errors = []

    def __init__(self, data_file, workspace_name=None):
        self.errors = []
        if HAS_MANTID:
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

                Logger("hfir_data_proxy").information("Loaded data file: %s" % data_file)
            except:
                Logger("hfir_data_proxy").error("Error loading data file:\n%s" % sys.exc_value)
                self.errors.append("Error loading data file:\n%s" % sys.exc_value)
