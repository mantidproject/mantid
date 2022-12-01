# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS script helpers for reduction of elastic powder data.
"""

import numpy as np

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_error import \
    DNSError
from mantid.simpleapi import (BinMD, CreateSingleValuedWorkspace, DivideMD,
                              GroupWorkspaces, IntegrateMDHistoWorkspace,
                              LoadDNSSCD, MinusMD, MultiplyMD, PlusMD, mtd,
                              DeleteWorkspace, SetMDUsingMask)


def background_subtraction(workspace_name, factor=1):
    """
    Subtraction of background files form other workspaces.
    """
    if workspace_name.startswith('empty'):
        return None
    if workspace_name.endswith('_sf'):
        field_name = workspace_name[-4:]
    else:
        field_name = workspace_name[-5:]
    background_name = '_'.join(('empty', field_name))
    workspace_norm = '_'.join((workspace_name, 'norm'))
    background_norm = '_'.join((background_name, 'norm'))
    try:
        mtd[background_name]
    except KeyError:
        raise_error(f'No background file for {field_name}')
        return mtd[workspace_name]
    ratio = DivideMD(mtd[workspace_norm], mtd[background_norm])
    scaling = ratio * factor
    background_scaled = MultiplyMD(mtd[background_name], scaling)
    mtd[workspace_name] = mtd[workspace_name] - background_scaled
    remove_nan_and_negative_from_ws(workspace_name)
    return mtd[workspace_name]


def flipping_ratio_correction(workspace):
    """
    Given SF channel, SF and NSF are corrected for finite flipping ratio.
    """
    # need to be fixed below
    if workspace.endswith('_nsf'):
        return False
    nsf_workspace = ''.join((workspace[:-2], 'nsf'))
    try:
        mtd[nsf_workspace]
    except KeyError:
        raise_error(f'Flipping ratio correction is selected, but no '
                    f'matching nsf workspace found for {workspace}')
        return False
    sf_workspace = workspace
    sf = sf_workspace[-4:]
    nsf = nsf_workspace[-5:]
    nicr_sf = '_'.join(('nicr', sf))
    nicr_nsf = '_'.join(('nicr', nsf))
    nicr_sf_norm = '_'.join((nicr_sf, 'norm'))
    nicr_nsf_norm = '_'.join((nicr_nsf, 'norm'))
    try:
        mtd[nicr_sf] # pylint: disable=pointless-statement
        mtd[nicr_nsf] # pylint: disable=pointless-statement
    except KeyError:
        raise_error(f'Flipping ratio correction is selected, but no '
                    f'matching NiCr workspace found for {workspace}')
        return False
    new_nicr_sf = DivideMD(nicr_sf, nicr_sf_norm)
    new_nicr_nsf = DivideMD(nicr_nsf, nicr_nsf_norm)

    # 1/k, where k = NSF/SF - 1
    inverse_fr_divider = MinusMD(new_nicr_nsf, new_nicr_sf)
    inverse_fr = DivideMD(new_nicr_sf, inverse_fr_divider)

    # apply correction
    nsf_sf_diff = MinusMD(nsf_workspace, sf_workspace)
    diff_ifr = MultiplyMD(nsf_sf_diff, inverse_fr)
    mtd[sf_workspace] = MinusMD(sf_workspace,
                                diff_ifr,
                                OutputWorkspace=sf_workspace)
    mtd[nsf_workspace] = PlusMD(nsf_workspace,
                                diff_ifr,
                                OutputWorkspace=nsf_workspace)
    return True


def load_all(data_dict, binning, normalize_to='monitor'):
    """
    Loading of multiple DNS files given in a dictionary to workspaces.
    """
    workspace_names = {}
    for sample_name, fields in data_dict.items():
        workspace_names[sample_name] = []
        path = data_dict[sample_name]['path']
        for field_name, file_numbers in fields.items():
            if field_name != 'path':
                workspace_name = "_".join((sample_name, field_name))
                workspace_names[sample_name].append(workspace_name)
                load_binned(workspace_name, binning, path, file_numbers,
                            normalize_to)
    return workspace_names


def load_binned(workspace_name, binning, path, file_numbers, normalize_to):
    """
    Loading of multiple DNS datafiles into a single workspace.
    """
    filepaths_l = []
    ad0 = f'Theta,{binning[0] / 2.0},{binning[1] / 2.0},{binning[2]}'
    two_theta_limits = f"{binning[0]},{binning[1]}"
    ad1 = 'Omega,0.0,359.0,1'
    ad2 = 'TOF,424.0,2000.0,1'
    binning = [ad0, ad1, ad2]
    filepaths_l.append(
        [f"{path}_{number:06d}.d_dat" for number in file_numbers])
    filepaths = ', '.join(filepaths_l[0])
    norm_name = "_".join((workspace_name, 'norm'))
    LoadDNSSCD(filepaths,
               OutputWorkspace=workspace_name,
               NormalizationWorkspace=norm_name,
               Normalization=normalize_to,
               LoadAs="raw",
               TwoThetaLimits=two_theta_limits)
    BinMD(InputWorkspace=workspace_name,
          OutputWorkspace=workspace_name,
          AxisAligned=True,
          AlignedDim0=binning[0],
          AlignedDim1=binning[1],
          AlignedDim2=binning[2])
    BinMD(InputWorkspace=norm_name,
          OutputWorkspace=norm_name,
          AxisAligned=True,
          AlignedDim0=binning[0],
          AlignedDim1=binning[1],
          AlignedDim2=binning[2])
    return mtd[workspace_name]


def raise_error(error):
    raise DNSError(error)


def vanadium_correction(workspace_name,
                        binning,
                        vana_set=None,
                        ignore_vana_fields=False,
                        sum_vana_sf_nsf=False):
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
    workspace_norm = '_'.join((workspace_name, 'norm'))
    if workspace_name.endswith('_sf'):
        field_name = workspace_name[-4:]
    else:
        field_name = workspace_name[-5:]
    if ignore_vana_fields:
        if vana_set:
            vana_list = []
            norm_list = []
            for field in vana_set:
                if field != 'path':
                    vana_name = '_'.join(('vana', field))
                    vana_norm_name = '_'.join((vana_name, 'norm'))
                    try:
                        remove_nan_and_negative_from_ws(vana_name)
                        remove_nan_and_negative_from_ws(vana_norm_name)
                        vana = mtd[vana_name]
                        vana_norm_name = mtd[vana_norm_name]
                    except KeyError:
                        raise_error(f'No vanadium file for field {field_name}.'
                                    ' ')
                        return mtd[workspace_name]
                    vana_list.append(vana)
                    norm_list.append(vana_norm_name)
            vana_sum = sum(vana_list)
            vana_sum_norm = sum(norm_list)
        else:
            raise_error(
                'Need to give vanadium dataset explicit if you want all'
                ' vanadium files to be added.')
    elif sum_vana_sf_nsf:
        polarization = field_name.split('_')[0]
        vana_nsf = '_'.join(('vana', polarization, 'nsf'))
        vana_sf = '_'.join(('vana', polarization, 'sf'))
        vana_nsf_norm = '_'.join((vana_nsf, 'norm'))
        vana_sf_norm = '_'.join((vana_sf, 'norm'))
        try:
            remove_nan_and_negative_from_ws(vana_sf)
            remove_nan_and_negative_from_ws(vana_sf_norm)
            vana_sf = mtd[vana_sf]
            vana_sf_norm = mtd[vana_sf_norm]
        except KeyError:
            raise_error(
                f'No vanadium file for {polarization}_sf .'
                ' You can choose to ignore'
                ' vanadium fields by selecting "Options"'
                ' -> "Detector Efficiency Correction" ->'
                ' "Ignore Vanadium Fields".')
            return mtd[workspace_name]
        try:
            remove_nan_and_negative_from_ws(vana_nsf)
            remove_nan_and_negative_from_ws(vana_nsf_norm)
            vana_nsf = mtd[vana_nsf]
            vana_nsf_norm = mtd[vana_nsf_norm]
        except KeyError:
            raise_error(
                f'No vanadium file for {polarization}_nsf. '
                f'You can choose to ignore'
                ' vanadium fields by selecting "Options"'
                ' -> "Detector Efficiency Correction" ->'
                ' "Ignore Vanadium Fields".')
            return mtd[workspace_name]
        vana_sum = vana_sf + vana_nsf
        vana_sum_norm = vana_sf_norm + vana_nsf_norm
    else:
        vana_name = '_'.join(('vana', field_name))
        vana_norm_name = '_'.join((vana_name, 'norm'))
        try:
            remove_nan_and_negative_from_ws(vana_name)
            remove_nan_and_negative_from_ws(vana_norm_name)
            vana_sum = mtd[vana_name]
            vana_sum_norm = mtd[vana_norm_name]
        except KeyError:
            raise_error(f'No vanadium file for {field_name}.'
                        ' You can choose to ignore'
                        ' vanadium fields by selecting "Options"'
                        ' -> "Detector Efficiency Correction" ->'
                        ' "Ignore Vanadium Fields".')
            return mtd[workspace_name]
    # common code, which will be run regardless of the case
    rounding_limit = 0.05
    int_lower_limit = (binning[0] - rounding_limit) / 2.0
    int_upper_limit = (binning[1] + rounding_limit) / 2.0
    vana_total = IntegrateMDHistoWorkspace(vana_sum,
                                           P1Bin=[int_lower_limit, int_upper_limit],
                                           P2Bin=[],
                                           P3Bin=[])
    vana_total_norm = IntegrateMDHistoWorkspace(vana_sum_norm,
                                                P1Bin=[int_lower_limit, int_upper_limit],
                                                P2Bin=[],
                                                P3Bin=[])
    vana_total = CreateSingleValuedWorkspace(
        DataValue=vana_total.getSignalArray()[0][0][0],
        ErrorValue=np.sqrt(vana_total.getErrorSquaredArray()[0][0][0]))
    vana_total_norm = CreateSingleValuedWorkspace(
        DataValue=vana_total_norm.getSignalArray()[0][0][0],
        ErrorValue=np.sqrt(vana_total_norm.getErrorSquaredArray()[0][0][0]))
    coef_u = vana_sum / vana_total
    coef_norm = vana_sum_norm / vana_total_norm
    coef = coef_u / coef_norm
    MultiplyMD(coef, workspace_norm, OutputWorkspace=workspace_norm)
    DivideMD(workspace_name, workspace_norm, OutputWorkspace=workspace_name)
    return mtd[workspace_name]


def xyz_separation(x_sf, y_sf, z_sf, z_nsf):
    """
    Separation of polarized experiments based on x,y,z SF and z NSF channel.
    """
    sample_name = x_sf[:-5]
    xsf_plus_ysf = PlusMD(x_sf, y_sf)
    twice_zsf = mtd[z_sf] * 2
    three_zsf = mtd[z_sf] * 3
    half_imag = xsf_plus_ysf - twice_zsf
    one_third_inc = three_zsf - xsf_plus_ysf
    one_third_inc = one_third_inc * 0.5
    mag_inc_sum = half_imag + one_third_inc
    mtd[f'{sample_name}_magnetic'] = half_imag * 2
    mtd[f'{sample_name}_spin_incoh'] = one_third_inc * 3
    mtd[f'{sample_name}_nuclear_coh'] = mtd[z_nsf] - mag_inc_sum
    GroupWorkspaces([
        f'{sample_name}_magnetic', f'{sample_name}_spin_incoh',
        f'{sample_name}_nuclear_coh'
    ],
        OutputWorkspace='separated')
    return [
        mtd[f'{sample_name}_nuclear_coh'],
        mtd[f'{sample_name}_magnetic'],
        mtd[f'{sample_name}_spin_incoh']
    ]


def non_magnetic_separation(sf_workspace_name, nsf_workspace_name):
    """
    Separation for non-magnetic samples based on names of workspaces
    for sf and nsf channels.
    """
    sample_name = sf_workspace_name[:-3]
    nuclear_coh_name = '_'.join((sample_name, 'nuclear_coh'))
    spin_incoh_name =  '_'.join((sample_name, 'spin_incoh'))
    mtd[nuclear_coh_name] = mtd[nsf_workspace_name] - 0.5 * mtd[sf_workspace_name]
    mtd[spin_incoh_name] = 1.5 * mtd[sf_workspace_name]
    GroupWorkspaces([mtd[nuclear_coh_name], mtd[spin_incoh_name]],
                    OutputWorkspace='separated')
    return [mtd[nuclear_coh_name], mtd[spin_incoh_name]]


def remove_special_values_ws(ws):
    '''
    Creates a mask for any non-positive values inside the workspace
    (including nans) and uses this mask to replace the corresponding
    values with 0.
    '''
    mask_ws = ~(ws > 0)
    SetMDUsingMask(InputWorkspace=ws,
                   OutputWorkspace=ws,
                   Value='0',
                   MaskWorkspace=mask_ws)
    DeleteWorkspace(mask_ws)
    return ws
