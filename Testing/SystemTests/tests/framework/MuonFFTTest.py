# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import CompareWorkspaces, CreateEmptyTableWorkspace, CropWorkspace, FFT, Load, PhaseQuad, ScaleX
from math import pi


class MuonFFTTest(systemtesting.MantidSystemTest):
    """Tests the FFT algorithm on a MUSR workspace, to check it can cope with rounding errors in X"""

    def runTest(self):
        Load(Filename="MUSR00022725.nxs", OutputWorkspace="MUSR00022725")
        CropWorkspace(InputWorkspace="MUSR00022725", OutputWorkspace="MUSR00022725", XMin=0, XMax=4, EndWorkspaceIndex=63)

        # create a PhaseTable with detector information
        tab = CreateEmptyTableWorkspace()
        tab.addColumn("int", "DetID")
        tab.addColumn("double", "Asym")
        tab.addColumn("double", "Phase")
        for i in range(0, 32):
            phi = 2 * pi * i / 32.0
            tab.addRow([i + 1, 0.2, phi])
        for i in range(0, 32):
            phi = 2 * pi * i / 32.0
            tab.addRow([i + 33, 0.2, phi])
        ows = PhaseQuad(InputWorkspace="MUSR00022725", PhaseTable="tab")

        # Offset by 1 us
        offset = ScaleX(ows, Factor="1", Operation="Add")

        # FFT should accept rounding errors in X without rebin
        FFT(ows, Real=0, Imaginary=1, AcceptXRoundingErrors=True, OutputWorkspace="MuonFFTResults")

        # FFT of offset should have different phase
        FFT(offset, Real=0, Imaginary=1, AcceptXRoundingErrors=True, AutoShift=True, OutputWorkspace="OffsetFFTResults")
        (result, _messages) = CompareWorkspaces("MuonFFTResults", "OffsetFFTResults")
        self.assertEqual(result, False)

    def validate(self):
        self.tolerance = 1e-8
        return ("MuonFFTResults", "MuonFFTMUSR00022725.nxs")
