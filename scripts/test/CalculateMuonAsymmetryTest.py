# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import CreateWorkspace, GausOsc, AddSampleLog, ConvertFitFunctionForMuonTFAsymmetry, \
    CalculateMuonAsymmetry, \
    AnalysisDataService, CompareWorkspaces
from mantid.api import FunctionFactory
import numpy as np


class SingleDomainDoublePulseFitTest(unittest.TestCase):
    def test_that_correctly_identifies_normalisation_for_artificial_double_pulse_data(self):
        delta = 0.33
        x = np.linspace(0., 15., 100)
        x_offset = np.linspace(delta / 2, 15. + delta / 2, 100)
        x_offset_neg = np.linspace(-delta / 2, 15. - delta / 2, 100)

        testFunction = GausOsc(Frequency=1.5, A=0.22)
        y1 = testFunction(x_offset_neg)
        y2 = testFunction(x_offset)
        N0 = 6.38
        y = N0 * (1 + y1 / 2 + y2 / 2)
        y_norm = y1 / 2 + y2 / 2
        unnormalised_workspace = CreateWorkspace(x, y)
        ws_to_normalise = CreateWorkspace(x, y)
        ws_correctly_normalised = CreateWorkspace(x, y_norm)
        AddSampleLog(Workspace='ws_to_normalise', LogName="analysis_asymmetry_norm", LogText="1")

        innerFunction = FunctionFactory.createInitialized('name=GausOsc,A=0.20,Sigma=0.2,Frequency=1.0,Phi=0')
        tf_function = ConvertFitFunctionForMuonTFAsymmetry(InputFunction=innerFunction,
                                                           WorkspaceList=['ws_to_normalise'])

        CalculateMuonAsymmetry(MaxIterations=100, EnableDoublePulse=True, PulseOffset=delta,
                               UnNormalizedWorkspaceList='unnormalised_workspace',
                               ReNormalizedWorkspaceList='ws_to_normalise',
                               OutputFitWorkspace='DoublePulseFit', StartX=0, InputFunction=str(tf_function),
                               Minimizer='Levenberg-Marquardt')

        double_parameter_workspace = AnalysisDataService.retrieve('DoublePulseFit_Parameters')
        values_column = double_parameter_workspace.column(1)
        # Check that the correct normalisation is found.
        self.assertAlmostEqual(values_column[0], N0, places=3)
        # Check that normalised data is correct
        result, message = CompareWorkspaces('ws_to_normalise', 'ws_correctly_normalised', Tolerance=1e-3)
        self.assertTrue(result)

        AnalysisDataService.clear()


if __name__ == '__main__':
    unittest.main()
