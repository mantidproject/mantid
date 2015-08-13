import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

class GetNegMuMuonicXRDTest(unittest.TestCase):
    au_muonic_xr = [8135.2,8090.6,8105.4,8069.4,5764.89,5594.97,3360.2,
                    3206.8,2474.22,2341.21,2304.44,1436.05,1391.58,1104.9,
                    899.14,869.98,405.654,400.143]
    as_muonic_xr = [1866.9,1855.8,436.6,427.5]
    y_pos = -0.001

    def test_muonic_xrd_single_ws_produced(self):
        #Setting up the work space manually
        au_peak_values = self.au_muonic_xr
        y_position = self.y_pos
        y_pos_ws = [y_position]*len(au_peak_values)
        au_muon_xr_ws = CreateWorkspace(au_peak_values, y_pos_ws[:])
        #Check that au_muon_xr_ws is not null
        self.assertFalse(au_muon_xr_ws==None)
        au_muon_group = GroupWorkspaces(au_muon_xr_ws)
        #Check that au_muon_group is not null
        self.assertFalse(au_muon_group==None)
        #Get the algorithm to produce the same workspace
        neg_mu_xr_ws = GetNegMuMuonicXRD("Au", -0.001)
        #Check that neg_mu_xr_ws is not null
        self.assertFalse(neg_mu_xr_ws==None)
        #Test number of workspaces in group
        self.assertEqual(au_muon_group.getNumberOfEntries(),
                        neg_mu_xr_ws.getNumberOfEntries())
        
        

    def test_muonic_xrd_group_workspace(self):
        y_position = self.y_pos
        au_peak_values = self.au_muonic_xr
        au_y_pos_ws = [y_position]*len(au_peak_values)
        as_peak_values = self.as_muonic_xr
        as_y_pos_ws = [y_position]*len(as_peak_values)

        au_muon_xr_ws = CreateWorkspace(au_peak_values,au_y_pos_ws[:])
        as_muon_xr_ws = CreateWorkspace(as_peak_values, as_y_pos_ws[:])

        ws_list = [au_muon_xr_ws,as_muon_xr_ws]
        grouped_muon_ws = GroupWorkspaces(ws_list)

if __name__ == '__main__':
    unittest.main()