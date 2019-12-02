# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS script helpers for elastic powder reduction
"""
from __future__ import (absolute_import, division, print_function)
import numpy as np

from mantid.simpleapi import LoadDNSSCD, mtd, BinMD, DivideMD, MultiplyMD
from mantid.simpleapi import MinusMD, CreateSingleValuedWorkspace
from mantid.simpleapi import IntegrateMDHistoWorkspace, PlusMD, GroupWorkspaces

from DNSReduction.data_structures.dns_error import DNSError


def background_substraction(workspacename, factor=1):
    """ Subtraction of background files form other workspaces
    """
    if workspacename.startswith('empty'):
        return None
    if workspacename.endswith('_sf'):
        fieldname = workspacename[-4:]
    else:
        fieldname = workspacename[-5:]
    backgroundname = '_'.join(('empty', fieldname))
    workspacenorm = '_'.join((workspacename, 'norm'))
    backgroundnorm = '_'.join((backgroundname, 'norm'))
    try:
        mtd[backgroundname]
    except KeyError:
        raise_error('No background file for {}'.format(fieldname))
        return mtd[workspacename]
    ratio = mtd[workspacenorm] / mtd[backgroundnorm]
    background_scaled = mtd[backgroundname] * ratio * factor
    mtd[workspacename] = mtd[workspacename] - background_scaled
    return mtd[workspacename]


def fliping_ratio_correction(workspace):
    """Given SF channel, SF and NSF are corrected for finite flipping ratio """
    if workspace.endswith('_nsf'):
        return
    nsf_workspace = ''.join((workspace[:-2], 'nsf'))
    try:
        mtd[nsf_workspace]
    except KeyError:
        raise_error('no matching nsf workspace found for {}'.format(workspace))
        return
    sf_workspace = workspace
    sf = sf_workspace[-4:]
    nsf = nsf_workspace[-5:]
    nicr_sf = '_'.join(('nicr', sf))
    nicr_nsf = '_'.join(('nicr', nsf))
    nicr_sf_norm = '_'.join((nicr_sf, 'norm'))
    nicr_nsf_norm = '_'.join((nicr_nsf, 'norm'))
    try:
        mtd[nicr_sf]
        mtd[nicr_nsf]
    except KeyError:
        raise_error(
            'no matching NiCr workspace found for {}'.format(workspace))
        return
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


def load_all(data_dict, binning, normalizeto='monitor', standard = False):
    """Loading of multiple DNS files given in a dictionary to workspaces
    """       
    workspacenames = {}
    for samplename, fields in data_dict.items():
        workspacenames[samplename] = []
        path = data_dict[samplename]['path']
        for fieldname, filenumbers in fields.items():
            if fieldname != 'path':
                workspacename = "_".join((samplename, fieldname))
                workspacenames[samplename].append(workspacename)
                load_binned(workspacename, binning, path, filenumbers,
                            normalizeto)
    return workspacenames


def load_binned(workspacename, binning, path, filenumbers, normalizeto):
    """Loading of multiple DNS datafiles into a single workspace
    """
    filepaths = []
    ad0 = 'Theta,{},{},{}'.format(binning[1] / 2.0, binning[2] / 2.0,
                                  binning[3])
    two_theta_limits = "{},{}".format(binning[1], binning[2])
    ad1 = 'Omega,0.0,359.0,1'
    ad2 = 'TOF,424.0,2000.0,1'
    binning = [ad0, ad1, ad2]
    filepaths.append(
        ["{}_{}.d_dat".format(path, number) for number in filenumbers])
    filepaths = ', '.join(filepaths[0])
    normname = "_".join((workspacename, 'norm'))
    LoadDNSSCD(filepaths,
               OutputWorkspace=workspacename,
               NormalizationWorkspace=normname,
               Normalization=normalizeto,
               LoadAs="raw",
               TwoThetaLimits=two_theta_limits)
    BinMD(InputWorkspace=workspacename,
          OutputWorkspace=workspacename,
          AxisAligned=True,
          AlignedDim0=binning[0],
          AlignedDim1=binning[1],
          AlignedDim2=binning[2])
    BinMD(InputWorkspace=normname,
          OutputWorkspace=normname,
          AxisAligned=True,
          AlignedDim0=binning[0],
          AlignedDim1=binning[1],
          AlignedDim2=binning[2])
    return mtd[workspacename]


def mmtd(workspacename):
    try:
        return mtd[workspacename]
    except KeyError:
        return None


def raise_error(error):
    raise DNSError(error)


def vanadium_correction(workspacename,
                        binning, 
                        vanaset=None,
                        ignore_vana_fields=False,
                        sum_vana_sf_nsf=False):
    """
    Correction of workspace for detector efficiency, angular coverage, lorentz
    factor based on vanadium data

    Key-Arguments
    vanaset = used Vanadium data, if not given fields matching sample are used
    ignore_vana_fields = if True fields of vanadium files will be ignored
    sum_vana_sf_nsf ) if True SF and NSF channels of vanadium are summed
    """
    workspacenorm = '_'.join((workspacename, 'norm'))
    if workspacename.endswith('_sf'):
        fieldname = workspacename[-4:]
    else:
        fieldname = workspacename[-5:]
    if ignore_vana_fields:
        if vanaset:
            vanalist = []
            normlist = []
            for field in vanaset:
                if field != 'path':
                    vananame = '_'.join(('vana', field))
                    vananorm = '_'.join((vananame, 'norm'))
                    try:
                        vana = mtd[vananame]
                        vana_norm = mtd[vananorm]
                    except KeyError:
                        raise_error('No vanadium file for field {}. '.format(
                            fieldname))
                        return mtd[workspacename]
                    vanalist.append(vana)
                    normlist.append(vana_norm)
            vana_sum = sum(vanalist)
            vana_sum_norm = sum(normlist)
        else:
            raise_error(
                'Need to give vanadium dataset explicit if you want all'\
                ' vandium files to be added.'
            )
    elif sum_vana_sf_nsf:
        polarization = fieldname.split('_')[0]
        vana_nsf = '_'.join(('vana', polarization, 'nsf'))
        vana_sf = '_'.join(('vana', polarization, 'sf'))
        vana_nsf_norm = '_'.join((vana_nsf, 'norm'))
        vana_sf_norm = '_'.join((vana_sf, 'norm'))
        try:
            vana_sf = mtd[vana_sf]
            vana_sf_norm = mtd[vana_sf_norm]
        except KeyError:
            raise_error(
                'No vanadium file for {}_sf . You can choose to ignore' \
                ' vanadium fields in the options.'.format(polarization))
            return mtd[workspacename]
        try:
            vana_nsf = mtd[vana_nsf]
            vana_nsf_norm = mtd[vana_nsf_norm]
        except KeyError:
            raise_error(
                'No vanadium file for {}_nsf. You can choose to ignore' \
                ' vanadium fields in the options.'.format(polarization))
            return mtd[workspacename]
        vana_sum = vana_sf + vana_nsf
        vana_sum_norm = vana_sf_norm + vana_nsf_norm
    else:
        vananame = '_'.join(('vana', fieldname))
        vananorm = '_'.join((vananame, 'norm'))
        try:
            vana_sum = mtd[vananame]
            vana_sum_norm = mtd[vananorm]
        except KeyError:
            raise_error('No vanadium file for {}. You can choose to ignore' \
                        ' vanadium fields in the options.'.format(fieldname))
            return mtd[workspacename]
    ### commen code, which will be run regardless of the case
    vana_total = IntegrateMDHistoWorkspace(vana_sum,
                                           P1Bin=[4.7, 124.8],
                                           P2Bin=[])
    vana_total_norm = IntegrateMDHistoWorkspace(vana_sum_norm,
                                                P1Bin=[4.7, 124.8],
                                                P2Bin=[])
    vana_total = CreateSingleValuedWorkspace(
        DataValue=vana_total.getSignalArray()[0][0][0],
        ErrorValue=np.sqrt(vana_total.getErrorSquaredArray()[0][0][0]))
    vana_total_norm = CreateSingleValuedWorkspace(
        DataValue=vana_total_norm.getSignalArray()[0][0][0],
        ErrorValue=np.sqrt(vana_total_norm.getErrorSquaredArray()[0][0][0]))
    coef_u = vana_sum / vana_total
    coef_norm = vana_sum_norm / vana_total_norm
    coef = coef_u / coef_norm
    MultiplyMD(coef, workspacenorm, OutputWorkspace=workspacenorm)
    DivideMD(workspacename, workspacenorm, OutputWorkspace=workspacename)
    return mtd[workspacename]


def xyz_seperation(x_sf, y_sf, z_sf, z_nsf):
    """Seperation of polarized experiments based on x,y,z SF and z NSF channel
    """

    samplename = x_sf[:-5]
    xsf_plus_ysf = PlusMD(x_sf, y_sf)
    twice_zsf = mtd[z_sf] * 2
    three_zsf = mtd[z_sf] * 3
    half_imag = xsf_plus_ysf - twice_zsf
    one_third_inc = three_zsf - xsf_plus_ysf
    one_third_inc = one_third_inc * 0.5
    mag_inc_sum = half_imag + one_third_inc
    mtd['{}_magnetic'.format(samplename)] = half_imag * 2
    mtd['{}_spin_incoh'.format(samplename)] = one_third_inc * 3
    mtd['{}_nuclear_coh'.format(samplename)] = mtd[z_nsf] - mag_inc_sum
    seperated = GroupWorkspaces([
        '{}_magnetic'.format(samplename), '{}_spin_incoh'.format(samplename),
        '{}_nuclear_coh'.format(samplename)
    ])
    return [
        mtd['{}_nuclear_coh'.format(samplename)],
        mtd['{}_magnetic'.format(samplename)],
        mtd['{}_spin_incoh'.format(samplename)]
    ]


def non_mag_sep(sf_workspace, nsf_workspace):
    """Seperation for non magnetic samples based on SF/NSF measurements"""
    samplename = sf_workspace[:-3]
    mtd['{}_nuclear_coh'.format(
        samplename)] = mtd[nsf_workspace] - 0.5 * mtd[sf_workspace]
    mtd['{}_spin_incoh'.format(samplename)] = 1.5 * mtd[sf_workspace]
    seperated = GroupWorkspaces([
        '{}_nuclear_coh'.format(samplename), '{}_spin_incoh'.format(samplename)
    ])
    return [
        mtd['{}_nuclear_coh'.format(samplename)],
        mtd['{}_spin_incoh'.format(samplename)]
    ]
