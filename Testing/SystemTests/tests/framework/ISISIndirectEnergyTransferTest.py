# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
from systemtesting import MantidSystemTest
from mantid.simpleapi import ISISIndirectEnergyTransfer


class ISISIndirectEnergyTransferTest(MantidSystemTest):
    def runTest(self):
        ISISIndirectEnergyTransfer(
            InputFiles="TSC05224.RAW",
            Instrument="TOSCA",
            Analyser="graphite",
            Reflection="002",
            SpectraRange="1,140",
            RebinString="-2.5,0.015,3,-0.005,1000",
            UnitX="DeltaE_inWavenumber",
            OutputWorkspace="TOSCA20000_graphite_002_Reduced",
        )

    def validate(self):
        self.tolerance = 1e-10
        return "tosca5224-glucose", "tosca5224-glucose-ref.nxs"
