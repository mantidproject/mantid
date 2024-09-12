# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.simpleapi import ISISIndirectDiffractionReduction
from systemtesting import MantidSystemTest


class ISISIndirectDiffractionReductionTest(MantidSystemTest):
    def requiredFiles(self):
        return ["osi89813.raw", "osiris_041_RES10.cal", "OSIRIS89813_diffspec_red.nxs"]

    def runTest(self):
        ISISIndirectDiffractionReduction(
            InputFiles=["OSI137793.RAW"],
            VanadiumFiles=["OSI137713.RAW"],
            CalFile="osiris_041_RES10.cal",
            GroupingMethod="File",
            GroupingFile="osiris_041_RES10.cal",
            Instrument="OSIRIS",
            Mode="diffspec",
            SpectraRange=[3, 962],
            OutputWorkspace="diffraction_result",
        )

    def validate(self):
        self.tolerance = 0.0001
        return "osiris137793_diffspec_red", "OSIRIS137793_diffspec_red.nxs"
