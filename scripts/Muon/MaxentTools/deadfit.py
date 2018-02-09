
from __future__ import (absolute_import, division, print_function)
import numpy as np
from Muon.MaxentTools.zft import ZFT

"""
     Works with the dead time data
"""


def DEADFIT(
            datum, sigma, datt, DETECT_a, DETECT_b, DETECT_d, DETECT_e, RUNDATA_res, RUNDATA_frames, RUNDATA_fnorm, RUNDATA_hists,
            MAXPAGE_n, MAXPAGE_f, PULSESHAPE_convol, SAVETIME_I2, mylog):
    (npts, ngroups) = datt.shape

    zr, zi = ZFT(MAXPAGE_f, PULSESHAPE_convol, DETECT_e, SAVETIME_I2)
    # new numpy array code
    isig = sigma**-2
    wiggle = np.outer(zr, DETECT_a) + np.outer(zi, DETECT_b)
    diff = datt - wiggle
    Ax = np.dot(DETECT_e**2, isig)
    Bx = np.dot(DETECT_e, isig * diff)
    Cx = np.dot(DETECT_e, isig * datt**2)
    Dx = np.sum(datt**4 * isig, axis=0)
    Ex = np.sum(diff * datt**2 * isig, axis=0)
    tau = (Bx * Cx - Ax * Ex) / (Ax * Dx - Cx * Cx)
    scale = (Bx / Ax) + tau * (Cx / Ax)
    SENSE_taud = tau * RUNDATA_res * \
        RUNDATA_hists * RUNDATA_frames * RUNDATA_fnorm
    DETECT_c = scale - DETECT_d
    DETECT_d = scale
    corr = datt**2 * tau[np.newaxis, :]
    datum = np.where(
        sigma <= 1.E3,
        datt +
        corr -
        np.outer(DETECT_e, DETECT_d),
        datum)

    mylog.debug("amplitudes of exponentials:" + str(DETECT_d))
    mylog.debug("changes this cycle:" + str(DETECT_c))
    mylog.debug("DEADTIMES:" + str(SENSE_taud))
    return datum, corr, DETECT_c, DETECT_d, SENSE_taud
