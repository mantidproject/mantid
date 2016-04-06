"""
Unit test for Vesuvio reduction

Assumes that mantid can be imported and the data paths
are configured to find the Vesuvio data
"""
import unittest
import platform
from mantid.api import AlgorithmManager
import vesuvio.testing as testing
import vesuvio.commands as vesuvio

class VesuvioTOFFitTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is not None:
            return
        # Cache a TOF workspace
        self.__class__._test_ws = testing.create_test_ws()

    # -------------- Success cases ------------------

    def test_single_run_produces_correct_output_workspace_index0_kfixed_no_background(self):
        profiles = "function=GramCharlier,width=[2, 5, 7],hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1;"\
                   "function=Gaussian,width=10;function=Gaussian,width=13;function=Gaussian,width=30;"

        alg = self._create_algorithm(InputWorkspace=self._test_ws, WorkspaceIndex=0,
                                     Masses=[1.0079, 16, 27, 133],
                                     MassProfiles=profiles,
                                     IntensityConstraints="[0,1,0,-4]")
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        self.assertEqual(7, output_ws.getNumberHistograms())
        self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

        self.assertAlmostEqual(0.0279822, output_ws.readY(0)[0])
        self.assertAlmostEqual(0.0063585, output_ws.readY(0)[-1])
        self.assertAlmostEqual(0.8383345e-04, output_ws.readY(1)[0])
        self.assertAlmostEqual(0.7507980e-04, output_ws.readY(1)[-1])

    def test_single_run_produces_correct_output_workspace_index1_kfixed_no_background(self):
        profiles = "function=GramCharlier,width=[2, 5, 7],hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1;"\
                   "function=Gaussian,width=10;function=Gaussian,width=13;function=Gaussian,width=30;"

        alg = self._create_algorithm(InputWorkspace=self._test_ws, WorkspaceIndex=1,
                                     Masses=[1.0079, 16, 27, 133],
                                     MassProfiles=profiles,
                                     IntensityConstraints="[0,1,0,-4]")
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        self.assertEqual(7, output_ws.getNumberHistograms())
        self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

        self.assertAlmostEqual(0.0155041, output_ws.readY(0)[0])
        self.assertAlmostEqual(-0.0070975, output_ws.readY(0)[-1])
        self.assertAlmostEqual(0.8693019e-05, output_ws.readY(1)[0])
        self.assertTrue(abs(0.746e-04 - output_ws.readY(1)[-1]) < 0.2e-06)

    def test_single_run_produces_correct_output_workspace_index0_kfixed_including_background(self):
        profiles = "function=GramCharlier,width=[2, 5, 7],hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1;"\
                   "function=Gaussian,width=10;function=Gaussian,width=13;function=Gaussian,width=30;"
        background = "function=Polynomial,order=3"

        alg = self._create_algorithm(InputWorkspace=self._test_ws, WorkspaceIndex=0,
                                     Masses=[1.0079, 16.0, 27.0, 133.0],
                                     MassProfiles=profiles,
                                     Background=background,
                                     IntensityConstraints="[0,1,0,-4]")

        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        self.assertEqual(8, output_ws.getNumberHistograms())
        self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

        self.assertAlmostEqual(0.0279822, output_ws.readY(0)[0])
        self.assertAlmostEqual(0.0063585, output_ws.readY(0)[-1])
        self.assertTrue(abs(-0.012 - output_ws.readY(1)[0]) < 0.002)
        self.assertTrue(abs(0.0056 - output_ws.readY(1)[-1]) < 0.0004)
    # -------------- Failure cases ------------------

    def test_empty_masses_raises_error(self):
        alg = self._create_algorithm()

        self.assertRaises(ValueError, alg.setProperty,
                          "Masses", [])

    def test_empty_functions_raises_error(self):
        alg = self._create_algorithm()

        self.assertRaises(ValueError, alg.setProperty, "MassProfiles", "")

    def test_number_functions_in_list_not_matching_length_masses_throws_error(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     Masses=[1.0079, 33],
                                     MassProfiles="function=Gaussian,width=5;")

        self.assertRaises(RuntimeError, alg.execute)

    # -------------- Helpers --------------------

    def _create_algorithm(self, **kwargs):
        alg = AlgorithmManager.createUnmanaged("VesuvioTOFFit")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("OutputWorkspace", "__unused")
        alg.setProperty("FitParameters", "__unused")
        for key, value in kwargs.iteritems():
            alg.setProperty(key, value)
        return alg

if __name__ == "__main__":
    unittest.main()

