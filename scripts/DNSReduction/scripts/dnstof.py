# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS script helpers for TOF powder reduction
"""
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

# import mantid algorithms
from mantid.simpleapi import LoadDNSLegacy, MergeRuns, DeleteWorkspaces
from mantid.simpleapi import GroupWorkspaces, ConvertUnits, ConvertToMD
from mantid.simpleapi import ConvertToMDMinMaxGlobal, MergeMD, BinMD, mtd
from mantid.simpleapi import ConvertToDistribution, CorrectKiKf

#-------------------------
# helper functions
#-------------------------


def convert_to_dE(gws, Ei):
    """Converting to dE"""
    dEws = '{}_dE'.format(gws)
    ConvertUnits(gws,
                 Target='DeltaE',
                 EMode='Direct',
                 EFixed=Ei,
                 OutputWorkspace=dEws)
    ConvertToDistribution(dEws)
    sws = '{}_dE_S'.format(gws)
    CorrectKiKf(dEws, OutputWorkspace=sws)


def get_sqw(gws_name, outws_name, b):
    """Conversion and Binning of workspace to Q vs dE  """
    gws = mtd[gws_name]
    minvals, maxvals = ConvertToMDMinMaxGlobal(gws[0], '|Q|', 'Direct')
    outws = 'g{}_mde'.format(outws_name)
    ConvertToMD(gws,
                QDimensions='|Q|',
                dEAnalysisMode='Direct',
                PreprocDetectorsWS="-",
                MinValues=minvals,
                MaxValues=maxvals,
                OutputWorkspace=outws)
    outws1 = '{}_mde'.format(outws_name)
    if gws.getNumberOfEntries() > 1:
        MergeMD(outws, OutputWorkspace=outws1)
    else:
        outws1 = outws

    xbins = int((b['qmax'] - b['qmin']) / b['qstep'])
    xmax = b['qmin'] + xbins * b['qstep']
    ad0 = '|Q|,{qmin},{qmax},{qbins}'.format(qmin=b['qmin'],
                                             qmax=xmax,
                                             qbins=xbins)

    ybins = int((b['dEmax'] - b['dEmin']) / b['dEstep'])
    ymax = b['dEmin'] + ybins * b['dEstep']
    ad1 = 'DeltaE,{ymin},{ymax},{ybins}'.format(ymin=b['dEmin'],
                                                ymax=ymax,
                                                ybins=ybins)
    BinMD(InputWorkspace=outws1,
          AlignedDim0=ad0,
          AlignedDim1=ad1,
          OutputWorkspace='{}_sqw'.format(outws_name))


def load_data(data, prefix, p):
    """Loading of multiple DNS powder TOF data in workspaces"""
    wslist = []
    ## bankpositions must be sorted, since script divides based on position
    bankpositions = sorted([x for x in data.keys() if x != 'path'])
    for i, bankposition in enumerate(bankpositions):
        wsname = "{}_{}".format(prefix, i + 1)
        pre_load_data(bankposition, wsname, p, data)
        wslist.append(wsname)
    GroupWorkspaces(wslist, OutputWorkspace=prefix)


def pre_load_data(bankposition, prefix, p, data):
    """
    Loading and merging of multiple DNS powder TOF datafiles into a workspace
    """
    wslist = []
    for rn in data[bankposition]:
        infile = '{0}_{1:06d}.d_dat'.format(data['path'], rn)
        wsname = 'ws_{0:06d}'.format(rn)
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
