from scripter import BaseScriptElement

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
        if IS_IN_MANTIDPLOT:
            # Allow for the old reducer, which uses Python API v1
            if self.PYTHON_API==1:
                from reduction.command_interface import ReductionSingleton
            else:
                from reduction_workflow.command_interface import ReductionSingleton
            self.log_text = ReductionSingleton().log_text
            try:
                if hasattr(ReductionSingleton(), "output_workspaces"):
                    for item in ReductionSingleton().output_workspaces:
                        mantidplot.plotSpectrum(item, 0, True)
            except:
                raise RuntimeError, "Could not plot resulting output\n  %s" % sys.exc_value