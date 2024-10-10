# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import mtd
from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, FileAction, FileProperty, WorkspaceProperty
from mantid.kernel import Direction, EnabledWhenProperty, IntBoundedValidator, Property, PropertyCriterion, StringListValidator
from mantid.simpleapi import FilterByXValue, GetIPTS, LoadEventPreNexus, LoadInstrument, LoadNexusLogs, NormaliseByCurrent
import os


class LoadPreNexusLive(DataProcessorAlgorithm):
    def category(self):
        return "DataHandling"

    def findLivefile(self, instrument):
        livepath = "/SNS/%s/shared/live/" % instrument
        filenames = os.listdir(livepath)

        filenames = [name for name in filenames if name.startswith(instrument)]
        filenames = [name for name in filenames if name.endswith("_live_neutron_event.dat")]

        if len(filenames) <= 0:
            raise RuntimeError("Failed to find live file for '%s'" % instrument)

        filenames.sort()

        runNumber = self.getProperty("RunNumber").value
        if runNumber != Property.EMPTY_INT:
            # convert to substring to look for
            runNumber = "%s_%d_live" % (instrument, runNumber)
            self.log().information("Looking for " + runNumber)

            filenames = [name for name in filenames if runNumber in name]
            if len(filenames) <= 0:
                raise RuntimeError("Failed to find live file '%s'" % runNumber)

        return os.path.join(livepath, filenames[-1])

    def findLogfile(self, instrument, runNumber):
        filename = self.getProperty("LogFilename").value
        if len(filename) > 0 and os.path.exists(filename):
            return filename

        try:
            iptsdir = GetIPTS(Instrument=instrument, RunNumber=runNumber)
            self.log().information("ipts %s" % iptsdir)
        except RuntimeError:
            msg = "Failed to determine the IPTS containing %s_%d" % (instrument, runNumber)
            self.log().warning(msg)
            return ""

        direc = os.path.join(iptsdir, "data")

        filenames = os.listdir(direc)
        filenames = [name for name in filenames if name.endswith("_event.nxs")]

        if len(filenames) <= 0:
            raise RuntimeError("Failed to find existing nexus file in '%s'" % iptsdir)

        filenames.sort()

        return os.path.join(direc, filenames[-1])

    def PyInit(self):
        instruments = ["BSS", "SNAP", "REF_M", "CNCS", "EQSANS", "VULCAN", "VENUS", "MANDI", "TOPAZ", "ARCS"]
        self.declareProperty("Instrument", "", StringListValidator(instruments), "Empty uses default instrument")

        runValidator = IntBoundedValidator()
        runValidator.setLower(1)
        self.declareProperty("RunNumber", Property.EMPTY_INT, runValidator, doc="Live run number to use (Optional, Default=most recent)")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output))

        self.declareProperty("NormalizeByCurrent", True, "Normalize by current")

        self.declareProperty("LoadLogs", True, "Attempt to load logs from an existing file")

        self.declareProperty(
            FileProperty("LogFilename", "", direction=Direction.Input, action=FileAction.OptionalLoad, extensions=["_event.nxs"]),
            doc="File containing logs to use (Optional)",
        )
        self.setPropertySettings("LogFilename", EnabledWhenProperty("LoadLogs", PropertyCriterion.IsDefault))

    def PyExec(self):
        instrument = self.getProperty("Instrument").value

        eventFilename = self.findLivefile(instrument)

        self.log().notice("Loading '%s'" % eventFilename)
        wkspName = self.getPropertyValue("OutputWorkspace")
        LoadEventPreNexus(EventFilename=eventFilename, OutputWorkspace=wkspName)

        # let people know what was just loaded
        wksp = mtd[wkspName]
        instrument = str(wksp.getInstrument().getName())
        runNumber = int(wksp.run()["run_number"].value)
        startTime = str(wksp.run().startTime())
        self.log().information("Loaded %s live run %d - starttime=%s" % (instrument, runNumber, startTime))

        if self.getProperty("NormalizeByCurrent").value:
            self.log().information("Normalising by current")
            NormaliseByCurrent(InputWorkspace=wkspName, Outputworkspace=wkspName)

        if self.getProperty("LoadLogs").value:
            logFilename = self.findLogfile(instrument, runNumber)
            if len(logFilename) > 0:
                self.log().information("Loading logs from %s" % logFilename)
                LoadNexusLogs(Workspace=wkspName, Filename=logFilename)
                wksp = mtd[wkspName]
                instrFilename = wksp.getInstrumentFilename(instrument, startTime)
                LoadInstrument(Workspace=wkspName, Filename=instrFilename, RewriteSpectraMap=True)

        # gets rid of many simple DAS errors
        FilterByXValue(InputWorkspace=wkspName, XMin=1)

        self.setProperty("OutputWorkspace", mtd[wkspName])


AlgorithmFactory.subscribe(LoadPreNexusLive)
