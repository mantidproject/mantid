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


class MagnetismReflectometryReductionConstQTest(stresstesting.MantidStressTest):
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
                                        ConstantQBinning=True,
                                        OutputWorkspace="r_24949")

    def validate(self):
        refl = mtd["r_24949"].dataY(0)
        return math.fabs(refl[1] - 0.648596877775159) < 0.002


class MagnetismReflectometryReductionConstQWLCutTest(stresstesting.MantidStressTest):
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
                                        UseWLTimeAxis=True,
                                        QMin=0.005,
                                        QStep=-0.01,
                                        TimeAxisStep=0.007,
                                        TimeAxisRange=[4.5, 10.5],
                                        SpecularPixel=126.9,
                                        ConstantQBinning=True,
                                        OutputWorkspace="r_24949")

    def validate(self):
        refl = mtd["r_24949"].dataY(0)
        return math.fabs(refl[1] - 0.648596877775159) < 0.002


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


class MRDIRPIXTest(stresstesting.MantidStressTest):
    """ Test data loading and cross-section extraction """
    def runTest(self):
        wsg = MRFilterCrossSections(Filename="REF_M_24949")
        ws_norm = LoadEventNexus(Filename="REF_M_24945",
                                 NXentryName="entry-Off_Off",
                                 OutputWorkspace="r_24945")
        #sc_angle = MRGetTheta(Workspace=wsg[0])
        # The logs have DANGLE0 = 4.50514 and DIRPIX = 204
        # Scatt angle = 0

        # 131.9: 0.00989410349765
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
                                        SpecularPixel=136.9,
                                        UseSANGLE=False,
                                        DirectPixelOverwrite=214,
                                        ConstantQBinning=False,
                                        OutputWorkspace="r_24949")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        return "r_24949", 'MagnetismReflectometryReductionTest.nxs'


class MRDANGLE0Test(stresstesting.MantidStressTest):
    """ Test data loading and cross-section extraction """
    def runTest(self):
        wsg = MRFilterCrossSections(Filename="REF_M_24949")
        ws_norm = LoadEventNexus(Filename="REF_M_24945",
                                 NXentryName="entry-Off_Off",
                                 OutputWorkspace="r_24945")
        theta = MRGetTheta(Workspace=wsg[0], UseSANGLE=False, SpecularPixel=127.9)
        theta0 = MRGetTheta(Workspace=wsg[0], UseSANGLE=False, SpecularPixel=126.9)
        dangle0 = wsg[0].getRun()['DANGLE0'].getStatistics().mean
        dangle0 += (theta-theta0)*2.0*180./math.pi

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
                                        SpecularPixel=127.9,
                                        UseSANGLE=False,
                                        DAngle0Overwrite=dangle0,
                                        ConstantQBinning=False,
                                        OutputWorkspace="r_24949")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
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


class MRInspectionOverwritesTest(stresstesting.MantidStressTest):
    def runTest(self):
        nxs_data = LoadEventNexus(Filename="REF_M_24949",
                                  NXentryName="entry-Off_Off",
                                  OutputWorkspace="r_24949")
        MRInspectData(Workspace=nxs_data, DirectPixelOverwrite=208.0, DAngle0Overwrite=5.0)

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

        # The logs have DANGLE0 = 4.50514 and DIRPIX = 204
        # Setting DIRPIX without setting a specular pixel shouldn't change anything
        self.assertAlmostEqual(MRGetTheta(Workspace=nxs_data, DirectPixelOverwrite=145), 0.0)

        # Setting DIRPIX and the specular pixel with move things
        # Move everything by 4 pixels and we should get the same answer (which depends only on the difference of the two)
        self.assertAlmostEqual(MRGetTheta(Workspace=nxs_data, DirectPixelOverwrite=208, SpecularPixel=130.1), 0.61249193272/180.0*math.pi)

        dangle0 = nxs_data.getRun()['DANGLE0'].value[0]
        self.assertAlmostEqual(MRGetTheta(Workspace=nxs_data, DAngle0Overwrite=dangle0+180.0), math.pi/2.0)

    def validate(self):
        return True
