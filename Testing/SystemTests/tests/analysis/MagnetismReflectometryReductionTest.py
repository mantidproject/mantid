#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid import *
from mantid.simpleapi import *


class MagnetismReflectometryReductionTest(stresstesting.MantidStressTest):
    def runTest(self):
        MagnetismReflectometryReduction(RunNumbers=['24949',],
                                        NormalizationRunNumber=24945,
                                        SignalPeakPixelRange=[125, 129],
                                        SubtractSignalBackground=True,
                                        SignalBackgroundPixelRange=[15, 105],
                                        ApplyNormalization=True,
                                        NormPeakPixelRange=[201, 205],
                                        SubtractNormBackground=True,
                                        NormBackgroundPixelRange=[10,127],
                                        CutLowResDataAxis=True,
                                        LowResDataAxisPixelRange=[91, 161],
                                        CutLowResNormAxis=True,
                                        LowResNormAxisPixelRange=[86, 174],
                                        CutTimeAxis=True,
                                        UseWLTimeAxis=False,
                                        QMin=0.005,
                                        QStep=-0.01,
                                        TimeAxisStep=40,
                                        TimeAxisRange=[25000, 54000],
                                        SpecularPixel=126.9,
                                        ConstantQBinning=False,
                                        EntryName='entry-Off_Off',
                                        OutputWorkspace="r_24949")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "r_24949", 'MagnetismReflectometryReductionTest.nxs'
