import sys
 
# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    import mantidsimple
    from  reduction.instruments.sans.sans_reduction_steps import LoadRun
    from reduction.instruments.sans.sans_reducer import SANSReducer
    import reduction.instruments.sans.hfir_instrument as hfir_instrument

    HAS_MANTID = True
except:
    HAS_MANTID = False 
    
# Check whether we have numpy
try:
    import numpy
    HAS_NUMPY = True
except:
    HAS_NUMPY = False

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
                
                if False and HAS_NUMPY:
                    nx_pixels = int(mtd[self.data_ws].getInstrument().getNumberParameter("number-of-x-pixels")[0])
                    ny_pixels = int(mtd[self.data_ws].getInstrument().getNumberParameter("number-of-y-pixels")[0])
                    nMonitors = int(mtd[self.data_ws].getInstrument().getNumberParameter("number-of-monitors")[0])
                    raw_data = numpy.zeros(nx_pixels*ny_pixels)
                    for i in range(nMonitors-1, nx_pixels*ny_pixels+nMonitors ):
                        raw_data[i-nMonitors] = mtd[self.data_ws].readY(i)[0]
                        
                    self.data = numpy.reshape(raw_data, (nx_pixels, ny_pixels), order='F')
            except:
                self.errors.append("Error loading data file:\n%s" % sys.exc_value)
            