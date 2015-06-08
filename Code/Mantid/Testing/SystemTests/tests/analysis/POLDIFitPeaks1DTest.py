#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *
import numpy as np

class POLDIFitPeaks1DTest(stresstesting.MantidStressTest):
    '''Checking results of PoldiFitPeaks1D.'''

    # The errors of fitted parameters in version 2 are a bit small
    # because of the "fabricated data", so a larger margin has to be allowed.
    versionDeltas = {1: 2.0e-4, 2: 1.5e-3}
    errorMultiplier = {1: 1.0, 2: 4.0}

    def runTest(self):
        dataFiles = ["poldi2013n006904", "poldi_2_phases_theoretical"]
        versions = [1, 2]
        deleteList = [[], ['12-16', '10', '9']]

        self.loadReferenceCorrelationData(dataFiles)
        self.loadReferenceFitResults(dataFiles)
        self.runPeakSearch(dataFiles, deleteList)
        self.runPoldiFitPeaks1D(dataFiles, versions)
        self.analyseResults(dataFiles, versions)

    def loadReferenceCorrelationData(self, filenames):
        for dataFile in filenames:
            Load(Filename="%s_reference.nxs" % (dataFile), OutputWorkspace=dataFile)

    def runPeakSearch(self, filenames, deleteList):
        for dataFile,deleteRowList in zip(filenames, deleteList):
            PoldiPeakSearch(InputWorkspace=dataFile,
                      MinimumPeakSeparation=8,
                      OutputWorkspace="%s_Peaks" % (dataFile))

            for deleteRows in deleteRowList:
                DeleteTableRows(TableWorkspace="%s_Peaks" % (dataFile), Rows=deleteRows)

    def loadReferenceFitResults(self, filenames):
        for dataFile in filenames:
            Load(Filename="%s_reference_1DFit.nxs" % (dataFile), OutputWorkspace="%s_reference_1DFit" % (dataFile))

    def runPoldiFitPeaks1D(self, filenames, versions):
        for dataFile, version in zip(filenames, versions):
            args = {"InputWorkspace": dataFile,
                          "FwhmMultiples": 4,
                          "PoldiPeakTable": "%s_Peaks" % (dataFile),
                          "OutputWorkspace": "%s_Peaks_Refined" % (dataFile),
                          "FitPlotsWorkspace": "%s_FitPlots" % (dataFile),
                          "Version": version}

            if version == 2:
                args["AllowedOverlap"] = 0.1

            PoldiFitPeaks1D(**args)

  # This test makes sure that:
  #  - standard deviations of position and relative fwhm are acceptably small (indicates reasonable fit)
  #  - refined peak positions are within one standard deviation of reference results obtained from existing program
  #  - fwhms do not deviate too much from reference results
  #  - currently, only the first 10 peaks are compared (as in the peak search test)
    def analyseResults(self, filenames, versions):
        for dataFile, version in zip(filenames, versions):
            calculatedPeaks = mtd["%s_Peaks_Refined" % (dataFile)]
            referencePeaks = mtd["%s_reference_1DFit" % (dataFile)]
            self.assertEqual(calculatedPeaks.rowCount(), referencePeaks.rowCount())

            positions = calculatedPeaks.column(2)
            referencePositions = [float(x) for x in referencePeaks.column(0)]

            fwhms = calculatedPeaks.column(4)
            referenceFwhms = [float(x) for x in referencePeaks.column(1)]

            for i in range(10):
          # extract position and fwhm with uncertainties
                positionparts = positions[i].split()
                position = [float(positionparts[0]), float(positionparts[2])]

                fwhmparts = fwhms[i].split()
                fwhm = [float(fwhmparts[0]), float(fwhmparts[2])]

                self.assertTrue(self.positionAcceptable(position))
                self.assertTrue(self.fwhmAcceptable(fwhm))

          # find closest reference peak
                deltas = np.array([np.abs(position[0] - x) for x in referencePositions])


                self.assertDelta(deltas.min(), 0.0, self.versionDeltas[version])
                minIndex = deltas.argmin()

                self.assertTrue(self.uncertainValueEqualsReference(position, referencePositions[minIndex], self.errorMultiplier[version]))
                self.assertDelta(fwhm[0], referenceFwhms[minIndex], self.versionDeltas[version])

    def positionAcceptable(self, position):
        return position[1] < 1e-3

    def fwhmAcceptable(self, fwhm):
        return fwhm[1] < 3e-3

    def uncertainValueEqualsReference(self, value, reference, sigmas):
        return np.abs(value[0] - reference) < (sigmas * value[1])
