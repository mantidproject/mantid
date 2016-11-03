#pylint: disable=bare-except,invalid-name
import sys
# Check whether Mantid is available
try:
    from mantid.api import AnalysisDataService
    from mantid.kernel import Logger
    logger = Logger("hfir_data_proxy")
    import mantid.simpleapi as api
    HAS_MANTID = True
except ImportError:
    HAS_MANTID = False


class DataProxy(object):
    """
        Class used to load a data file temporarily to extract header information:
        HFIR SANS Data files have the following properties (parsed from the data file!)
        "sample-detector-distance-offset"
        "sample-detector-distance"
        "sample-si-window-distance"
        "sample_detector_distance"
    """
    wavelength = None
    wavelength_spread = None
    sample_detector_distance = None
    sample_detector_distance_offset = None
    sample_si_window_distance = None
    # If it was moved before that's where the distance is:
    sample_detector_distance_moved = None
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
                self.sample_detector_distance = ws.getRun().getProperty("sample-detector-distance").value
                self.sample_detector_distance_offset = ws.getRun().getProperty("sample-detector-distance-offset").value
                self.sample_si_window_distance = ws.getRun().getProperty("sample-si-window-distance").value
                self.sample_detector_distance_moved = ws.getRun().getProperty("sample_detector_distance").value

                self.sample_thickness = ws.getRun().getProperty("sample-thickness").value
                self.beam_diameter = ws.getRun().getProperty("beam-diameter").value

                logger.notice("Loaded data file: %s" % data_file)
            except:
                logger.error("Error loading data file:\n%s" % sys.exc_value)
                self.errors.append("Error loading data file:\n%s" % sys.exc_value)
