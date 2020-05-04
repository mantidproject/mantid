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


class LRReductionWithReference(PythonAlgorithm):
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LRReductionWithReference"

    def version(self):
        return 1

    def summary(self):
        return "REFL reduction using a reference measurement for normalization"

    def PyInit(self):
        self.declareProperty(StringArrayProperty("RunNumbers"), "List of run numbers to process")
        self.declareProperty(WorkspaceProperty("InputWorkspace", "",
                                               Direction.Input, PropertyMode.Optional),
                             "Optionally, we can provide a workspace directly")
        self.declareProperty("NormalizationRunNumber", 0, "Run number of the normalization run to use")
        self.declareProperty(IntArrayProperty("SignalPeakPixelRange", [123, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the data peak")
        self.declareProperty("SubtractSignalBackground", True,
                             doc='If true, the background will be subtracted from the data peak')
        self.declareProperty(IntArrayProperty("SignalBackgroundPixelRange", [123, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the background. Default:(123,137)")
        self.declareProperty("NormFlag", True, doc="If true, the data will be normalized")
        self.declareProperty(IntArrayProperty("NormPeakPixelRange", [127, 133],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the normalization peak")
        self.declareProperty("SubtractNormBackground", True,
                             doc="If true, the background will be subtracted from the normalization peak")
        self.declareProperty(IntArrayProperty("NormBackgroundPixelRange", [127, 137],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range defining the background for the normalization")
        self.declareProperty("LowResDataAxisPixelRangeFlag", True,
                             doc="If true, the low resolution direction of the data will be cropped according "
                                 + "to the lowResDataAxisPixelRange property")
        self.declareProperty(IntArrayProperty("LowResDataAxisPixelRange", [115, 210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use in the low resolution direction of the data")
        self.declareProperty("LowResNormAxisPixelRangeFlag", True,
                             doc="If true, the low resolution direction of the normalization run will be cropped "
                                 + "according to the LowResNormAxisPixelRange property")
        self.declareProperty(IntArrayProperty("LowResNormAxisPixelRange", [115, 210],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use in the low resolution direction of the normalizaion run")
        self.declareProperty(FloatArrayProperty("TOFRange", [0., 340000.],
                                                FloatArrayLengthValidator(2), direction=Direction.Input),
                             "TOF range to use")
        self.declareProperty("TOFRangeFlag", True,
                             doc="If true, the TOF will be cropped according to the TOF range property")
        self.declareProperty("QMin", 0.05, doc="Minimum Q-value")
        self.declareProperty("QStep", 0.02, doc="Step size in Q. Enter a negative value to get a log scale")
        self.declareProperty("AngleOffset", 0.0, doc="angle offset (degrees)")
        self.declareProperty("AngleOffsetError", 0.0, doc="Angle offset error (degrees)")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")
        self.declareProperty("ScalingFactorFile", "", doc="Scaling factor configuration file")
        self.declareProperty("SlitTolerance", 0.02, doc="Tolerance for matching slit positions")
        self.declareProperty("SlitsWidthFlag", True,
                             doc="Looking for perfect match of slits width when using Scaling Factor file")
        self.declareProperty("IncidentMediumSelected", "", doc="Incident medium used for those runs")
        self.declareProperty("GeometryCorrectionFlag", False, doc="Use or not the geometry correction")
        self.declareProperty("FrontSlitName", "S1", doc="Name of the front slit")
        self.declareProperty("BackSlitName", "Si", doc="Name of the back slit")
        self.declareProperty("TOFSteps", 40.0, doc="TOF step size")
        self.declareProperty("CropFirstAndLastPoints", True, doc="If true, we crop the first and last points")
        self.declareProperty("ApplyPrimaryFraction", False, doc="If true, the primary fraction correction will be applied")
        self.declareProperty(IntArrayProperty("PrimaryFractionRange", [117, 197],
                                              IntArrayLengthValidator(2), direction=Direction.Input),
                             "Pixel range to use for calculating the primary fraction correction.")

    #pylint: disable=too-many-locals,too-many-branches
    def PyExec(self): # noqa

AlgorithmFactory.subscribe(LRReductionWithReference)
