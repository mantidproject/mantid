# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,too-many-public-methods,too-many-arguments
from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
import mantid.simpleapi as api
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os, json

class SavePlot1DAsJsonTest(unittest.TestCase):

    def test_save_one_curve(self):
        """ Test to Save one curve
        """
        datawsname = "constant energy cut"
        E, I, err, dQ = self._createOneQCurve(datawsname)

        # Execute
        out_path = "tempout_curve.json"
        alg_test = run_algorithm(
            "SavePlot1DAsJson",
            InputWorkspace = datawsname,
            JsonFilename = out_path)
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        d = json.load(open(out_path))[datawsname]
        self.assertEqual(d['type'], 'point')
        self._checkData(d, E, I, err, dE=dQ)
        # Delete the output file
        os.remove(out_path)
        return


    def test_save_one_histogram(self):
        """ Test to Save one histogram
        """
        datawsname = "TestOneHistogram"
        E, I, err = self._createOneHistogram(datawsname)

        # Execute
        out_path = "tempout_hist.json"
        alg_test = run_algorithm(
            "SavePlot1DAsJson",
            InputWorkspace = datawsname,
            JsonFilename = out_path)
        # Executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        d = json.load(open(out_path))[datawsname]
        self._checkData(d, E, I, err)
        # test overwrite
        alg_test = run_algorithm(
            "SavePlot1DAsJson",
            InputWorkspace = datawsname,
            JsonFilename = out_path)
        # Delete the output file
        os.remove(out_path)
        return


    def test_save_two_curves(self):
        """ Test to Save two curves
        """
        datawsname = "TestTwoCurves"
        E, I, err, I2, err2 = self._createTwoCurves(datawsname)
        # Execute
        out_path = "tempout_2curves.json"
        alg_test = run_algorithm(
            "SavePlot1DAsJson",
            InputWorkspace = datawsname,
            JsonFilename = out_path)
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        d = json.load(open(out_path))[datawsname]
        self._checkData(d, E, I, err)
        self._checkData(d, E, I2, err2, ID="2")
        # Delete the output file
        os.remove(out_path)
        return


    def test_save_one_curve_withdesignatedname(self):
        """ Test to Save one curve with a name specified by client
        """
        datawsname = "TestOneCurve"
        plotname = "myplot"
        E, I, err = self._createOneCurve(datawsname)
        # Execute
        out_path = "tempout_curve_withname.json"
        alg_test = run_algorithm(
            "SavePlot1DAsJson",
            InputWorkspace = datawsname,
            JsonFilename = out_path,
            PlotName = plotname)
        # executed?
        self.assertTrue(alg_test.isExecuted())
        # Verify ....
        d = json.load(open(out_path))[plotname]
        self._checkData(d, E, I, err)
        # Delete the output file
        os.remove(out_path)
        return


    def _checkData(self, s, E, I, err, ID="1", dE=None):
        d0 = s["data"][ID]
        np.testing.assert_array_equal(d0[0], E)
        np.testing.assert_array_equal(d0[1], I)
        np.testing.assert_array_equal(d0[2], err)
        if dE is not None:
            np.testing.assert_array_equal(d0[3], dE)
        return


    def _createOneCurve(self, datawsname):
        """ Create data workspace
        """
        E = np.arange(-50, 50, 10.0)
        I = 1000 * np.exp(-E**2/10**2)
        err = I ** .5
        # create workspace
        dataws = api.CreateWorkspace(
            DataX = E, DataY = I, DataE = err, NSpec = 1,
            UnitX = "Energy")
        # Add to data service
        AnalysisDataService.addOrReplace(datawsname, dataws)
        return E, I, err


    def _createOneQCurve(self, datawsname):
        """ Create data workspace
        """
        Q = np.arange(0, 13, 1.0)
        dQ = 0.1*Q
        I = 1000 * np.exp(-Q**2/10**2)
        err = I ** .5
        # create workspace
        dataws = api.CreateWorkspace(
            DataX = Q, DataY = I, DataE = err, NSpec = 1,
            UnitX = "Momentum")
        dataws.setDx(0, dQ)
        # Add to data service
        AnalysisDataService.addOrReplace(datawsname, dataws)
        return Q, I, err, dQ


    def _createOneHistogram(self, datawsname):
        """ Create data workspace
        """
        E = np.arange(-50.5, 50, 1.0)
        Ecenters = (E[:-1] + E[1:]) / 2
        I = 1000 * np.exp(-Ecenters**2/10**2)
        err = I ** .5
        # create workspace
        dataws = api.CreateWorkspace(
            DataX = E, DataY = I, DataE = err, NSpec = 1,
            UnitX = "Energy(meV)")
        # Add to data service
        AnalysisDataService.addOrReplace(datawsname, dataws)
        return E, I, err


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
        return E, I, err, I2, err2


if __name__ == '__main__':
    unittest.main()
