# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.api import (AlgorithmFactory, FileAction, FileProperty, PropertyMode, PythonAlgorithm, WorkspaceProperty)
from mantid.kernel import Direction
from mantid.utils.nomad.diagnostics import _NOMADMedianDetectorTest


class NOMADMedianDetectorTest(PythonAlgorithm, _NOMADMedianDetectorTest):

    def category(self):
        """ Mantid required
        """
        return "Diagnostics"

    def seeAlso(self):
        return ["MedianDetectorTest"]

    def name(self):
        """ Mantid required
        """
        return "NOMADMedianDetectorTest"

    def summary(self):
        """ Mantid required
        """
        return "Identifies detector pixels and whole tubes having total numbers of counts over a user defined maximum" \
               " or less than a user define minimum."

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty('InputWorkspace', defaultValue='', direction=Direction.Input,
                              optional=PropertyMode.Mandatory),
            doc='Workspace containing reference pixel intensities')

        self.declareProperty(
            FileProperty(name='ConfigurationFile', defaultValue='', direction=Direction.Input, extensions=['.yml'],
                         action=FileAction.Load),
            doc='YML file specifying collimation states and unused eight-packs')

        self.declareProperty(name='SolidAngleNorm', defaultValue=True, direction=Direction.Input,
                             doc='Normalize each pixel by its solid angle?')

        self.declareProperty(
            FileProperty('OutputMaskXML', defaultValue='', extensions=['.xml'],
                         action=FileAction.Save),
            doc='Output masked pixels in XML format')

        self.declareProperty(
            FileProperty('OutputMaskASCII', defaultValue='', extensions=['.txt'],
                         action=FileAction.Save),
            doc='Output masked pixels in single-column ASCII file')

    def PyExec(self):
        # initialize data structures 'config' and 'intensities'
        self.config = self.parse_yaml(self.getProperty('ConfigurationFile').value)
        self.intensities = self._get_intensities(self.getProperty('InputWorkspace').value,
                                                 self.getProperty('SolidAngleNorm').value)
        # calculate the mask, and export it to XML and/or single-column ASCII file
        mask_composite = self.mask_by_tube_intensity | self.mask_by_pixel_intensity
        for exported in ['OutputMaskXML', 'OutputMaskASCII']:
            if self.getProperty(exported).value:
                self.export_mask(mask_composite, self.getProperty(exported).value)


AlgorithmFactory.subscribe(NOMADMedianDetectorTest)
