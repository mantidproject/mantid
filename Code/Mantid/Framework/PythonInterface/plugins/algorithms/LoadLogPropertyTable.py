#pylint: disable=no-init,invalid-name
import time
import datetime
import numbers
import bisect
import numpy
from mantid.api import * # PythonAlgorithm, AlgorithmFactory, WorkspaceProperty
from mantid.kernel import * # StringArrayProperty
from mantid.simpleapi import * # needed for Load

class LoadLogPropertyTable(PythonAlgorithm):



    def summary(self):
        """ Return summary
        """
        return "Creates a table of Run number against the log values for that run for a range of files.\
         It can use a single log value or a list of log values."

    # same concept as built in "CreateLogPropertyTable" but loads its own workspaces and needn't hold all in memory at once
    # select log values to put in table (list)
    # special cases for:
    # beamlog_(counts, frames, etc): last few points end up in next run's log. Find Maximum.
    # comment (separate function)
    # time series, take average for t>0 (if available)
    def PyInit(self):
        self.declareProperty(FileProperty(name="FirstFile",defaultValue="",action=FileAction.Load,extensions = ["nxs","raw"]),"The first file to load from")
        self.declareProperty(FileProperty(name="LastFile",defaultValue="",action=FileAction.Load,extensions = ["nxs","raw"]),"The Last file to load from, must be in the same directory, all files in between will also be used")
        self.declareProperty(StringArrayProperty("LogNames",direction=Direction.Input),"The comma seperated list of properties to include. \nThe full list will be printed if an invalid value is used.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace","",Direction.Output),"Table of results")

    def category(self):
        return "Utility;Muon"

    def getGeneralLogValue(self,ws,name,begin):
        # get log value
        # average time series over run
        # for beamlog, etc return flag=true and value to push into previous run
        if name=="comment":
            return (ws.getComment(),False,0)

        try:
            v=ws.getRun().getProperty(name)
        except:
            possibleLogs = ws.getRun().keys()
            possibleLogs.insert(0,'comment')
            message =  "The log name '" + name + "' was not found, possible choices are: " + str(possibleLogs)
            raise ValueError(message)
        try:
            times2=[]
            if hasattr(v,"unfiltered"):
                v=v.unfiltered()
            for tt in v.times:
                times2.append((datetime.datetime(*(time.strptime(str(tt),"%Y-%m-%dT%H:%M:%S")[0:6]))-begin).total_seconds())
        except:
            #print "probably not a time series"
            pass

        if name[0:8]=="Beamlog_" and (name.find("Counts")>0 or name.find("Frames")>0):
            i=bisect.bisect_right(times2,2) # allowance for "slow" clearing of DAE
            #print "returning max beam log, list cut 0:",i,":",len(times2)
            return (numpy.amax(v.value[i:]),True,numpy.amax(v.value[:i]))
        if v.__class__.__name__ =="TimeSeriesProperty_dbl" or v.__class__.__name__ =="FloatTimeSeriesProperty":
            i=bisect.bisect_left(times2,0)
            return (numpy.average(v.value[i:]),False,0)
        return (v.value,False,0)

    def PyExec(self):

        file1=self.getProperty("FirstFile").value
        file9=self.getProperty("LastFile").value
        i1=file1.rindex('.')
        j1=i1-1
        while file1[j1-1].isdigit():
            j1=j1-1
        firstnum=int(file1[j1:i1])
        i9=file9.rindex('.')
        j9=i9-1
        while file9[j9-1].isdigit():
            j9=j9-1
        lastnum=int(file9[j9:i9])
        if file1[:j9] != file9[:j9]:
            raise Exception("Files from different directories or instruments")
        if file1[i1:] != file9[i9:]:
            raise Exception("Files of different types")
        if i1-j1 != i9-j9:
            raise Exception("File numbering error")
        if lastnum < firstnum:
            raise Exception("Run numbers must increase")

        # table. Rows=runs, columns=logs (col 0 = run number)
        collist=self.getProperty("LogNames").value
        ows=WorkspaceFactory.createTable()
        ows.addColumn("int","RunNumber")

        # loop and load files. Absolute numbers for now.
        for ff in range(firstnum,lastnum+1):
            thispath=file1[:j1]+str(ff).zfill(i1-j1)+file1[i1:]
            returnTuple=None
            try:
                returnTuple=Load(Filename=thispath,OutputWorkspace="__CopyLogsTmp",SpectrumMin=1, SpectrumMax=1)
            except:
                continue

            #check if the return type is atuple
            if type(returnTuple) == tuple:
                loadedWs=returnTuple[0]
            else:
                loadedWs = returnTuple

            #check if the ws is a group
            ws = loadedWs
            if ws.id() == 'WorkspaceGroup':
                ws=ws[0]

            begin=datetime.datetime(*(time.strptime(ws.getRun().getProperty("run_start").value,"%Y-%m-%dT%H:%M:%S")[0:6])) # start of day
            vallist=[ff]
            for cc in collist:
                try:
                    (cv,leftover,lval)=self.getGeneralLogValue(ws,cc,begin)
                except ValueError:
                    #this is a failure to find the named log
                    DeleteWorkspace(loadedWs)
                    raise
                vallist.append(cv)
                if ff==firstnum:
                    if isinstance(cv, numbers.Number):
                        ows.addColumn("double",cc)
                    else:
                        ows.addColumn("str",cc)
                if leftover and ff>firstnum:
                    if lval>ows.cell(cc,ff-firstnum-1):
                        ows.setCell(cc,ff-firstnum-1,lval)
            ows.addRow(vallist)
            DeleteWorkspace(loadedWs)


        self.setProperty("OutputWorkspace",ows)

AlgorithmFactory.subscribe(LoadLogPropertyTable())
