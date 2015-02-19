"""
    Data catalog for EQSANS
"""
from reduction_gui.reduction.sans.data_cat import DataCatalog as BaseCatalog
from reduction_gui.reduction.sans.data_cat import DataSet
from data_cat import DataType
import re
import time
import datetime

# Check whether Mantid is available
try:
    from mantid.api import AnalysisDataService
    import mantid.simpleapi as api
    HAS_MANTID = True
except:
    HAS_MANTID = False

try:
    import mantidplot
    IN_MANTIDPLOT = True
except:
    IN_MANTIDPLOT = False

class EQSANSDataType(DataType):
    TABLE_NAME="eqsans_datatype"

class EQSANSDataSet(DataSet):
    TABLE_NAME="eqsans_dataset"
    data_type_cls = EQSANSDataType

    def __init__(self, run_number, title, run_start, duration, sdd):
        super(EQSANSDataSet, self).__init__(run_number, title, run_start, duration, sdd)

    @classmethod
    def load_meta_data(cls, file_path, outputWorkspace):
        try:
            if IN_MANTIDPLOT:
                script = "LoadEventNexus(Filename='%s', OutputWorkspace='%s', MetaDataOnly=True)" % (file_path, outputWorkspace)
                mantidplot.runPythonScript(script, True)
                if not AnalysisDataService.doesExist(outputWorkspace):
                    return False
            else:
                api.LoadEventNexus(Filename=file_path, OutputWorkspace=outputWorkspace, MetaDataOnly=True)
            return True
        except:
            return False

    @classmethod
    def handle(cls, file_path):
        """
            Return a DB handle for the given file, such as a run number
        """
        file_path = file_path.strip()
        r_re = re.search("EQSANS_([0-9]+)_event", file_path)
        if r_re is not None:
            return r_re.group(1)
        else:
            # Check whether we simply have a run number
            try:
                run = int(file_path)
                return file_path
            except:
                return None
        return None

    @classmethod
    def read_properties(cls, ws, run, cursor):
        def read_prop(prop):
            try:
                ws_object = AnalysisDataService.retrieve(ws)
                return str(ws_object.getRun().getProperty(prop).value)
            except:
                return ""
        def read_series(prop):
            try:
                ws_object = AnalysisDataService.retrieve(ws)
                return float(ws_object.getRun().getProperty(prop).getStatistics().mean)
            except:
                return -1

        runno = read_prop("run_number")
        if runno=="":
            runno = run

        title = read_prop("run_title")
        t_str = read_prop("start_time")
        # Get rid of the training microseconds
        toks = t_str.split('.')
        if len(toks)>=2:
            t_str=toks[0]
        t = datetime.datetime.strptime(t_str, '%Y-%m-%dT%H:%M:%S')
        # TZ offset
        offset = datetime.datetime.now()-datetime.datetime.utcnow()
        t = t+offset
        run_start = t.strftime('%y-%m-%d %H:%M')

        duration = read_prop("duration")
        try:
            duration = float(duration)
        except:
            duration = 0

        sdd = read_series("detectorZ")

        d = EQSANSDataSet(runno, title, run_start, duration, sdd)
        d.insert_in_db(cursor)
        return d


class DataCatalog(BaseCatalog):
    extension = "nxs"
    data_set_cls = EQSANSDataSet

    def __init__(self, replace_db=False):
        super(DataCatalog, self).__init__(replace_db)
