import unittest
from mantid import MatrixWorkspace, AnalysisDataService
from mantid.simpleapi import CreateWorkspace
from testhelpers import run_algorithm
import numpy as np

class CreateWorkspaceTest(unittest.TestCase):
  
    def test_create_with_1D_numpy_array(self):
        x = np.array([1.,2.,3.,4.])
        y = np.array([1.,2.,3.])
        e = np.sqrt(np.array([1.,2.,3.]))
        
        wksp = CreateWorkspace(DataX=x, DataY=y,DataE=e,NSpec=1,UnitX='TOF')
        self.assertTrue(isinstance(wksp, MatrixWorkspace))
        self.assertEquals(wksp.getNumberHistograms(), 1)
        
        self.assertEquals(len(wksp.readY(0)), len(y))
        self.assertEquals(len(wksp.readX(0)), len(x))
        self.assertEquals(len(wksp.readE(0)), len(e))

        for index in range(len(y)):
            self.assertEquals(wksp.readY(0)[index], y[index])
            self.assertEquals(wksp.readE(0)[index], e[index])
            self.assertEquals(wksp.readX(0)[index], x[index])
        # Last X value
        self.assertEquals(wksp.readX(0)[len(x)-1], x[len(x)-1])
        AnalysisDataService.Instance().remove("wksp")

    def test_create_with_2D_numpy_array(self):
        x = np.array([1.,2.,3.,4.])
        y = np.array([[1.,2.,3.],[4.,5.,6.]])
        e = np.sqrt(y)

        wksp = CreateWorkspace(DataX=x, DataY=y,DataE=e,NSpec=2,UnitX='TOF')
        self.assertTrue(isinstance(wksp, MatrixWorkspace))
        self.assertEquals(wksp.getNumberHistograms(), 2)

        for i in [0,1]:
            for j in range(len(y[0])):
                self.assertEquals(wksp.readY(i)[j], y[i][j])
                self.assertEquals(wksp.readE(i)[j], e[i][j])
                self.assertEquals(wksp.readX(i)[j], x[j])
            # Last X value
            self.assertEquals(wksp.readX(i)[len(x)-1], x[len(x)-1])
        
        AnalysisDataService.Instance().remove("wksp")
        
    def test_with_data_from_other_workspace(self):
        wsname = 'LOQ'
        alg = run_algorithm('Load', Filename='LOQ48127.raw', OutputWorkspace=wsname, SpectrumMax=2, child=True)
        loq = alg.getProperty("OutputWorkspace").value
        
        x = loq.extractX()
        y = loq.extractY()
        e = loq.extractE()
        
        wksp = CreateWorkspace(DataX=x, DataY=y,DataE=e,NSpec=2,UnitX='Wavelength')
        self.assertTrue(isinstance(wksp, MatrixWorkspace))
        self.assertEquals(wksp.getNumberHistograms(), 2)
        
        for i in [0,1]:
            for j in range(len(y[0])):
                self.assertEquals(wksp.readY(i)[j], loq.readY(i)[j])
                self.assertEquals(wksp.readE(i)[j], loq.readE(i)[j])
                self.assertEquals(wksp.readX(i)[j], loq.readX(i)[j])
            # Last X value
            self.assertEquals(wksp.readX(i)[len(x)-1], loq.readX(i)[len(x)-1])

        AnalysisDataService.Instance().remove("wksp")
