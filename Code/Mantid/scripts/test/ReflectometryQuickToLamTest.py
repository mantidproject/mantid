import unittest
import numpy
from mantid.simpleapi import *
from mantid.api import *
from isis_reflgui import quick

class ReflectometryQuickToLamTest(unittest.TestCase):
    
    __wsName = None
    
    def setUp(self):
        self.__wsName = "TestWorkspace"
        LoadISISNexus(Filename='INTER00013460', OutputWorkspace=self.__wsName)
    
    def tearDown(self):
        if self.__wsName in mtd.getObjectNames():
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
        
        
    def test_check_lambda_range(self):
        
        # Expected Min and Max x wavelength values on output.
        inst = mtd[self.__wsName].getInstrument()
        expectedLambdaMin = inst.getNumberParameter('LambdaMin')[0]
        expectedLambdaMax = inst.getNumberParameter('LambdaMax')[0]
        
        firstWSName = mtd[self.__wsName].name()
        
        # Run Quick
        quick.toLam(firstWSName, firstWSName)
        
        # Get output workspace
        detectorWSInLam = mtd['_D' + self.__wsName] 
        
        # Check that output workspace has been cropped.
        x = detectorWSInLam.readX(0)
        self.assertAlmostEquals(expectedLambdaMin, x[0], 0)
        self.assertAlmostEquals(expectedLambdaMax, x[-1], 0)
        
    def test_workspace_splitting_monitor_detector(self):
        # Expected Min and Max x wavelength values on output.
        inst = mtd[self.__wsName].getInstrument()
        originalNHisto = mtd[self.__wsName].getNumberHistograms()
        
        pointDetectorStartIndex = inst.getNumberParameter('PointDetectorStart')[0]
        pointDetectorStopIndex = inst.getNumberParameter('PointDetectorStop')[0]
        multiDetectorStartIndex = inst.getNumberParameter('MultiDetectorStart')[0]
        
        histoRangeMonitor = pointDetectorStartIndex # zero to pointDetectorStartIndex
        histoRangePointDetector = originalNHisto - pointDetectorStartIndex
        histoRangeMultiDetector = originalNHisto - multiDetectorStartIndex
        
        firstWSName = mtd[self.__wsName].name()
        
        # Run Quick
        quick.toLam(firstWSName, firstWSName)
        
        # Get output workspace
        pointDetectorWSInLam = mtd['_D' + self.__wsName] 
        monitorWSInLam = mtd['_M' + self.__wsName] 
        
        # Check histogram ranges
        self.assertEquals(histoRangeMonitor, monitorWSInLam.getNumberHistograms())
        self.assertEquals(histoRangePointDetector, pointDetectorWSInLam.getNumberHistograms())
        
if __name__ == '__main__':
    unittest.main()