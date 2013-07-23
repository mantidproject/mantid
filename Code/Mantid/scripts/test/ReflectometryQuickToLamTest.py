import unittest
import numpy
from mantid.simpleapi import *
from mantid.api import *
from isis_reflgui import quick

class ReflectometryQuickToLamTest(unittest.TestCase):
    
    __wsName = None
    
    def __init__(self, methodName='runTest'):
        super(ReflectometryQuickToLamTest, self).__init__(methodName)
        self.__wsName = "TestWorkspace"
        LoadISISNexus(Filename='INTER00013460', OutputWorkspace=self.__wsName)
    
    def __del__(self):
        DeleteWorkspace(mtd[self.__wsName])
        
    def test_basic_type_checks(self):
        firstWSName = mtd[self.__wsName].name()
        
        # Run Quick
        quick.toLam(firstWSName, firstWSName)
        
        object_names = mtd.getObjectNames()
        self.assertTrue('_W'+self.__wsName in object_names)
        self.assertTrue('_M'+self.__wsName in object_names)
        
        ''' Internal renaming of inputs. Quick will need refactoring to stop this sort of thing, but 
            at least it get's documented here.
        '''
        detectorWSInTOF = mtd['_W' + self.__wsName]
        monitiorWSInTOF = mtd['_M' + self.__wsName]
        # Fetch output workspace
        detectorWSInLam = mtd['_W' + self.__wsName + '_lam'] 
        # Check workspace type
        self.assertTrue(isinstance(detectorWSInLam, MatrixWorkspace))
        # Check units have switched to wavelength (hence lam - lambda in name)
        self.assertEquals("Wavelength", detectorWSInLam.getAxis(0).getUnit().caption())
        
        

    
if __name__ == '__main__':
    unittest.main()