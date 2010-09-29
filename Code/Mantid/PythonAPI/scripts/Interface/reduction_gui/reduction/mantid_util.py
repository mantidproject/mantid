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
    
class DataFileProxy(object):
    
    wavelength = None
    wavelength_spread = None
    sample_detector_distance = None
    
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
            except:
                self.errors.append("Error loading data file:\n%s" % sys.exc_value)
            
            
            