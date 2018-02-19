
from __future__ import (absolute_import, division, print_function)
import numpy as np
from Muon.MaxentTools.zft import ZFT

# translation of modab.for
# P unused apart from passing to ZFT which doesn't use it
# params ngroups, npts inferred from size of datum


def MODAB(
     hists,
     datum,
     sigma,
     MISSCHANNELS_mm,
     MAXPAGE_f,
     PULSESHAPE_convol,
     DETECT_e,
     SAVETIME_I2,
     mylog):
    npts, ngroups = datum.shape
    zr, zi = ZFT(MAXPAGE_f, PULSESHAPE_convol, DETECT_e, SAVETIME_I2)
    DETECT_a = np.zeros([ngroups])
    DETECT_b = np.zeros([ngroups])
    # SENSE_phi=np.zeros([ngroups])
    # AMPS_amp=np.zeros([ngroups])
    for j in range(ngroups):
        if(hists[j] != 0):
            x = zr / sigma[:, j]
            y = zi / sigma[:, j]
            s11 = np.sum(x**2)
            s22 = np.sum(y**2)
            s12 = np.sum(x * y)
            sa = np.sum(datum[:, j] * x / sigma[:, j])
            sb = np.sum(datum[:, j] * y / sigma[:, j])
            DETECT_a[j] = (sa * s22 - sb * s12) / (s11 * s22 - s12 * s12)
            DETECT_b[j] = (sb * s11 - sa * s12) / (s11 * s22 - s12 * s12)
    AMPS_amp = np.sqrt(DETECT_a**2 + DETECT_b**2)
    SENSE_phi = np.arctan2(DETECT_b, DETECT_a)  # *180.0/math.pi
    s = np.sum(AMPS_amp) / float(ngroups - MISSCHANNELS_mm)
    mylog.notice("AMPLITUDES" + str(AMPS_amp / s))
    mylog.notice("PHASES =" + str(SENSE_phi))
    AMPS_amp = AMPS_amp / s
    DETECT_a = DETECT_a / s
    DETECT_b = DETECT_b / s
    return (SENSE_phi, DETECT_a, DETECT_b, AMPS_amp)
