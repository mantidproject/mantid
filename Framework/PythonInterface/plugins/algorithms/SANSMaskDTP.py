# pylint: disable=no-init,invalid-name
from __future__ import absolute_import, division, print_function

from mantid.api import (AlgorithmFactory, MatrixWorkspaceProperty,
                        PythonAlgorithm)
from mantid.kernel import Direction, IntArrayProperty, logger
from mantid.simpleapi import MaskBTP


class SANSMaskDTP(PythonAlgorithm):
    """
    Class to generate grouping file
    """
    def category(self):
        """ Mantid required
        """
        return "Transforms\\Masking;SANS\\Utility"

    def seeAlso(self):
        return ["MaskDetectors", "MaskInstrument"]

    def name(self):
        """ Mantid required
        """
        return "SANSMaskDTP"

    def summary(self):
        """ Mantid required
        """
        return "Algorithm to mask detectors in particular detectors, tube, or pixels."

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty(
                "InputWorkspace", "", direction=Direction.InOut),
            doc="Input Workspace")

        self.declareProperty("Detector", 1,
                             doc="Detector number to be masked.")

        self.declareProperty(IntArrayProperty(name="Tube", values=[]),
                             doc="Tube(s) to be masked. If empty, will apply to all tubes")

        self.declareProperty(IntArrayProperty(name="Pixel", values=[]),
                             doc="Pixel(s) to be masked. If empty, will apply to all pixels")

        self.declareProperty(IntArrayProperty(
            name="MaskedDetectors", direction=Direction.Output),
            doc="List of masked detectors")


    def PyExec(self):
        masking = MaskBTP(Workspace=self.getProperty("InputWorkspace").valueAsStr,
                          Bank=self.getProperty("Detector").value,
                          Tube=self.getProperty("Tube").valueAsStr,
                          Pixel=self.getProperty("Pixel").valueAsStr)

        self.setProperty("MaskedDetectors", masking)


AlgorithmFactory.subscribe(SANSMaskDTP)
