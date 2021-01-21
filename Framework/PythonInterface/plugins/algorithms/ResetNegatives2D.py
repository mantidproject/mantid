# Mantid Repository : https://github.com/mantidproject/mantid

#

# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,

#   NScD Oak Ridge National Laboratory, European Spallation Source,

#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS

# SPDX - License - Identifier: GPL - 3.0 +



from mantid import mtd
from mantid.api import (AlgorithmFactory, PythonAlgorithm, WorkspaceProperty)
from mantid.kernel import Direction
from mantid.simpleapi import Scale

import numpy as np


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
        self.declareProperty(WorkspaceProperty('Workspace',
                                               '',
                                               direction=Direction.Input),
                             doc='Workspace that should be used.')


    def PyExec(self):
        data = mtd[self.getPropertyValue('Workspace')]
        intMin = np.min(data.extractY())
        # Check if minimal Intensity is negative. If it is, add -1*intMin to all intensities
        if intMin < 0:
            Scale(InputWorkspace=data, OutputWorkspace=data, Factor=-intMin, Operation="Add")


AlgorithmFactory.subscribe(ResetNegatives2D)