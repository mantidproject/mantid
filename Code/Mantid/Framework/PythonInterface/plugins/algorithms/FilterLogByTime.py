#pylint: disable=no-init,invalid-name
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
import numpy
import sys

class FilterLogByTime(PythonAlgorithm):

    def category(self):
        return "Events\\EventFiltering"

    def name(self):
        return "FilterLogByTime"

    def summary(self):
        return "Filters a log between time intervals and applies a user defined operation to the result."

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("InputWorkspace", "", direction=Direction.Input), "Input workspace")
        log_validator = StringMandatoryValidator()
        self.declareProperty(name="LogName", defaultValue="", direction=Direction.Input, validator=log_validator, doc="Log name to filter by")
        self.declareProperty(name="StartTime", defaultValue=-sys.float_info.max, validator=FloatBoundedValidator(), direction=Direction.Input, doc="Start time for filtering. Seconds after run start")
        self.declareProperty(name="EndTime", defaultValue=sys.float_info.max, validator=FloatBoundedValidator(), direction=Direction.Input, doc="End time for filtering. Seconds after run start")
        self.declareProperty(name="Method",defaultValue="mean", validator=StringListValidator(["mean","min", "max", "median"]), doc="Statistical method to use to generate ResultStatistic output")
        self.declareProperty(FloatArrayProperty(name="FilteredResult", direction=Direction.Output), doc="Filtered values between specified times.")
        self.declareProperty(name="ResultStatistic", defaultValue=0.0, direction=Direction.Output, doc="Requested statistic")

    def PyExec(self):
        in_ws = self.getProperty("InputWorkspace").value
        log_name = self.getProperty("LogName").value
        start_time = self.getProperty("StartTime").value
        end_time = self.getProperty("EndTime").value
        method = self.getProperty("Method").value
        if start_time == -sys.float_info.max:
            start_time = None
        if end_time == sys.float_info.max:
            end_time = None
        if start_time and end_time and (start_time > end_time):
            raise ValueError("StartTime > EndTime, %s > %s" % (str(start_time), str(end_time)))

        values = self.__filter(in_ws, log_name, start_time, end_time)
        stats = self.__statistics(values, method)
        self.setProperty("FilteredResult", values)
        self.setProperty("ResultStatistic", float(stats))
        self.log().information("Stats %s" % str(stats))
        self.log().information("Time filtered results %s" % str(values))

    def __filter(self, ws, logname, starttime=None, endtime=None):
        run = ws.getRun()
        runstart = run.startTime().total_nanoseconds()
        tstart = runstart
        tend = run.endTime().total_nanoseconds()
        nanosecond = int(1e9)
        if starttime:
            tstart = runstart + (starttime * nanosecond)
        if  endtime:
            tend = runstart + (endtime * nanosecond)
        log = run.getLogData(logname)
        if not hasattr(log, "times"):
            raise ValueError("log called %s is not a FloatTimeSeries log" % logname)

        times = numpy.array(map(lambda t: t.total_nanoseconds(), log.times))

        values = numpy.array(log.value)
        mask = (tstart <= times) & (times <= tend) # Get times between filter start and end.
        filteredvalues = values[mask]
        return filteredvalues

    def __statistics(self, values, operation):
        op = getattr(numpy, operation)
        return op(values)

AlgorithmFactory.subscribe(FilterLogByTime)
