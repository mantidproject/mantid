"""
    Scripter class
"""
import time
from scripter import BaseReductionScripter

class %INSTR_NAME%Scripter(BaseReductionScripter):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """
    
    def __init__(self, name="%INSTR_NAME%"):
        super(%INSTR_NAME%Scripter, self).__init__(name=name)        
    
    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        script = "# Reduction script\n"
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())
        
        script += "from MantidFramework import *\n"
        script += "mtd.initialise(False)\n"
        script += "from mantidsimple import *"
        script += "\n"
        
        # The following code translates each panel/tab into python script
        for item in self._observers:
            if item.state() is not None:
                script += str(item.state())
                
        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()
        
        return script
        

    