
from __future__ import (absolute_import, division, print_function)
import numpy as np
from Muon.MaxentTools.zft import ZFT


def MODBAK(
     hists,
     datum,
     sigma,
     DETECT_a,
     DETECT_b,
     DETECT_e,
     DETECT_d,
     MAXPAGE_f,
     PULSESHAPE_convol,
     SAVETIME_I2,
     mylog):
    npts, ngroups = datum.shape
    DETECT_c = np.zeros([ngroups])
    zr, zi = ZFT(MAXPAGE_f, PULSESHAPE_convol, DETECT_e, SAVETIME_I2)
    for j in range(ngroups):
        if(hists[j] != 0):
            isigsq = np.where(sigma[:, j] > 1.E3, 0.0, sigma[:, j]**-2)
            diff = datum[:, j] - (DETECT_a[j] * zr + DETECT_b[j] * zi)
            s = np.sum(diff * DETECT_e * isigsq)
            t = np.sum(isigsq * DETECT_e)
            scale = s / t
            DETECT_c[j] = scale
            DETECT_d[j] = DETECT_d[j] + scale
            datum[:, j] = datum[:, j] - scale * \
                np.where(sigma[:, j] > 1.E3, 0.0, DETECT_e)
    mylog.debug("exponentials:" + str(DETECT_d))
    mylog.debug("changes this cycle:" + str(DETECT_c))
    return DETECT_c, DETECT_d
