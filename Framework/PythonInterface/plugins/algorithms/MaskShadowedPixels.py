# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
import mantid


class MaskShadowedPixels(mantid.api.PythonAlgorithm):

    def category(self):
        return "DataHandling\\Nexus"

    def seeAlso(self):
        return ["MaskShadowedPixels"]

    def name(self):
        return "MaskShadowedPixels"

    def summary(self):
        return "Save constant wavelength powder diffraction data to a GSAS file in FXYE format"

    def PyInit(self):
        self.declareProperty(mantid.api.WorkspaceProperty('InputWorkspace',
                                                          '',
                                                          mantid.kernel.Direction.Input),
                             "Workspace to save")
        self.declareProperty(mantid.api.FileProperty('OutputFilename',
                                                     '',
                                                     action=mantid.api.FileAction.Save,
                                                     extensions=['.gss']),
                             doc='Name of the GSAS file to save to')

    def validateInputs(self):
        """Validate input properties
        (virtual method)
        """
        issues = dict()

        return issues

    def PyExec(self):
        """
        Main method to execute the algorithm
        """
        # Get input
        wksp = self.getProperty('InputWorkspace').value
        assert wksp, 'Input workspace cannot be None'

        # TODO - To be implemented

        return


# Register the algorithm
mantid.api.AlgorithmFactory.subscribe(MaskShadowedPixels)
