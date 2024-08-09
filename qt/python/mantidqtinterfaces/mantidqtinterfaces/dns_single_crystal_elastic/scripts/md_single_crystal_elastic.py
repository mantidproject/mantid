# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script helpers for reduction of single crystal elastic data.
"""

from mantid.simpleapi import BinMD, LoadDNSSCD, mtd


def load_all(data_dict, binning, params, standard=False):
    """
    Loading of multiple DNS files given in a dictionary to workspaces.
    """
    workspace_names = {}
    for sample_name, fields in data_dict.items():
        workspace_names[sample_name] = []
        path = data_dict[sample_name]["path"]
        for field_name, file_numbers in fields.items():
            if field_name != "path":
                workspace_name = "_".join((sample_name, field_name))
                workspace_names[sample_name].append(workspace_name)
                load_binned(workspace_name, binning, params, path, file_numbers, standard)
    return workspace_names


def load_binned(workspace_name, binning, params, path, file_numbers, standard):
    """
    Loading of multiple DNS datafiles into a single workspace.
    """
    ad0 = f"Theta,{binning['two_theta_binning'][0] / 2.0},{binning['two_theta_binning'][1] / 2.0},{binning['two_theta_binning'][2]}"
    ad1 = f"Omega,{binning['omega_binning'][0]},{binning['omega_binning'][1]},{binning['omega_binning'][2]}"
    filepaths = [path.replace("*" * len(str(number)), str(number)) for number in list(file_numbers)]
    filepaths = ", ".join(filepaths)
    norm_name = "_".join((workspace_name, "norm"))
    if workspace_name.endswith("_sf"):
        field_name = workspace_name[-4:]
    else:
        field_name = workspace_name[-5:]
    LoadDNSSCD(
        FileNames=filepaths,
        OutputWorkspace=workspace_name,
        NormalizationWorkspace=norm_name,
        Normalization=params["norm_to"],
        a=params["a"],
        b=params["b"],
        c=params["c"],
        alpha=params["alpha"],
        beta=params["beta"],
        gamma=params["gamma"],
        OmegaOffset=params["omega_offset"],
        HKL1=params["hkl1"],
        HKL2=params["hkl2"],
        LoadAs="raw",
        SaveHuberTo=f"huber_{field_name}",
    )
    BinMD(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, AxisAligned=True, AlignedDim0=ad0, AlignedDim1=ad1)
    BinMD(InputWorkspace=norm_name, OutputWorkspace=norm_name, AxisAligned=True, AlignedDim0=ad0, AlignedDim1=ad1)
    return mtd[workspace_name]
