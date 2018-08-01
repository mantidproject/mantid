# pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi
from mantid.api import MatrixWorkspaceProperty, AlgorithmFactory
from mantid.kernel import Direction, logger
import re



class SANSMaskDTP(mantid.api.PythonAlgorithm):
    """ Class to generate grouping file
    """

    INSTRUMENTS = {
        'BIOSANS': [
            {
                "id": 1,
                "name": "detector1",
                "tubes": 192,
                "pixels": 256
            },
            {
                "id": 2,
                "name": "wing_detector",
                "tubes": 160,
                "pixels": 256
            },
        ],
        'GPSANS': [
            {
                "id": 1,
                "name": "detector1",
                "tubes": 192,
                "pixels": 256
            },
        ],
        'EQ-SANS': [
            {
                "id": 1,
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
            mantid.api.MatrixWorkspaceProperty(
                "InputWorkspace", "", direction = Direction.Input),
            doc="Input Workspace")

        self.declareProperty(
            "Detector", "",
            doc="Detector(s) number(s) to be masked. If empty, will apply to all Detectors")
        
        self.declareProperty(
            "Tube", "",
            doc="Tube(s) to be masked. If empty, will apply to all tubes")
        
        self.declareProperty(
            "Pixel", "",
            doc="Pixel(s) to be masked. If empty, will apply to all pixels")
        
    def _validate_instrument_name(self):
        valid_instruments = self.INSTRUMENTS.keys()
        if self.instrument_name not in valid_instruments:
            raise RuntimeError("Instrument {} not valid. Valid instruments are {}.".format(
                self.instrument_name, valid_instruments) +
                " Make sure the instrument name is defined in the workspace.")

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
            if len(elem) == 1: # a number
                yield int(elem[0])
            elif len(elem) == 2: # a range inclusive
                start, end = map(int, elem)
                for i in range(start, end+1):
                    yield i
            else: # more than one hyphen
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
        sequence = sequence.replace(" ", "") # remove spaces
        if not sequence: # empty
            return range(min_value, max_value)
        elif re.match(self.PATTERN_COLON_RANGE, sequence):
            return self._double_colon_range(sequence, max_value)
        elif re.match(self.PATTERN_HYPHEN_RANGE, sequence):
            return self._hyphen_range(sequence)
        else:
            raise ValueError('See the format of the fields in the documentation')

    def PyExec(self):
        ws = self.getProperty("InputWorkspace").value
        self.instrument = ws.getInstrument()
        self.instrument_name = self.instrument.getName()
        self._validate_instrument_name()

        detector_str = self.getProperty("Detector").value
        tube_str = self.getProperty("Tube").value
        pixel_str = self.getProperty("Pixel").value

        if detector_str:
            detectors = self._string_to_int(detector_str, 1, len(self.INSTRUMENTS[self.instrument_name]))
            logger.debug("Detectors: {}".format(detectors))
        if tube_str:
            tubes = self._string_to_int(tube_str, 1, self.INSTRUMENTS[self.instrument_name]["tubes"])
            logger.debug("Tubes: {}".format(tubes))
        if pixel_str:
            pixels = self._string_to_int(pixel_str, 1, self.INSTRUMENTS[self.instrument_name]["pixels"])
            logger.debug("Pixels: {}".format(pixels))



AlgorithmFactory.subscribe(SANSMaskDTP)
