"""
    Data catalog for HFIR SANS
"""
from data_cat import DataCatalog as BaseCatalog
from data_cat import DataSet
from data_cat import DataType
import os
import time

class HFIRDataType(DataType):
    TABLE_NAME="hfir_datatype"
    
class HFIRDataSet(DataSet):
    TABLE_NAME="hfir_dataset"
    data_type_cls = HFIRDataType
    python_api = 1
    
    def __init__(self, run_number, title, run_start, duration, sdd, python_api=1):
        super(HFIRDataSet, self).__init__(run_number, title, run_start, 
                                          duration, sdd, python_api=1)

    @classmethod
    def load_meta_data(cls, file_path, outputWorkspace, python_api=1):
        try:
            if python_api==1:
                import mantidsimple as api
            else:
                import mantid.simpleapi as api
            api.LoadSpice2D(Filename=file_path, OutputWorkspace=outputWorkspace)
            return True
        except:
            return False

    @classmethod
    def handle(cls, file_path):
        """
            Return a DB handle for the given file, such as a run number
        """
        handle = os.path.splitext(os.path.basename(file_path))[0]
        if handle=="":
            return None
        return handle
    
    @classmethod
    def find_with_api(cls, file_path, cursor, process_files=True, python_api=1):
        """
            Find an entry in the database, or create on as needed
        """
        run = cls.handle(file_path)
        if run is None:
            return None
        
        t = (run,)
        cursor.execute('select * from %s where run=?'% cls.TABLE_NAME, t)
        rows = cursor.fetchall()

        if len(rows) == 0:
            if process_files:
                log_ws = "__log"                
                if cls.load_meta_data(file_path, 
                                      outputWorkspace=log_ws,
                                      python_api=python_api):
                    return cls.read_properties(log_ws, run, cursor, 
                                               python_api=python_api)
                else:
                    return None
            else:
                return None
        else:
            row = rows[0]
            return DataSet(row[1], row[2], row[3], row[4], row[5], id=row[0])
        

    @classmethod
    def read_properties(cls, ws, run, cursor, python_api=1):
        def read_prop(prop):
            try:
                if python_api==1:
                    from MantidFramework import mtd
                    return str(mtd[ws].getRun().getProperty(prop).value)
                else:
                    from mantid.api import AnalysisDataService
                    ws_object = AnalysisDataService.retrieve(ws) 
                    return str(ws_object.getRun().getProperty(prop).value)
            except:
                return ""
        def read_series(prop):
            try:
                if python_api==1:
                    from MantidFramework import mtd
                    return float(mtd[ws].getRun().getProperty(prop).getStatistics().mean)
                else:
                    from mantid.api import AnalysisDataService
                    ws_object = AnalysisDataService.retrieve(ws) 
                    return str(ws_object.getRun().getProperty(prop).getStatistics().mean)
            except:
                return -1
        
        title = read_prop("run_title")
        t_str = read_prop("start_time")
        t = time.strptime(t_str, '%Y-%m-%d %H:%M:%S')
        run_start = time.strftime('%y-%m-%d %H:%M', t)

        duration = read_prop("timer")
        try:
            duration = float(duration)
        except:
            duration = 0
        
        sdd = float(read_prop("sample-detector-distance"))

        d = HFIRDataSet(run, title, run_start, duration, sdd)
        d.insert_in_db(cursor)
        return d
    

class DataCatalog(BaseCatalog):
    extension = "xml"
    data_set_cls = HFIRDataSet

    def __init__(self, replace_db=False):
        super(DataCatalog, self).__init__(replace_db)