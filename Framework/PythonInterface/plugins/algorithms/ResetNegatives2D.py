# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid import mtd
from mantid.api import (AlgorithmFactory, PythonAlgorithm, WorkspaceProperty)
from mantid.kernel import Direction, EnabledWhenProperty, PropertyCriterion
from mantid.simpleapi import Scale, CreateWorkspace

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
        self.declareProperty(
            'AddMinimum',
            True,
            direction=Direction.Input,
            doc=
            'If set to True, adds the most negative intensity to all intensities.'
        )
        self.declareProperty(
            'ResetValue',
            0,
            direction=Direction.Input,
            doc='Set negative intensities to the specified value (default=0).')
        self.setPropertySettings(
            'ResetValue',
            EnabledWhenProperty('AddMinimum', PropertyCriterion.IsNotDefault))

    def getInputs(self):
        self.data = mtd[self.getPropertyValue('Workspace')]
        self._addMin = self.getProperty('AddMinimum').value
        self._resetValue = self.getProperty('ResetValue').value

    def PyExec(self):
        self.getInputs()

        xData = self.data.extractX()
        yData = self.data.extractY()

        eData = self.data.extractE()
        if self._addMin:
            intMin = np.min(yData)
            # Check if minimal Intensity is negative. If it is, add -1*intMin to all intensities
            if intMin < 0:
                Scale(InputWorkspace=self.data,
                      OutputWorkspace=self.data,
                      Factor=-intMin,
                      Operation="Add")
        else:
            yDataNew = np.where(yData < 0, self._resetValue, yData)
            CreateWorkspace(OutputWorkspace=self.data,
                            DataX=xData,
                            DataY=yDataNew,
                            DataE=eData,
                            NSpec=self.data.getNumberHistograms(),
                            ParentWorkspace=self.data)


AlgorithmFactory.subscribe(ResetNegatives2D)
