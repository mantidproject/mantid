# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script helpers for powder TOF reduction.
"""

# import mantid algorithms
from mantid.simpleapi import (
    BinMD,
    ConvertToDistribution,
    ConvertToMD,
    ConvertToMDMinMaxGlobal,
    ConvertUnits,
    CorrectKiKf,
    DeleteWorkspaces,
    GroupWorkspaces,
    LoadDNSLegacy,
    MergeMD,
    MergeRuns,
    mtd,
)


def convert_to_de(gws, e_fixed):
    """
    Converting to dE.
    """
    d_e_ws = f"{gws}_dE"
    ConvertUnits(gws, Target="DeltaE", EMode="Direct", EFixed=e_fixed, OutputWorkspace=d_e_ws)
    ConvertToDistribution(d_e_ws)
    sws = f"{gws}_dE_S"
    CorrectKiKf(d_e_ws, OutputWorkspace=sws)


def get_sqw(gws_name, out_ws_name, b):
    """
    Conversion and binning of workspace to Q vs dE.
    """
    gws = mtd[gws_name]
    min_vals, max_vals = ConvertToMDMinMaxGlobal(gws[0], "|Q|", "Direct")
    out_ws = f"g{out_ws_name}_mde"
    ConvertToMD(
        gws,
        QDimensions="|Q|",
        dEAnalysisMode="Direct",
        PreprocDetectorsWS="-",
        MinValues=min_vals,
        MaxValues=max_vals,
        OutputWorkspace=out_ws,
    )
    out_ws_1 = f"{out_ws_name}_mde"
    if gws.getNumberOfEntries() > 1:
        MergeMD(out_ws, OutputWorkspace=out_ws_1)
    else:
        out_ws_1 = out_ws

    x_bins = int((b["q_max"] - b["q_min"]) / b["q_step"])
    x_max = b["q_min"] + x_bins * b["q_step"]
    ad0 = f"|Q|,{b['q_min']},{x_max},{x_bins}"

    y_bins = int((b["dE_max"] - b["dE_min"]) / b["dE_step"])
    y_max = b["dE_min"] + y_bins * b["dE_step"]
    ad1 = f"DeltaE,{b['dE_min']},{y_max},{y_bins}"
    BinMD(InputWorkspace=out_ws_1, AlignedDim0=ad0, AlignedDim1=ad1, OutputWorkspace=f"{out_ws_name}_sqw")


def load_data(data, prefix, p):
    """
    Loading of multiple DNS powder TOF data in workspaces.
    """
    ws_list = []
    # bank positions must be sorted, since script divides based on position
    bank_positions = sorted([x for x in data.keys() if x != "path"])
    for i, bank_position in enumerate(bank_positions):
        ws_name = f"{prefix}_{i + 1}"
        pre_load_data(bank_position, ws_name, p, data)
        ws_list.append(ws_name)
    GroupWorkspaces(ws_list, OutputWorkspace=prefix)


def pre_load_data(bank_position, prefix, p, data):
    """
    Loading and merging of multiple DNS powder TOF datafiles into a workspace.
    """
    ws_list = []
    for rn in data[bank_position]:
        star_pattern = "*" * len(str(rn))
        in_file = data["path"].replace(star_pattern, str(rn))
        ws_name = f"ws_{rn}"
        if p["wavelength"] > 0:
            LoadDNSLegacy(in_file, Normalization="no", ElasticChannel=p["e_channel"], Wavelength=p["wavelength"], OutputWorkspace=ws_name)
        else:
            LoadDNSLegacy(in_file, Normalization="no", ElasticChannel=p["e_channel"], OutputWorkspace=ws_name)
        ws_list.append(ws_name)

    ws = MergeRuns(ws_list, SampleLogsSum="mon_sum,duration", SampleLogsTimeSeries="deterota,T1,T2,Tsp", OutputWorkspace=prefix)
    if p["delete_raw"]:
        DeleteWorkspaces(ws_list)
    return ws
