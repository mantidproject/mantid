# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name

import systemtesting
from mantid.simpleapi import ConvertUnits, Load
from isis_reflectometry.l2q import l2q


class L2QScriptTest(systemtesting.MantidSystemTest):
    def runTest(self):
        ws = Load(Filename="INTER00013469.nxs")
        ws = ConvertUnits(InputWorkspace=ws, Target="Wavelength", AlignBins=1)
        # Io=CropWorkspace(InputWorkspace=ws,XMin=0.8,XMax=14.5,StartWorkspaceIndex=2,EndWorkspaceIndex=2)
        # D=CropWorkspace(InputWorkspace=ws,XMin=0.8,XMax=14.5,StartWorkspaceIndex=3)
        # I= Divide(LHSWorkspace=D,RHSWorkspace=Io,AllowDifferentNumberSpectra=True)
        detector_component_name = "linear-detector"
        sample_component_name = "some-surface-holder"
        theta = 0.7
        l2q(ws, detector_component_name, theta, sample_component_name)  # This generates an output workspace called IvsQ

    def validate(self):
        self.disableChecking.append("Instrument")
        return "IvsQ", "L2QReferenceResult.nxs"
