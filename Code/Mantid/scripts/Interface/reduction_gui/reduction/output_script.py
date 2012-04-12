from scripter import BaseScriptElement

# Check whether we are running in MantidPlot
IS_IN_MANTIDPLOT = False
try:
    from MantidFramework import *
    mtd.initialise()
    from reduction.command_interface import *
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
        print 'in output_script, inside update'
        if IS_IN_MANTIDPLOT:
            self.log_text = ReductionSingleton().log_text
            try:
                for item in ReductionSingleton().output_workspaces:
                    mantidplot.plotSpectrum(item, 0, True)
            except:
                raise RuntimeError, "Could not plot resulting output\n  %s" % sys.exc_value