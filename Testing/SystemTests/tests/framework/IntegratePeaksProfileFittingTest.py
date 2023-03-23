# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from numpy.testing import assert_allclose

from systemtesting import MantidSystemTest
from mantid.api import mtd
from mantid.simpleapi import LoadNexus, FindUBUsingFFT, ConvertToMD, IntegratePeaksProfileFitting


class IntegratePeaksProfileFittingTest(MantidSystemTest):
    r"""
    def requiredFiles(self):
        return ['TOPAZ_39037_bank29.nxs',  # input events
                'TOPAZ_39037_peaks_short.nxs',  # input peaks
                'bl11_moderatorCoefficients_2018.dat'] # moderator coefficients
    """

    def runTest(self):
        r"""Calculate intensities for a set of peaks. The first and second peaks in the
        table corresponds to satellite peak HKL=(1.5, 1.5,0) and main peak (1,1,0)"""
        LoadNexus(Filename="TOPAZ_39037_bank29.nxs", OutputWorkspace="events")
        LoadNexus(Filename="TOPAZ_39037_peaks_short.nxs", OutputWorkspace="peaks_input")
        FindUBUsingFFT(PeaksWorkspace="peaks_input", MinD=5.0, MaxD=10.0)
        ConvertToMD(InputWorkspace="events", QDimensions="Q3D", dEAnalysisMode="Elastic", Q3DFrames="Q_lab", OutputWorkspace="md")

        IntegratePeaksProfileFitting(
            OutputPeaksWorkspace="peaks_output",
            OutputParamsWorkspace="params_ws",
            ModeratorCoefficientsFile="bl11_moderatorCoefficients_2018.dat",
            InputWorkspace="md",
            PeaksWorkspace="peaks_input",
        )

        table = mtd["peaks_output"]
        # intensities for the first two peaks
        assert_allclose(table.column("Intens")[0:2], [302.9, 10013.7], atol=1.0)
