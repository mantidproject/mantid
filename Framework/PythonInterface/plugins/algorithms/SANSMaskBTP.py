# pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi
import mantid.api
import mantid.kernel
import numpy
from collections import defaultdict


class SANSMaskBTP(mantid.api.PythonAlgorithm):
    """ Class to generate grouping file
    """

    {
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

    def category(self):
        """ Mantid required
        """
        return "Transforms\\Masking;SANS\\Utility"

    def seeAlso(self):
        return ["MaskDetectors", "MaskInstrument"]

    def name(self):
        """ Mantid required
        """
        return "MaskBTP"

    def summary(self):
        """ Mantid required
        """
        return "Algorithm to mask detectors in particular banks, tube, or pixels."

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
        """ yield each integer from a complex range string like "1::2"

        """
        frm, to, step = s.split(':')
        if not to:
            to = max_value        
        return range(frm, to, step+1)



    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty(
            "Workspace", "",
            direction=mantid.kernel.Direction.InOut,
            "Input workspace (optional)")
        
        self.declareProperty(
            "Bank", "", doc="Bank(s) to be masked. If empty, will apply to all banks")
        
        self.declareProperty(
            "Tube", "", doc="Tube(s) to be masked. If empty, will apply to all tubes")
        
        self.declareProperty(
            "Pixel", "", doc="Pixel(s) to be masked. If empty, will apply to all pixels")
        
        self.declareProperty(
            mantid.kernel.IntArrayProperty(
                name="MaskedDetectors", 
                direction=mantid.kernel.Direction.Output),
                doc="List of  masked detectors")


    # pylint: disable=too-many-branches
    def PyExec(self):
        ws = self.getProperty("Workspace").value
        self.instrument = None
        self.instname = self.getProperty("Instrument").value
        
        bankString = self.getProperty("Bank").value
        tubeString = self.getProperty("Tube").value
        pixelString = self.getProperty("Pixel").value

        if ws is not None:
            self.instrument = ws.getInstrument()
            self.instname = self.instrument.getName()



mantid.api.AlgorithmFactory.subscribe(MaskBTP)
