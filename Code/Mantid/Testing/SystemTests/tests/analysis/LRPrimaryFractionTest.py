#pylint: disable=no-init
import stresstesting
from mantid import *
from mantid.simpleapi import *

class LRPrimaryFractionTest(stresstesting.MantidStressTest):
    def runTest(self):
        workspace = LoadEventNexus(Filename="REF_L_123711")
        self.scaling_factor = LRPrimaryFraction(InputWorkspace=workspace)

    def validate(self):
        ref = [0.887220655191, 0.00257167461136]
        for i in range(2):
            if abs(self.scaling_factor[i]-ref[i])>0.00001:
                logger.error("Output did not match [%s +- %s]: got [%s +- %s]" % (ref[0], ref[1],
                                                                                  self.scaling_factor[0],
                                                                                  self.scaling_factor[1]))
                return False
        return True

class LRPrimaryFractionWithRangeTest(stresstesting.MantidStressTest):
    def runTest(self):
        workspace = LoadEventNexus(Filename="REF_L_119816")
        self.scaling_factor = LRPrimaryFraction(InputWorkspace=workspace,
                                                SignalRange=[120, 190])

    def validate(self):
        ref = [0.970345598555, 0.00524646496021]
        for i in range(2):
            if abs(self.scaling_factor[i]-ref[i])>0.00001:
                logger.error("Output did not match [%s +- %s]: got [%s +- %s]" % (ref[0], ref[1],
                                                                                  self.scaling_factor[0],
                                                                                  self.scaling_factor[1]))
                return False
        return True

class ApplyToReducedDataTest(stresstesting.MantidStressTest):
    def runTest(self):
        #TODO: The reduction algorithm should not require an absolute path
        scaling_factor_file = FileFinder.getFullPath("directBeamDatabaseFall2014_IPTS_11601_2.cfg")
        
        LiquidsReflectometryReduction(RunNumbers=[119816],
                                      NormalizationRunNumber=119692,
                                      SignalPeakPixelRange=[155, 165],
                                      SubtractSignalBackground=True,
                                      SignalBackgroundPixelRange=[146, 165],
                                      NormFlag=True,
                                      NormPeakPixelRange=[154, 162],
                                      NormBackgroundPixelRange=[151, 165],
                                      SubtractNormBackground=True,
                                      LowResDataAxisPixelRangeFlag=True,
                                      LowResDataAxisPixelRange=[99, 158],
                                      LowResNormAxisPixelRangeFlag=True,
                                      LowResNormAxisPixelRange=[118, 137],
                                      TOFRange=[9610, 22425],
                                      IncidentMediumSelected='2InDiamSi',
                                      GeometryCorrectionFlag=False,
                                      QMin=0.005,
                                      QStep=0.01,
                                      AngleOffset=0.009,
                                      AngleOffsetError=0.001,
                                      ScalingFactorFile=scaling_factor_file,
                                      SlitsWidthFlag=True,
                                      CropFirstAndLastPoints=False,
                                      ApplyPrimaryFraction=True,
                                      PrimaryFractionRange=[120,190],
                                      OutputWorkspace='reflectivity_119816')

        ws_fraction = CreateSingleValuedWorkspace(DataValue=0.970345598555,
                                                  ErrorValue=0.00524646496021)
        Divide(LHSWorkspace='reflectivity_119816', RHSWorkspace=ws_fraction,
               OutputWorkspace='reflectivity_119816')

    def validate(self):
        # Because we a re-using the reference data from another test,
        # the errors won't quite be the same. Increase the tolerance value
        # and replace the error on the first and last points by 1.0.
        self.tolerance = 0.00001
        data_e = mtd["reflectivity_119816"].dataE(0)
        data_e[0] = 1.0
        data_e[len(data_e)-1] = 1.0
        
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Sample')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        return "reflectivity_119816", 'LiquidsReflectometryReductionTestWithBackground.nxs'

