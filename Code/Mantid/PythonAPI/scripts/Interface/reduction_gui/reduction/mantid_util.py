import sys
 
# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    from SANSReductionSteps import LoadRun
    from SANSReducer import SANSReducer
    import HFIRInstrument
    HAS_MANTID = True
except:
    HAS_MANTID = False 
    
# Check whether we have numpy
try:
    import numpy
    HAS_NUMPY = True
except:
    HAS_NUMPY = False
    
class DataFileProxy(object):
    
    wavelength = None
    wavelength_spread = None
    sample_detector_distance = None
    data = None
    
    ## Error log
    errors = []
    
    def __init__(self, data_file):
        if HAS_MANTID:
            try:
                reducer = SANSReducer()
                reducer.set_instrument(HFIRInstrument.HFIRSANS())
                loader = LoadRun(str(data_file))
                loader.execute(reducer, "raw_data_file")
                x = mtd["raw_data_file"].dataX(0)
                self.wavelength = (x[0]+x[1])/2.0
                self.wavelength_spread = x[1]-x[0]
                self.sample_detector_distance = reducer.instrument.sample_detector_distance
                
                if HAS_NUMPY:
                    raw_data = numpy.zeros(reducer.instrument.nx_pixels*reducer.instrument.ny_pixels)
                    for i in range(reducer.instrument.nMonitors-1, reducer.instrument.nx_pixels*reducer.instrument.ny_pixels+reducer.instrument.nMonitors ):
                        raw_data[i-reducer.instrument.nMonitors] = mtd["raw_data_file"].readY(i)[0]
                        
                    self.data = numpy.reshape(raw_data, (reducer.instrument.nx_pixels, reducer.instrument.ny_pixels), order='F')
            except:
                self.errors.append("Error loading data file:\n%s" % sys.exc_value)
            
            
            