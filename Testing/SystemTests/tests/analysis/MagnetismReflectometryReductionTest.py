#pylint: disable=no-init,attribute-defined-outside-init
import stresstesting
from mantid import *
from mantid.simpleapi import *
import math


class MagnetismReflectometryReductionTest(stresstesting.MantidStressTest):
    def runTest(self):
        wsg = MRFilterCrossSections(Filename="REF_M_24949")
        MagnetismReflectometryReduction(InputWorkspace=wsg[0],
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
                                        OutputWorkspace="r_24949")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "r_24949", 'MagnetismReflectometryReductionTest.nxs'


class MRFilterCrossSectionsTest(stresstesting.MantidStressTest):
    """ Test data loading and cross-section extraction """
    def runTest(self):
        wsg = MRFilterCrossSections(Filename="REF_M_24949")
        MagnetismReflectometryReduction(InputWorkspace=str(wsg[0]),
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
                                        OutputWorkspace="r_24949")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "r_24949", 'MagnetismReflectometryReductionTest.nxs'


class MRFilterCrossSectionsWithWorkspaceTest(stresstesting.MantidStressTest):
    """ Test data loading and cross-section extraction """
    def runTest(self):
        ws_input = LoadEventNexus(Filename="REF_M_24949",
                                  NXentryName="entry-Off_Off",
                                  OutputWorkspace="r_24949")
        # Since we are using a older data file for testing, add the
        # polarizer/analyzer info. This will also test the edge case where
        # there is no analyzer or polarizer, which should just be the
        # same as a simple load.
        AddSampleLog(Workspace=ws_input, LogName='polarizer',
                     LogText="0",
                     LogType='Number Series', LogUnit='')
        AddSampleLog(Workspace=ws_input, LogName='analyzer',
                     LogText="0",
                     LogType='Number Series', LogUnit='')
        wsg = MRFilterCrossSections(InputWorkspace=ws_input)
        MagnetismReflectometryReduction(InputWorkspace=wsg[0],
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
                                        OutputWorkspace="r_24949")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "r_24949", 'MagnetismReflectometryReductionTest.nxs'


class MRNormaWorkspaceTest(stresstesting.MantidStressTest):
    """ Test data loading and cross-section extraction """
    def runTest(self):
        wsg = MRFilterCrossSections(Filename="REF_M_24949")
        ws_norm = LoadEventNexus(Filename="REF_M_24945",
                                 NXentryName="entry-Off_Off",
                                 OutputWorkspace="r_24945")
        MagnetismReflectometryReduction(InputWorkspace=wsg[0],
                                        NormalizationWorkspace=ws_norm,
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
                                        OutputWorkspace="r_24949")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "r_24949", 'MagnetismReflectometryReductionTest.nxs'


class MROutputTest(stresstesting.MantidStressTest):
    """ Test the MR output algorithm """
    def runTest(self):
        wsg = MRFilterCrossSections(Filename="REF_M_24949")
        ws_norm = LoadEventNexus(Filename="REF_M_24945",
                                 NXentryName="entry-Off_Off",
                                 OutputWorkspace="r_24945")
        MagnetismReflectometryReduction(InputWorkspace=wsg[0],
                                        NormalizationWorkspace=ws_norm,
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
                                        OutputWorkspace="r_24949")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "r_24949", 'MagnetismReflectometryReductionTest.nxs'


class MRInspectionTest(stresstesting.MantidStressTest):
    def runTest(self):
        nxs_data = LoadEventNexus(Filename="REF_M_24949",
                                  NXentryName="entry-Off_Off",
                                  OutputWorkspace="r_24949")
        MRInspectData(Workspace=nxs_data)

    def validate(self):
        # Simple test to verify that we flagged the data correctly
        return mtd["r_24949"].getRun().getProperty("is_direct_beam").value == "False"


class MRGetThetaTest(stresstesting.MantidStressTest):
    """ Test that the MRGetTheta algorithm produces correct results """
    def runTest(self):
        nxs_data = LoadEventNexus(Filename="REF_M_24949",
                                  NXentryName="entry-Off_Off",
                                  OutputWorkspace="r_24949")
        self.assertAlmostEqual(MRGetTheta(Workspace=nxs_data, UseSANGLE=True), 0.606127/180.0*math.pi)
        self.assertAlmostEqual(MRGetTheta(Workspace=nxs_data, UseSANGLE=True, AngleOffset=math.pi), 180.606127/180.0*math.pi)
        self.assertAlmostEqual(MRGetTheta(Workspace=nxs_data, SpecularPixel=126.1), 0.61249193272/180.0*math.pi)
        # In the present case, DANGLE = DANGLE0, so we expect 0 if nothing else is passed
        self.assertAlmostEqual(MRGetTheta(Workspace=nxs_data), 0.0)

    def validate(self):
        return True
