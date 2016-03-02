"""
Unit test for Vesuvio reduction

Assumes that mantid can be imported and the data paths
are configured to find the Vesuvio data
"""
import unittest

from mantid.api import AlgorithmManager
from mantid.simpleapi import LoadVesuvio
import vesuvio.commands as vesuvio

class VesuvioTOFFitTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is not None:
            return
        # Cache a TOF workspace
        self.__class__._test_ws = \
            vesuvio.load_and_crop_data(runs="15039-15045",
                                                ip_file="Vesuvio_IP_file_test.par",
                                                spectra="135-136")

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

        self.assertAlmostEqual(0.000928695463881635, output_ws.readY(0)[0])
        self.assertAlmostEqual(0.00722948549525415, output_ws.readY(0)[-1])
        self.assertAlmostEqual(1.3595170868608914e-05, output_ws.readY(1)[0])
        self.assertAlmostEqual(7.365455189265588e-05, output_ws.readY(1)[-1])

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

        self.assertAlmostEqual(-0.005852648610523481, output_ws.readY(0)[0])
        self.assertAlmostEqual(-0.013112461599666836, output_ws.readY(0)[-1])
        self.assertAlmostEqual(1.4735154307712157e-05, output_ws.readY(1)[0])
        self.assertAlmostEqual(7.6920365161795522e-05, output_ws.readY(1)[-1])


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

        self.assertAlmostEqual(0.000928695463881635, output_ws.readY(0)[0])
        self.assertAlmostEqual(0.00722948549525415, output_ws.readY(0)[-1])
        self.assertAlmostEqual(-0.0053373057114913395, output_ws.readY(1)[0])
        self.assertAlmostEqual(0.0031160012861094839, output_ws.readY(1)[-1])

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

