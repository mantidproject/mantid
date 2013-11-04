from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
import numpy
import sys

class FilterLogByTime(PythonAlgorithm):

    def category(self):
        return "Filtering"

    def name(self):
        return "FilterLogByTime"

    def PyInit(self):    
        self.declareProperty(WorkspaceProperty("InputWorkspace", "", direction=Direction.Input), "Input workspace")
        log_validator = StringMandatoryValidator() 
        self.declareProperty(name="LogName", defaultValue="", direction=Direction.Input, validator=log_validator, doc="Log name to filter by")
        self.declareProperty(name="StartTime", defaultValue=sys.float_info.min, validator=FloatBoundedValidator(), direction=Direction.Input, doc="Start time for filtering")
        self.declareProperty(name="EndTime", defaultValue=sys.float_info.max, validator=FloatBoundedValidator(), direction=Direction.Input, doc="Start time for filtering")
        self.declareProperty(FloatArrayProperty(name="FilteredResult", direction=Direction.Output), doc="Output stitched workspace")
        self.declareProperty(name="ResultStatistic", defaultValue=0.0, direction=Direction.Output, doc="Requested statistic")
    
    def PyExec(self):
        in_ws = self.getProperty("InputWorkspace").value
        log_name = self.getProperty("LogName").value
        start_time = self.getProperty("StartTime").value
        end_time = self.getProperty("EndTime").value
        if start_time == sys.float_info.min:
            start_time = None
        if end_time == sys.float_info.max:
            end_time = None
        if start_time > end_time:
            raise ValueError("StartTime > EndTime")
        
        values = self.__filter(in_ws, log_name)
        stats = self.__statistics(values)
        self.setProperty("FilteredResult", values)
        self.setProperty("ResultStatistic", float(stats))

    def __filter(self, ws, logname, starttime=None, endtime=None):
        run = ws.getRun()
        tstart = run.startTime()
        tend = run.endTime()
        nanosecond = int(1e9)
        if starttime:
            tstart = tstart + (starttime * nanosecond)
        if  endtime:
            tend = tend + (endtime * nanosecond)
        log = run.getLogData(logname)
        times = numpy.array(log.times)
        values = numpy.array(log.value)
        mask = (tstart < times) & (times < tend) # Get times between filter start and end.
        filteredvalues = values[mask]
        return filteredvalues
    
    def __statistics(self, values, operation='mean'):
        op = getattr(numpy, operation)
        return op(values)
    
AlgorithmFactory.subscribe(FilterLogByTime)