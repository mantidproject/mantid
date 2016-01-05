import unittest,os
import mantid
#import pymantidplot
import numpy as np

# First test that the limits have not been changed, and that the arithmetics work.
# Then checks that the figure is plotted.

class QECoverageTest(unittest.TestCase):
    def test_simple(self):
        # Some random Ei
        Ei=55.1
        Et=-Ei/5

        # Checks the correct number of workspaces are created
        q,e = mantid.simpleapi.QECoverage('12.3,'+str(Ei))
        wsnames = [ val for val in mantid.api.AnalysisDataService.getObjectNames() if val.startswith('QECoverage') ]
        self.assertEqual(len(wsnames),2)

        # Use previous result to check Merlin (default instrument)
        # Manually work out what qmax and qmin for Merlin is.
        qmax = np.sqrt((2*1.67e-27/(6.63e-34/(2*np.pi))**2)*(2*Ei*1.6e-22-Et*1.6e-22-2*1.6e-22*np.sqrt(Ei*(Ei-Et))*np.cos(np.deg2rad(135.69))))/1e10
        qmin = np.sqrt((2*1.67e-27/(6.63e-34/(2*np.pi))**2)*(2*Ei*1.6e-22-2*1.6e-22*np.sqrt(Ei*Ei)*np.cos(np.deg2rad(2.838))))/1e10
        # Checks that the values are equal to 3 decimal places
        self.assertAlmostEqual(max(q),qmax,3)
        self.assertAlmostEqual(min(q),qmin,3)
        # Checks that max of E is Ei
        self.assertEqual(max(e),Ei)

        # Run the calculation for Mari
        q,e = mantid.simpleapi.QECoverage(str(Ei),'MARI')
        # Manually work out what qmax and qmin for Merlin is.
        qmax = np.sqrt((2*1.67e-27/(6.63e-34/(2*np.pi))**2)*(2*Ei*1.6e-22-Et*1.6e-22-2*1.6e-22*np.sqrt(Ei*(Ei-Et))*np.cos(np.deg2rad(134.14))))/1e10
        qmin = np.sqrt((2*1.67e-27/(6.63e-34/(2*np.pi))**2)*(2*Ei*1.6e-22-2*1.6e-22*np.sqrt(Ei*Ei)*np.cos(np.deg2rad(3.43))))/1e10
        # Checks that the values are equal to 3 decimal places
        self.assertAlmostEqual(max(q),qmax,3)
        self.assertAlmostEqual(min(q),qmin,3)
        # Checks that max of E is Ei
        self.assertEqual(max(e),Ei)
        
        # Run the calculation for Maps
        q,e = mantid.simpleapi.QECoverage(str(Ei),'MAPS')
        # Manually work out what qmax and qmin for Merlin is.
        qmax = np.sqrt((2*1.67e-27/(6.63e-34/(2*np.pi))**2)*(2*Ei*1.6e-22-Et*1.6e-22-2*1.6e-22*np.sqrt(Ei*(Ei-Et))*np.cos(np.deg2rad(59.8))))/1e10
        qmin = np.sqrt((2*1.67e-27/(6.63e-34/(2*np.pi))**2)*(2*Ei*1.6e-22-2*1.6e-22*np.sqrt(Ei*Ei)*np.cos(np.deg2rad(3.0))))/1e10
        # Checks that the values are equal to 3 decimal places
        self.assertAlmostEqual(max(q),qmax,3)
        self.assertAlmostEqual(min(q),qmin,3)
        # Checks that max of E is Ei
        self.assertEqual(max(e),Ei)

        # Run the calculation for LET
        q,e = mantid.simpleapi.QECoverage(str(Ei),'LET')
        # Manually work out what qmax and qmin for Merlin is.
        qmax = np.sqrt((2*1.67e-27/(6.63e-34/(2*np.pi))**2)*(2*Ei*1.6e-22-Et*1.6e-22-2*1.6e-22*np.sqrt(Ei*(Ei-Et))*np.cos(np.deg2rad(140.))))/1e10
        qmin = np.sqrt((2*1.67e-27/(6.63e-34/(2*np.pi))**2)*(2*Ei*1.6e-22-2*1.6e-22*np.sqrt(Ei*Ei)*np.cos(np.deg2rad(2.65))))/1e10
        # Checks that the values are equal to 3 decimal places
        self.assertAlmostEqual(max(q),qmax,3)
        self.assertAlmostEqual(min(q),qmin,3)
        # Checks that max of E is Ei
        self.assertEqual(max(e),Ei)

if __name__=="__main__":
    unittest.main()
