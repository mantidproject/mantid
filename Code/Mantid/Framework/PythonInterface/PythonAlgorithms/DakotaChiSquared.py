"""*WIKI* 
Compare two nexus files containing matrix workspaces and output chi squared into a file
*WIKI*"""

from mantid.api import PythonAlgorithm, registerAlgorithm,WorkspaceProperty
from mantid.kernel import Direction,IntBoundedValidator,FloatBoundedValidator
import mantid.simpleapi 
import mantid
import numpy
from string import *

class DakotaChiSquared(PythonAlgorithm):
    """ Get incident energy from a monitor and some detectors
    """
    def category(self):
        """ Return category
        """
        return "PythonAlgorithms;Utility\\Workspaces"
    
    def name(self):
        """ Return name
        """
        return "DakotaChiSquared"
    
    def PyInit(self):
        """ Declare properties
        """
	f1=mantid.api.FileProperty("DataFile","",mantid.api.FileAction.Load,".nxs")
        self.declareProperty(f1,"Input Nexus file containing data.")	
	f2=mantid.api.FileProperty("CalculatedFile","",mantid.api.FileAction.Load,".nxs")
        self.declareProperty(f2,"Input Nexus file containing calculated data.")
	fout=mantid.api.FileProperty("OutputFile","",mantid.api.FileAction.Save,".xml")
        self.declareProperty(fout,"Output filename containing chi^2.")
	self.declareProperty("ChiSquared",0.0,Direction.Output)
        
        return
    
    def PyExec(self):
        """ Main execution body
        """
	f1 = self.getProperty("DataFile").value
	f2 = self.getProperty("CalculatedFile").value
	fout = self.getProperty("OutputFile").value

        __w1=mantid.simpleapi.Load(f1)
        __w2=mantid.simpleapi.Load(f2)   

	#TODO validata inputs   
	__diff=__w1-__w2
        
	__soe=mantid.simpleapi.SignalOverError(__diff)
	__soe2=__soe*__soe
	__soe2=mantid.simpleapi.ReplaceSpecialValues(__soe2,0,0,0,0)
	data=__soe2.extractY()

	chisquared=numpy.sum(data)

	f = open(fout,'w')
	f.write(str(chisquared)+' obj_fn\n')
	f.close()
        self.setProperty("ChiSquared",chisquared)
	mantid.simpleapi.DeleteWorkspace(__w1.getName())
	mantid.simpleapi.DeleteWorkspace(__w2.getName())
	mantid.simpleapi.DeleteWorkspace(__diff.getName())
	mantid.simpleapi.DeleteWorkspace(__soe.getName())
	mantid.simpleapi.DeleteWorkspace(__soe2.getName())
        return 
    
    
registerAlgorithm(DakotaChiSquared)
