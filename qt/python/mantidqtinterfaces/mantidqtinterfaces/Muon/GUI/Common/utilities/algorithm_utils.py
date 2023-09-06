# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import PHASEQUAD_IM, PHASEQUAD_RE
import mantid.simpleapi as mantid
from mantid.kernel import Logger
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import remove_ws, retrieve_ws
from copy import copy
from mantid.kernel import PhysicalConstants as const


muon_logger = Logger("Muon-Algs")


def create_empty_table(name):
    alg = mantid.AlgorithmManager.create("CreateEmptyTableWorkspace")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty("OutputWorkspace", name)
    alg.execute()
    return retrieve_ws(name)


def run_MuonPreProcess(parameter_dict):
    """
    Apply the MuonPreProcess algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated workspace name.
    """
    alg = mantid.AlgorithmManager.create("MuonPreProcess")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperties(parameter_dict)
    alg.execute()
    return parameter_dict["OutputWorkspace"]


def run_MuonGroupingCounts(parameter_dict, workspace_name):
    """
    Apply the MuonGroupingCounts algorithm with the properties supplied through
    the input dictionary of {property_name:property_value} pairs.
    Returns the calculated workspace name.
    """
    alg = mantid.AlgorithmManager.create("MuonGroupingCounts")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", workspace_name)
    alg.setProperties(parameter_dict)
    alg.execute()
    return workspace_name


def run_MuonPairingAsymmetry(parameter_dict, workspace_name):
    """
    Apply the MuonPairingAsymmetry algorithm with the properties supplied through
    the input dictionary of {property_name:property_value} pairs.
    Returns the calculated workspace name.
    """
    alg = mantid.AlgorithmManager.create("MuonPairingAsymmetry")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty("OutputWorkspace", workspace_name)
    alg.setProperties(parameter_dict)
    alg.execute()
    return workspace_name


def run_EstimateMuonAsymmetryFromCounts(parameter_dict, workspace_name, unormalised_workspace_name):
    """
    Apply the run_EstimateMuonAsymmetryFromCounts algorithm with the properties supplied through
    the input dictionary of {property_name:property_value} pairs.
    Returns the calculated workspace name.
    """
    alg = mantid.AlgorithmManager.create("EstimateMuonAsymmetryFromCounts")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", workspace_name)
    alg.setProperty("OutputUnNormWorkspace", unormalised_workspace_name)
    alg.setProperties(parameter_dict)
    alg.execute()
    return workspace_name, unormalised_workspace_name


def run_CalMuonDetectorPhases(parameter_dict, alg, fitted_workspace_name):
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setRethrows(True)
    alg.setProperty("DataFitted", fitted_workspace_name)
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("DetectorTable").valueAsStr, alg.getProperty("DataFitted").valueAsStr


