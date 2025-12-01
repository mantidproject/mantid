# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, DataProcessorAlgorithm


class StitchByBackground(DataProcessorAlgorithm):
    def name(self):
        return "StitchByBackground"

    def category(self):
        return "Utility"

    def summary(self):
        return "Stitch banks together at a given Q value, without rebinning the data to preserve resolution."

    def seeAlso(self):
        return []

    def checkGroups(self):
        return False

    def validateInputs(self):
        pass

    def PyInit(self):
        pass

    def PyExec(self):
        pass


# Register the algorithm to appear in the API.
AlgorithmFactory.subscribe(StitchByBackground)
