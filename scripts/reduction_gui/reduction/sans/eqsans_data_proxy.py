# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys

# Check whether Mantid is available
try:
    from mantid.api import AnalysisDataService  # noqa
    from mantid.kernel import Logger  # noqa
    import mantid.simpleapi as api

    HAS_MANTID = True
except:
    HAS_MANTID = False


class DataProxy(object):
    """
    Class used to load a data file temporarily to extract header information
    """

    data_ws = None

    ## Error log
    errors = []

    def __init__(self, data_file, workspace_name=None):
        self.errors = []
        if HAS_MANTID:
            try:
                if workspace_name is None:
                    self.data_ws = "__raw_data_file"
                else:
                    self.data_ws = str(workspace_name)
                try:
                    api.LoadEventNexus(Filename=data_file, OutputWorkspace=workspace_name)
                except:
                    self.errors.append("Error loading data file as Nexus event file:\n%s" % sys.exc_info()[1])
                    api.Load(Filename=data_file, OutputWorkspace=workspace_name)
                    self.errors = []
            except:
                self.data_ws = None
                self.errors.append("Error loading data file:\n%s" % sys.exc_info()[1])
