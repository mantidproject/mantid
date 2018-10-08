# pylint: disable=no-init,invalid-name
from __future__ import absolute_import, division, print_function

import re

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

    # This is the pattern for tubes, pixels
    PATTERN_HYPHEN_RANGE = r"^\d+([\-,]\d+)*$"
    PATTERN_COLON_RANGE = r"^\d+\:\d*\:\d+$"

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
                "InputWorkspace", "", direction=Direction.InOut,),
            doc="Input Workspace")

        self.declareProperty(
            "Detector", "1",  # Default 1
            doc="Detector number to be masked.")

        self.declareProperty(
            "Tube", "",
            doc="Tube(s) to be masked. If empty, will apply to all tubes")

        self.declareProperty(
            "Pixel", "",
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
        self.instrument_name = self.instrument.getName()

        # Validate instrument
        valid_instruments = self.INSTRUMENTS.keys()
        if self.instrument_name not in valid_instruments:
            message = "Instrument {} not valid. Valid instruments are {}.  Make" \
                      "sure the instrument name is defined in the workspace.".format(
                          self.instrument_name, valid_instruments)
            issues['InputWorkspace'] = message
            return issues

        # Validate detector: must be an int
        detector_str = self.getProperty("Detector").value

        try:
            detector_int = int(detector_str)
            number_of_detectors = len(self.INSTRUMENTS[self.instrument_name])
            if detector_int > number_of_detectors:
                message = "Detector must be less than the number of detectors: " \
                          "{}".format(number_of_detectors)
                issues['Detector'] = message
        except ValueError:
            message = "Detector must be an integer value"
            issues['Detector'] = message

        return issues

    def _hyphen_range(self, s):
        """ yield each integer from a complex range string like "1-9,12, 15-20,23"

        >>> list(hyphen_range('1-9,12, 15-20,23'))
        [1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 15, 16, 17, 18, 19, 20, 23]

        >>> list(hyphen_range('1-9,12, 15-20,2-3-4'))
        Traceback (most recent call last):
            ...
        ValueError: format error in 2-3-4
        """
        for x in s.split(','):
            elem = x.split('-')
            if len(elem) == 1:  # a number
                yield int(elem[0])
            elif len(elem) == 2:  # a range inclusive
                start, end = map(int, elem)
                for i in range(start, end+1):
                    yield i
            else:  # more than one hyphen
                raise ValueError('format error in %s' % x)

    def _double_colon_range(self, s, max_value):
        """
        yield each integer from a complex range string like:
        1::2 or 1:10:2
        Note that no validation is done here!!
        """
        try:
            frm, to, step = map(int, s.split(':'))
        except ValueError:
            to = max_value
            frm, step = map(int, s.split('::'))
        return range(frm, to+1, step)

    def _string_to_int(self, sequence, min_value, max_value):
        sequence = sequence.replace(" ", "")  # remove spaces
        if not sequence:  # empty
            return range(min_value, max_value+1)
        elif re.match(self.PATTERN_COLON_RANGE, sequence):
            return self._double_colon_range(sequence, max_value)
        elif re.match(self.PATTERN_HYPHEN_RANGE, sequence):
            return list(self._hyphen_range(sequence))
        else:
            raise ValueError(
                'See the format of the fields in the documentation')

    def PyExec(self):

        detector_str = self.getProperty("Detector").value
        tube_str = self.getProperty("Tube").value
        pixel_str = self.getProperty("Pixel").value

        # This was validated above, so we can safely convert it to int
        detectors = int(detector_str)
        logger.debug("Detectors: {}".format(detectors))

        tubes = self._string_to_int(
            tube_str, 1,
            self.INSTRUMENTS[self.instrument_name][detectors-1]["tubes"])
        logger.debug("Tubes: {}".format(tubes))

        pixels = self._string_to_int(
            pixel_str, 1,
            self.INSTRUMENTS[self.instrument_name][detectors-1]["pixels"])
        logger.debug("Pixels: {}".format(pixels))

        # Let's mast it
        detectors_to_mask = []
        # get detector name
        detector = self.instrument.getComponentByName(
            self.INSTRUMENTS[self.instrument_name][detectors-1]["name"])
        # iterate tubes
        for t in tubes:
            tube_component = detector[t-1]
            for p in pixels:
                # EQSANS needs an extra array !!! pixel128 = tube0[0][128]
                if tube_component.nelements() == 1:
                    tube_component = tube_component[0]
                pixel_component = tube_component[p-1]
                detectors_to_mask.append(pixel_component.getID())

        logger.debug("IDs: {}".format(detectors_to_mask))

        if len(detectors_to_mask) > 0:
            MaskDetectors(Workspace=self.ws, DetectorList=detectors_to_mask,
                          EnableLogging=False)
            logger.information(
                "{} detectors masked.".format(len(detectors_to_mask)))
        else:
            logger.information("No detectors within this range!")

        self.setProperty("MaskedDetectors", detectors_to_mask)


AlgorithmFactory.subscribe(SANSMaskDTP)
