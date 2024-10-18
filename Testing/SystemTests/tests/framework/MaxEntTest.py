# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import CropWorkspace, GroupWorkspaces, Load, MaxEnt, Rebin, RemoveExpDecay


class MaxEntTest(systemtesting.MantidSystemTest):
    """Tests the MaxEnt algorithm on a MUSR workspace"""

    def runTest(self):
        Load(Filename="MUSR00022725.nxs", OutputWorkspace="MUSR00022725")
        CropWorkspace(InputWorkspace="MUSR00022725", OutputWorkspace="MUSR00022725", XMin=0.11, XMax=4, EndWorkspaceIndex=0)
        RemoveExpDecay(InputWorkspace="MUSR00022725", OutputWorkspace="MUSR00022725")
        Rebin(InputWorkspace="MUSR00022725", OutputWorkspace="MUSR00022725", Params="0.016")
        _evolChi, _evolAngle, _image, _data = MaxEnt(InputWorkspace="MUSR00022725", A=0.001, ChiTargetOverN=1.5, MaxAngle=0.001)
        GroupWorkspaces(InputWorkspaces="_evolAngle,_image,_data", OutputWorkspace="MaxEntResults")

    def validate(self):
        self.tolerance = 5e-2
        return ("MaxEntResults", "MaxEntMUSR00022725.nxs")
