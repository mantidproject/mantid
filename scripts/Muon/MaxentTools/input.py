# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import math

"""
    raw data was in rdata[hist,bin]
    work in progress depending on who calls INPUT and why...
    transfer data from rdata[hists, raw bins] to datum,datt[groups, bins starting t0 and length npts]
    rdata[itzero]=t0 (might be off the start of the array), rdata[i1stgood]=first good rdata[itotal]=last good
   eliminate intermediate array "coldstore". Even "rdata" is somewhat obsolete...
"""

# parameter p not used!


def INPUT(
    nhists,
    ngroups,
    npts,
    CHANNELS_itzero,
    CHANNELS_i1stgood,
    CHANNELS_itotal,
    RUNDATA_res,
    RUNDATA_frames,
    GROUPING_group,
    DATALL_rdata,
    FAC_factor,
    SENSE_taud,
    mylog,
):
    datum = np.zeros([npts, ngroups])
    sigma = np.zeros([npts, ngroups])
    RUNDATA_hists = np.zeros([ngroups])
    if CHANNELS_i1stgood < CHANNELS_itzero:
        CHANNELS_i1stgood = CHANNELS_itzero
    i1 = CHANNELS_i1stgood - CHANNELS_itzero
    i2 = CHANNELS_itotal - CHANNELS_itzero
    mylog.debug("rdata has shape [{0}]".format(DATALL_rdata.shape))
    mylog.debug("datum has shape [{0}]".format(datum.shape))
    for j in range(nhists):
        idest = GROUPING_group[j]
        if idest >= 0:
            mylog.debug("adding rdata[{0},{1}:{2}] to datum[{3}:{4},{5}]".format(j, CHANNELS_i1stgood, CHANNELS_itotal, i1, i2, idest))
            datum[i1:i2, idest] += DATALL_rdata[j, CHANNELS_i1stgood:CHANNELS_itotal]
            RUNDATA_hists[idest] += 1
    totfcounts = np.sum(datum)
    MISSCHANNELS_mm = np.count_nonzero(RUNDATA_hists == 0)
    gg = np.nonzero(RUNDATA_hists)[0]  # indices of "good" groups with at least one data set
    mylog.notice("Good muons (used for normn. to 10 Mev)={0}".format(totfcounts))
    mylog.debug("group, no. of hists")
    for i in range(ngroups):
        mylog.debug("{0} {1}".format(i, RUNDATA_hists[i]))
    # normalise to 10 Mevents
    RUNDATA_fnorm = 1.0e7 / totfcounts
    sigma[:] = 1.0e15  # default for "no data here"
    sigma[i1:i2, gg] = np.sqrt(datum[i1:i2, gg] + 2.0) * RUNDATA_fnorm
    datum[i1:i2, gg] *= RUNDATA_fnorm
    datt = np.array(datum)
    tau = SENSE_taud / (RUNDATA_res * RUNDATA_hists * RUNDATA_frames * RUNDATA_fnorm)
    overloads = np.argwhere(datum * tau[np.newaxis, :] > 1.0)
    for i, j in overloads:
        mylog.warning("overload [{0},{1}]: count={2} taud={3}".format(i, j, datum[i, j], tau[j]))
    corr = tau[np.newaxis, :] * datum**2
    sigma = np.where(datum > 0.5, sigma * (1.0 + 0.5 * corr / datum), sigma)
    datum = datum + corr

    FAC_ratio = math.sqrt(float((i2 - i1) * len(gg)) / float(npts * (ngroups - MISSCHANNELS_mm)))
    FAC_facfake = FAC_factor * FAC_ratio
    sigma = sigma * FAC_facfake
    zerosigs = np.argwhere(sigma == 0.0)
    for i, j in zerosigs:
        mylog.warning("ZERO SIG!!! [{0},{1}]".format(i, j))

    return (datum, sigma, corr, datt, MISSCHANNELS_mm, RUNDATA_fnorm, RUNDATA_hists, FAC_facfake, FAC_ratio)
