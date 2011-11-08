"""
    Data catalog for EQSANS
"""
from reduction_gui.reduction.sans.data_cat import DataCatalog as BaseCatalog
from reduction_gui.reduction.sans.data_cat import DataSet as BaseDataSet   
import re
import time

# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    import mantidsimple
    HAS_MANTID = True
except:
    HAS_MANTID = False    

class EQSANSDataSet(BaseDataSet):
    TABLE_NAME="eqsans_dataset"
    def __init__(self, run_number, title, run_start, duration, sdd):
        super(EQSANSDataSet, self).__init__(run_number, title, run_start, duration, sdd)

    @classmethod
    def load_meta_data(cls, file_path, outputWorkspace):
        try:
            mantidsimple.LoadEventNexus(file_path, OutputWorkspace=outputWorkspace, MetaDataOnly=True)
            return True
        except:
            return False

    @classmethod
    def handle(cls, file_path):
        """
            Return a DB handle for the given file, such as a run number
        """
        r_re = re.search("EQSANS_([0-9]+)_event", file_path)
        if r_re is None:
            return None
        
        return r_re.group(1)
    
    @classmethod
    def read_properties(cls, ws, run, cursor):
        def read_prop(prop):
            try:
                return str(mtd[ws].getRun().getProperty(prop).value)
            except:
                return ""
        def read_series(prop):
            try:
                return float(mtd[ws].getRun().getProperty(prop).getStatistics().mean)
            except:
                return -1
        
        runno = read_prop("run_number")
        if runno=="":
            runno = run
            
        title = read_prop("run_title")
        t_str = read_prop("start_time")
        t = time.strptime(t_str, '%Y-%m-%dT%H:%M:%S')
        run_start = time.strftime('%y-%m-%d %H:%M', t)
            
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