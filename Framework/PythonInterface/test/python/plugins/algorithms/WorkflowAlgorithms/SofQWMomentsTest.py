# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import CompareWorkspaces, CreateSampleWorkspace, Load, LoadInstrument, ScaleX, SofQW, SofQWMoments


class SofQWMomentsTest(unittest.TestCase):
    def createSampleWorkspace(self):
        function = "name=Lorentzian,Amplitude=1,PeakCentre=5,FWHM=1"

        workspace = CreateSampleWorkspace(
            WorkspaceType="Histogram", Function="User Defined", UserDefinedFunction=function, XMin=0, XMax=10, BinWidth=0.01, XUnit="DeltaE"
        )

        # Shift to center on 0 and then scale to size
        workspace = ScaleX(workspace, -5, "Add")
        workspace = ScaleX(workspace, 0.1)
        LoadInstrument(Workspace=workspace, InstrumentName="IRIS", RewriteSpectraMap=True)
        return workspace

    def test_that_SOfQWMoments_produces_a_workspace_with_the_expected_number_of_histograms_and_blocksize(self):
        workspace = self.createSampleWorkspace()

        workspace = SofQW(workspace, "0.4, 0.1, 1.8", EMode="Indirect", EFixed="1.845")
        workspace = SofQWMoments(workspace)

        self.assertEqual(workspace.getNumberHistograms(), 5)
        self.assertEqual(workspace.blocksize(), 14)

    def test_that_SOfQWMoments_produces_the_workspace_expected(self):
        input_workspace = Load("iris26176_graphite002_sqw")

        output_workspace = SofQWMoments(InputWorkspace=input_workspace)

        expected_workspace = Load("iris26176_graphite002_sqw_moments")
        self.assertListEqual(output_workspace.dataX(0).tolist(), expected_workspace.dataX(0).tolist())
        self.assertListEqual(output_workspace.dataY(0).tolist(), expected_workspace.dataY(0).tolist())
        self.assertListEqual(output_workspace.dataE(0).tolist(), expected_workspace.dataE(0).tolist())
        self.assertTrue(CompareWorkspaces(expected_workspace, output_workspace, 1e-8)[0])


if __name__ == "__main__":
    unittest.main()
