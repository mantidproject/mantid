
from __future__ import (absolute_import, division, print_function)
import numpy as np

from Muon.MaxentTools.input import INPUT
from Muon.MaxentTools.start import START
from Muon.MaxentTools.back import BACK
from Muon.MaxentTools.maxent import MAXENT
from Muon.MaxentTools.deadfit import DEADFIT
from Muon.MaxentTools.modbak import MODBAK
from Muon.MaxentTools.modamp import MODAMP
from Muon.MaxentTools.modab import MODAB
from Muon.MaxentTools.outspec import OUTSPEC


def MULTIMAX(
      POINTS_nhists, POINTS_ngroups, POINTS_npts, CHANNELS_itzero, CHANNELS_i1stgood, CHANNELS_itotal, RUNDATA_res, RUNDATA_frames,
      GROUPING_group, DATALL_rdata, FAC_factor, SENSE_taud, MAXPAGE_n, filePHASE,
      PULSES_def, PULSES_npulse, FLAGS_fitdead, FLAGS_fixphase, SAVETIME_i2,
      OuterIter, InnerIter, mylog, prog, phaseconvWS, TZERO_fine,deadDetectors):
    #
    base = np.zeros([MAXPAGE_n])
    (datum, sigma, corr, datt, MISSCHANNELS_mm, RUNDATA_fnorm, RUNDATA_hists, FAC_facfake, FAC_ratio) = INPUT(
        POINTS_nhists, POINTS_ngroups, POINTS_npts, CHANNELS_itzero, CHANNELS_i1stgood, CHANNELS_itotal,
        RUNDATA_res, RUNDATA_frames, GROUPING_group, DATALL_rdata, FAC_factor, SENSE_taud, mylog)

    (DETECT_e, PULSESHAPE_convol) = START(
        POINTS_npts, PULSES_npulse, RUNDATA_res, MAXPAGE_n, TZERO_fine, mylog)

    (datum, DETECT_a, DETECT_b, DETECT_d, FASE_phase) = BACK(
        RUNDATA_hists, datum, sigma, DETECT_e, filePHASE, mylog)
    SAVETIME_ngo = -1
    MAXPAGE_f = None
    for j in range(OuterIter):  # outer "alpha chop" iterations?
        SAVETIME_ngo = SAVETIME_ngo + 1
        mylog.information("CYCLE NUMBER=" + str(SAVETIME_ngo))
        (sigma, base, HERITAGE_iter, MAXPAGE_f, FAC_factor, FAC_facfake) = MAXENT(
            datum, sigma, PULSES_def, base, InnerIter, False,
            SAVETIME_ngo, MAXPAGE_n, MAXPAGE_f, PULSESHAPE_convol, DETECT_a,
            DETECT_b, DETECT_e, FAC_factor, FAC_facfake, SAVETIME_i2, mylog, prog)

        if(FLAGS_fitdead):
            (datum, corr, DETECT_c, DETECT_d, SENSE_taud) = DEADFIT(
                datum, sigma, datt, DETECT_a, DETECT_b, DETECT_d, DETECT_e, RUNDATA_res, RUNDATA_frames, RUNDATA_fnorm, RUNDATA_hists,
                MAXPAGE_n, MAXPAGE_f, PULSESHAPE_convol, SAVETIME_i2, mylog)
        else:
            (DETECT_c, DETECT_d) = MODBAK(RUNDATA_hists, datum, sigma, DETECT_a, DETECT_b,
                                          DETECT_e, DETECT_d, MAXPAGE_f, PULSESHAPE_convol, SAVETIME_i2, mylog)

        if(FLAGS_fixphase):
            (SENSE_phi, DETECT_a, DETECT_b, AMPS_amp) = MODAMP(RUNDATA_hists, datum, sigma,
                                                               MISSCHANNELS_mm, FASE_phase, MAXPAGE_f, PULSESHAPE_convol, DETECT_e,
                                                               SAVETIME_i2, mylog)
        else:
            (SENSE_phi, DETECT_a, DETECT_b, AMPS_amp) = MODAB(RUNDATA_hists, datum, sigma,
                                                              MISSCHANNELS_mm, MAXPAGE_f, PULSESHAPE_convol, DETECT_e, SAVETIME_i2, mylog)
        # outout per-iteration debug info
        if(phaseconvWS):
            offset = 0
            for k in range(POINTS_ngroups+len(deadDetectors)):
                phaseConvX_k = phaseconvWS.dataX(k)
                phaseConvY_k = phaseconvWS.dataY(k)
                if k+1 in deadDetectors:
                    offset+=1
                    phaseConvX_k[j + 1] = (j + 1) * 1.0
                    phaseConvY_k[j + 1] = 0.0
                else:
                    phaseConvX_k[j + 1] = (j + 1) * 1.0
                    phaseConvY_k[j + 1] = SENSE_phi[k-offset]

        prog.report((j + 1) * InnerIter, "")
                    # finished outer loop, jump progress bar

    (OUTSPEC_test, OUTSPEC_guess) = OUTSPEC(datum, MAXPAGE_f, sigma, datt, CHANNELS_itzero, CHANNELS_itotal,
                                            PULSESHAPE_convol, FAC_ratio, DETECT_a, DETECT_b, DETECT_d, DETECT_e, SAVETIME_i2,
                                            RUNDATA_fnorm, mylog)

    return (
                            MISSCHANNELS_mm, RUNDATA_fnorm, RUNDATA_hists, MAXPAGE_f, FAC_factor, FAC_facfake, FAC_ratio,
                            DETECT_a, DETECT_b, DETECT_c, DETECT_d, DETECT_e, PULSESHAPE_convol, SENSE_taud, FASE_phase, SAVETIME_ngo,
                            AMPS_amp, SENSE_phi, OUTSPEC_test, OUTSPEC_guess)
