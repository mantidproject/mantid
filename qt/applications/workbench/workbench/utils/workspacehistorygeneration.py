# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidworkbench package


from mantid.api import AlgorithmManager, AnalysisDataService as ADS
from mantid import UsageService


# To ignore an algorithm in project recovery please put it's name here, this is done to stop these algorithm
# calls from being saved. e.g. MonitorLiveData is ignored because StartLiveData is the only one that is needed
# to restart this workspace.
ALGS_TO_IGNORE = [
    "MonitorLiveData",
    "EnggSaveGSASIIFitResultsToHDF5",
    "EnggSaveSinglePeakFitResultsToHDF5",
    "ExampleSaveAscii",
    "SANSSave",
    "SaveAscii",
    "SaveBankScatteringAngles",
    "SaveCSV",
    "SaveCalFile",
    "SaveCanSAS1D",
    "SaveDaveGrp",
    "SaveDetectorsGrouping",
    "SaveDiffCal",
    "SaveDiffFittingAscii",
    "SaveFITS",
    "SaveFocusedXYE",
    "SaveFullprofResolution",
    "SaveGDA",
    "SaveGEMMAUDParamFile",
    "SaveGSASInstrumentFile",
    "SaveGSS",
    "SaveHKL",
    "SaveIsawDetCal",
    "SaveIsawPeaks",
    "SaveIsawQvector",
    "SaveIsawUB",
    "SaveLauenorm",
    "SaveMD",
    "SaveMask",
    "SaveNISTDAT",
    "SaveNXSPE",
    "SaveNXTomo",
    "SaveNXcanSAS",
    "SaveNexus",
    "SaveNexusPD",
    "SaveNexusProcessed",
    "SaveOpenGenieAscii",
    "SavePAR",
    "SavePDFGui",
    "SavePHX",
    "SaveParameterFile",
    "SavePlot1D",
    "SavePlot1DAsJson",
    "SaveRKH",
    "SaveReflections",
    "SaveReflectometryAscii",
    "SaveSESANS",
    "SaveSPE",
    "SaveTBL",
    "SaveVTK",
    "SaveVulcanGSS",
    "SaveYDA",
    "SaveZODS",
]

# If you want to ignore an algorithms' property, then add to the string below in the format:
# "AlgorithmName + PropertyName". The final string should look like "a + b , c + d, ...".
# This uses string representation to pass to the C++ algorithm. The outer delimiter is `,`
# and the inner delimiter is `+` for the list of lists. e.g. [[a, b],[c, d]] = "a + b , c + d".
ALG_PROPERTIES_TO_IGNORE = "StartLiveData + MonitorLiveData"


def get_workspace_history_list(workspace):
    """
    Return a list of commands that will recover the state of a workspace.
    """
    alg_name = "GeneratePythonScript"
    alg = AlgorithmManager.createUnmanaged(alg_name, 1)
    alg.setChild(True)
    alg.setLogging(False)
    alg.initialize()
    alg.setProperty("InputWorkspace", workspace)
    alg.setProperty("IgnoreTheseAlgs", ALGS_TO_IGNORE)
    alg.setProperty("IgnoreTheseAlgProperties", ALG_PROPERTIES_TO_IGNORE)
    alg.setPropertyValue("StartTimestamp", UsageService.getStartTime().toISO8601String())
    alg.setProperty("AppendTimestamp", True)
    alg.setProperty("AppendExecCount", True)
    alg.execute()
    history = alg.getPropertyValue("ScriptText")
    return history.split("\n")[6:]  # trim the header and import


def convert_list_to_string(to_convert, add_new_line=True, fix_comments=False):
    string = ""
    for line in to_convert:
        if isinstance(line, str):
            string += line
        elif fix_comments and isinstance(line, tuple):
            string += line[0]
        else:
            # Assume it is a list and continue
            string += convert_list_to_string(line, add_new_line)
        if add_new_line:
            string += "\n"
    return string


def guarantee_unique_lines(script):
    if not script:
        return script
    alg_name = "OrderWorkspaceHistory"
    alg = AlgorithmManager.createUnmanaged(alg_name, 1)
    alg.setChild(True)
    alg.setLogging(False)
    alg.initialize()
    alg.setPropertyValue("InputString", script)
    alg.execute()
    script = alg.getPropertyValue("OutputString")
    return script


def get_all_workspace_history_from_ads():
    workspace_histories = []
    for workspace in ADS.getObjectNames():
        workspace_histories.append(get_workspace_history_list(workspace))
    script = convert_list_to_string(workspace_histories)
    return guarantee_unique_lines(script)
