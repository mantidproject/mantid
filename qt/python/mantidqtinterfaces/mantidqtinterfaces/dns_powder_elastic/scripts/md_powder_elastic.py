# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script helpers for reduction of elastic powder data.
"""

import numpy as np

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_error import DNSError
from mantid.simpleapi import (
    BinMD,
    CreateSingleValuedWorkspace,
    DivideMD,
    GroupWorkspaces,
    IntegrateMDHistoWorkspace,
    LoadDNSSCD,
    MinusMD,
    MultiplyMD,
    PlusMD,
    mtd,
    DeleteWorkspace,
    SetMDUsingMask,
    logger,
)


def background_subtraction(workspace_name, factor=1):
    """
    Subtraction of background files form other workspaces.
    """
    if workspace_name.startswith("empty"):
        return None
    if workspace_name.endswith("_sf"):
        field_name = workspace_name[-4:]
    else:
        field_name = workspace_name[-5:]
    background_name = "_".join(("empty", field_name))
    workspace_norm_name = "_".join((workspace_name, "norm"))
    background_norm_name = "_".join((background_name, "norm"))
    try:
        mtd[background_name]
    except KeyError:
        raise_error(f"No background file for {field_name}")
        return mtd[workspace_name]
    ratio = DivideMD(mtd[workspace_norm_name], mtd[background_norm_name])
    scaling = ratio * factor
    background_scaled = MultiplyMD(mtd[background_name], scaling)
    MinusMD(mtd[workspace_name], background_scaled, OutputWorkspace=mtd[workspace_name])
    return mtd[workspace_name]


def flipping_ratio_correction(workspace_name):
    """
    Given SF channel, SF and NSF are corrected for finite flipping ratio.
    """
    sf_workspace_name = workspace_name
    if workspace_name.endswith("_nsf"):
        return False
    nsf_workspace_name = "".join((workspace_name[:-2], "nsf"))
    try:
        mtd[nsf_workspace_name]
    except KeyError:
        raise_error(f"Flipping ratio correction is selected, but no matching nsf workspace found for {workspace_name}")
        return False
    sf_field_name = sf_workspace_name[-4:]
    nsf_field_name = nsf_workspace_name[-5:]
    nicr_sf_field_name = "_".join(("nicr", sf_field_name))
    nicr_nsf_field_name = "_".join(("nicr", nsf_field_name))
    nicr_sf_field_norm_name = "_".join((nicr_sf_field_name, "norm"))
    nicr_nsf_field_norm_name = "_".join((nicr_nsf_field_name, "norm"))
    try:
        mtd[nicr_sf_field_name]  # pylint: disable=pointless-statement
        mtd[nicr_nsf_field_name]  # pylint: disable=pointless-statement
    except KeyError:
        raise_error(f"Flipping ratio correction is selected, but no matching NiCr workspace found for {workspace_name}")
        return False
    new_nicr_sf = DivideMD(mtd[nicr_sf_field_name], mtd[nicr_sf_field_norm_name])
    new_nicr_nsf = DivideMD(mtd[nicr_nsf_field_name], mtd[nicr_nsf_field_norm_name])

    # 1/k, where k = NSF/SF - 1
    inverse_fr_divider = MinusMD(new_nicr_nsf, new_nicr_sf)
    inverse_fr = DivideMD(new_nicr_sf, inverse_fr_divider)

    # apply correction
    nsf_sf_diff = MinusMD(mtd[nsf_workspace_name], mtd[sf_workspace_name])
    diff_ifr = MultiplyMD(nsf_sf_diff, inverse_fr)
    MinusMD(mtd[sf_workspace_name], diff_ifr, OutputWorkspace=mtd[sf_workspace_name])
    PlusMD(mtd[nsf_workspace_name], diff_ifr, OutputWorkspace=mtd[nsf_workspace_name])
    return True


def load_all(data_dict, binning, normalize_to="monitor"):
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
                load_binned(workspace_name, binning, path, file_numbers, normalize_to)
    return workspace_names


def load_binned(workspace_name, binning, path, file_numbers, normalize_to):
    """
    Loading of multiple DNS datafiles into a single workspace.
    """
    two_theta_limits = f"{binning[0]},{binning[1]}"
    ad0 = f"Theta,{binning[0] / 2.0},{binning[1] / 2.0},{binning[2]}"
    ad1 = "Omega,0.0,359.0,1"
    ad2 = "TOF,424.0,2000.0,1"
    dimension_binning = [ad0, ad1, ad2]
    filepaths = [path.replace("*" * len(str(number)), str(number)) for number in list(file_numbers)]
    filepaths = ", ".join(filepaths)
    norm_name = "_".join((workspace_name, "norm"))
    LoadDNSSCD(
        filepaths,
        OutputWorkspace=workspace_name,
        NormalizationWorkspace=norm_name,
        Normalization=normalize_to,
        LoadAs="raw",
        TwoThetaLimits=two_theta_limits,
    )
    BinMD(
        InputWorkspace=workspace_name,
        OutputWorkspace=workspace_name,
        AxisAligned=True,
        AlignedDim0=dimension_binning[0],
        AlignedDim1=dimension_binning[1],
        AlignedDim2=dimension_binning[2],
    )
    BinMD(
        InputWorkspace=norm_name,
        OutputWorkspace=norm_name,
        AxisAligned=True,
        AlignedDim0=dimension_binning[0],
        AlignedDim1=dimension_binning[1],
        AlignedDim2=dimension_binning[2],
    )
    return mtd[workspace_name]


def raise_error(error):
    raise DNSError(error)


def vanadium_correction(workspace_name, binning, vana_set=None, ignore_vana_fields=False, sum_vana_sf_nsf=False):
    """
    Correction of workspace for detector efficiency, angular coverage,
    and Lorentz factor based on vanadium data.

    Key-Arguments:
    vana_set: set of selected vanadium data. If not given, the fields
    matching those of the selected sample data are used.
    ignore_vana_fields: if True, the fields of vanadium files will
    be ignored.
    sum_vana_sf_nsf: if True, SF and NSF channels of vanadium are
    summed.
    """
    vana_sum = None
    vana_sum_norm = None
    workspace_norm_name = "_".join((workspace_name, "norm"))
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
                    vana_norm_name = "_".join((vana_name, "norm"))
                    try:
                        vana = mtd[vana_name]
                        vana_norm = mtd[vana_norm_name]
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
        vana_nsf_name = "_".join(("vana", polarization, "nsf"))
        vana_sf_name = "_".join(("vana", polarization, "sf"))
        vana_nsf_norm_name = "_".join((vana_nsf_name, "norm"))
        vana_sf_norm_name = "_".join((vana_sf_name, "norm"))
        try:
            vana_sf = mtd[vana_sf_name]
            vana_sf_norm = mtd[vana_sf_norm_name]
        except KeyError:
            raise_error(
                f"No vanadium file for {polarization}_sf ."
                " You can choose to ignore"
                ' vanadium fields by selecting "Options"'
                ' -> "Detector Efficiency Correction" ->'
                ' "Ignore Vanadium Fields".'
            )
            return mtd[workspace_name]
        try:
            vana_nsf = mtd[vana_nsf_name]
            vana_nsf_norm = mtd[vana_nsf_norm_name]
        except KeyError:
            raise_error(
                f"No vanadium file for {polarization}_nsf. "
                f"You can choose to ignore"
                ' vanadium fields by selecting "Options"'
                ' -> "Detector Efficiency Correction" ->'
                ' "Ignore Vanadium Fields".'
            )
            return mtd[workspace_name]
        vana_sum = vana_sf + vana_nsf
        vana_sum_norm = vana_sf_norm + vana_nsf_norm
    else:
        vana_name = "_".join(("vana", field_name))
        vana_norm_name = "_".join((vana_name, "norm"))
        try:
            vana_sum = mtd[vana_name]
            vana_sum_norm = mtd[vana_norm_name]
        except KeyError:
            raise_error(
                f"No vanadium file for {field_name}."
                " You can choose to ignore"
                ' vanadium fields by selecting "Options"'
                ' -> "Detector Efficiency Correction" ->'
                ' "Ignore Vanadium Fields".'
            )
            return mtd[workspace_name]
    # common code, which will be run regardless of the case
    rounding_limit = 0.05
    int_lower_limit = (binning[0] - rounding_limit) / 2.0
    int_upper_limit = (binning[1] + rounding_limit) / 2.0
    remove_special_values_ws(vana_sum)
    vana_total = IntegrateMDHistoWorkspace(vana_sum, P1Bin=[int_lower_limit, int_upper_limit], P2Bin=[], P3Bin=[])
    remove_special_values_ws(vana_sum_norm)
    vana_total_norm = IntegrateMDHistoWorkspace(vana_sum_norm, P1Bin=[int_lower_limit, int_upper_limit], P2Bin=[], P3Bin=[])
    vana_total = CreateSingleValuedWorkspace(
        DataValue=vana_total.getSignalArray()[0][0][0], ErrorValue=np.sqrt(vana_total.getErrorSquaredArray()[0][0][0])
    )
    vana_total_norm = CreateSingleValuedWorkspace(
        DataValue=vana_total_norm.getSignalArray()[0][0][0], ErrorValue=np.sqrt(vana_total_norm.getErrorSquaredArray()[0][0][0])
    )
    coef_u = vana_sum / vana_total
    coef_norm = vana_sum_norm / vana_total_norm
    coef = coef_u / coef_norm
    MultiplyMD(coef, mtd[workspace_norm_name], OutputWorkspace=mtd[workspace_norm_name])
    DivideMD(mtd[workspace_name], mtd[workspace_norm_name], OutputWorkspace=mtd[workspace_name])
    return mtd[workspace_name]


def xyz_separation(x_sf, y_sf, z_sf, z_nsf):
    """
    Separation of polarized experiments based on names of workspaces
    for x_sf, y_sf, z_sf and z_nsf channels.
    """
    sample_name = x_sf[:-5]
    magnetic_name = "_".join((sample_name, "magnetic"))
    spin_incoh_name = "_".join((sample_name, "spin_incoh"))
    nuclear_coh_name = "_".join((sample_name, "nuclear_coh"))

    x_sf_plus_y_sf = PlusMD(mtd[x_sf], mtd[y_sf])
    twice_z_sf = mtd[z_sf] * 2.0
    three_z_sf = mtd[z_sf] * 3.0
    half_magnetic = x_sf_plus_y_sf - twice_z_sf
    one_third_incoh = 0.5 * (three_z_sf - x_sf_plus_y_sf)

    mtd[magnetic_name] = half_magnetic * 2.0
    mtd[spin_incoh_name] = one_third_incoh * 3.0
    mtd[nuclear_coh_name] = mtd[z_nsf] - half_magnetic - one_third_incoh
    GroupWorkspaces([mtd[magnetic_name], mtd[spin_incoh_name], mtd[nuclear_coh_name]], OutputWorkspace="separated")
    return [mtd[nuclear_coh_name], mtd[magnetic_name], mtd[spin_incoh_name]]


def non_magnetic_separation(sf_workspace_name, nsf_workspace_name):
    """
    Separation for non-magnetic samples based on names of workspaces
    for sf and nsf channels.
    """
    sample_name = sf_workspace_name[:-3]
    nuclear_coh_name = "_".join((sample_name, "nuclear_coh"))
    spin_incoh_name = "_".join((sample_name, "spin_incoh"))
    mtd[nuclear_coh_name] = mtd[nsf_workspace_name] - 0.5 * mtd[sf_workspace_name]
    mtd[spin_incoh_name] = 1.5 * mtd[sf_workspace_name]
    GroupWorkspaces([mtd[nuclear_coh_name], mtd[spin_incoh_name]], OutputWorkspace="separated")
    return [mtd[nuclear_coh_name], mtd[spin_incoh_name]]


def remove_special_values_ws(ws):
    """
    Creates a mask for any +/-infinity or +-nan values inside the
    workspace and uses this mask to replace the corresponding values
    with 0.
    """
    logger.warning(
        f"Warning: 'nan' and 'inf' values in the Workspace {ws} have been replaced by 0 upon a call of the vanadium_correction() function."
    )
    mask_ws = ~((ws > -np.inf) & (ws < np.inf))
    SetMDUsingMask(InputWorkspace=ws, OutputWorkspace=ws, Value="0", MaskWorkspace=mask_ws)
    DeleteWorkspace(mask_ws)
    return ws
