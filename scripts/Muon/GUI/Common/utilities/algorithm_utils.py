# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
from mantid.kernel import Logger

muon_logger = Logger('Muon-Algs')


def run_MuonPreProcess(parameter_dict):
    """
    Apply the MuonPreProcess algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated workspace.
    """
    alg = mantid.AlgorithmManager.create("MuonPreProcess")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_MuonGroupingCounts(parameter_dict):
    """
    Apply the MuonGroupingCounts algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated workspace.
    """
    alg = mantid.AlgorithmManager.create("MuonGroupingCounts")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_MuonPairingAsymmetry(parameter_dict):
    """
    Apply the MuonPairingAsymmetry algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated workspace.
    """
    alg = mantid.AlgorithmManager.create("MuonPairingAsymmetry")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_MuonGroupingAsymmetry(parameter_dict):
    """
    Apply the MuonGroupingCounts algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated workspace.
    """
    alg = mantid.AlgorithmManager.create("MuonGroupingAsymmetry")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_CalMuonDetectorPhases(parameter_dict):
    alg = mantid.AlgorithmManager.create("CalMuonDetectorPhases")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setRethrows(True)
    alg.setProperty("DataFitted", "__NotUsed")
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("DetectorTable").value


def run_PhaseQuad(parameters_dict):
    alg = mantid.AlgorithmManager.create('PhaseQuad')
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", "__NotUsed")
    alg.setProperties(parameters_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_PaddingAndApodization(parameters_dict):
    alg = mantid.AlgorithmManager.create("PaddingAndApodization")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", "__NotUsed")
    alg.setProperties(parameters_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_FFT(parameters_dict):
    alg = mantid.AlgorithmManager.create("FFT")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", "__NotUsed")
    alg.setProperties(parameters_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_MuonMaxent(parameters_dict, alg):
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setRethrows(True)
    alg.setProperty("OutputWorkspace", "__NotUsed")
    alg.setProperty("OutputPhaseTable", "__NotUsedPhase")
    alg.setProperty("OutputDeadTimeTable", "__NotUsedDead")
    alg.setProperty("ReconstructedSpectra", "__NotUsedRecon")
    alg.setProperty("PhaseConvergenceTable", "__NotUsedConverge")
    alg.setProperties(parameters_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value

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
