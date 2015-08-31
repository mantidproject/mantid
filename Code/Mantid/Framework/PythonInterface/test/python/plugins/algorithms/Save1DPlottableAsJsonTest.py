import unittest
import numpy as np
import mantid.simpleapi as api
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os

class SaveVulcanGSSTest(unittest.TestCase):

    def test_save_one_curve(self):
        """ Test to Save one curve
        """
        datawsname = "TestOneSurve"
        self._createOneCurve(datawsname)

        # Execute
        out_path = "tempout_curve.json"
        alg_test = run_algorithm(
            "Save1DPlottableAsJson", 
            InputWorkspace = datawsname,
            JsonFilename = out_path)
        
        self.assertTrue(alg_test.isExecuted())

        # Verify ....
        
        # Delete the output file
        # os.remove(out_path)
        return


    def test_save_one_histogram(self):
        """ Test to Save one histogram
        """
        datawsname = "TestOneHistogram"
        self._createOneHistogram(datawsname)

        # Execute
        out_path = "tempout_hist.json"
        alg_test = run_algorithm(
            "Save1DPlottableAsJson", 
            InputWorkspace = datawsname,
            JsonFilename = out_path)
        
        self.assertTrue(alg_test.isExecuted())
        
        # Verify ....

        # Delete the output file
        # os.remove(out_path)
        return


    def test_save_two_curves(self):
        """ Test to Save two curves
        """
        datawsname = "TestTwoCurves"
        self._createTwoCurves(datawsname)
        
        # Execute
        out_path = "tempout_2curves.json"
        alg_test = run_algorithm(
            "Save1DPlottableAsJson", 
            InputWorkspace = datawsname,
            JsonFilename = out_path)
        
        self.assertTrue(alg_test.isExecuted())
        
        # Verify ....

        # Delete the output file
        # os.remove(out_path)
        return


    def _createOneCurve(self, datawsname):
        """ Create data workspace
        """
        E = np.arange(-50, 50, 1.0)
        I = 1000 * np.exp(-E**2/10**2)
        err = I ** .5
        
        dataws = api.CreateWorkspace(
            DataX = E, DataY = I, DataE = err, NSpec = 1, 
            UnitX = "Energy(meV)")
        
        # Add to data service
        AnalysisDataService.addOrReplace(datawsname, dataws)
        return dataws


    def _createOneHistogram(self, datawsname):
        """ Create data workspace
        """
        E = np.arange(-50.5, 50, 1.0)
        Ecenters = (E[:-1] + E[1:]) / 2
        I = 1000 * np.exp(-Ecenters**2/10**2)
        err = I ** .5
        
        dataws = api.CreateWorkspace(
            DataX = E, DataY = I, DataE = err, NSpec = 1, 
            UnitX = "Energy(meV)")
        
        # Add to data service
        AnalysisDataService.addOrReplace(datawsname, dataws)
        return dataws


    def _createTwoCurves(self, datawsname):
        """ Create data workspace
        """
        E = np.arange(-50, 50, 1.0)
        # curve 1
        I = 1000 * np.exp(-E**2/10**2)
        err = I ** .5
        # curve 2
        I2 = 1000 * (1+np.sin(E/5*np.pi))
        err2 = I ** .5
        
        # workspace
        ws = WorkspaceFactory.create(
            "Workspace2D", NVectors=2, 
            XLength = E.size, YLength = I.size
            )
        # curve1
        ws.dataX(0)[:] = E
        ws.dataY(0)[:] = I
        ws.dataE(0)[:] = err
        # curve2
        ws.dataX(1)[:] = E
        ws.dataY(1)[:] = I2
        ws.dataE(1)[:] = err2
        
        # Add to data service
        AnalysisDataService.addOrReplace(datawsname, ws)
        return ws


if __name__ == '__main__': unittest.main()
