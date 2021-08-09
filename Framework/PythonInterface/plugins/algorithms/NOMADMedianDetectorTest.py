# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


# package imports
from mantid.api import FileAction, FileProperty, PropertyMode, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction
from mantid.utils.nomad._median_detector_test import _NOMADMedianDetectorTest


class NOMADMedianDetectorTest(PythonAlgorithm, _NOMADMedianDetectorTest):
    def category(self):
        return "Diagnostics"

    def seeAlso(self):
        return ["MedianDetectorTest"]

    def name(self):
        return "NOMADMedianDetectorTest"

    def summary(self):
        return "Identifies detector pixels and whole tubes having total numbers of counts over a user defined maximum" \
               " or less than a user define minimum."

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty('InputWorkspace', defaultValue='', direction=Direction.Input,
                              optional=PropertyMode.Mandatory),
            doc='Workspace containing reference pixel intensities')
        self.declareProperty(
            FileProperty(name='Configuration', defaultValue='', direction=Direction.Input, extensions=['.yml'],
                         action=FileAction.Load, optional=PropertyMode.Mandatory),
            doc='YML file specifying collimation states and unused eight-packs')
        self.declareProperty(
            FileProperty(name='OutputMaskXML', defaultValue='', direction=Direction.Output, extensions=['.xml'],
                         action=FileAction.Save, optional=PropertyMode.Mandatory)
        )

    def PyExec(self):
        pass
