# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi
import mantid.kernel
import mantid.api
import mantid.geometry
import numpy


class MaskAngle(mantid.api.PythonAlgorithm):
    """ Mask detectors between specified angles based on angle type required
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
        angleValidator.setBounds(-180.,180.)
        self.declareProperty(name="MinAngle", defaultValue=0.0, validator=angleValidator,
                             direction=mantid.kernel.Direction.Input, doc="Angles above MinAngle are going to be masked")
        self.declareProperty(name="MaxAngle", defaultValue=180.0, validator=angleValidator,
                             direction=mantid.kernel.Direction.Input, doc="Angles below MaxAngle are going to be masked")
        self.declareProperty('Angle', 'TwoTheta',
                             mantid.kernel.StringListValidator(['TwoTheta', 'Phi', 'InPlane']),
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

        angleMin = self.getProperty('MinAngle').value
        angleMax = self.getProperty('MaxAngle').value
        if self.getProperty('Angle').value != 'InPlane':
            angleMin = numpy.fabs(angleMin)
            angleMax = numpy.fabs(angleMax)
        if angleMin >= angleMax:
            msg = 'MinAngle ({}) must be less than MaxAngle ({})'.format(angleMin, angleMax)
            issues['MinAngle'] = msg
            issues['MaxAngle'] = msg

        return issues

    def _get_phi(self, spectra_pos):
        '''
        The implementation here assumes that z is the beam direction.
        That assumption is not universally true, it depends on the geometry configuration.
        This returns the phi spherical coordinate value
        '''
        return numpy.fabs(numpy.arctan2(spectra_pos.Y(), spectra_pos.X()))

    def _get_in_plane(self, spectra_pos):
        '''
        The implementation here assumes that z is the beam direction and x is in plane.
        That assumption is not universally true, it depends on the geometry configuration.
        This returns the angle from the z-axis constrained in-plane
        '''
        return numpy.arctan2(spectra_pos.X(), spectra_pos.Z())

    def PyExec(self):
        ws = self.getProperty("Workspace").value
        ttmin = numpy.radians(self.getProperty("MinAngle").value)
        ttmax = numpy.radians(self.getProperty("MaxAngle").value)
        if ttmin > ttmax :
            raise ValueError("MinAngle > MaxAngle, please check angle range for masking")

        angle_phi = self.getProperty('Angle').value == 'Phi'
        angle_in_plane = self.getProperty('Angle').value == 'InPlane'
        if not angle_in_plane:
            if ttmin < 0.:
                self.log().information('Using absolute value of MinAngle')
                ttmin = numpy.fabs(ttmin)
            if ttmax < 0.:
                self.log().information('Using absolute value of MaxAngle')
                ttmax = numpy.fabs(ttmax)
        spectrum_info = ws.spectrumInfo()
        detector_info = ws.detectorInfo()
        det_ids = detector_info.detectorIDs()
        masked_ids = list()
        for spectrum in spectrum_info:
            if not spectrum.isMonitor:
                # Get the first detector of spectrum. Ignore time aspects.
                if angle_phi:
                    val = self._get_phi(spectrum.position)
                elif angle_in_plane:
                    val = self._get_in_plane(spectrum.position)
                else:  # Two theta
                    val =spectrum.twoTheta

                if val>= ttmin and val<= ttmax:
                    detectors = spectrum.spectrumDefinition
                    for j in range(len(detectors)):
                        masked_ids.append(det_ids[detectors[j][0]])
        if not masked_ids:
            self.log().information("no detectors within this range")
        else:
            mantid.simpleapi.MaskDetectors(Workspace=ws,DetectorList=numpy.array(masked_ids))
        self.setProperty("MaskedDetectors", numpy.array(masked_ids))


mantid.api.AlgorithmFactory.subscribe(MaskAngle)
