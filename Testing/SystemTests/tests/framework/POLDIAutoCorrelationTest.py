# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import CreateWorkspace, Fit, Load, LoadInstrument, LoadSINQFile, PoldiAutoCorrelation, PoldiTruncateData
import numpy as np


class POLDIAutoCorrelationTest(systemtesting.MantidSystemTest):
    """This test checks that the results of PoldiAutoCorrelation match the expected outcome."""

    def runTest(self):
        dataFiles = ["poldi2013n006903", "poldi2013n006904", "poldi2014n019874", "poldi2014n019881"]

        self.loadReferenceData(dataFiles)
        self.runAutoCorrelation(dataFiles)
        self.analyseResults(dataFiles)

    def loadReferenceData(self, filenames):
        for dataFile in filenames:
            Load(Filename="%s_reference.nxs" % (dataFile), OutputWorkspace="%s_reference" % (dataFile))

    def runAutoCorrelation(self, filenames):
        for dataFile in filenames:
            LoadSINQFile(Instrument="POLDI", Filename=dataFile + ".hdf", OutputWorkspace=dataFile)
            LoadInstrument(Workspace=dataFile, InstrumentName="POLDI", RewriteSpectraMap=True)
            PoldiTruncateData(InputWorkspace=dataFile, OutputWorkspace=dataFile)
            PoldiAutoCorrelation(InputWorkspace=dataFile, wlenmin=1.1, wlenmax=5.0, OutputWorkspace=dataFile + "Corr")

    def analyseResults(self, filenames):
        for dataFile in filenames:
            workspaceNameTemplate = "Comparison_%s" % (dataFile)

            referenceData = mtd["%s_reference" % (dataFile)].dataY(0)
            calculatedData = mtd["%sCorr" % (dataFile)].dataY(0)

            self.assertEqual(
                calculatedData.shape[0],
                referenceData.shape[0],
                "Number of d-values does not match for %s (is: %i, should: %i)"
                % (dataFile, calculatedData.shape[0], referenceData.shape[0]),
            )

            CreateWorkspace(referenceData, calculatedData, OutputWorkspace=workspaceNameTemplate)

            fitNameTemplate = "Fit_%s" % (dataFile)
            Fit(
                "name=LinearBackground",
                mtd[workspaceNameTemplate],
                StartX=np.min(referenceData),
                EndX=np.max(referenceData),
                Output=fitNameTemplate,
            )

            fitResult = mtd[fitNameTemplate + "_Parameters"]

            slope = fitResult.cell(1, 1)
            self.assertDelta(slope, 1.0, 1e-4, "Slope is larger than 1.0 for %s (is: %d)" % (dataFile, slope))

            relativeSlopeError = fitResult.cell(1, 2) / slope
            self.assertLessThan(
                relativeSlopeError, 5e-4, "Relative error of slope is too large for %s (is: %d)" % (dataFile, relativeSlopeError)
            )

            intercept = fitResult.cell(0, 1)
            self.assertDelta(intercept, 0.0, 1.0, "Intercept deviates too far from 0 %s (is: %d)" % (dataFile, intercept))

            relativeInterceptError = fitResult.cell(0, 2) / intercept
            self.assertLessThan(
                relativeInterceptError,
                1,
                "Relative error of intercept is too large for %s (is: %d)" % (dataFile, relativeInterceptError),
            )

            residuals = mtd[fitNameTemplate + "_Workspace"].dataY(2)
            maxAbsoluteResidual = np.max(np.abs(residuals))
            self.assertLessThan(
                maxAbsoluteResidual, 1.0, "Maximum absolute residual is too large for %s (is: %d)" % (dataFile, maxAbsoluteResidual)
            )
