"""*WIKI* 
== Usage ==

== Source Code ==
The source code for the Python Algorithm may be viewed at: [http://trac.mantidproject.org/mantid/browser/trunk/Code/Mantid/Framework/PythonAPI/PythonAlgorithms/OSIRISDiffractionReduction.py OSIRISDiffractionReduction.py]

The source code for the reducer class which is used may be viewed at: [http://trac.mantidproject.org/mantid/browser/trunk/Code/Mantid/scripts/Inelastic/osiris_diffraction_reducer.py osiris_diffraction_reducer.py]

*WIKI*"""

from MantidFramework import *
from mantidsimple import *

import osiris_diffraction_reducer as odr

class OSIRISDiffractionReduction(PythonAlgorithm):
    """This Python Algorithm handles the reducer for OSIRIS Diffraction Data.
    """
    
    def PyInit(self):
        """Initialise algorithm.
        """
        self.declareListProperty("Sample", int)
        self.declareListProperty("Vanadium", int)
        self.declareProperty("CalFile", "")
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction.Output)

    def PyExec(self):
        """Execute the reducer. Basically just runs the file finder for the run
        numbers given.
        """
        mtd.settings["default.instrument"] = 'OSIRIS'
        if self.getPropertyValue("OutputWorkspace") != "":
            reducer = odr.OsirisDiffractionReducer(
                OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
        else:
            reducer = odr.OsirisDiffractionReducer()

        for sam in self.getProperty("Sample"): # Sample files
            try:
                val = FileFinder.findRuns(str(sam))[0]
            except IndexError:
                sys.exit("Unable to find run: "+str(sam))
            reducer.append_data_file(val)            
        for van in self.getProperty("Vanadium"):
            try:
                val = FileFinder.findRuns(str(van))[0]
            except IndexError:
                sys.exit("Could not locate vanadium file.")
            reducer.append_vanadium_file(val)
        
        reducer.set_cal_file(self.getProperty("CalFile"))        
        reducer.reduce()
        
        self.setProperty("OutputWorkspace", mtd[reducer.result_workspace()])
        
    def category(self):
        return 'Diffraction;PythonAlgorithms'

mtd.registerPyAlgorithm(OSIRISDiffractionReduction())
