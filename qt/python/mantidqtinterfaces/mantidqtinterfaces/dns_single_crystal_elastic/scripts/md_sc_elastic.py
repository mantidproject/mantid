# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS script helpers for elastic powder reduction
"""

# pylint: disable=unused-import
from mantidqtinterfaces.dns_powder_elastic.scripts.md_powder_elastic \
    import fliping_ratio_correction, raise_error, background_substraction
__all__ = ('background_substraction','fliping_ratio_correction')

import numpy as np
from mantid.simpleapi import (BinMD, CreateSingleValuedWorkspace, DivideMD,
                              LoadDNSSCD, MultiplyMD, mtd)


def load_all(data_dict, binning, params, standard=False):
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
                load_binned(workspacename, binning, params, path, filenumbers,
                            standard)
    return workspacenames


def load_binned(workspacename, binning, params, path, filenumbers, standard):
    # pylint: disable=too-many-arguments
    """Loading of multiple DNS datafiles into a single workspace
    """
    ad0 = f"Theta,{binning['twoTheta'][0] / 2.0}," \
          f"{binning['twoTheta'][1] / 2.0}," \
          f"{binning['twoTheta'][2]}"
    ad1 = f"Omega,{binning['Omega'][0]}," \
          f"{binning['Omega'][1]}," \
          f"{binning['Omega'][2]}"
    filepaths = [f"{path}_{number:06d}.d_dat" for number in list(filenumbers)]
    filepaths = ', '.join(filepaths)
    normname = "_".join((workspacename, 'norm'))
    if workspacename.endswith('_sf'):
        fieldname = workspacename[-4:]
    else:
        fieldname = workspacename[-5:]
    if not standard:
        LoadDNSSCD(FileNames=filepaths,
                   OutputWorkspace=workspacename,
                   NormalizationWorkspace=normname,
                   Normalization=params['norm_to'],
                   a=params['a'],
                   b=params['b'],
                   c=params['c'],
                   alpha=params['alpha'],
                   beta=params['beta'],
                   gamma=params['gamma'],
                   OmegaOffset=params['omega_offset'],
                   HKL1=params['hkl1'],
                   HKL2=params['hkl2'],
                   LoadAs='raw',
                   SaveHuberTo=f'huber_{fieldname}')
    else:
        LoadDNSSCD(FileNames=filepaths,
                   OutputWorkspace=workspacename,
                   NormalizationWorkspace=normname,
                   Normalization=params['norm_to'],
                   a=params['a'],
                   b=params['b'],
                   c=params['c'],
                   alpha=params['alpha'],
                   beta=params['beta'],
                   gamma=params['gamma'],
                   OmegaOffset=params['omega_offset'],
                   HKL1=params['hkl1'],
                   HKL2=params['hkl2'],
                   LoadAs='raw',
                   LoadHuberFrom=f'huber_{fieldname}')
    BinMD(InputWorkspace=workspacename,
          OutputWorkspace=workspacename,
          AxisAligned=True,
          AlignedDim0=ad0,
          AlignedDim1=ad1)

    BinMD(InputWorkspace=normname,
          OutputWorkspace=normname,
          AxisAligned=True,
          AlignedDim0=ad0,
          AlignedDim1=ad1)

    return mtd[workspacename]


def vanadium_correction(workspacename,
                        vanaset=None,
                        ignore_vana_fields=False,
                        sum_vana_sf_nsf=False):
    # pylint: disable=too-many-locals

    """
    Correction of workspace for detector efficiency, angular coverage, lorentz
    factor based on vanadium data

    Key-Arguments
    vanaset = used Vanadium data, if not given fields matching sample are used
    ignore_vana_fields = if True fields of vanadium files will be ignored
    sum_vana_sf_nsf ) if True SF and NSF channels of vanadium are summed
    """
    vana_sum = None
    vana_sum_norm = None
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
                        raise_error(f'No vanadium file for field {fieldname}.')
                        return mtd[workspacename]
                    vanalist.append(vana)
                    normlist.append(vana_norm)
            vana_sum = sum(vanalist)
            vana_sum_norm = sum(normlist)
        else:
            raise_error(
                'Need to give vanadium dataset explicit if you want all'
                ' vandium files to be added.')
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
                f'No vanadium file for {polarization}_sf . You can choose to'
                f' ignore vanadium fields in the options.')
            return mtd[workspacename]
        try:
            vana_nsf = mtd[vana_nsf]
            vana_nsf_norm = mtd[vana_nsf_norm]
        except KeyError:
            raise_error(
                f'No vanadium file for {polarization}_nsf. You can choose to'
                f' ignore vanadium fields in the options.')
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
            raise_error(f'No vanadium file for {fieldname}. You can choose to '
                        f'ignore vanadium fields in the options.')
            return mtd[workspacename]
    # common code, which will be run regardless of the case

    sum_signal = np.nan_to_num(vana_sum.getSignalArray())
    total_signal = np.sum(sum_signal)
    sum_error = np.nan_to_num(vana_sum.getErrorSquaredArray())
    total_error = np.sum(sum_error)
    sum_signal_norm = np.nan_to_num(vana_sum_norm.getSignalArray())
    total_signal_norm = np.sum(sum_signal_norm)
    sum_error_norm = np.nan_to_num(vana_sum_norm.getErrorSquaredArray())
    total_error_norm = np.sum(sum_error_norm)

    vana_total = CreateSingleValuedWorkspace(DataValue=total_signal,
                                             ErrorValue=np.sqrt(total_error))
    vana_total_norm = CreateSingleValuedWorkspace(
        DataValue=total_signal_norm, ErrorValue=np.sqrt(total_error_norm))

    coef_u = vana_sum / vana_total
    coef_norm = vana_sum_norm / vana_total_norm
    coef = coef_u / coef_norm
    MultiplyMD(coef, workspacenorm, OutputWorkspace=workspacenorm)
    DivideMD(workspacename, workspacenorm, OutputWorkspace=workspacename)
    return mtd[workspacename]
