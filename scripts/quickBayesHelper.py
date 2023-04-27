# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid.simpleapi as s_api

from typing import List, Dict
from numpy import ndarray
import numpy as np

try:
    from quickBayes.fitting.fit_engine import FitEngine
except (Exception, Warning):
    import subprocess

    print(
        subprocess.Popen(
            "python -m pip install -U --no-deps quickBayes==1.0.0b6",
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdin=subprocess.PIPE,
        ).communicate()
    )
    from quickBayes.fitting.fit_engine import FitEngine

def create_ws(OutputWorkspace, DataX, DataY, NSpec,
              UnitX, YUnitLabel, VerticalAxisUnit, VerticalAxisValues, DataE=None):

    alg = s_api.AlgorithmManager.create("CreateWorkspace")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty('OutputWorkspace', OutputWorkspace)
    alg.setProperty('DataX', DataX)
    alg.setProperty('DataY', DataY)
    alg.setProperty('NSpec', NSpec)
    alg.setProperty('UnitX', UnitX)
    alg.setProperty('YUnitLabel', YUnitLabel)
    alg.setProperty('VerticalAxisUnit', VerticalAxisUnit)
    alg.setProperty('VerticalAxisValues', VerticalAxisValues)
    if DataE is not None:
        alg.setProperty('DataE', DataE)

    alg.execute()
    return alg.getProperty("OutputWorkspace")



def make_fit_ws(engine: FitEngine, max_features: int, ws_list: List, x_unit: str,  name: str) -> List:
    """
    Simple function for creating a fit ws
    :param engine: the quickBayes fit engine used
    :param max_features: the maximum number of features (e.g. lorentzians)
    :param ws_list: list of fit workspaces (inout)
    :param x_unit: the x axis unit
    :param name: name of the output
    :return the list of fitting workspaces
    """
    x = list(engine._x_data)
    y = list(engine._y_data)
    axis_names = ["data"]
    for j in range(max_features):
        x_data, fit, e, df, de = engine.get_fit_values(j)

        y += list(fit) + list(df)
        x += list(x_data) + list(x_data)
        axis_names.append(f"fit {j+1}")
        axis_names.append(f"diff {j+1}")
        create_ws(
            OutputWorkspace=f"{name}_workspace",
            DataX=np.array(x),
            DataY=np.array(y),
            NSpec=len(axis_names),
            UnitX=x_unit,
            YUnitLabel="",
            VerticalAxisUnit="Text",
            VerticalAxisValues=axis_names)
        ws_list.append(f"{name}_workspace")
    return ws_list


def make_results(results: Dict['str', ndarray], results_errors: Dict['str', ndarray],
                 x_data: ndarray, x_unit: str, max_features: int, name: str):
    """
    Takes the output of quickBayes and makes Mantid workspaces
    :param results: dict of quickBayes parameter results
    :param results_errors: dict of quickBayes parameter errors
    :param x_data: the x data for plotting the results (e.g. Q)
    :param x_unit: the x unit
    :param max_features: the maximum number of features used
    :param name: the name of the output worksapce
    :return workspace of fit paramters and workspace of loglikelihoods (probs)
    """
    axis_names = []
    y_data = []
    e_data = []
    prob = []

    for key in results.keys():
        if "loglikelihood" in key:
            prob += list(results[key])
        else:
            y_data.append(results[key])
            e_data.append(results_errors[key])
            axis_names.append(key)

    params = create_ws(
        OutputWorkspace=f"{name}_results",
        DataX=np.array(x_data),
        DataY=np.array(y_data),
        DataE=np.array(e_data),
        NSpec=len(axis_names),
        UnitX=x_unit,
        YUnitLabel="",
        VerticalAxisUnit="Text",
        VerticalAxisValues=axis_names
    )

    prob_ws = create_ws(
        OutputWorkspace=f"{name}_prob",
        DataX=np.array(x_data),
        DataY=np.array(prob),
        NSpec=max_features,
        UnitX=x_unit,
        YUnitLabel="",
        VerticalAxisUnit="Text",
        VerticalAxisValues=[f"{k + 1} feature(s)" for k in range(max_features)]
    )
    return f"{name}_results", f"{name}_prob",


def add_sample_logs(workspace, sample_logs: List, data_ws):
    """
    Method for adding sample logs to results
    :param workspace: the workspace to add sample logs too
    :param sample_logs: new sample logs to add
    :param data_ws: the workspace to copy sample logs from
    """
    for k in sample_logs:
        print("mooo", k)
    print()
    #s_api.CopyLogs(InputWorkspace=data_ws, OutputWorkspace=workspace)
    #s_api.AddSampleLogMultiple(Workspace=workspace, LogNames=[log[0] for log in sample_logs], LogValues=[log[1] for log in sample_logs])
