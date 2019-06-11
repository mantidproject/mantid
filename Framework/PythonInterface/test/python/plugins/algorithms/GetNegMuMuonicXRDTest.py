# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

class GetNegMuMuonicXRDTest(unittest.TestCase):
    au_muonic_xr = [8135.2,8090.6,8105.4,8069.4,5764.89,5594.97,3360.2,
                    3206.8,2474.22,2341.21,2304.44,1436.05,1391.58,1104.9,
                    899.14,869.98,405.654,400.143]
    as_muonic_xr = [1866.9,1855.8,436.6,427.5]

    #TESTING FOR ONE WORKSPACE IN GROUP WORKSPACE
    def test_muonic_xrd_single_ws_in_group(self):
        self.au_muonic_xr.sort()
        self.as_muonic_xr.sort()
        #Setting up the work space manually
        au_peak_values = self.au_muonic_xr
        y_position = -0.001 #same as default used by GetNegMuMuonic
        y_pos_ws = [y_position]*len(au_peak_values)
        au_muon_xr_ws = CreateWorkspace(au_peak_values[:], y_pos_ws[:])
        #Check that au_muon_xr_ws is not null
        self.assertNotEqual(au_muon_xr_ws, None)
        au_muon_group = GroupWorkspaces(au_muon_xr_ws)
        #Check that au_muon_group is not null
        self.assertNotEqual(au_muon_group, None)
        #Get the algorithm to produce the same workspace
        neg_mu_xr_group = GetNegMuMuonicXRD("Au") #testing default y-Axis position value
        #Check that neg_mu_xr_ws is not null
        self.assertNotEqual(neg_mu_xr_group, None)
        #Test number of workspaces in group
        self.assertEqual(au_muon_group.getNumberOfEntries(),
                        neg_mu_xr_group.getNumberOfEntries())
        self.assertEqual(au_muon_group.size(),  1)
        self.assertEqual(neg_mu_xr_group.size(),  1)

        #now testing the one workspace in the workspace group
        neg_mu_xr_ws = neg_mu_xr_group[0]
        au_muon_ws = au_muon_group[0]
        #check number of histograms are equal
        self.assertEqual(neg_mu_xr_ws.getNumberHistograms(), au_muon_ws.getNumberHistograms())
        #check number of bins is equal
        self.assertEqual(au_muon_ws.blocksize(), neg_mu_xr_ws.blocksize())

        #check length of XValues is the same
        self.assertEqual(len(au_muon_ws.readX(0)), len(neg_mu_xr_ws.readX(0)))
        #check all the XValues are the same

        #For RHEL6 (running an older version of python) this assert is not yet implemented:
        #self.assertItemsEqual(au_muon_ws.readX(0),neg_mu_xr_ws.readX(0))
        #INSTEAD we will use a simple for loop
        for x_value in range(len(au_muon_ws.readX(0))):
            self.assertEqual(au_muon_ws.readX(0)[x_value], neg_mu_xr_ws.readX(0)[x_value])

        #check length of YValues is the same
        self.assertEqual(len(au_muon_ws.readY(0)), len(neg_mu_xr_ws.readY(0)))
        #check all the YValues are the same

        #For RHEL6 (running an older version of python) this assert is not yet implemented:
        #self.assertItemsEqual(au_muon_ws.readY(0),neg_mu_xr_ws.readY(0))
        #INSTEAD we will use a simple for loop
        for y_value in range(len(au_muon_ws.readY(0))):
            self.assertEqual(au_muon_ws.readY(0)[y_value], neg_mu_xr_ws.readY(0)[y_value])

    #TESTING FOR MORE THAN ONE WORKSPACE IN GROUP WORKSPACE
    def test_muonic_xrd_more_than_one_ws_in_group(self):
        self.au_muonic_xr.sort()
        self.as_muonic_xr.sort()
        y_position = 0.2
        #Setting up au_muonic workspace
        au_peak_values = self.au_muonic_xr
        #check to see if workspace has been set to non-None value
        self.assertNotEqual(au_peak_values,  None)

        au_y_pos_ws = [y_position]*len(au_peak_values)

        #setting up as_muonic workspace
        as_peak_values = self.as_muonic_xr
        #check to see if workspace has been set to non-None value
        self.assertNotEqual(as_peak_values,  None)

        as_y_pos_ws = [y_position]*len(as_peak_values)

        au_muon_xr_ws = CreateWorkspace(au_peak_values,au_y_pos_ws[:])
        #check to see if workspace creation was successful
        self.assertNotEqual(au_muon_xr_ws,  None)
        as_muon_xr_ws = CreateWorkspace(as_peak_values, as_y_pos_ws[:])
        #check to see if workspace creation was successful
        self.assertNotEqual(as_muon_xr_ws,  None)

        ws_list = [au_muon_xr_ws,as_muon_xr_ws]
        grouped_muon_ws = GroupWorkspaces(ws_list)
        #check to see whether grouping workspaces was successful
        self.assertNotEqual(grouped_muon_ws,  None)

        #Run algorithm that creates muonic_xr group workspace
        group_muonic_xr_ws = GetNegMuMuonicXRD("Au,As", 0.2)
        #check that this has assigned value correctly
        self.assertNotEqual(group_muonic_xr_ws,  None)

        #Compare histograms for each of the workspaces in GroupWorkspaces created
        self.assertEqual(grouped_muon_ws[0].getNumberHistograms(), group_muonic_xr_ws[0].getNumberHistograms())
        self.assertEqual(grouped_muon_ws[1].getNumberHistograms(), group_muonic_xr_ws[1].getNumberHistograms())

        #Compare length of X values read from each workspace in grouped workspace
        self.assertEqual(len(grouped_muon_ws[0].readX(0)), len(group_muonic_xr_ws[0].readX(0)))
        self.assertEqual(len(grouped_muon_ws[1].readX(0)), len(group_muonic_xr_ws[1].readX(0)))

        #Compare X values read from each workspace in grouped workspace

        #For RHEL6 (running an older version of python) this assert is not yet implemented:
        #self.assertItemsEqual(grouped_muon_ws[0].readX(0), group_muonic_xr_ws[0].readX(0))
        #self.assertItemsEqual(grouped_muon_ws[1].readX(0), group_muonic_xr_ws[1].readX(0))
        #INSTEAD we will use a simple for loop
        for x_value in range(len(grouped_muon_ws[0].readX(0))):
            self.assertEqual(grouped_muon_ws[0].readX(0)[x_value], group_muonic_xr_ws[0].readX(0)[x_value])
        for x_value in range(len(grouped_muon_ws[1].readX(0))):
            self.assertEqual(grouped_muon_ws[1].readX(0)[x_value], group_muonic_xr_ws[1].readX(0)[x_value])
        #Compare length of Y values read from each workspace in grouped workspace
        self.assertEqual(len(grouped_muon_ws[0].readY(0)), len(group_muonic_xr_ws[0].readY(0)))
        self.assertEqual(len(grouped_muon_ws[1].readY(0)), len(group_muonic_xr_ws[1].readY(0)))

        #Compare Y values read from each workspace in grouped workspace

        #For RHEL6 (running an older version of python) this assert is not yet implemented:
        #self.assertItemsEqual(grouped_muon_ws[0].readY(0), group_muonic_xr_ws[0].readY(0))
        #self.assertItemsEqual(grouped_muon_ws[1].readY(0), group_muonic_xr_ws[1].readY(0))
        #INSTEAD we will use a simple for loop
        for y_value in range(len(grouped_muon_ws[0].readY(0))):
            self.assertEqual(grouped_muon_ws[0].readY(0)[y_value], group_muonic_xr_ws[0].readY(0)[y_value])
        for y_value in range(len(grouped_muon_ws[1].readY(0))):
            self.assertEqual(grouped_muon_ws[1].readY(0)[y_value], group_muonic_xr_ws[1].readY(0)[y_value])

if __name__ == '__main__':
    unittest.main()
