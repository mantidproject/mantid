import systemtesting
from mantid.simpleapi import PowderReduceP2D


class PowderReduceP2DTest(sytemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6
        setUp()

    def setUp(self):
        self._sample = _sampleEventData()
        self._vana = _vanadiumEvent_Data()
        self._empty = _emptyEventData()
        self._calFile = _calFile()
        self._twoThetaMin = _twoThetaMin()
        self._twoThetaMax = _twoThetaMax()
        self._wavelengthCenter = _wavelengthCenter()
        self._lambdaMin = _lambdaMin()
        self._lambdaMax = _lambdaMax()
        self._dMin = _dMin()
        self._dMax = _dMax()
        self._dpMin = _dPMin()
        self._dpMax = _dpMax()
        self._dSpaceBinning = _dSpaceBinning()
        self._dPerpendiclarBinning = _dPerpendicularBinning()
        self._FWHM = _FWHM()
        self._tolerance = _tolerance()

        self._reference = _loadReference()
        self._outputFile = _outputFile()

    def runTest(self):
        powder_reduce_P2D = PowderReduceP2D(SampleData=self._sample, OutputFile=self._outputFile, DoIntensityCorrection = True,
                                            VanaData = self._vana, DoBackgroundCorrection = True, EmptyData = self._empty,
                                            DoEdgebinning = False, CalFile = self._calFile,
                                            TwoThetaMin = self._twoThetaMin, TwoThetaMax = self._twoThetaMax,
                                            WavelengthCenter = self._wavelengthCenter, LambdaMin = self._lambdaMin,
                                            LambdaMax = self._lambdaMax, DMin = self._dMin, DMax = self._dMax, DpMin = self._dpMin,
                                            DpMax = self._dpMax, dSpaceBinning = self._dSpaceBinning,
                                            dPerpendicularBinning = self._dPerpendicularBining, FWHM = self._FWHM,
                                            Tolerance = self._tolerance)
        powder_reduce_P2D.powder_reduce_P2D()

    def validateMethod(self):
        return 'ValidateAscii'

    def validate(self):
        return 'PowderReduceP2D_Test.p2d', 'PowderReduceP2D_reference.p2d'

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
        return "Path/to/reference/file.p2d"

    def _outputFile(self):
        return 'PowderReduceP2D_Test'


if __name__ == '__main__':
    unittest.main()
