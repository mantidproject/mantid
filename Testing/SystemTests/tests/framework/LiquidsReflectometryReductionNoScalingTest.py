# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.api import FileFinder
from mantid.simpleapi import LiquidsReflectometryReduction


class ApplyScalingFactorTest(systemtesting.MantidSystemTest):
    def runTest(self):
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")

        LiquidsReflectometryReduction(
            RunNumbers=[119814],
            NormalizationRunNumber=119690,
            SignalPeakPixelRange=[154, 166],
            SubtractSignalBackground=True,
            SignalBackgroundPixelRange=[151, 169],
            ErrorWeighting=True,
            NormFlag=True,
            NormPeakPixelRange=[154, 160],
            NormBackgroundPixelRange=[151, 163],
            SubtractNormBackground=True,
            LowResDataAxisPixelRangeFlag=True,
            LowResDataAxisPixelRange=[99, 158],
            LowResNormAxisPixelRangeFlag=True,
            LowResNormAxisPixelRange=[98, 158],
            TOFRange=[29623.0, 42438.0],
            IncidentMediumSelected="2InDiamSi",
            GeometryCorrectionFlag=False,
            QMin=0.005,
            QStep=0.01,
            AngleOffset=0.009,
            AngleOffsetError=0.001,
            ApplyScalingFactor=False,
            ScalingFactorFile=scaling_factor_file,
            SlitsWidthFlag=True,
            CropFirstAndLastPoints=False,
            OutputWorkspace="reflectivity_119814_no_scaling",
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "reflectivity_119814_no_scaling", "REFL_119814_combined_data_no_scaling.nxs"
