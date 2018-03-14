from __future__ import (absolute_import, print_function)

import numpy as np

import unittest
from mantid.simpleapi import *
from mantid.api import *

class MuonMaxEntTest(unittest.TestCase):
    def genData(self):
         x_data = np.linspace(0,30.,100)
         e_data = np.cos(0.0*x_data)
         y_data = np.sin(2.3*x_data+0.1)*np.exp(-x_data/2.19703)
         inputData= CreateWorkspace(DataX=x_data,DataY=y_data,DataE=e_data,UnitX='Time')
         return inputData

    def genData2(self):
         x_data = np.linspace(0,30.,100)
         e_data = []
         y_data = []
         for xx in x_data:
               y_data.append(np.sin(2.3*xx+0.1)*np.exp(-xx/2.19703))
               e_data.append(np.cos(0.2*xx))
         for xx in x_data:
               y_data.append(np.sin(4.3*xx+0.2)*np.exp(-xx/2.19703))
               e_data.append( np.cos(0.2*xx))

         inputData= CreateWorkspace(DataX=x_data,DataY=y_data,DataE=e_data,NSpec=2,UnitX='Time')
         return inputData

    def cleanUp(self):
        DeleteWorkspace("InputData")
        DeleteWorkspace("freq")
        DeleteWorkspace("time")
        DeleteWorkspace("phase")

    def test_executes(self):
        inputData = self.genData()
        MuonMaxent(InputWorkspace=inputData,Npts=32768,FitDeaDTime=False,FixPhases=True ,OuterIterations=1,InnerIterations=1,OutputWorkspace='freq',ReconstructedSpectra='time',OutputPhaseTable="phase")
        freq = AnalysisDataService.retrieve("freq")   
        time = AnalysisDataService.retrieve("time")
        phase = AnalysisDataService.retrieve("phase")
        self.assertEqual(freq.getNumberHistograms(),1)    
        self.assertEqual(time.getNumberHistograms(),1)    
        self.assertEqual(phase.rowCount(),1)    
        self.cleanUp()

    def test_exitOnNAN(self):
        inputData = self.genData()
        try:
            MuonMaxent(InputWorkspace=inputData,Npts=32768,FitDeaDTime=False,FixPhases=True ,OuterIterations=10,InnerIterations=10,OutputWorkspace='freq',ReconstructedSpectra='time',OutputPhaseTable="phase")
        except RuntimeError:
            pass
        else:
           self.fail("should throw an error as it will get stuck in a loop due to NAN values")
        finally:
           DeleteWorkspace("InputData")

    def test_multipleSpec(self):
        inputData = self.genData2()
        MuonMaxent(InputWorkspace=inputData,Npts=32768,FitDeaDTime=False,FixPhases=True ,OuterIterations=1,InnerIterations=1,OutputWorkspace='freq',ReconstructedSpectra='time',OutputPhaseTable="phase")
        freq = AnalysisDataService.retrieve("freq")   
        time = AnalysisDataService.retrieve("time")
        phase = AnalysisDataService.retrieve("phase")
        self.assertEqual(freq.getNumberHistograms(),1)    
        self.assertEqual(time.getNumberHistograms(),2)    
        self.assertEqual(phase.rowCount(),2)    
        self.cleanUp()
 
if __name__ == '__main__':
    unittest.main()
