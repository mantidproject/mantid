from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
import os
import math
import sys

class PoldiSample:
  def __init__(self, initString):
    self._name, self._runList = self.parseInitString(initString)

  def name(self):
    return self._name

  def runList(self):
    return self._runList

  def runNames(self):
    return [x[0] for x in self._runList]

  def parseInitString(self, initString):
    parts = initString.split(':')

    if not len(parts) == 2:
      raise RuntimeError("Unable to parse init string.")

    sampleName = parts[0]
    runDict = self.parseRunDictString(parts[1])

    return (sampleName, runDict)

  def parseRunDictString(self, runDictString):
    runRanges = filter(bool, runDictString.strip().split(';'))

    newRunList = []
    for runRange in runRanges:
      runPoints = self.parseRunRangeString(runRange)
      newRunList += runPoints

    return newRunList

  def parseRunRangeString(self, runRangeString):
    runRangeParameters = [int(x) for x in runRangeString.split(',')]

    if not len(runRangeParameters) == 3:
      raise RuntimeError("Unable to parse run range string.")

    begin, end, step = runRangeParameters

    runList = []
    for i in range(begin, end + 1, step):
      runList.append((i + step - 1, [str(x) for x in range(i, i + step)]))

    print runList

    return runList

class PoldiProject:
  def __init__(self, projectName, sampleListFile):
    self._name = projectName
    self._sampleList = self.parseSampleListFile(sampleListFile)

  def sampleList(self):
    return self._sampleList

  def name(self):
    return self._name

  def parseSampleListFile(self, sampleListFile):
    fh = open(sampleListFile, 'r')

    sampleList = []
    for line in fh:
      if len(line.strip()) > 0 and not line.strip()[0] == '#':
        sampleList.append(PoldiSample(line))

    return sampleList

class PoldiBatchFitIndividualPeaks(PythonAlgorithm):
  prefixes = {'raw': 'raw_',
              'data': 'data_'}

  suffixes = {'correlation': '_correlation',
              'raw_peaks': '_peaks_raw',
              'fit_peaks_1d': '_peaks_fit_1d',
              'fit_peaks_2d': '_peaks_fit_2d',
              'calculated_2d': '_spectrum_fitted_2d',
              'residuals': '_residuals'}

  def category(self):
    return "Workflow\\Poldi"

  def name(self):
    return "PoldiBatchFitIndividualPeaks"

  def summary(self):
    return "Load a file with instructions for batch processing POLDI fits."

  def PyInit(self):
    self.declareProperty("ProjectName", "", "Project name to use for labeling")
    self.declareProperty(FileProperty(name="SampleListFile", defaultValue="",
                                      action=FileAction.Load))
    self.declareProperty(FileProperty(name="OutputDirectory", defaultValue="",
                                      action=FileAction.OptionalDirectory))
    self.declareProperty(FileProperty(name="CrystalStructures",
                                      defaultValue="",
                                      action=FileAction.OptionalLoad))
    self.declareProperty("PeakNumber", 10, "Number of peaks to fit.")

  def PyExec(self):
    poldiProject = PoldiProject(self.getProperty("ProjectName"),
                                self.getProperty("SampleListFile").valueAsStr)

    sampleCount = len(poldiProject.sampleList())
    progress = Progress(self, start=0.0, end=1.0, nreports=sampleCount)

    for i, sample in enumerate(poldiProject.sampleList()):
      self.processSample(sample)
      progress.report("Processed sample " + str(i + 1) + " of " + str(sampleCount))

  def processSample(self, poldiSample):
    sampleList = poldiSample.runList()

    for dataPoint in sampleList:
      self.processDataPoint(dataPoint)

  def processDataPoint(self, dataPointTuple):
    self.loadDataPoint(dataPointTuple)
    self.runPoldiAnalysis(dataPointTuple[0])

  def loadDataPoint(self, dataPointTuple):
    dataWorkspaceName = self.prefixes['data'] + str(dataPointTuple[0])

    rawWorkspaceNames = [
      self.prefixes['raw'] + str(x) for x in dataPointTuple[1]]

    for numor,rawWsName in zip(dataPointTuple[1], rawWorkspaceNames):
      LoadSINQ(Instrument="POLDI", Year=2014,
               Numor=numor, OutputWorkspace=rawWsName)
      LoadInstrument(rawWsName, InstrumentName="POLDI")

    if len(dataPointTuple[1]) > 1:
      PoldiMerge(rawWorkspaceNames, OutputWorkspace=dataWorkspaceName)

      for ws in rawWorkspaceNames:
        DeleteWorkspace(ws)

    else:
      RenameWorkspace(rawWorkspaceNames[0], OutputWorkspace=dataWorkspaceName)

    return dataWorkspaceName

  def runPoldiAnalysis(self, dataPointName):
    dataWorkspaceName = self.prefixes['data'] + str(dataPointName)

    correlationWsName = dataWorkspaceName + self.suffixes['correlation']
    PoldiAutoCorrelation(dataWorkspaceName, wlenmin=1.1, wlenmax=5.0,
                         OutputWorkspace=correlationWsName)

    peakSearchWsName = dataWorkspaceName + self.suffixes['raw_peaks']
    PoldiPeakSearch(correlationWsName, OutputWorkspace=peakSearchWsName)

    DeleteTableRows(peakSearchWsName,
                    self.getProperty("PeakNumber").valueAsStr + '-30')

    peakFit1DWsName = dataWorkspaceName + self.suffixes['fit_peaks_1d']
    PoldiFitPeaks1D(correlationWsName, PoldiPeakTable=peakSearchWsName,
                    OutputWorkspace=peakFit1DWsName)

    peakFit2DWsName = dataWorkspaceName + self.suffixes['fit_peaks_2d']
    spectrum2DCalc = dataWorkspaceName + self.suffixes['calculated_2d']
    PoldiFitPeaks2D(dataWorkspaceName, PoldiPeakWorkspace=peakFit1DWsName,
                    MaximumIterations=100, OutputWorkspace=spectrum2DCalc,
                    RefinedPoldiPeakWorkspace=peakFit2DWsName)

    residualsWsName = dataWorkspaceName + self.suffixes['residuals']
    PoldiAnalyseResiduals(dataWorkspaceName, spectrum2DCalc,
                          OutputWorkspace=residualsWsName,
                          MaxIterations=5)

AlgorithmFactory.subscribe(PoldiBatchFitIndividualPeaks)