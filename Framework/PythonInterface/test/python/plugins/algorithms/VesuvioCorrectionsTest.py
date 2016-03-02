#pylint: disable=too-many-public-methods,invalid-name

"""
Unit test for Vesuvio corrections steps

Assumes that mantid can be imported and the data paths
are configured to find the Vesuvio data
"""
import unittest

from mantid.api import *
from mantid import logger
import mantid.simpleapi as ms
import vesuvio.commands as vesuvio

class VesuvioCorrectionsTest(unittest.TestCase):

    _test_ws = None
    _test_container_ws = None

    def setUp(self):
        if self._test_ws is None:
        # Cache a TOF workspace
            self.__class__._test_ws = \
                vesuvio.load_and_crop_data(runs="15039-15045",
                                                    ip_file="Vesuvio_IP_file_test.par",
                                                    spectra="135-136")
        if self._test_container_ws is None:
            self.__class__._test_container_ws = \
                vesuvio.load_and_crop_data(runs="15036",
                                                    ip_file="Vesuvio_IP_file_test.par",
                                                    spectra="135-136")


    # -------------- Success cases ------------------


    # def xtest_smooth_uses_requested_number_of_points(self):
    #     alg = self._create_algorithm(InputWorkspace=self._test_ws,
    #                                  Smoothing="Neighbour", SmoothingOptions="NPoints=3",
    #                                  BadDataError=-1)
    #     alg.execute()
    #     output_ws = alg.getProperty("OutputWorkspace").value

    #     self.assertEqual(2, output_ws.getNumberHistograms())
    #     self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
    #     self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

    #     self.assertAlmostEqual(0.0071192100574770656, output_ws.readY(0)[0])
    #     self.assertAlmostEqual(0.011063685963343062, output_ws.readY(0)[-1])
    #     self.assertAlmostEqual(0.002081985833021438, output_ws.readY(1)[0])
    #     self.assertAlmostEqual(-0.01209794157313937, output_ws.readY(1)[-1])


    # def xtest_mask_only_masks_over_threshold(self):
    #     err_start = self._test_ws.readE(1)[-1]
    #     self._test_ws.dataE(1)[-1] = 1.5e6

    #     alg = self._create_algorithm(InputWorkspace=self._test_ws,
    #                                  Smoothing="None", BadDataError=1.0e6)
    #     alg.execute()
    #     self._test_ws.dataE(1)[-1] = err_start
    #     output_ws = alg.getProperty("OutputWorkspace").value

    #     self.assertEqual(2, output_ws.getNumberHistograms())
    #     self.assertAlmostEqual(50.0, output_ws.readX(0)[0])
    #     self.assertAlmostEqual(562.0, output_ws.readX(0)[-1])

    #     self.assertAlmostEqual(0.00092869546388163471, output_ws.readY(0)[0])
    #     self.assertAlmostEqual(0.007229485495254151, output_ws.readY(0)[-1])
    #     self.assertAlmostEqual(-0.005852648610523481, output_ws.readY(1)[0])
    #     # Masked
    #     self.assertAlmostEqual(0.0, output_ws.readY(1)[-1])


    def test_gamma_and_ms_correct(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     GammaBackground=True,
                                     FitParameters=self._create_dummy_fit_parameters(),
                                     Masses=self._create_dummy_masses(),
                                     MassProfiles=self._create_dummy_profiles(),
                                     CorrectionWorkspaces='__Correction',
                                     CorrectedWorkspaces='__Corrected',
                                     OutputWorkspace='__Output',
                                     LinearFitResult='__LinearFit')

        alg.execute()
        self.assertTrue(alg.isExecuted())

        corrections_wsg = alg.getProperty("CorrectionWorkspaces").value
        self.assertTrue(isinstance(corrections_wsg, WorkspaceGroup))
        self.assertEqual(len(corrections_wsg), 2)

        corrected_wsg = alg.getProperty("CorrectedWorkspaces").value
        self.assertTrue(isinstance(corrected_wsg, WorkspaceGroup))
        self.assertEqual(len(corrected_wsg), 2)

        output_ws = alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(output_ws, MatrixWorkspace))
        self.assertEqual(output_ws.getNumberHistograms(), 1)

        linear_params = alg.getProperty("LinearFitResult").value
        self.assertTrue(isinstance(linear_params, ITableWorkspace))
        self.assertEqual(linear_params.columnCount(), 3)
        self.assertEqual(linear_params.rowCount(), 6)


    def test_ms_correct_with_container(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     ContainerWorkspace=self._test_container_ws,
                                     GammaBackground=False,
                                     FitParameters=self._create_dummy_fit_parameters(),
                                     Masses=self._create_dummy_masses(),
                                     MassProfiles=self._create_dummy_profiles())

        alg.execute()
        self.assertTrue(alg.isExecuted())

        corrections_wsg = alg.getProperty("CorrectionWorkspaces").value
        self.assertTrue(isinstance(corrections_wsg, WorkspaceGroup))
        self.assertEqual(len(corrections_wsg), 2)

        corrected_wsg = alg.getProperty("CorrectedWorkspaces").value
        self.assertTrue(isinstance(corrected_wsg, WorkspaceGroup))
        self.assertEqual(len(corrected_wsg), 2)

        output_ws = alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(output_ws, MatrixWorkspace))
        self.assertEqual(output_ws.getNumberHistograms(), 1)

        linear_params = alg.getProperty("LinearFitResult").value
        self.assertTrue(isinstance(linear_params, ITableWorkspace))
        self.assertEqual(linear_params.columnCount(), 3)
        self.assertEqual(linear_params.rowCount(), 9)


    def test_gamma_and_ms_correct_with_container(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     ContainerWorkspace=self._test_container_ws,
                                     FitParameters=self._create_dummy_fit_parameters(),
                                     Masses=self._create_dummy_masses(),
                                     MassProfiles=self._create_dummy_profiles())

        alg.execute()
        self.assertTrue(alg.isExecuted())

        corrections_wsg = alg.getProperty("CorrectionWorkspaces").value
        print corrections_wsg
        self.assertTrue(isinstance(corrections_wsg, WorkspaceGroup))
        self.assertEqual(len(corrections_wsg), 2)

        corrected_wsg = alg.getProperty("CorrectedWorkspaces").value
        self.assertTrue(isinstance(corrected_wsg, WorkspaceGroup))
        self.assertEqual(len(corrected_wsg), 2)

        output_ws = alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(output_ws, MatrixWorkspace))
        self.assertEqual(output_ws.getNumberHistograms(), 1)

        linear_params = alg.getProperty("LinearFitResult").value
        self.assertTrue(isinstance(linear_params, ITableWorkspace))
        self.assertEqual(linear_params.columnCount(), 3)
        self.assertEqual(linear_params.rowCount(), 9)


    def test_gamma_and_ms_correct_with_container_fixed_scaling(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     ContainerWorkspace=self._test_container_ws,
                                     GammaBackground=True,
                                     FitParameters=self._create_dummy_fit_parameters(),
                                     Masses=self._create_dummy_masses(),
                                     MassProfiles=self._create_dummy_profiles(),
                                     ContainerScale=0.1,
                                     GammaBackgroundScale=0.2)

        alg.execute()
        self.assertTrue(alg.isExecuted())

        corrections_wsg = alg.getProperty("CorrectionWorkspaces").value
        self.assertTrue(isinstance(corrections_wsg, WorkspaceGroup))
        self.assertEqual(len(corrections_wsg), 2)

        corrected_wsg = alg.getProperty("CorrectedWorkspaces").value
        self.assertTrue(isinstance(corrected_wsg, WorkspaceGroup))
        self.assertEqual(len(corrected_wsg), 2)

        output_ws = alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(output_ws, MatrixWorkspace))
        self.assertEqual(output_ws.getNumberHistograms(), 1)

        linear_params = alg.getProperty("LinearFitResult").value
        self.assertTrue(isinstance(linear_params, ITableWorkspace))
        self.assertEqual(linear_params.columnCount(), 3)
        self.assertEqual(linear_params.rowCount(), 9)

        self.assertAlmostEqual(linear_params.cell(0, 0), 0.1)
        self.assertAlmostEqual(linear_params.cell(0, 1), 0.0)
        self.assertAlmostEqual(linear_params.cell(0, 2), 1.0)
        self.assertAlmostEqual(linear_params.cell(0, 3), 0.2)
        self.assertAlmostEqual(linear_params.cell(0, 4), 0.0)
        self.assertAlmostEqual(linear_params.cell(0, 5), 1.0)
        self.assertAlmostEqual(linear_params.cell(0, 7), 0.0)
        self.assertAlmostEqual(linear_params.cell(0, 8), 1.0)


    # -------------- Failure cases ------------------


    def test_running_without_fit_params_raises_error(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     Masses=self._create_dummy_masses(),
                                     MassProfiles=self._create_dummy_profiles())
        self.assertRaises(RuntimeError, alg.execute)

    def test_running_without_masses_raises_error(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     FitParameters=self._create_dummy_fit_parameters(),
                                     MassProfiles=self._create_dummy_profiles())
        self.assertRaises(RuntimeError, alg.execute)


    def test_running_without_profiles_raises_error(self):
        alg = self._create_algorithm(InputWorkspace=self._test_ws,
                                     FitParameters=self._create_dummy_fit_parameters(),
                                     Masses=self._create_dummy_masses())
        self.assertRaises(RuntimeError, alg.execute)


    # -------------- Helpers --------------------


    def _create_algorithm(self, **kwargs):
        alg = AlgorithmManager.createUnmanaged("VesuvioCorrections")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("OutputWorkspace", "__unused")
        for key, value in kwargs.iteritems():
            alg.setProperty(key, value)
        return alg


    def _create_dummy_fit_parameters(self):
        params = ms.CreateEmptyTableWorkspace(OutputWorkspace='__VesuvioCorrections_test_fit_params')

        params.addColumn('str', 'Name')
        params.addColumn('float', 'Value')
        params.addColumn('float', 'Error')

        params.addRow(['f0.Width', 4.72912, 0.41472])
        params.addRow(['f0.FSECoeff', 0.557332, 0])
        params.addRow(['f0.C_0', 11.8336, 1.11468])
        params.addRow(['f1.Width', 10, 0])
        params.addRow(['f1.Intensity', 2.21085, 0.481347])
        params.addRow(['f2.Width', 13, 0])
        params.addRow(['f2.Intensity', 1.42443, 0.583283])
        params.addRow(['f3.Width', 30, 0])
        params.addRow(['f3.Intensity', 0.499497, 0.28436])
        params.addRow(['f4.A0', -0.00278903, 0.00266163])
        params.addRow(['f4.A1', 14.5313, 22.2307])
        params.addRow(['f4.A2', -5475.01, 35984.4])
        params.addRow(['Cost function value', 2.34392, 0])

        return params


    def _create_dummy_masses(self):
        return [1.0079, 16.0, 27.0, 133.0]


    def _create_dummy_profiles(self):
        return "function=GramCharlier,hermite_coeffs=[1, 0, 0],k_free=0,sears_flag=1,width=[2, 5, 7];function=Gaussian,width=10;function=Gaussian,width=13;function=Gaussian,width=30"


if __name__ == "__main__":
    unittest.main()
