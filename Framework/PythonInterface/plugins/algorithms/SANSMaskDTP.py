# pylint: disable=no-init,invalid-name
from __future__ import absolute_import, division, print_function

import numpy as np

from mantid.api import (AlgorithmFactory, MatrixWorkspaceProperty,
                        PythonAlgorithm)
from mantid.kernel import Direction, IntArrayProperty, logger
from mantid.simpleapi import MaskDetectors


class SANSMaskDTP(PythonAlgorithm):
    """
    Class to generate grouping file
    """

    INSTRUMENTS = {
        'BIOSANS': [
            {
                "name": "detector1",
                "tubes": 192,
                "pixels": 256
            },
            {
                "name": "wing_detector",
                "tubes": 160,
                "pixels": 256
            },
        ],
        'CG2': [
            {
                "name": "detector1",
                "tubes": 192,
                "pixels": 256
            },
        ],
        'EQ-SANS': [
            {
                "name": "detector1",
                "tubes": 192,
                "pixels": 256
            },
        ],
    }

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

    def validateInputs(self):
        '''
        Called before everything else to make sure the inputs are valid
        '''

        issues = dict()

        self.ws = self.getProperty("InputWorkspace").value
        self.instrument = self.ws.getInstrument()
        instrument_name = self.instrument.getName()

        # Validate instrument
        valid_instruments = self.INSTRUMENTS.keys()
        if instrument_name not in valid_instruments:
            message = "Instrument {} not valid. Valid instruments are {}.  Make" \
                      "sure the instrument name is defined in the workspace.".format(
                          instrument_name, valid_instruments)
            issues['InputWorkspace'] = message
            return issues

        # Validate detector
        detector_int = self.getProperty("Detector").value
        number_of_detectors = len(self.INSTRUMENTS[instrument_name])
        if detector_int > number_of_detectors:
            message = "Detector {detector_int} must be less than or equal to the number of detectors: " \
                      "{}".format(number_of_detectors)
            issues['Detector'] = message

        return issues

    def _fix_list_of_ids(self, sequence, min_value, max_value):
        if len(sequence) == 0:  # empty
            return np.arange(min_value, max_value+1)
        else:
            return sequence[sequence <= max_value]  # remove invalid/extra ids

    def PyExec(self):
        detectorNum = self.getProperty("Detector").value
        logger.debug("Detector: {}".format(detectorNum))
        detectorOptions = self.INSTRUMENTS[self.instrument.getName()][detectorNum-1]

        tubes = self.getProperty("Tube").value
        tubes = self._fix_list_of_ids(tubes, 1, detectorOptions["tubes"])
        logger.debug("Tubes: {}".format(tubes))

        pixels = self.getProperty("Pixel").value
        pixels = self._fix_list_of_ids(pixels, 1, detectorOptions["pixels"])
        logger.debug("Pixels: {}".format(pixels))

        # Let's mast it
        detectors_to_mask = []
        # get detector name
        detector = self.instrument.getComponentByName(detectorOptions["name"])
        if not detector:
            raise RuntimeError('Failed to find component "{}"'.format(detectorOptions["name"]))
        # iterate tubes
        for t in tubes:
            tube_component = detector[int(t-1)]  # TODO should be fixed in python API
            for p in pixels:
                # EQSANS needs an extra array !!! pixel128 = tube0[0][128]
                if tube_component.nelements() == 1:
                    tube_component = tube_component[0]
                pixel_component = tube_component[int(p-1)]  # TODO should be fixed in python API
                detectors_to_mask.append(pixel_component.getID())

        logger.debug("IDs: {}".format(detectors_to_mask))

        if len(detectors_to_mask) > 0:
            MaskDetectors(Workspace=self.ws, DetectorList=detectors_to_mask,
                          EnableLogging=False)
            logger.information("{} detectors masked.".format(len(detectors_to_mask)))
        else:
            logger.information("No detectors within this range!")

        self.setProperty("MaskedDetectors", detectors_to_mask)


AlgorithmFactory.subscribe(SANSMaskDTP)
