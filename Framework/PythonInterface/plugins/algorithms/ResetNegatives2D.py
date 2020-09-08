# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid import mtd
from mantid.api import (AlgorithmFactory, PythonAlgorithm, WorkspaceProperty, Progress)
from mantid.kernel import Direction

class ResetNegatives2D(PythonAlgorithm):
    def category(self):
        return 'Diffraction\\DataHandling'

    def summary(self):
        """
        summary of the algorithm
        :return:
        """
        return "The algorithm used to reset negative values in 2D workspaces."

    def name(self):
        return "ResetNegatives2D"

    def seeAlso(self):
        return ["PowderReduceP2D", "Bin2DPowderDiffraction", "SaveP2D"]

    def PyInit(self):
        self.declareProperty(WorkspaceProperty('Workspace', '', direction=Direction.Input), doc='Workspace that should be used.')

    def PyExec(self):
        data = mtd[self.getPropertyValue('Workspace')]
        ndp = data.getDimension(1).getNBins()
        last_dp = -1
        last_d = -1
        intMin = 1
        n = 0

        # Iterate through each Spectrum (dp cuts)
        for cdp in range(ndp):
            dp = data.getDimension(1).getX(cdp)

            if (dp == last_dp): continue
            last_dp = dp
            # iterate through each dValue 
            for cd in range(data.getDimension(0).getNBins()):
                Y = data.dataY(cdp)[cd]
                n += 1
                # If intensity Y is smaller then minimum intensity intMin, set minimum intensity intMin to intensity Y
                if Y < intMin: 
                    intMin = Y

        # Check if minimal Intensity is negative. If it is, add -1*intMin to all intensities
        if intMin < 0:
            for cdp in range(ndp):
                intData = data.readY(cdp)
                data.setY(cdp, intData + intMin*-1)


AlgorithmFactory.subscribe(ResetNegatives2D)
