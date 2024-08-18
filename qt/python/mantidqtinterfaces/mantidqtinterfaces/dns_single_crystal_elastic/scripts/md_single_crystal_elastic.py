# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script helpers for reduction of single crystal elastic data.
"""

from mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic import flipping_ratio_correction, raise_error, background_subtraction

__all__ = ("background_subtraction", "flipping_ratio_correction")

import numpy as np
from mantid.simpleapi import BinMD, CreateSingleValuedWorkspace, DivideMD, LoadDNSSCD, MultiplyMD, mtd


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
    sample_fields = params["sample_fields"]
    if workspace_name.endswith("_sf"):
        field_name = workspace_name[-4:]
        sample_fields_to_try = [field for field in sample_fields if field.endswith("_sf")]
    else:
        field_name = workspace_name[-5:]
        sample_fields_to_try = [field for field in sample_fields if field.endswith("_nsf")]
    if not standard:
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
    else:
        try:
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
                LoadHuberFrom=f"huber_{field_name}",
            )
        # handling of std data when sample and vana fields are different but ignore
        # vanadium fields option is selected
        except ValueError:
            if params["ignore_vana"]:
                for field in sample_fields_to_try:
                    try:
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
                            LoadHuberFrom=f"huber_{field}",
                        )
                        # leave the for loop on first success
                        break
                    except ValueError:
                        continue
    BinMD(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, AxisAligned=True, AlignedDim0=ad0, AlignedDim1=ad1)
    BinMD(InputWorkspace=norm_name, OutputWorkspace=norm_name, AxisAligned=True, AlignedDim0=ad0, AlignedDim1=ad1)
    return mtd[workspace_name]


def vanadium_correction(workspace_name, vana_set=None, ignore_vana_fields=False, sum_vana_sf_nsf=False):
    """
    Correction of workspace for detector efficiency, angular coverage, lorentz
    factor based on vanadium data.

    Key arguments:
    vana_set: set of selected vanadium data. If not given, the fields
    matching those of the selected sample data are used.
    ignore_vana_fields: if True, fields of vanadium files will be ignored.
    sum_vana_sf_nsf: if True, SF and NSF channels of vanadium are summed.
    """
    vana_sum = None
    vana_sum_norm = None
    workspace_norm = "_".join((workspace_name, "norm"))
    if workspace_name.endswith("_sf"):
        field_name = workspace_name[-4:]
    else:
        field_name = workspace_name[-5:]
    if ignore_vana_fields:
        if vana_set:
            vana_list = []
            norm_list = []
            for field in vana_set:
                if field != "path":
                    vana_name = "_".join(("vana", field))
                    vana_norm = "_".join((vana_name, "norm"))
                    try:
                        vana = mtd[vana_name]
                        vana_norm = mtd[vana_norm]
                    except KeyError:
                        raise_error(f"No vanadium file for field {field_name}.")
                        return mtd[workspace_name]
                    vana_list.append(vana)
                    norm_list.append(vana_norm)
            vana_sum = sum(vana_list)
            vana_sum_norm = sum(norm_list)
        else:
            raise_error("Need to give vanadium dataset explicit if you want all vanadium files to be added.")
    elif sum_vana_sf_nsf:
        polarization = field_name.split("_")[0]
        vana_nsf = "_".join(("vana", polarization, "nsf"))
        vana_sf = "_".join(("vana", polarization, "sf"))
        vana_nsf_norm = "_".join((vana_nsf, "norm"))
        vana_sf_norm = "_".join((vana_sf, "norm"))
        try:
            vana_sf = mtd[vana_sf]
            vana_sf_norm = mtd[vana_sf_norm]
        except KeyError:
            raise_error(f"No vanadium file for {polarization}_sf . You can choose to ignore vanadium fields in the options.")
            return mtd[workspace_name]
        try:
            vana_nsf = mtd[vana_nsf]
            vana_nsf_norm = mtd[vana_nsf_norm]
        except KeyError:
            raise_error(f"No vanadium file for {polarization}_nsf. You can choose to ignore vanadium fields in the options.")
            return mtd[workspace_name]
        vana_sum = vana_sf + vana_nsf
        vana_sum_norm = vana_sf_norm + vana_nsf_norm
    else:
        vana_name = "_".join(("vana", field_name))
        vana_norm = "_".join((vana_name, "norm"))
        try:
            vana_sum = mtd[vana_name]
            vana_sum_norm = mtd[vana_norm]
        except KeyError:
            raise_error(f"No vanadium file for {field_name}. You can choose to ignore vanadium fields in the options.")
            return mtd[workspace_name]
    # common code, which will be run regardless of the case

    sum_signal = np.nan_to_num(vana_sum.getSignalArray())
    total_signal = np.sum(sum_signal)
    sum_error = np.nan_to_num(vana_sum.getErrorSquaredArray())
    total_error = np.sum(sum_error)
    sum_signal_norm = np.nan_to_num(vana_sum_norm.getSignalArray())
    total_signal_norm = np.sum(sum_signal_norm)
    sum_error_norm = np.nan_to_num(vana_sum_norm.getErrorSquaredArray())
    total_error_norm = np.sum(sum_error_norm)

    vana_total = CreateSingleValuedWorkspace(DataValue=total_signal, ErrorValue=np.sqrt(total_error))
    vana_total_norm = CreateSingleValuedWorkspace(DataValue=total_signal_norm, ErrorValue=np.sqrt(total_error_norm))

    coef_u = vana_sum / vana_total
    coef_norm = vana_sum_norm / vana_total_norm
    coef = coef_u / coef_norm
    MultiplyMD(coef, workspace_norm, OutputWorkspace=workspace_norm)
    DivideMD(workspace_name, workspace_norm, OutputWorkspace=workspace_name)
    return mtd[workspace_name]
