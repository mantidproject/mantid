# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""
Data catalog for EQSANS
"""

from reduction_gui.reduction.sans.data_cat import DataCatalog as BaseCatalog
from reduction_gui.reduction.sans.data_cat import DataSet, DataType
from reduction_gui.reduction.scripter import execute_script
import re
import datetime

# Check whether Mantid is available
try:
    from mantid.api import AnalysisDataService
    from mantid.simpleapi import LoadEventNexus

    HAS_MANTID = True
except:
    HAS_MANTID = False

try:
    import mantidplot  # noqa

    IN_MANTIDPLOT = True
except:
    IN_MANTIDPLOT = False


class EQSANSDataType(DataType):
    TABLE_NAME = "eqsans_datatype"


class EQSANSDataSet(DataSet):
    TABLE_NAME = "eqsans_dataset"
    data_type_cls = EQSANSDataType

    def __init__(self, run_number, title, run_start, duration, sdd):
        super(EQSANSDataSet, self).__init__(run_number, title, run_start, duration, sdd)

    @classmethod
    def load_meta_data(cls, file_path, outputWorkspace):
        try:
            if IN_MANTIDPLOT:
                script = "LoadEventNexus(Filename='%s', OutputWorkspace='%s', MetaDataOnly=True)" % (file_path, outputWorkspace)
                execute_script(script)
                if not AnalysisDataService.doesExist(outputWorkspace):
                    return False
            else:
                LoadEventNexus(Filename=file_path, OutputWorkspace=outputWorkspace, MetaDataOnly=True)
            return True
        except:
            return False

    @classmethod
    def handle(cls, file_path):
        """
        Return a DB handle for the given file, such as a run number
        This will handle file formats in two formats:
        EQSANS_([0-9]+)_event
        EQSANS_([0-9]+).nxs
        """
        file_path = file_path.strip()
        r_re = re.search(r"EQSANS_([0-9]+)(_event|\.nxs)", file_path)
        if r_re is not None:
            return r_re.group(1)
        else:
            # Check whether we simply have a run number
            try:
                int(file_path)
                return file_path
            except:
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
        if runno == "":
            runno = run

        title = read_prop("run_title")
        t_str = read_prop("start_time")
        # Get rid of the training microseconds
        toks = t_str.split(".")
        if len(toks) >= 2:
            t_str = toks[0]
        t = datetime.datetime.strptime(t_str, "%Y-%m-%dT%H:%M:%S")
        # TZ offset
        offset = datetime.datetime.now() - datetime.datetime.utcnow()
        t = t + offset
        run_start = t.strftime("%y-%m-%d %H:%M")

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
    extension = ["nxs", "nxs.h5"]
    data_set_cls = EQSANSDataSet

    def __init__(self, replace_db=False):
        super(DataCatalog, self).__init__(replace_db)
