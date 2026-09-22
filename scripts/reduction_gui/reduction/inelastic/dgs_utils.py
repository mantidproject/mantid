# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os

IS_IN_MANTIDPLOT = False
try:
    import mantidplot  # noqa
    from mantid.kernel import config
    from mantid.api import AnalysisDataService
    from mantid.simpleapi import LoadEmptyInstrument

    IS_IN_MANTIDPLOT = True
except:
    pass


class InstrumentParameters(object):
    instrument_name = None
    # ComponentInfo is owned by its workspace, so the empty-instrument workspace is kept
    # (hidden) in the ADS rather than removed as the legacy instrument object allowed.
    _workspace_name = None

    def __init__(self, inst_name):
        if self.instrument_name is None:
            self._load_param_file(inst_name)
        else:
            if self.instrument_name != inst_name:
                self._load_param_file(inst_name)

    def _load_param_file(self, inst_name):
        InstrumentParameters.instrument_name = inst_name
        if IS_IN_MANTIDPLOT:
            idf_loc = config.getInstrumentDirectory()
            idf_pattern = os.path.join(idf_loc, "%s_Definition*.xml") % inst_name
            import glob

            idf_files = glob.glob(idf_pattern)
            if InstrumentParameters._workspace_name is not None:
                AnalysisDataService.remove(InstrumentParameters._workspace_name)
            workspace_name = "__%s_instrument_parameters" % inst_name
            LoadEmptyInstrument(Filename=str(idf_files[0]), OutputWorkspace=workspace_name)
            InstrumentParameters._workspace_name = workspace_name

    def _self_check(self):
        if self._workspace_name is None:
            raise ValueError("Instrument was not loaded, cannot retrieve parameters.")

    def get_parameter(self, name):
        default = -1
        try:
            self._self_check()
        except ValueError:
            return default

        component_info = AnalysisDataService.retrieve(self._workspace_name).componentInfo()
        root = component_info.root()
        type_name = component_info.getParameterType(root, name)
        if type_name == "double":
            val = component_info.getNumberParameter(root, name)
        elif type_name == "bool":
            val = component_info.getBoolParameter(root, name)
        elif type_name == "string":
            val = component_info.getStringParameter(root, name)
            if val[0] == "None":
                return None
        elif type_name == "int":
            val = component_info.getIntParameter(root, name)
        else:
            return default
        try:
            return val[0]
        except IndexError:
            return default

    def get_bool_param(self, parname):
        default = False
        try:
            self._self_check()
        except ValueError:
            return default

        param = self.get_parameter(parname)
        if param < 0:
            return False
        return bool(param)
