#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi
import mantid.kernel
import mantid.api
import mantid.geometry
import numpy
import math

class MaskAngle(mantid.api.PythonAlgorithm):
    """ Class to generate grouping file
    """

    def category(self):
        """ Mantid required
        """
        return "Transforms\\Masking"

    def seeAlso(self):
        return [ "MaskDetectors" ]

    def name(self):
        """ Mantid require
        """
        return "MaskAngle"

    def summary(self):
        """ Mantid require
        """
        return "Algorithm to mask detectors with scattering angles in a given interval (in degrees)."

    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty("Workspace", "",direction=mantid.kernel.Direction.Input),
                             "Input workspace")

        angleValidator=mantid.kernel.FloatBoundedValidator()
        angleValidator.setBounds(0.,180.)
        self.declareProperty(name="MinAngle", defaultValue=0.0, validator=angleValidator,
                             direction=mantid.kernel.Direction.Input, doc="Angles above MinAngle are going to be masked")
        self.declareProperty(name="MaxAngle", defaultValue=180.0, validator=angleValidator,
                             direction=mantid.kernel.Direction.Input, doc="Angles below MaxAngle are going to be masked")
        self.declareProperty('Angle', 'TwoTheta',
                             mantid.kernel.StringListValidator(['TwoTheta', 'Phi']),
                             'Which angle to use')
        self.declareProperty(mantid.kernel.IntArrayProperty(name="MaskedDetectors", direction=mantid.kernel.Direction.Output),
                             doc="List of detector masked, with scattering angles between MinAngle and MaxAngle")

    def validateInputs(self):
        issues = dict()
        ws = self.getProperty("Workspace").value
        hasInstrument = True
        if type(ws).__name__ == "WorkspaceGroup" and len(ws) > 0:
            for item in ws:
                hasInstrument = hasInstrument and len(item.componentInfo()) > 0
        else:
            hasInstrument = len(ws.componentInfo()) > 0
        if not hasInstrument:
            issues["Workspace"] = "Workspace must have an associated instrument."
        return issues

    def PyExec(self):
        ws = self.getProperty("Workspace").value
        ttmin = numpy.radians(self.getProperty("MinAngle").value)
        ttmax = numpy.radians(self.getProperty("MaxAngle").value)
        if ttmin > ttmax :
            raise ValueError("MinAngle > MaxAngle, please check angle range for masking")

        angle = self.getProperty('Angle').value

        numspec = ws.getNumberHistograms()
        spectrumInfo = ws.spectrumInfo()
        detectorInfo = ws.detectorInfo()
        componentInfo = ws.componentInfo()
        det_ids = detectorInfo.detectorIDs()
        masked_ids = list()

        if angle == 'Phi':
            for i in range(numspec):
                if not spectrumInfo.isMonitor(i):
                    det_index = spectrumInfo.getSpectrumDefinition(i)[0][0]
                    pos = detectorInfo.position(det_index)
                    phi = math.fabs(math.atan2(pos.Y(), pos.X()))
                    if phi>= ttmin and phi<= ttmax:
                        detectorInfo.setMasked(det_index, True)
                        masked_ids.append(det_ids[det_index])
        else:
            beam = componentInfo.l1() 
            for i in range(numspec):
                if not spectrumInfo.isMonitor(i):
                    det_index = spectrumInfo.getSpectrumDefinition(i)[0][0]
                    tt=detectorInfo.twoTheta(det_index)
                    if tt>= ttmin and tt<= ttmax:
                        detectorInfo.setMasked(det_index, True)
                        masked_ids.append(det_ids[det_index])

        if not masked_ids:
            self.log().information("no detectors within this range")
        self.setProperty("MaskedDetectors", numpy.array(masked_ids))


mantid.api.AlgorithmFactory.subscribe(MaskAngle)
