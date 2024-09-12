# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from Muon.MaxentTools.zft import ZFT


def OUTSPEC(
    datum,
    f,
    sigma,
    datt,
    CHANNELS_itzero,
    CHANNELS_itotal,
    PULSESHAPE_convol,
    FAC_ratio,
    DETECT_a,
    DETECT_b,
    DETECT_d,
    DETECT_e,
    SAVETIME_I2,
    RUNDATA_fnorm,
    mylog,
):
    npts, ngroups = datum.shape
    guess = np.zeros([npts, ngroups])
    chi = np.zeros([ngroups])
    zr, zi = ZFT(f, PULSESHAPE_convol, DETECT_e, SAVETIME_I2)
    guess = np.outer(zr, DETECT_a) + np.outer(zi, DETECT_b)
    test0 = (datum - guess) / sigma
    test = test0 * FAC_ratio
    chi = np.sum(test0, axis=0)
    ibads9 = np.greater_equal(test, 5.0)
    chi = chi / float(npts)
    for j in range(ngroups):
        ibads8 = np.nonzero(ibads9[:, j])[0]
        for i in ibads8[0:10]:
            mylog.warning("devn .gt. 5 std devs on point {:4d} of group {:2d}".format(i, j))
        if len(ibads8) > 10:
            mylog.warning("... lots of baddies in group {:2d}".format(j))
    mylog.debug("contribs to chi**2 from each group:" + str(chi))
    nphalf = int(npts / 2)
    tmp1 = np.where(
        np.less_equal(
            sigma[nphalf:,],
            1.0e3,
        ),
        guess[nphalf:, :] / sigma[nphalf:, :] ** 2,
        0.0,
    )
    sum = np.sum(tmp1)
    nsum = np.count_nonzero(tmp1)
    if nsum == 0:
        mylog.debug(" no data in last half, so last half sum = 0")
    else:
        bum = sum / nsum
        mylog.debug("Points:{0}".format(npts))
        mylog.debug("Last half sum:{0}".format(bum))
    # add in backgrounds (no dead time adjustment yet)
    guess = (guess + np.outer(DETECT_e, DETECT_d)) / RUNDATA_fnorm  # convert from 10Mevent normalisation to original stats

    return test, guess
