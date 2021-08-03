# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,attribute-defined-outside-init
import os
import systemtesting
import numpy as np
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

        self._success = self.verify_result(r)

    def validate(self):
        return self._success

    def verify_result(self, test_output):
        """Verify reduced result

        Parameters
        ----------
        test_output: Workspace2D

        Returns
        -------
        bool
            same or not

        """
        test_vec_x = test_output.readX(0)
        test_vec_y = test_output.readY(0)
        test_vec_e = test_output.readE(0)

        gold_data = np.array([
            [4.459747512458072521e-02, 4.190316438906013260e-05, 6.788916891170880312e-06],
            [4.548942462707233902e-02, 1.020991718819984204e-04, 9.137333113692204542e-06],
            [4.639921311961378581e-02, 1.987300867245147932e-04, 1.174399723206277852e-05],
            [4.732719738200606707e-02, 2.491141929180866574e-04, 1.228521075888894327e-05],
            [4.827374132964618036e-02, 2.950759911033949950e-04, 1.327064763505331283e-05],
            [4.923921615623910730e-02, 2.581707802023960191e-04, 1.224244424265238484e-05],
            [5.022400047936388667e-02, 2.000949791095801952e-04, 1.052603860332742742e-05],
            [5.122848048895116413e-02, 1.370428080792203797e-04, 8.606237912066744673e-06],
            [5.225305009873018602e-02, 7.682283355042543742e-05, 6.400028456651255277e-06],
            [5.329811110070479391e-02, 3.365427069301658587e-05, 4.912848937775578688e-06],
            [5.436407332271889215e-02, 3.702561168890299226e-05, 5.139163315042721672e-06],
            [5.545135478917326971e-02, 6.653766511128365196e-05, 6.096189648159447920e-06],
            [5.656038188495673275e-02, 8.553105690284036828e-05, 6.490331793321423549e-06],
            [5.769158952265586449e-02, 9.324165632599107031e-05, 6.882046552727619744e-06],
            [5.884542131310897817e-02, 9.063408425309007316e-05, 6.632821426642359985e-06],
            [6.002232973937116078e-02, 7.208993739551022059e-05, 6.208362633900933051e-06],
            [6.122277633415858927e-02, 4.674787034832309364e-05, 4.873118172004430237e-06],
            [6.244723186084175787e-02, 1.545170235633725240e-05, 4.172221129761643066e-06],
            [6.369617649805858761e-02, 2.050196979267359794e-05, 4.317092534781950271e-06],
            [6.497010002801975603e-02, 2.607598663558337339e-05, 4.493498613858755242e-06],
            [6.626950202858014616e-02, 3.479436672475715524e-05, 4.700986860260099838e-06],
            [6.759489206915175741e-02, 4.647801938354077046e-05, 5.025183185493301046e-06],
            [6.894678991053479478e-02, 2.774194436440772257e-05, 4.498419856741113195e-06],
            [7.032572570874548457e-02, 1.891415342768797269e-05, 3.986578825505409981e-06],
            [7.173224022292037927e-02, 1.163997214270704413e-05, 3.387820085791146456e-06],
            [7.316688502737879463e-02, 8.933234513645791069e-06, 3.067642782555342797e-06],
            [7.463022272792636802e-02, 0.000000000000000000e+00, 1.000000000000000000e+00],
            [7.612282718248489233e-02, 1.786939410058457190e-05, 3.490211338769381519e-06],
            [7.764528372613457852e-02, 2.214246788845121504e-05, 3.589778906557234445e-06],
            [7.919818940065727342e-02, 1.498218151421818294e-05, 3.225317009590560723e-06],
            [8.078215318867040806e-02, 8.425515229197527370e-06, 3.038014187333074397e-06],
            [8.239779625244381123e-02, 1.584949445471820436e-05, 3.090720126591387245e-06],
            [8.404575217749268856e-02, 9.753121714888718453e-06, 3.101691453433978713e-06],
            [8.572666722104255177e-02, 1.127041181884739210e-05, 3.032178459915847022e-06],
            [8.744120056546339503e-02, 5.004766905992740022e-06, 2.930305831520003059e-06],
            [8.919002457677266404e-02, 6.342493813062531518e-06, 2.818737961763079874e-06],
            [9.097382506830811733e-02, 1.020645788792139804e-05, 2.795352567065606455e-06],
            [9.279330156967428855e-02, 7.130242081005994041e-06, 2.951115229998396123e-06],
            [9.464916760106778515e-02, 0.000000000000000000e+00, 1.000000000000000000e+00],
            [9.654215095308912864e-02, 0.000000000000000000e+00, 1.000000000000000000e+00],
            [9.847299397215092398e-02, 0.000000000000000000e+00, 1.000000000000000000e+00]
        ])

        try:
            np.testing.assert_allclose(test_vec_x, gold_data[:, 0])
            np.testing.assert_allclose(test_vec_y, gold_data[:, 1])
            np.testing.assert_allclose(test_vec_e, gold_data[:, 2])
            success = True
        except AssertionError as assert_err:
            print(f'{assert_err}')
            success = False

        return success
