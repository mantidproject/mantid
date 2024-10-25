# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import time
import datetime
import numbers
import bisect
import numpy
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, WorkspaceFactory, WorkspaceProperty
from mantid.kernel import Direction, StringArrayProperty


class LoadLogPropertyTable(PythonAlgorithm):
    def summary(self):
        """Return summary"""
        return "Creates a table of Run number against the log values for that run for a range of files.\
         It can use a single log value or a list of log values."

    def seeAlso(self):
        return ["LoadLog", "LoadMuonLog"]

    # same concept as built in "CreateLogPropertyTable" but loads its own workspaces and needn't hold all in memory at once
    # select log values to put in table (list)
    # special cases for:
    # beamlog_(counts, frames, etc): last few points end up in next run's log. Find Maximum.
    # comment (separate function)
    # time series, take average for t>0 (if available)
    def PyInit(self):
        self.declareProperty(
            FileProperty(name="FirstFile", defaultValue="", action=FileAction.Load, extensions=["nxs", "raw"]),
            "The first file to load from",
        )
        self.declareProperty(
            FileProperty(name="LastFile", defaultValue="", action=FileAction.Load, extensions=["nxs", "raw"]),
            "The Last file to load from, must be in the same directory, all files in between will also be used",
        )
        self.declareProperty(
            StringArrayProperty("LogNames", direction=Direction.Input),
            "The comma seperated list of properties to include. \n" + "The full list will be printed if an invalid value is used.",
        )
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output), "Table of results")

    def category(self):
        return "DataHandling\\Logs;Muon\\DataHandling"

    def getGeneralLogValue(self, ws, name, begin):
        # get log value
        # average time series over run
        # for beamlog, etc return flag=true and value to push into previous run
        if name == "comment":
            return (ws.getComment(), False, 0)

        try:
            v = ws.getRun().getProperty(name)
        except:
            possibleLogs = list(ws.getRun().keys())
            possibleLogs.insert(0, "comment")
            message = "The log name '" + name + "' was not found, possible choices are: " + str(possibleLogs)
            raise ValueError(message)
        try:
            times2 = []
            if hasattr(v, "unfiltered"):
                v = v.unfiltered()
            for tt in v.times:
                times2.append((datetime.datetime(*(time.strptime(str(tt), "%Y-%m-%dT%H:%M:%S")[0:6])) - begin).total_seconds())
        except:  # pylint: disable=bare-except
            # print "probably not a time series"
            pass
        if name[0:8] == "Beamlog_" and (name.find("Counts") > 0 or name.find("Frames") > 0):
            i = bisect.bisect_right(times2, 2)  # allowance for "slow" clearing of DAE
            # print "returning max beam log, list cut 0:",i,":",len(times2)
            return (numpy.amax(v.value[i:]), True, numpy.amax(v.value[:i]))
        if v.__class__.__name__ == "TimeSeriesProperty_dbl" or v.__class__.__name__ == "FloatTimeSeriesProperty":
            i = bisect.bisect_left(times2, 0)
            return (numpy.average(v.value[i:]), False, 0)
        return (v.value, False, 0)

    # pylint: disable=too-many-branches
    def PyExec(self):
        firstFileName = self.getProperty("FirstFile").value
        lastFileName = self.getProperty("LastFile").value

        firstRunNum, firstFileFirstDigit, firstFileLastDigit = self.getRunNumber(firstFileName)
        lastRunNum, lastFileFirstDigit, LastFileLastDigit = self.getRunNumber(lastFileName)

        if firstFileName[:lastFileFirstDigit] != lastFileName[:lastFileFirstDigit]:
            raise Exception("Files from different directories or instruments")
        if firstFileName[firstFileName.rindex(".")] != lastFileName[firstFileName.rindex(".")]:
            raise Exception("Files of different types")
        if firstFileLastDigit - firstFileFirstDigit != LastFileLastDigit - lastFileFirstDigit:
            raise Exception("File numbering error")
        if lastRunNum < firstRunNum:
            raise Exception("Run numbers must increase")

        # table. Rows=runs, columns=logs (col 0 = run number)
        collist = self.getProperty("LogNames").value
        wsOutput = WorkspaceFactory.createTable()
        wsOutput.addColumn("int", "RunNumber")

        # loop and load files. Absolute numbers for now.
        for loopRunNum in range(firstRunNum, lastRunNum + 1):
            # create a file path for intervening files, based from the 1st filename
            thispath = (
                firstFileName[:firstFileFirstDigit]
                + str(loopRunNum).zfill(firstFileLastDigit - firstFileFirstDigit)
                + firstFileName[firstFileLastDigit:]
            )

            loadedWs = self.loadMetaData(thispath)
            if loadedWs is None:
                continue

            # check if the ws is a group
            ws = loadedWs
            if ws.id() == "WorkspaceGroup":
                ws = ws[0]

            begin = datetime.datetime(
                *(time.strptime(ws.getRun().getProperty("run_start").value, "%Y-%m-%dT%H:%M:%S")[0:6])
            )  # start of day
            vallist = [loopRunNum]
            for col in collist:
                try:
                    (colValue, leftover, lval) = self.getGeneralLogValue(ws, col, begin)
                except ValueError:
                    # this is a failure to find the named log
                    raise
                vallist.append(colValue)
                if loopRunNum == firstRunNum:
                    if isinstance(colValue, numbers.Number):
                        wsOutput.addColumn("double", col)
                    else:
                        wsOutput.addColumn("str", col)
                if leftover and loopRunNum > firstRunNum:
                    if lval > wsOutput.cell(col, loopRunNum - firstRunNum - 1):
                        wsOutput.setCell(col, loopRunNum - firstRunNum - 1, lval)
            wsOutput.addRow(vallist)

        self.setProperty("OutputWorkspace", wsOutput)

    def loadMetaData(self, thispath):
        loadedWs = None
        try:
            loadAlg = self.createChildAlgorithm("Load")
            # set Filename first
            loadAlg.setProperty("Filename", thispath)
            loadAlg.setProperty("OutputWorkspace", "__CopyLogsTmp")
            try:
                # try to set MetaDataOnly
                loadAlg.setProperty("MetaDataOnly", True)
            except (ValueError, RuntimeError):
                # If that fails set SpectrumMin and SpectrumMax
                loadAlg.setProperty("SpectrumMin", 1)
                loadAlg.setProperty("SpectrumMax", 1)
            loadAlg.execute()

            outWSPropName = "OutputWorkspace"
            try:
                loadedWs = loadAlg.getProperty(outWSPropName).value
            except RuntimeError:
                raise RuntimeError("No output workspace for " + thispath)
        except (ValueError, RuntimeError):
            return None

        return loadedWs

    def getRunNumber(self, fileName):
        # Find last . and step back until you find a digit
        lastDigitIndex = fileName.rindex(".")
        while not fileName[lastDigitIndex - 1].isdigit():
            lastDigitIndex -= 1

        # Keep going back until you find the start of the number sequence
        firstDigitIndex = lastDigitIndex - 1
        while fileName[firstDigitIndex - 1].isdigit():
            firstDigitIndex -= 1
        runNumber = int(fileName[firstDigitIndex:lastDigitIndex])
        return runNumber, firstDigitIndex, lastDigitIndex


AlgorithmFactory.subscribe(LoadLogPropertyTable())
