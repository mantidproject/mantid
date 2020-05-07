# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
import time
import math
import os
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *
from functools import reduce #pylint: disable=redefined-builtin

# Unable to generate this props list in PyInit using AlgorithmManager
# from LiquidsReflectometryReduction to copy properties here
LR_ALG_FOR_PROPS = "LiquidsReflectometryReduction"
PROPS_TO_COPY = [
    'RunNumbers',
    'InputWorkspace',
    'NormalizationRunNumber',
    'SignalPeakPixelRange',
    'SubtractSignalBackground',
    'SignalBackgroundPixelRange',
    'NormFlag',
    'NormPeakPixelRange',
    'SubtractNormBackground',
    'NormBackgroundPixelRange',
    'LowResDataAxisPixelRangeFlag',
    'LowResDataAxisPixelRange',
    'LowResNormAxisPixelRangeFlag',
    'LowResNormAxisPixelRange',
    'TOFRange',
    'TOFRangeFlag',
    'QMin',
    'QStep',
    'AngleOffset',
    'AngleOffsetError',
    'OutputWorkspace',
    'ApplyScalingFactor',
    'ScalingFactorFile',
    'SlitTolerance',
    'SlitsWidthFlag',
    'IncidentMediumSelected',
    'GeometryCorrectionFlag',
    'FrontSlitName',
    'BackSlitName',
    'TOFSteps',
    'CropFirstAndLastPoints',
    'ApplyPrimaryFraction',
    'PrimaryFractionRange']

class LRReductionWithReference(DataProcessorAlgorithm):
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LRReductionWithReference"

    def version(self):
        return 1

    def summary(self):
        return "REFL reduction using a reference measurement for normalization"

    def PyInit(self):
        self.copyProperties(LR_ALG_FOR_PROPS, PROPS_TO_COPY)

    def PyExec(self):
        wksp = self.getProperty("InputWorkspace").value
        self.setProperty('OutputWorkspace', wksp)

AlgorithmFactory.subscribe(LRReductionWithReference)
