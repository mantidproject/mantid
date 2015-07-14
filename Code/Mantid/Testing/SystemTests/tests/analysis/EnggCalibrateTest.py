#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

class EnginXFocusWithVanadiumCorrection(stresstesting.MantidStressTest):

    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self.difc = -1
        self.zero = -1
        self.difc_b2 = -1
        self.zero_b2 = -1

    def runTest(self):
        pass

    def validate(self):
        pass

    def cleanup(self):
        pass

class EnginXCalibrateFullThenCalibrateTest(stresstesting.MantidStressTest):

    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        # difc and zero parameters for GSAS
        self.difc = -1
        self.difc_b2 = -1
        self.zero_b2 = -1
        self.zero = -1
        # table workspace with detector positions
        self.posTable = None

    def runTest(self):
        # These lines run a 'CalibrateFull' and calibrate for the instrument EnginX

        # This must be the long Ceria (CeO2) run for calibrate-full
        long_calib_ws = Load(Filename = 'ENGINX00193749.nxs')

        # This must be the (big) Vanadium (V-Nb) run for vanadium corrections
        van_ws = Load(Filename = 'ENGINX00236516.nxs')

        positions = EnggCalibrateFull(Workspace = long_calib_ws,
                                      VanadiumWorkspace = van_ws,
                                      Bank = '1',
                                      ExpectedPeaks = '1.3529, 1.6316, 1.9132')
        self.posTable = positions

        # Bank 1
        (self.difc, self.zero) = EnggCalibrate(InputWorkspace = long_calib_ws,
                                               VanadiumWorkspace = van_ws,
                                               Bank = '1',
                                               ExpectedPeaks = '2.7057,1.9132,1.6316,1.5621,1.3528,0.9566',
                                               DetectorPositions = self.posTable)

        # Bank 2
        (self.difc_b2, self.zero_b2) = EnggCalibrate(InputWorkspace = long_calib_ws,
                                                     VanadiumWorkspace = van_ws,
                                                     Bank = '2',
                                                     ExpectedPeaks = '2.7057,1.9132,1.6316,1.5621,1.3528,0.9566',
                                                     DetectorPositions = self.posTable)

    def _relErrLessDelta(self, val, ref, epsilon):
        """
        Checks that a value 'val' does not defer from a reference value 'ref' by 'epsilon'
        or more. This method compares the relative error. An epsilon of 0.1 means a relative
        difference of 10 % = 100*0.1 %

        @param val :: value obtained from a calculation or algorithm
        @param ref :: (expected) reference value
        @param epsilon :: acceptable relative error (error tolerance)

        @returns if val differs in relative terms from ref by less than epsilon
        """
        if 0 == ref:
            return False
        return (abs((ref-val)/ref) < epsilon)

    def validate(self):
        # === check detector positions table produced by EnggCalibrateFull
        self.assertTrue(self.posTable)
        self.assertEquals(self.posTable.columnCount(), 9)
        self.assertEquals(self.posTable.rowCount(), 1200)
        self.assertEquals(self.posTable.cell(88, 0), 100089)   # det ID
        self.assertEquals(self.posTable.cell(200, 0), 101081)  # det ID

        # this will be used as a comparison delta of 'delta' %
        exdelta = 1e-5
        # Note that the reference values are given with 12 digits more for reference than
        # for assert-comparison purposes (comparisons are not that picky, by far)
        self.assertTrue(self._relErrLessDelta(self.posTable.cell(100, 3), 1.49010562897, exdelta))
        #self.assertDelta(self.posTable.cell(100, 3), 1.49010562897, delta)
        self.assertTrue(self._relErrLessDelta(self.posTable.cell(400, 4), 1.65264105797, exdelta))
        self.assertTrue(self._relErrLessDelta(self.posTable.cell(200, 5), 0.296705961227, exdelta))
        self.assertTrue(self._relErrLessDelta(self.posTable.cell(610, 7), 18585.1738281, exdelta))
        self.assertTrue(self._relErrLessDelta(self.posTable.cell(1199, 8), -1.56501817703, exdelta))

        # === check difc, zero parameters for GSAS produced by EnggCalibrate
        # Mac fitting tests produce differences for some reason.
        import sys
        if sys.platform == "darwin":
            delta_darwin = 5e-3
            exdelta = delta_darwin

        # Bank 1
        self.assertTrue(self._relErrLessDelta(self.difc, 18405.0526862, exdelta))
        self.assertTrue(self._relErrLessDelta(self.zero, -0.835863958543, exdelta))

        # Bank 2
        self.assertTrue(self._relErrLessDelta(self.difc_b2, 18391.1104257, exdelta))
        self.assertTrue(self._relErrLessDelta(self.zero_b2, -8.66653176951, exdelta))

    def cleanup(self):
        mtd.remove('positions')
