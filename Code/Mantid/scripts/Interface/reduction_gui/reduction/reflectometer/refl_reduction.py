"""
    This class holds all the necessary information to create a reduction script.
"""
import time
import os
from reduction_gui.reduction.scripter import BaseReductionScripter

class REFLReductionScripter(BaseReductionScripter):
    """
        Reduction scripter for REFL
    """

    def __init__(self, name="REFL"):
        super(REFLReductionScripter, self).__init__(name=name)

    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        script = "# %s reduction script\n" % self.instrument_name
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())

        script += "import mantid\n"
        script += "from mantid.simpleapi import *\n"
        script += "\n"
        script += "#remove all previous workspaces\n"
        script += "list_mt = AnalysisDataService.getObjectNames()\n"
        script += "for _mt in list_mt:\n"
        script += "    if _mt.find('_scaled') != -1:\n"
        script += "        AnalysisDataService.remove(_mt)\n"
        script += "    if _mt.find('reflectivity') != -1:\n"
        script += "        AnalysisDataService.remove(_mt)\n"
        script += "\n"

        for item in self._observers:
            if item.state() is not None:
                script += str(item.state())

        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()

        return script


