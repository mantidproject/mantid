# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.simpleapi import OSIRISDiffractionReduction
from systemtesting import MantidSystemTest


class OSIRISDiffractionReductionTest(MantidSystemTest):

    def requiredFiles(self):
        return ["OSI10203.raw", "OSI10156.raw", "OSI10241.raw", "OSI10242.RAW", "osiris_041_RES10.cal", "OSIRIS10203_diff_red.nxs"]

    def runTest(self):
        OSIRISDiffractionReduction(
            Sample=["OSI10203.raw"],
            CalFile="osiris_041_RES10.cal",
            Vanadium=["OSI10156.raw"],
            Container=["OSI10241.raw", "OSI10242.RAW"],
            ContainerScaleFactor=0.5,
            SpectraMin=3,
            SpectraMax=361,
            GroupingMethod="File",
            GroupingFile="osiris_041_RES10.cal",
            OutputWorkspace="diffraction_result",
        )

    def validate(self):
        self.tolerance = 0.0001
        return "diffraction_result", "OSIRIS10203_diff_red.nxs"
