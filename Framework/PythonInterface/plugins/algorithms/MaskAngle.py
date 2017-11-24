#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi
import mantid.kernel
import mantid.api
import numpy


class MaskAngle(mantid.api.PythonAlgorithm):
    """ Class to generate grouping file
    """

    def category(self):
        """ Mantid required
        """
        return "Transforms\\Masking"

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
                             direction=mantid.kernel.Direction.Input, doc="Angles above StartAngle are going to be masked")
        self.declareProperty(name="MaxAngle", defaultValue=0.0, validator=angleValidator,
                             direction=mantid.kernel.Direction.Input, doc="Angles above StartAngle are going to be masked")
        self.declareProperty(mantid.kernel.IntArrayProperty(name="MaskedDetectors", direction=mantid.kernel.Direction.Output),
                             doc="List of detector masked, with scatterin angles between MinAngle and MaxAngle")

    def validateInputs(self):
        issues = dict()

        ws = self.getProperty("Workspace").value

        try:
            if type(ws).__name__ == "WorkspaceGroup":
                for w in ws:
                    w.getInstrument().getSource().getPos()
            else:
                ws.getInstrument().getSource().getPos()
        except (RuntimeError, ValueError, AttributeError, TypeError):
            issues["Workspace"] = "Workspace must have an associated instrument."

        return issues

    def PyExec(self):
        ws = self.getProperty("Workspace").value
        ttmin = self.getProperty("MinAngle").value
        ttmax = self.getProperty("MaxAngle").value
        if ttmin > ttmax :
            raise ValueError("MinAngle > MaxAngle, please check angle range for masking")

        detlist=[]

        numspec = ws.getNumberHistograms()
        source=ws.getInstrument().getSource().getPos()
        sample=ws.getInstrument().getSample().getPos()
        spectrumInfo = ws.spectrumInfo()
        for i in range(numspec):
            if not spectrumInfo.isMonitor(i):
                det = ws.getDetector(i)
                tt=numpy.degrees(det.getTwoTheta(sample,sample-source))
                if tt>= ttmin and tt<= ttmax:
                    detlist.append(det.getID())

        if len(detlist)> 0:
            mantid.simpleapi.MaskDetectors(Workspace=ws,DetectorList=detlist)
        else:
            self.log().information("no detectors within this range")
        self.setProperty("MaskedDetectors", numpy.array(detlist))


mantid.api.AlgorithmFactory.subscribe(MaskAngle)
