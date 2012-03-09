"""
    This class holds all the necessary information to create a reduction script.
    This is a fake version of the Reducer for testing purposes.
"""
import time
from reduction_gui.reduction.scripter import BaseReductionScripter
# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    HAS_MANTID = True
except:
    HAS_MANTID = False  

class REFLReductionScripter(BaseReductionScripter):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """
    
    def __init__(self, name="REFL"):
        super(REFLReductionScripter, self).__init__(name=name)        
    
    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        script = "# %s reduction script\n" % self.instrument_name
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())
        
        script += "import os\n"
        script += "from MantidFramework import *\n"
        script += "mtd.initialise(False)\n"
        script += "from mantidsimple import *\n\n"
                
        script += "REF_RED_OUTPUT_MESSAGE = ''\n\n"
        
        for item in self._observers:
            if item.state() is not None:
                script += str(item.state())
                
        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()
        
        return script
   
    def apply(self):
        """
            Apply the reduction process
        """
        if HAS_MANTID:
            script = self.to_script(None)
            try:
                t0 = time.time()
                exec script
                delta_t = time.time()-t0
                print REF_RED_OUTPUT_MESSAGE
                print "Reduction time: %5.2g sec" % delta_t
                # Update scripter
                for item in self._observers:
                    if item.state() is not None:
                        item.state().update()
            except:
                # Update scripter [Duplicated code because we can't use 'finally' on python 2.4]
                for item in self._observers:
                    if item.state() is not None:
                        # Things might be broken, so update what we can
                        try:
                            item.state().update()
                        except:
                            pass
                raise RuntimeError, sys.exc_value
        else:
            raise RuntimeError, "Reduction could not be executed: Mantid could not be imported"
        

    