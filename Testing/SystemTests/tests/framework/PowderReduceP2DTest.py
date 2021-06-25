import systemtesting
from mantid.simpleapi import PowderReduceP2D


class PowderReduceP2DTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6
        self.setUp()

    def setUp(self):
        self.sample = self._sampleEventData()
        self.vana = self._vanadiumEventData()
        self.empty = self._emptyEventData()
        self.calFile = self._calFile()
        self.twoThetaMin = self._twoThetaMin()
        self.twoThetaMax = self._twoThetaMax()
        self.wavelengthCenter = self._wavelengthCenter()
        self.lambdaMin = self._lambdaMin()
        self.lambdaMax = self._lambdaMax()
        self.dMin = self._dMin()
        self.dMax = self._dMax()
        self.dpMin = self._dPMin()
        self.dpMax = self._dpMax()
        self.dSpaceBinning = self._dSpaceBinning()
        self.dPerpendiclarBinning = self._dPerpendicularBinning()
        self.FWHM = self._FWHM()
        self.tolerance = self._tolerance()

        self.reference = _loadReference()
        self.outputFile = _outputFile()

    def runTest(self):
        powder_reduce_P2D = PowderReduceP2D(SampleData=self.sample, OutputFile=self.outputFile, DoIntensityCorrection = True,
                                            VanaData = self.vana, DoBackgroundCorrection = True, EmptyData = self.empty,
                                            DoEdgebinning = False, CalFile = self.calFile,
                                            TwoThetaMin = self.twoThetaMin, TwoThetaMax = self.twoThetaMax,
                                            WavelengthCenter = self.wavelengthCenter, LambdaMin = self.lambdaMin,
                                            LambdaMax = self.lambdaMax, DMin = self.dMin, DMax = self.dMax, DpMin = self.dpMin,
                                            DpMax = self.dpMax, dSpaceBinning = self.dSpaceBinning,
                                            dPerpendicularBinning = self.dPerpendicularBining, FWHM = self.FWHM,
                                            Tolerance = self.tolerance)
        powder_reduce_P2D.powder_reduce_P2D()

    def validateMethod(self):
        return 'ValidateAscii'

    def validate(self):
        return self.outputFile + '.p2d', self.reference

    def _sampleEventData(self):
        """path to sample event data used for testing the algorithm"""
        return "PG3_4844_event.nxs"

    def _vanadiumEventData(self):
        """path to vanadium event data used for testing the algorithm"""
        return "PG3_4866_event.nxs"

    def _emptyEventData(self):
        """path to empty event data used for testing the algorithm"""
        return "PG3_5226_event.nxs"

    def _calFile(self):
        """path to calibration File used for testing the algorithm"""
        return 'PG3_FERNS_d4832_2011_08_24.cal'

    def _twoThetaMin(self):
        """2theta min used for testing the algorithm"""
        return 10

    def _twoThetaMax(self):
        """2theta max used for testing the algorithm"""
        return 150

    def _wavelengthCenter(self):
        """center wavelength used for testing the algorithm"""
        return 0.566

    def _lambdaMin(self):
        """lambda min used for testing the algorithm"""
        return 0.1

    def _lambdaMax(self):
        """lambda max used for testing the algorithm"""
        return 2

    def _dMin(self):
        """d min used for testing the algorithm"""
        return 0.1

    def _dMax(self):
        """d max used for testing the algorithm"""
        return 2

    def _dpMin(self):
        """dp min used for testing the algorithm"""
        return 0.4

    def _dpMax(self):
        """dp max used for testing the algorithm"""
        return 2

    def _dSpaceBinning(self):
        """d binning used for testing the algorithm"""
        return -0.02

    def _dPerpendicularBinning(self):
        """d perpendicular binning used for testing the algorithm"""
        return 0.1

    def _FWHM(self):
        """FWHM used for testing the algorithm"""
        return 7

    def _tolerance(self):
        """tolerance used for testing the algorithm"""
        return 4

    def _loadReference(self):
        return 'PowderReduceP2D_reference.p2d'

    def _outputFile(self):
        return 'PowderReduceP2D_Test'


if __name__ == '__main__':
    unittest.main()
