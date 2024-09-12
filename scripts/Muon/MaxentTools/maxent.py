# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import math
from Muon.MaxentTools.opus import OPUS
from Muon.MaxentTools.tropus import TROPUS
from Muon.MaxentTools.project import PROJECT
from Muon.MaxentTools.move import MOVE

# translated from MAXENT.for
"""

"""
# common /SPACE/ is used only here and below
# max changed to itermax to avoid overwriting built in function


def warningMsg(values, name, mylog):
    if not np.all(np.isfinite(values)):
        mylog.warning(name + " has some NaNs or Infs")


def MAXENT(
    datum,
    sigma,
    flat,
    base,
    itermax,
    sumfix,
    SAVETIME_ngo,
    MAXPAGE_n,
    MAXPAGE_f,
    PULSESHAPE_convol,
    DETECT_a,
    DETECT_b,
    DETECT_e,
    FAC_factor,
    FAC_facfake,
    SAVETIME_i2,
    mylog,
    prog,
):
    npts, ngroups = datum.shape
    p = npts * ngroups
    xi = np.zeros([MAXPAGE_n, 3])
    eta = np.zeros([npts, ngroups, 3])
    #
    SPACE_blank = flat
    if SPACE_blank != 0:
        base = np.zeros([MAXPAGE_n])
        base[:] = SPACE_blank
    else:
        SPACE_blank = np.mean(base)
    SPACE_chizer = float(p)
    SPACE_chtarg = SPACE_chizer
    HERITAGE_iter = 0
    m = 3
    if SAVETIME_ngo > 0:
        HERITAGE_iter = 1
    else:
        MAXPAGE_f = np.array(base)
    test = 99.0  # temporary for 1st test
    SPACE_chisq = SPACE_chizer * 2.0  # temporary for 1st test
    mylog.debug("entering loop with spectrum from {0} to {1}".format(np.amin(MAXPAGE_f), np.amax(MAXPAGE_f)))
    while HERITAGE_iter <= itermax and (HERITAGE_iter <= 1 or not (test < 0.02 and abs(SPACE_chisq / SPACE_chizer - 1) < 0.01)):  # label 6
        mylog.debug("start loop, iter={} ngo={} test={} chisq={}".format(HERITAGE_iter, SAVETIME_ngo, test, SPACE_chisq / SPACE_chizer))
        mylog.debug("entering loop with spectrum from {0} to {1}".format(np.amin(MAXPAGE_f), np.amax(MAXPAGE_f)))
        ox = OPUS(MAXPAGE_f, SAVETIME_i2, PULSESHAPE_convol, DETECT_a, DETECT_b, DETECT_e)
        warningMsg(ox, "ox", mylog)
        mylog.debug("ox from {0} to {1}".format(np.amin(ox), np.amax(ox)))
        a = ox - datum
        SPACE_chisq = np.sum(a**2 / sigma**2)
        ox = 2 * a / (sigma**2)
        cgrad = TROPUS(ox, SAVETIME_i2, PULSESHAPE_convol, DETECT_a, DETECT_b, DETECT_e)
        warningMsg(cgrad, "cgrad", mylog)
        mylog.debug("cgrad from {0} to {1}".format(np.amin(cgrad), np.amax(cgrad)))
        SPACE_xsum = np.sum(MAXPAGE_f)
        sgrad = -np.log(MAXPAGE_f / base) / SPACE_blank
        warningMsg(sgrad, "sgrad", mylog)
        mylog.debug("sgrad from {0} to {1}".format(np.amin(sgrad), np.amax(sgrad)))
        snorm = math.sqrt(np.sum(sgrad**2 * MAXPAGE_f))
        cnorm = math.sqrt(np.sum(cgrad**2 * MAXPAGE_f))
        tnorm = np.sum(sgrad * cgrad * MAXPAGE_f)
        mylog.debug("snorm={} cnorm={} tnorm={}".format(snorm, cnorm, tnorm))
        a = 1.0
        b = 1.0 / cnorm
        c = 1.0 / cnorm
        if HERITAGE_iter != 0:
            test = math.sqrt(0.5 * abs(1.0 - tnorm / (snorm * cnorm)))
            # also eliminate NaNs!
            if test < 1.0e-7 or not np.isfinite(test):
                test = 1.0e-7
            a = 1.0 / (snorm * 2.0 * test)
            b = 1.0 / (cnorm * 2.0 * test)
        else:
            test = 0.0
        if math.isnan(a) or math.isnan(b) or math.isnan(c):
            mylog.debug("aa={} b={} c={}".format(a, b, c))
            raise ValueError("invalid value: a={} b={} c={}".format(a, b, c))
        mylog.debug("a={} b={} c={}".format(a, b, c))
        xi[:, 0] = MAXPAGE_f * c * cgrad
        xi[:, 1] = MAXPAGE_f * (a * sgrad - b * cgrad)
        warningMsg(xi[:, 0], "xi[,0]", mylog)
        warningMsg(xi[:, 1], "xi[,1]", mylog)
        if sumfix:
            PROJECT(0, MAXPAGE_n, xi)
            PROJECT(1, MAXPAGE_n, xi)
        eta[:, :, 0] = OPUS(xi[:, 0], SAVETIME_i2, PULSESHAPE_convol, DETECT_a, DETECT_b, DETECT_e)
        eta[:, :, 1] = OPUS(xi[:, 1], SAVETIME_i2, PULSESHAPE_convol, DETECT_a, DETECT_b, DETECT_e)
        warningMsg(eta[:, :, 0], "eta[,,0]", mylog)
        warningMsg(eta[:, :, 1], "eta[,,1]", mylog)
        ox = eta[:, :, 1] / (sigma**2)
        xi[:, 2] = TROPUS(ox, SAVETIME_i2, PULSESHAPE_convol, DETECT_a, DETECT_b, DETECT_e)
        warningMsg(xi[:, 2], "xi[,2]", mylog)
        a = 1.0 / math.sqrt(np.sum(xi[:, 2] ** 2 * MAXPAGE_f))
        xi[:, 2] = xi[:, 2] * MAXPAGE_f * a
        if sumfix:
            PROJECT(2, MAXPAGE_n, xi)
        eta[:, :, 2] = OPUS(xi[:, 2], SAVETIME_i2, PULSESHAPE_convol, DETECT_a, DETECT_b, DETECT_e)
        warningMsg(eta[:, :, 2], "eta[,,2]", mylog)
        # loop DO 17, DO 18
        SPACE_s1 = np.dot(sgrad, xi)
        SPACE_c1 = np.dot(cgrad, xi) / SPACE_chisq
        # loops DO 19,DO 20, DO 21 harder to completely remove
        SPACE_s2 = np.zeros([3, 3])
        SPACE_c2 = np.zeros([3, 3])
        for L in range(m):
            for k in range(m):
                if L <= k:
                    SPACE_s2[k, L] = -np.sum(xi[:, k] * xi[:, L] / MAXPAGE_f) / SPACE_blank
                    SPACE_c2[k, L] = np.sum(eta[:, :, k] * eta[:, :, L] / sigma**2) * 2.0 / SPACE_chisq
                else:  # make symmetric
                    SPACE_s2[k, L] = SPACE_s2[L, k]
                    SPACE_c2[k, L] = SPACE_c2[L, k]
        s = -np.sum(MAXPAGE_f * np.log(MAXPAGE_f / (base * math.e))) / (SPACE_blank * math.e)  # spotted missing minus sign!
        a = s * SPACE_blank * math.e / SPACE_xsum
        mylog.notice(
            "{:3}    {:10.4}  {:10.4}  {:10.4}  {:10.4}  {:10.4}".format(HERITAGE_iter, test, s, SPACE_chtarg, SPACE_chisq, SPACE_xsum)
        )
        SPACE_beta = np.array([-0.5 * SPACE_c1[0] / SPACE_c2[0, 0], 0.0, 0.0])
        warningMsg(SPACE_beta, "SPACE_beta", mylog)
        if HERITAGE_iter != 0:
            (sigma, SPACE_chtarg, SPACE_beta, FAC_factor, FAC_facfake) = MOVE(
                sigma,
                SPACE_chisq,
                SPACE_chizer,
                SPACE_xsum,
                SPACE_c1,
                SPACE_c2,
                SPACE_s1,
                SPACE_s2,
                SPACE_blank,
                FAC_factor,
                FAC_facfake,
                mylog,
            )
        # do 23,24
        MAXPAGE_f += np.dot(xi, SPACE_beta)
        MAXPAGE_f = np.where(MAXPAGE_f < 0, 1.0e-3 * SPACE_blank, MAXPAGE_f)
        a = np.sum(MAXPAGE_f)
        if sumfix:
            MAXPAGE_f /= a
        # 50
        HERITAGE_iter += 1
        prog.report("chisq=" + str(SPACE_chisq))

    return (sigma, base, HERITAGE_iter, MAXPAGE_f, FAC_factor, FAC_facfake)
