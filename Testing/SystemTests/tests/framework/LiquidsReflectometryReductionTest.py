# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,attribute-defined-outside-init
import os
import systemtesting
from mantid import *

from mantid.simpleapi import *


def get_file_path(filename):
    """
        Get the location where a test will write its temporary output file.
    """
    alg = AlgorithmManager.createUnmanaged("LRReflectivityOutput")
    alg.initialize()
    alg.setPropertyValue('OutputFilename', filename)
    return alg.getProperty('OutputFilename').value


class LiquidsReflectometryReductionTest(systemtesting.MantidSystemTest):
    def runTest(self):
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")

        LiquidsReflectometryReduction(RunNumbers=[119814],
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
                                      IncidentMediumSelected='2InDiamSi',
                                      GeometryCorrectionFlag=False,
                                      QMin=0.005,
                                      QStep=0.01,
                                      AngleOffset=0.009,
                                      AngleOffsetError=0.001,
                                      ScalingFactorFile=scaling_factor_file,
                                      SlitsWidthFlag=True,
                                      CropFirstAndLastPoints=False,
                                      OutputWorkspace='reflectivity_119814')

    def validate(self):
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "reflectivity_119814", 'REFL_119814_combined_data_v2.nxs'


class LRReflectivityOutputTest(systemtesting.MantidSystemTest):
    """
        Test the reflectivity output algorithm
    """
    def runTest(self):
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        LiquidsReflectometryReduction(RunNumbers=[119814],
                                      NormalizationRunNumber=119690,
                                      SignalPeakPixelRange=[154, 166],
                                      SubtractSignalBackground=True,
                                      SignalBackgroundPixelRange=[151, 169],
                                      NormFlag=True,
                                      NormPeakPixelRange=[154, 160],
                                      NormBackgroundPixelRange=[151, 163],
                                      SubtractNormBackground=True,
                                      LowResDataAxisPixelRangeFlag=True,
                                      LowResDataAxisPixelRange=[99, 158],
                                      LowResNormAxisPixelRangeFlag=True,
                                      LowResNormAxisPixelRange=[98, 158],
                                      TOFRange=[29623.0, 42438.0],
                                      IncidentMediumSelected='2InDiamSi',
                                      GeometryCorrectionFlag=False,
                                      QMin=0.005,
                                      QStep=0.01,
                                      AngleOffset=0.009,
                                      AngleOffsetError=0.001,
                                      ScalingFactorFile=scaling_factor_file,
                                      SlitsWidthFlag=True,
                                      CropFirstAndLastPoints=False,
                                      OutputWorkspace='reflectivity_119814')

        output_path = get_file_path('lr_output.txt')

        # Remove the output file if it exists
        if os.path.isfile(output_path):
            os.remove(output_path)

        LRReflectivityOutput(ReducedWorkspaces=["reflectivity_119814"], OutputFilename=output_path)

        # Read in the first line of the output file to determine success
        self._success = False
        if os.path.isfile(output_path):
            with open(output_path, 'r') as fd:
                content = fd.read()
                if content.startswith('# Experiment IPTS-11601 Run 119814'):
                    self._success = True
            os.remove(output_path)
        else:
            print("Error: expected output file '{}' not found.".format(output_path))

    def validate(self):
        return self._success


class LiquidsReflectometryReductionSimpleErrorTest(systemtesting.MantidSystemTest):
    """
        Test algorithm LiquidsReflectometryReduction with simple binning
    """
    def runTest(self):
        # '/SNS/users/wzz/Mantid_Project/builds/testing/sf_185602_Si2InDisk_auto.cfg'
        scaling_factor_file = FileFinder.getFullPath("sf_185602_Si2InDisk_auto.cfg")

        error_weighting = False
        r = LiquidsReflectometryReduction(RunNumbers=[185746],
                                          NormalizationRunNumber=185610,
                                          SignalPeakPixelRange=[133, 145],
                                          SubtractSignalBackground=True,
                                          SignalBackgroundPixelRange=[129, 149],
                                          NormFlag=True,
                                          NormPeakPixelRange=[135, 143],
                                          NormBackgroundPixelRange=[132, 146],
                                          SubtractNormBackground=True,
                                          ErrorWeighting=error_weighting,
                                          LowResDataAxisPixelRangeFlag=True,
                                          LowResDataAxisPixelRange=[75, 165],
                                          LowResNormAxisPixelRangeFlag=True,
                                          LowResNormAxisPixelRange=[115, 142],
                                          TOFRange=[9965.86996219, 23253.6965784],
                                          TOFSteps=40,
                                          IncidentMediumSelected='Si2InDisk',
                                          GeometryCorrectionFlag=False,
                                          QMin=0.005,
                                          QStep=0.02,
                                          AngleOffset=-0.008,
                                          AngleOffsetError=0.001,
                                          ScalingFactorFile=scaling_factor_file,
                                          SlitsWidthFlag=True,
                                          ApplyPrimaryFraction=True,
                                          SlitTolerance=0.02,
                                          PrimaryFractionRange=[5, 295],
                                          OutputWorkspace='_reflectivity_error')

        self._success = True
        if not r:
            self._success = False

    def validate(self):
        return self._success
