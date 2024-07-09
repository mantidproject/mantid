# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from math import pi
from mantid.simpleapi import *


class MuonMaxEntTest(systemtesting.MantidSystemTest):
    """Tests the MaxEnt algorithm on a MUSR workspace"""

    def fixPhasesTest(self, phases0):
        MuonMaxEnt(
            InputWorkspace="MUSR00022725",
            InputPhaseTable="tab",
            Npts="16384",
            FixPhases=True,
            OutputWorkspace="tmp",
            OutputPhaseTable="tmpPhase",
        )
        tmp = AnalysisDataService.retrieve("tmpPhase")
        phases = tmp.column(2)
        self.assertListEqual(phases0, phases)

    def runTest(self):
        Load(Filename="MUSR00022725.nxs", OutputWorkspace="MUSR00022725")
        tab = CreateEmptyTableWorkspace()
        tab.addColumn("int", "Spectrum number")
        tab.addColumn("double", "Asymmetry")
        tab.addColumn("double", "Phase")
        for i in range(0, 32):
            phi = 2.0 * pi * i / 32.0
            tab.addRow([i + 1, 0.2, phi])
        for i in range(0, 32):
            phi = 2.0 * pi * i / 32.0
            tab.addRow([i + 33, 0.2, phi])

        # first do with fixed phases
        MuonMaxent(
            InputWorkspace="MUSR00022725",
            InputPhaseTable="tab",
            Npts="16384",
            FixPhases=True,
            OutputWorkspace="freq0",
            OutputPhaseTable="PhasesOut0",
            ReconstructedSpectra="time0",
        )
        # then do with fitting phases
        MuonMaxent(
            InputWorkspace="MUSR00022725",
            InputPhaseTable="tab",
            Npts="16384",
            FixPhases=False,
            OutputWorkspace="freq",
            OutputPhaseTable="PhasesOut",
            ReconstructedSpectra="time",
        )

        GroupWorkspaces(InputWorkspaces="freq0,phasesOut0,time0,freq,phasesOut,time", OutputWorkspace="MuonMaxEntResults")

    def validate(self):
        self.tolerance = 5e-2
        return ("MuonMaxEntResults", "MuonMaxEntMUSR00022725.nxs")
