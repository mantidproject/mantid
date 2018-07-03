#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty
from mantid.kernel import Direction, FloatBoundedValidator
import mantid.simpleapi
import math


class MRGetTheta(PythonAlgorithm):
    """ Get the theta scattering angle for the Magnetism Reflectometer """

    def category(self):
        """ Return category """
        return "Reflectometry\\SNS"

    def name(self):
        """ Return name """
        return "MRGetTheta"

    def summary(self):
        """ Return summary """
        return "Get the theta scattering angle for the Magnetism Reflectometer (radians)."

    def PyInit(self):
        """ Declare properties """
        self.declareProperty(WorkspaceProperty("Workspace", "", Direction.Input), "Workspace containing MR data")
        self.declareProperty("AngleOffset", 0.,FloatBoundedValidator(lower=0.), "Angle offset (rad)")
        self.declareProperty("UseSANGLE", False, doc="If True, use SANGLE as the scattering angle. If False, use DANGLE.")
        self.declareProperty("SpecularPixel", 0., doc="Pixel position of the specular reflectivity [optional]")
        self.declareProperty("Theta", 0., direction=Direction.Output, doc="Scattering angle theta [rad]")

        return

    def PyExec(self):
        """ Main execution body """
        _w=self.getProperty("Workspace").value

        angle_offset = self.getProperty("AngleOffset").value

        if self.getProperty("UseSANGLE").value:
            theta = self.read_log(_w, 'SANGLE', target_units='rad', assumed_units='deg')
        else:
            dangle = self.read_log(_w, 'DANGLE', target_units='rad', assumed_units='deg')
            dangle0 = self.read_log(_w, 'DANGLE0', target_units='rad', assumed_units='deg')
            det_distance = self.read_log(_w, 'SampleDetDis', target_units='m', assumed_units='mm')
            direct_beam_pix = _w.getRun()['DIRPIX'].getStatistics().mean
            ref_pix = self.getProperty("SpecularPixel").value
            if ref_pix == 0.:
                ref_pix = direct_beam_pix

            # Get pixel size from instrument properties
            if _w.getInstrument().hasParameter("pixel-width"):
                pixel_width = float(_w.getInstrument().getNumberParameter("pixel-width")[0]) / 1000.0
            else:
                mantid.simpleapi.logger.warning("Not pixel width found in instrument, assuming 0.7 mm.")
                pixel_width = 0.0007

            theta = (dangle - dangle0) / 2.0 + ((direct_beam_pix - ref_pix) * pixel_width) / (2.0 * det_distance)

        self.setProperty("Theta", abs(theta) + angle_offset)
        return

    def read_log(self, ws, name, target_units='', assumed_units=''):
        """
            Read a log value, taking care of units.
            If the log entry has no units, the target units are assumed.
            :param ws: workspace
            :param str name: name of the property to read
            :param str target_units: units to convert to
            :param str assumed_units: units of origin, if not specified in the log itself
        """
        _units = {'m': {'mm': 1000.0,},
                  'mm': {'m': 0.001,},
                  'deg': {'rad': math.pi/180.,},
                  'rad': {'deg': 180./math.pi,},
                  }
        prop = ws.getRun().getProperty(name)
        value = prop.getStatistics().mean

        # If the property has units we don't recognize, use the assumed units
        units = prop.units if prop.units in _units else assumed_units

        if units in _units and target_units in _units[units]:
            return value * _units[units][target_units]
        return value

AlgorithmFactory.subscribe(MRGetTheta)
