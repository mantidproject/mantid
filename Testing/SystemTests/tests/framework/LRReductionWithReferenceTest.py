# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import json

import systemtesting
from mantid.api import FileFinder
from mantid.simpleapi import LRReductionWithReference


class LRReductionWithRefrenceTest(systemtesting.MantidSystemTest):
    """
    Test the reflectivity reduction with a reference algorithm
    """

    def skipTests(self):
        try:
            import refl1d  # noqa: F401
        except ImportError:
            return True
        return False

    def runTest(self):
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")

        # Create the Refl1D theoretical model
        refl1d_model_parameters = dict(
            back_sld=2.07,
            back_roughness=1.0,
            front_sld=0,
            layers=[
                dict(thickness=10, sld=3.5, isld=0, roughness=2),
            ],
            scale=1,
            background=0,
        )
        refl1d_model_json = json.dumps(refl1d_model_parameters)

        LRReductionWithReference(
            RunNumbers=[119814],
            NormalizationRunNumber=119690,
            SignalPeakPixelRange=[154, 166],
            SubtractSignalBackground=True,
            SignalBackgroundPixelRange=[151, 169],
            NormFlag=True,
            NormPeakPixelRange=[154, 166],
            NormBackgroundPixelRange=[151, 163],
            SubtractNormBackground=True,
            LowResDataAxisPixelRangeFlag=True,
            LowResDataAxisPixelRange=[99, 158],
            LowResNormAxisPixelRangeFlag=True,
            LowResNormAxisPixelRange=[98, 158],
            TOFRange=[29623.0, 42438.0],
            TOFRangeFlag=True,
            QMin=0.005,
            QStep=0.01,
            AngleOffset=0.009,
            AngleOffsetError=0.001,
            OutputWorkspace="reflectivity_119814_with_reference",
            ApplyScalingFactor=False,
            ScalingFactorFile=scaling_factor_file,
            SlitTolerance=0.02,
            SlitsWidthFlag=True,
            IncidentMediumSelected="2InDiamSi",
            GeometryCorrectionFlag=False,
            CropFirstAndLastPoints=False,
            Refl1DModelParameters=refl1d_model_json,
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "reflectivity_119814_with_reference", "REFL_119814_combined_data_with_reference.nxs"
