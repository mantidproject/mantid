
from __future__ import (absolute_import, division, print_function)
import numpy as np
import math


def START(npts, PULSES_npulse, RUNDATA_res, MAXPAGE_n, TZERO_fine, mylog):
    Tmuon = 2.19704
    Tpion = 0.026
    TAUlife = Tmuon / RUNDATA_res
    DETECT_e = np.exp(-np.arange(npts) / TAUlife)
    # create convolution factor for finite pulse width and 2 pulse
    # WARNING, ISIS specific constants here!
    hpulsew = .05  # subject to accelerator tune, pulse slicers, etc
    pulse2t = .324  # at 800MeV, different at 700MeV
    if(PULSES_npulse == 1):
        pulse2t = 0.0
    fperchan = 1. / (RUNDATA_res * float(npts) * 2.)
    ww = np.arange(MAXPAGE_n) * fperchan * 2. * math.pi
    # GW(I) is parabolic proton pulse ft GW(0)=1
    gw = (
        3 / hpulsew) * (
        np.sin(
            ww * hpulsew) / (
                hpulsew**2 * ww**3) - np.cos(
                    ww * hpulsew) / (
                        hpulsew * ww**2))  # may throw warning about div/0 at i=0
    gw[0] = 1.0
    aa = gw / (1 + (ww * Tpion)**2)
    PULSESHAPE_convolr = aa * \
        (np.cos(ww * pulse2t / 2) - np.tanh(pulse2t / (2 * Tmuon))
         * np.sin(ww * pulse2t / 2) * ww * Tpion)
    PULSESHAPE_convoli = -aa * \
        (np.tanh(pulse2t / (2 * Tmuon)) * np.sin(ww * pulse2t / 2)
         + np.cos(pulse2t * ww / 2) * ww * Tpion)

    PULSESHAPE_convol = PULSESHAPE_convolr + 1.j * PULSESHAPE_convoli
    mylog.notice("convol before time shift:" + str(PULSESHAPE_convol[0:3]))
    # adjust for T0 not being an exact bin boundary (JSL)
    # adjust such that time zero is mean muon arrival as calculated by
    # frequency scan, NOT centre of proton pulse parabola (single pulse case
    # only for now)
    PULSESHAPE_convol = PULSESHAPE_convol * \
        np.exp(1.j * (TZERO_fine + Tpion) * ww)
    mylog.notice("convol after time shift:" + str(PULSESHAPE_convol[0:3]))

    return (DETECT_e, PULSESHAPE_convol)
