# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS script helpers for TOF powder reduction
"""

# import mantid algorithms
from mantid.simpleapi import (BinMD, ConvertToDistribution, ConvertToMD,
                              ConvertToMDMinMaxGlobal, ConvertUnits,
                              CorrectKiKf, DeleteWorkspaces, GroupWorkspaces,
                              LoadDNSLegacy, MergeMD, MergeRuns, mtd)


def convert_to_d_e(gws, efixed):
    """Converting to dE"""
    d_e_ws = f'{gws}_dE'
    ConvertUnits(gws,
                 Target='DeltaE',
                 EMode='Direct',
                 EFixed=efixed,
                 OutputWorkspace=d_e_ws)
    ConvertToDistribution(d_e_ws)
    sws = f'{gws}_dE_S'
    CorrectKiKf(d_e_ws, OutputWorkspace=sws)


def get_sqw(gws_name, outws_name, b):
    """Conversion and Binning of workspace to Q vs dE  """
    gws = mtd[gws_name]
    minvals, maxvals = ConvertToMDMinMaxGlobal(gws[0], '|Q|', 'Direct')
    outws = f'g{outws_name}_mde'
    ConvertToMD(gws,
                QDimensions='|Q|',
                dEAnalysisMode='Direct',
                PreprocDetectorsWS="-",
                MinValues=minvals,
                MaxValues=maxvals,
                OutputWorkspace=outws)
    outws1 = f'{outws_name}_mde'
    if gws.getNumberOfEntries() > 1:
        MergeMD(outws, OutputWorkspace=outws1)
    else:
        outws1 = outws

    xbins = int((b['qmax'] - b['qmin']) / b['qstep'])
    xmax = b['qmin'] + xbins * b['qstep']
    ad0 = f"|Q|,{b['qmin']},{xmax},{xbins}"

    ybins = int((b['dEmax'] - b['dEmin']) / b['dEstep'])
    ymax = b['dEmin'] + ybins * b['dEstep']
    ad1 = f"DeltaE,{b['dEmin']},{ymax},{ybins}"
    BinMD(InputWorkspace=outws1,
          AlignedDim0=ad0,
          AlignedDim1=ad1,
          OutputWorkspace=f'{outws_name}_sqw')


def load_data(data, prefix, p):
    """Loading of multiple DNS powder TOF data in workspaces"""
    wslist = []
    # bankpositions must be sorted, since script divides based on position
    bankpositions = sorted([x for x in data.keys() if x != 'path'])
    for i, bankposition in enumerate(bankpositions):
        wsname = f"{prefix}_{i + 1}"
        pre_load_data(bankposition, wsname, p, data)
        wslist.append(wsname)
    GroupWorkspaces(wslist, OutputWorkspace=prefix)


def pre_load_data(bankposition, prefix, p, data):
    """
    Loading and merging of multiple DNS powder TOF datafiles into a workspace
    """
    wslist = []
    for rn in data[bankposition]:
        infile = f"{data['path']}_{rn:06d}.d_dat"
        wsname = f"ws_{rn:06d}"
        if p['wavelength'] > 0:
            LoadDNSLegacy(infile,
                          Normalization='no',
                          ElasticChannel=p['e_channel'],
                          Wavelength=p['wavelength'],
                          OutputWorkspace=wsname)
        else:
            LoadDNSLegacy(infile,
                          Normalization='no',
                          ElasticChannel=p['e_channel'],
                          OutputWorkspace=wsname)
        wslist.append(wsname)

    ws = MergeRuns(wslist,
                   SampleLogsSum='mon_sum,duration',
                   SampleLogsTimeSeries='deterota,T1,T2,Tsp',
                   OutputWorkspace=prefix)
    if p['delete_raw']:
        DeleteWorkspaces(wslist)
    return ws
