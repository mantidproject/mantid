from __future__ import (absolute_import, division, print_function)

import unittest
import platform
from mantid.simpleapi import *
from mantid.api import WorkspaceGroup

if platform.system() == "Windows":
    class BayesStretchTest(unittest.TestCase):

        _res_ws = None
        _sample_ws = None
        _resnorm_ws = None
        _num_bins = None
        _num_hists = None

        def setUp(self):
            self._res_ws = Load(Filename='irs26173_graphite002_res.nxs',
                                OutputWorkspace='__BayesStretchTest_Resolution')
            self._sample_ws = Load(Filename='irs26176_graphite002_red.nxs',
                                OutputWorkspace='__BayesStretchTest_Sample')
            self._num_hists = self._sample_ws.getNumberHistograms()


        def tearDown(self):
            """
            Remove workspaces from ADS.
            """
            DeleteWorkspace(self._sample_ws)
            DeleteWorkspace(self._res_ws)


#----------------------------------Algorithm tests----------------------------------------

        def test_Simple_Run(self):
            """
            Test Lorentzian fit for BayesStretch
            """
            fit_group, contour = BayesStretch(SampleWorkspace=self._sample_ws,
                                              ResolutionWorkspace=self._res_ws)
            self._validate_shape(contour, fit_group)
            self._validate_value(contour, fit_group)


#-------------------------------- Failure cases ------------------------------------------

        def test_invalid_e_min(self):
            """
            Test that an EMin of less than the data range is invalid
            """
            self.assertRaises(RuntimeError,
                              BayesStretch,
                              OutputWorkspaceFit='fit_group',
                              OutputWorkspaceContour='contour',
                              SampleWorkspace=self._sample_ws,
                              ResolutionWorkspace=self._res_ws,
                              EMin=-10)

        def test_invalid_e_max(self):
            """
            Test that an EMin of less than the data range is invalid
            """
            self.assertRaises(RuntimeError,
                              BayesStretch,
                              OutputWorkspaceFit='fit_group',
                              OutputWorkspaceContour='contour',
                              SampleWorkspace=self._sample_ws,
                              ResolutionWorkspace=self._res_ws,
                              EMax=10)


#--------------------------------Validate results-----------------------------------------

        def _validate_shape(self, contour, fit_group):
            """
            Validates that the output workspaces are of the correct type, units and shape.
            """

            # Test size/shape of contour group
            self.assertTrue(isinstance(contour, WorkspaceGroup))
            self.assertEquals(contour.getNumberOfEntries(), self._num_hists)
            self.assertEquals(contour.getItem(0).getNumberHistograms(), 50)
            self.assertEquals(contour.getItem(0).blocksize(), 30)

            # Test size/shape of fitting group
            self.assertTrue(isinstance(fit_group, WorkspaceGroup))
            self.assertEquals(fit_group.getNumberOfEntries(), 2)
            self.assertEquals(fit_group.getItem(0).getNumberHistograms(), self._num_hists)
            self.assertEquals(fit_group.getItem(0).blocksize(), 50)
            self.assertEquals(fit_group.getItem(1).getNumberHistograms(), self._num_hists)
            self.assertEquals(fit_group.getItem(1).blocksize(), 30)


        def _validate_value(self, contour, fit_group):
            """
            Validates that the output workspaces have expected values
            with values from the last known correct version
            """

            # Test values of contour
            contour_ws_0 = contour.getItem(0)
            self.assertEquals(round(contour_ws_0.dataY(13)[11],49), 2.8026e-45)
            self.assertEquals(round(contour_ws_0.dataY(14)[11],47), 7.41147e-42)
            self.assertEquals(round(contour_ws_0.dataY(15)[11],49), 1.26117e-44)
            self.assertEquals(round(contour_ws_0.dataY(15)[12],46), 2.49389e-41)

            # Test values of fit_group
            fit_ws_sigma = fit_group.getItem(0)
            self.assertEquals(round(fit_ws_sigma.dataY(0)[13],49), 2.8026e-45)
            self.assertEquals(round(fit_ws_sigma.dataY(0)[14],46), 7.4115e-42)
            self.assertEquals(round(fit_ws_sigma.dataY(0)[48],39), 9.87282e-34)
            self.assertEquals(round(fit_ws_sigma.dataY(0)[49],45), 1.74277e-40)
            fit_ws_beta = fit_group.getItem(1)
            self.assertEquals(round(fit_ws_beta.dataY(0)[11],47), 7.16204e-42)
            self.assertEquals(round(fit_ws_beta.dataY(0)[12],33), 1.3059e-29)
            self.assertEquals(round(fit_ws_beta.dataY(0)[21],33), 1.15045e-28)
            self.assertEquals(round(fit_ws_beta.dataY(0)[22],49), 8.96831e-44)
     

    if __name__=="__main__":
        unittest.main()
