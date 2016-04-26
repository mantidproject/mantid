"""
Unit test for Vesuvio pre-fitting steps

Assumes that mantid can be imported and the data paths
are configured to find the Vesuvio data
"""
import unittest

from mantid.api import AlgorithmManager
import vesuvio.commands as vesuvio
import vesuvio.testing as testing

class VesuvioPreFitTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is not None:
            return
        # Cache a TOF workspace
        self.__class__._test_ws = testing.create_test_ws()

    # -------------- Success cases ------------------

    def test_smooth_uses_requested_number_of_points(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     Smoothing="Neighbour", SmoothingOptions="NPoints=3",
                                     BadDataError=-1)
        alg.execute()
        output_ws = alg.getProperty("OutputWorkspace").value

        self.assertEqual(2, output_ws.getNumberHistograms())
        self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

        self.assertAlmostEqual(0.09337135, output_ws.readY(0)[0])
        self.assertAlmostEqual(0.00060365, output_ws.readY(0)[-1])
        self.assertAlmostEqual(0.084563, output_ws.readY(1)[0])
        self.assertAlmostEqual(-0.00403775, output_ws.readY(1)[-1])

    def test_mask_only_masks_over_threshold(self):
        err_start = self._test_ws.readE(1)[-1]
        self._test_ws.dataE(1)[-1] = 1.5e6

        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     Smoothing="None", BadDataError=1.0e6)
        alg.execute()
        self._test_ws.dataE(1)[-1] = err_start
        output_ws = alg.getProperty("OutputWorkspace").value

        self.assertEqual(2, output_ws.getNumberHistograms())
        self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
        self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

        self.assertAlmostEqual(0.0279822, output_ws.readY(0)[0])
        self.assertAlmostEqual(0.0063585, output_ws.readY(0)[-1])
        self.assertAlmostEqual(0.0155041, output_ws.readY(1)[0])
        # Masked
        self.assertAlmostEqual(0.0, output_ws.readY(1)[-1])

    # -------------- Failure cases ------------------

    def test_invalid_smooth_opt_raises_error_on_validate(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     Smoothing="Neighbour", SmoothingOptions="npts=3")
        self.assertRaises(RuntimeError, alg.execute)

    # -------------- Helpers --------------------

    def _create_algorithm(self, **kwargs):
        alg = AlgorithmManager.createUnmanaged("VesuvioPreFit")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("OutputWorkspace", "__unused")
        for key, value in kwargs.iteritems():
            alg.setProperty(key, value)
        return alg


if __name__ == "__main__":
    unittest.main()

