from scripter import BaseScriptElement

# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    from reduction.instruments.sans.hfir_command_interface import *
    HAS_MANTID = True
except:
    HAS_MANTID = False  
    
# Check whether we are running in MantidPlot
IS_IN_MANTIDPLOT = False
try:
    import mantidplot
    IS_IN_MANTIDPLOT = True
except:
    pass
 
class Output(BaseScriptElement):
    log_text = ''
    data = None
    
    def update(self):
        """
            Update data members according to reduction results
        """
        self.log_text = ReductionSingleton().log_text
        if HAS_MANTID and ReductionSingleton()._azimuthal_averager is not None:
            self.data = ReductionSingleton()._azimuthal_averager.get_data(ReductionSingleton()._data_files.keys()[0])
    
            if IS_IN_MANTIDPLOT:
                try:
                    mantidplot.plotSpectrum(ReductionSingleton()._data_files.keys()[0]+ReductionSingleton()._azimuthal_averager._suffix, 0, True)
                except:
                    raise RuntimeError, "Could not plot resulting I(Q)\n  %s" % sys.exc_value