def run_PhaseQuad(parameters_dict, phase_quad_workspace_name):
    alg = mantid.AlgorithmManager.create("PhaseQuad")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", phase_quad_workspace_name)
    alg.setProperties(parameters_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_PaddingAndApodization(parameters_dict, output_workspace):
    alg = mantid.AlgorithmManager.create("PaddingAndApodization")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", output_workspace)
    alg.setProperties(parameters_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_FFT(parameters_dict):
    alg = mantid.AlgorithmManager.create("FFT")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", "__NotUsed")
    alg.setProperties(parameters_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_MuonMaxent(parameters_dict, alg, output_workspace_name):
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", output_workspace_name)
    alg.setProperty("OutputPhaseTable", "__NotUsedPhase")
    alg.setProperty("OutputDeadTimeTable", "__NotUsedDead")
    alg.setProperty("ReconstructedSpectra", "__NotUsedRecon")
    alg.setProperty("PhaseConvergenceTable", "__NotUsedConverge")
    alg.setProperties(parameters_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_Fit(parameters_dict, alg):
    create_output = parameters_dict["CreateOutput"] if "CreateOutput" in parameters_dict else True

    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setRethrows(True)
    alg.setProperty("CreateOutput", create_output)
    pruned_parameter_dict = {
        key: value for key, value in parameters_dict.items() if key not in ["InputWorkspace", "StartX", "EndX", "Exclude"]
    }
    alg.setProperties(pruned_parameter_dict)
    alg.setProperty("InputWorkspace", parameters_dict["InputWorkspace"])
    alg.setProperty("StartX", parameters_dict["StartX"])
    alg.setProperty("EndX", parameters_dict["EndX"])
    if "Exclude" in parameters_dict:
        alg.setProperty("Exclude", parameters_dict["Exclude"])
    alg.execute()
    if create_output:
        return (
            alg.getProperty("OutputWorkspace").valueAsStr,
            alg.getProperty("OutputParameters").valueAsStr,
            alg.getProperty("Function").value,
            alg.getProperty("OutputStatus").value,
            alg.getProperty("OutputChi2overDoF").value,
            alg.getProperty("OutputNormalisedCovarianceMatrix").valueAsStr,
        )
    else:
        return alg.getProperty("Function").value, alg.getProperty("OutputStatus").value, alg.getProperty("OutputChi2overDoF").value


def run_simultaneous_Fit(parameters_dict, alg):
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setRethrows(True)
    alg.setProperty("CreateOutput", True)
    pruned_parameter_dict = {
        key: value for key, value in parameters_dict.items() if key not in ["InputWorkspace", "StartX", "EndX", "Exclude"]
    }
    alg.setProperties(pruned_parameter_dict)

    for index, input_workspace in enumerate(parameters_dict["InputWorkspace"]):
        index_str = "_" + str(index) if index else ""
        alg.setProperty("InputWorkspace" + index_str, input_workspace)
        alg.setProperty("StartX" + index_str, parameters_dict["StartX"][index])
        alg.setProperty("EndX" + index_str, parameters_dict["EndX"][index])
        if "Exclude" in parameters_dict:
            alg.setProperty("Exclude" + index_str, parameters_dict["Exclude"][index])

    alg.execute()

    return (
        alg.getProperty("OutputWorkspace").valueAsStr,
        alg.getProperty("OutputParameters").valueAsStr,
        alg.getProperty("Function").value,
        alg.getProperty("OutputStatus").value,
        alg.getProperty("OutputChi2overDoF").value,
        alg.getProperty("OutputNormalisedCovarianceMatrix").valueAsStr,
    )


def run_CalculateMuonAsymmetry(parameters_dict, alg):
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setRethrows(True)
    alg.setProperties(parameters_dict)
    alg.execute()
    return (
        alg.getProperty("OutputWorkspace").valueAsStr,
        alg.getProperty("OutputParameters").valueAsStr,
        alg.getProperty("OutputFunction").value,
        alg.getProperty("OutputStatus").value,
        alg.getProperty("ChiSquared").value,
        alg.getProperty("OutputNormalisedCovarianceMatrix").valueAsStr,
    )


def run_AppendSpectra(ws1, ws2):
    """
    Apply the AppendSpectra algorithm to two given workspaces (no checks made).
    Returns the appended workspace.
    """
    alg = mantid.AlgorithmManager.create("AppendSpectra")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperty("InputWorkspace1", ws1)
    alg.setProperty("InputWorkspace2", ws2)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_AlphaCalc(parameter_dict):
    """
    Apply the AlphaCalc algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated value of alpha.
    """
    alg = mantid.AlgorithmManager.create("AlphaCalc")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("Alpha").value


def run_Plus(parameter_dict):
    """
    Apply the AlphaCalc algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated value of alpha.
    """
    alg = mantid.AlgorithmManager.create("Plus")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


# cannot import the strings from the context as it causes a circular import
def convert_to_field(workspace_name, output_name):
    """
    Apply the ConvertAxisByFormula algorithm to convert from MHz to Field.
    """
    alg = mantid.AlgorithmManager.create("ConvertAxisByFormula")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty("InputWorkspace", workspace_name)
    alg.setProperty("OutputWorkspace", output_name)
    alg.setProperty("Formula", "x * 1. / " + str(const.MuonGyromagneticRatio))
    alg.setProperty("AxisTitle", "Field")
    alg.setProperty("AxisUnits", "Gauss")
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def convert_to_freq(workspace_name, output_name):
    """
    Apply the ConvertAxisByFormula algorithm to convert from Field to MHz.
    """
    alg = mantid.AlgorithmManager.create("ConvertAxisByFormula")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty("InputWorkspace", workspace_name)
    alg.setProperty("OutputWorkspace", output_name)
    alg.setProperty("Formula", "x * " + str(const.MuonGyromagneticRatio))
    alg.setProperty("AxisTitle", "Frequency")
    alg.setProperty("AxisUnits", "MHz")
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def extract_single_spec(ws, spec, output_workspace_name):
    alg = mantid.AlgorithmManager.create("ExtractSingleSpectrum")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty("InputWorkspace", ws)
    alg.setProperty("OutputWorkspace", output_workspace_name)
    alg.setProperty("WorkspaceIndex", spec)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def rebin_ws(ws, params):
    alg = mantid.AlgorithmManager.create("Rebin")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty("InputWorkspace", ws)
    alg.setProperty("OutputWorkspace", ws)
    alg.setProperty("params", params)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def split_phasequad(name):
    Re_name = copy(name).replace(PHASEQUAD_IM, "")
    Im_name = copy(name).replace(PHASEQUAD_RE, "")

    Re = extract_single_spec(name, 0, Re_name)
    Im = extract_single_spec(name, 1, Im_name)
    remove_ws(name)
    return [Re, Im]


def apply_deadtime(ws, output, table):
    alg = mantid.AlgorithmManager.create("ApplyDeadTimeCorr")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty("InputWorkspace", ws)
    alg.setProperty("OutputWorkspace", output)
    alg.setProperty("DeadTimeTable", table)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_minus(lhs_workspace, rhs_workspace, output_name):
    alg = mantid.AlgorithmManager.create("Minus")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty("LHSWorkspace", lhs_workspace)
    alg.setProperty("RHSWorkspace", rhs_workspace)
    alg.setProperty("OutputWorkspace", output_name)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_crop_workspace(ws, start, end):
    alg = mantid.AlgorithmManager.create("CropWorkspace")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("InputWorkspace", ws)
    alg.setProperty("OutputWorkspace", ws)
    alg.setProperty("XMin", start)
    alg.setProperty("XMax", end)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_create_single_valued_workspace(parameter_dict):
    alg = mantid.AlgorithmManager.create("CreateSingleValuedWorkspace")
    alg.initialize()
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_clone_workspace(parameter_dict):
    alg = mantid.AlgorithmManager.create("CloneWorkspace")
    alg.initialize()
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_convert_to_points(name):
    alg = mantid.AlgorithmManager.create("ConvertToPointData")
    alg.initialize()
    alg.setProperty("InputWorkspace", name)
    alg.setProperty("OutputWorkspace", name)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_convert_to_histogram(name):
    alg = mantid.AlgorithmManager.create("ConvertToHistogram")
    alg.initialize()
    alg.setProperty("InputWorkspace", name)
    alg.setProperty("OutputWorkspace", name)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_divide(LHS, RHS, name):
    alg = mantid.AlgorithmManager.create("Divide")
    alg.initialize()
    alg.setProperty("LHSWorkspace", LHS)
    alg.setProperty("RHSWorkspace", RHS)
    alg.setProperty("OutputWorkspace", name)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_create_workspace(x_data, y_data, name):
    alg = mantid.AlgorithmManager.create("CreateWorkspace")
    alg.initialize()
    alg.setProperty("DataX", x_data)
    alg.setProperty("DataY", y_data)
    alg.setProperty("OutputWorkspace", name)
    alg.execute()
    return alg.getProperty("OutputWorkspace").valueAsStr


def run_create_workspace_without_storing(x_data, y_data, name):
    alg = mantid.AlgorithmManager.create("CreateWorkspace")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("DataX", x_data)
    alg.setProperty("DataY", y_data)
    alg.setProperty("OutputWorkspace", name)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def evaluate_function(input_ws_name: str, fun: str, out_ws_name: str):
    """
    Evaluates the guess workspace for the input workspace and function
    :param input_ws_name: Name of the workspace in the ADS
    :param fun: Function to be evaluated
    :param out_ws_name: Output workspace name
    """
    mantid.AnalysisDataService.retrieve(input_ws_name)
    alg = mantid.AlgorithmManager.createUnmanaged("EvaluateFunction")
    alg.initialize()
    alg.setProperty("Function", fun)
    alg.setProperty("InputWorkspace", input_ws_name)
    alg.setProperty("OutputWorkspace", out_ws_name)
    alg.execute()
