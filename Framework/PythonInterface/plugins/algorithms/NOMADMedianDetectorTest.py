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
            FileProperty(name='Configuration', defaultValue='', direction=Direction.Input, extensions=['.yml'],
                         action=FileAction.Load),
            doc='YML file specifying collimation states and unused eight-packs')

        self.declareProperty(
            FileProperty(name='OutputMaskXML', defaultValue='', direction=Direction.Output, extensions=['.xml'],
                         action=FileAction.Save),
            doc='Output masked pixels in XML format')

    def PyExec(self):
        # initialize data structures 'config' and 'intensities'
        self.config = self.parse_yaml(self.getProperty('Configuration').value)
        self.intensities = self._get_intensities(self.getProperty('InputWorkspace').value)
        # calculate the mask, and export it to XML file
        mask_composite = self.mask_by_tube_intensity | self.mask_by_pixel_intensity
        self.export_mask(mask_composite, self.getProperty('OutputMaskXML').value)


AlgorithmFactory.subscribe(NOMADMedianDetectorTest)
