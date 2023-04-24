# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid.simpleapi as s_api

from typing import List
import numpy as np

try:
    from quickBayes.fitting.fit_engine import FitEngine
except (Exception, Warning):
    import subprocess

    print(
        subprocess.Popen(
            "python -m pip install -U --no-deps quickBayes==1.0.0b5",
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdin=subprocess.PIPE,
        ).communicate()
    )
    from quickBayes.fitting.fit_engine import FitEngine


def make_fit_ws(engine: FitEngine, max_num_peaks: int, name: str, ws_list: List, x_unit: str):
    x = list(engine._x_data)
    y = list(engine._y_data)
    axis_names = ["data"]
    for j in range(max_num_peaks):
        x_data, fit, e, df, de = engine.get_fit_values(j)

        y += list(fit) + list(df)
        x += list(x_data) + list(x_data)
        axis_names.append(f"fit {j+1}")
        axis_names.append(f"diff {j+1}")

    ws_list.append(
        s_api.CreateWorkspace(
            OutputWorkspace=f"{name}_workspace",
            DataX=np.array(x),
            DataY=np.array(y),
            NSpec=len(axis_names),
            UnitX=x_unit,
            YUnitLabel="",
            VerticalAxisUnit="Text",
            VerticalAxisValues=axis_names,
        )
    )
    return ws_list


def make_results(results, results_errors, x_data, x_unit, max_features, name):
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
    params = s_api.CreateWorkspace(
        OutputWorkspace=f"{name}_results",
        DataX=np.array(x_data),
        DataY=np.array(y_data),
        DataE=np.array(e_data),
        NSpec=len(axis_names),
        UnitX=x_unit,
        YUnitLabel="",
        VerticalAxisUnit="Text",
        VerticalAxisValues=axis_names,
    )

    prob_ws = s_api.CreateWorkspace(
        OutputWorkspace=f"{name}_prob",
        DataX=np.array(x_data),
        DataY=np.array(prob),
        NSpec=max_features,
        UnitX=x_unit,
        YUnitLabel="",
        VerticalAxisUnit="Text",
        VerticalAxisValues=[f"{k + 1} feature(s)" for k in range(max_features)],
    )
    return params, prob_ws


def add_sample_logs(workspace, sample_logs, data_ws):
    s_api.CopyLogs(InputWorkspace=data_ws, OutputWorkspace=workspace)
    s_api.AddSampleLogMultiple(Workspace=workspace, LogNames=[log[0] for log in sample_logs], LogValues=[log[1] for log in sample_logs])
