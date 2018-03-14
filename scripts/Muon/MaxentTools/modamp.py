
from __future__ import (absolute_import, division, print_function)
import numpy as np
from Muon.MaxentTools.zft import ZFT

""
#


def MODAMP(
     hists,
     datum,
     sigma,
     MISSCHANNELS_MM,
     FASE_phase,
     MAXPAGE_f,
     PULSESHAPE_convol,
     DETECT_e,
     SAVETIME_I2,
     mylog):
    npts, ngroups = datum.shape
    zr, zi = ZFT(MAXPAGE_f, PULSESHAPE_convol, DETECT_e, SAVETIME_I2)
    cs = np.cos(FASE_phase)
    sn = np.sin(FASE_phase)
    AMPS_amp = np.zeros([ngroups])
    for i in range(ngroups):
        if(hists[i] != 0):
            hij = cs[i] * zr + sn[i] * zi
            s = np.sum(datum[:, i] * hij / (sigma[:, i]**2))
            t = np.sum(hij**2 / (sigma[:, i]**2))
            AMPS_amp[i] = s / t
    v = np.sum(AMPS_amp**2) / float(ngroups - MISSCHANNELS_MM)
    AMPS_amp = AMPS_amp / v
    DETECT_a = AMPS_amp * cs
    DETECT_b = AMPS_amp * sn
    SENSE_phi = np.array(FASE_phase)  # *180.0/np.pi
    mylog.debug("amplitudes" + str(AMPS_amp))
    mylog.debug("fixed phases" + str(SENSE_phi))
    return SENSE_phi, DETECT_a, DETECT_b, AMPS_amp
