# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# pylint: disable=line-too-long, invalid-name, too-many-locals, unused-variable

"""
Contains a class to calculate the possible reps, resolution and flux for a direct geometry disk chopper
spectrometer. Python implementation by D J Voneshen based on the original Matlab program of R I Bewley.
"""

import numpy as np
import copy


def findLine(chop_times, chopDist, moderator_limits):
    """
    Calculates the lines on the limit of each chopper

    findline(chop_times, chopDist, moderator_limits)

    chop_times: a list of the opening and closing times of the chopper within the time frame
    chopDist: a list of the distance from moderator to chopper in meters
    moderator_limits: the earliest and latest times that neutrons can leave the moderator in microseconds
    """
    lines = []
    for i in range(len(chop_times)):
        # final chopper openings
        leftM = (-chopDist) / (moderator_limits[0] - chop_times[i][0])
        rightM = (-chopDist) / (moderator_limits[1] - chop_times[i][1])
        leftC = -leftM * moderator_limits[0]
        rightC = -rightM * moderator_limits[1]
        if leftM > 0 and rightM > 0:
            lines.append([[leftM, leftC], [rightM, rightC]])
    return lines


def checkPath(chop_times, lines, chopDist, chop5Dist):
    """
    A recursive function to check for lines which can satisfy a window in the next chopper
    """
    if len(chop_times) > 1:
        # recursive bit
        lines = checkPath(chop_times[1:], lines, chopDist[1:], chop5Dist)
    newLines = []
    for line in lines:
        # for each line check to see if there is an opening in the right time window
        # fast first
        earlyT = (chopDist[0] - line[0][1]) / line[0][0]
        # then slow
        lateT = (chopDist[0] - line[1][1]) / line[1][0]

        # then compare this time window to when this chopper is open, keep the range if it is possible
        for i in range(len(chop_times[0])):
            if (chop_times[0][i][0] < earlyT) and ((chop_times[0][i][1] > lateT)):
                # the chopper window is larger than the maximum possible spread, change nothing
                newLines.append(line)
            elif (chop_times[0][i][0] > earlyT) and ((chop_times[0][i][1] < lateT)):
                # both are within the window, draw a new box
                chop5_open = (chop5Dist - line[0][1]) / line[0][0]
                leftM = (chopDist[0] - chop5Dist) / (chop_times[0][i][0] - chop5_open)
                leftC = chop5Dist - leftM * chop5_open
                chop5_close = (chop5Dist - line[1][1]) / line[1][0]
                rightM = (chopDist[0] - chop5Dist) / (chop_times[0][i][1] - chop5_close)
                rightC = chop5Dist - rightM * chop5_close
                newLines.append([[leftM, leftC], [rightM, rightC]])
            elif ((chop_times[0][i][1] < lateT) and (chop_times[0][i][1] > earlyT)) and (chop_times[0][i][0] < earlyT):
                # the left most range is fine but the right most is outside the window. Redefine it
                chop5_close = (chop5Dist - line[1][1]) / line[1][0]
                rightM = (chopDist[0] - chop5Dist) / (chop_times[0][i][1] - chop5_close)
                rightC = chop5Dist - rightM * chop5_close
                newLines.append([line[0], [rightM, rightC]])
            elif (chop_times[0][i][1] > lateT) and ((chop_times[0][i][0] > earlyT) and (chop_times[0][i][0] < lateT)):
                # the leftmost range is outside the chopper window
                chop5_open = (chop5Dist - line[0][1]) / line[0][0]
                leftM = (chopDist[0] - chop5Dist) / (chop_times[0][i][0] - chop5_open)
                leftC = chop5Dist - leftM * chop5_open
                newLines.append([[leftM, leftC], line[1]])
    return newLines


def calcEnergy(lines, samDist):
    """
    Calculates the energies of neutrons which can pass through choppering openings.
    """
    Ei = np.zeros(len(lines))
    massN = 1.674927e-27
    for i in range(len(lines)):
        # look at the middle of the time window
        x0 = -lines[i][0][1] / lines[i][0][0]
        x1 = ((samDist - lines[i][0][1]) / lines[i][0][0] + (samDist - lines[i][1][1]) / lines[i][1][0]) / 2.0
        v = samDist / (x1 - x0)
        Ei[i] = (v * 1e6) ** 2 * massN / 2.0 / 1.60217662e-22
    return Ei


def calcRes(ei, chop_times, lastChopDist, samDist, detDist, guide, slot):
    """
    # for each incident energy work out the moderator and chopper component of the resolution
    """
    res = []
    percent = []
    chwid = []
    modwid = []
    # IMPORTANT POINT
    # The chopper opening times are the full opening, for the resolution we want FWHM
    # consequently divide each by a factor of 2 here
    # END IMPORTANT POINT
    # IMPORTANT POINT 2
    # important point 1 is only valid when guide>=slot
    # if slot>guide the transmission function is a trapezium and it is more complex
    # END  IMPORTANT POINT2
    if guide >= slot:
        chop_width = [(chop_times[0][1] - chop_times[0][0]) / 2.0, (chop_times[1][1] - chop_times[1][0]) / 2.0]
    else:
        totalOpen = chop_times[1][1] - chop_times[1][0]
        flat_time = (slot - guide) * totalOpen / slot
        triangleTime = guide * totalOpen / slot / 2.0  # /2 for FWHM of the triangles
        chop_width = [(chop_times[0][1] - chop_times[0][0]) / 2.0, (flat_time + triangleTime)]
    for energy in ei:
        lamba = np.sqrt(81.81 / energy)
        # this is the experimentally determined FWHM of moderator
        mod_FWHM = -3.143 * lamba**2 + 49.28 * lamba + 0.535
        # the effective width at chopper 1
        mod_eff = 0.6666 * mod_FWHM
        # when running chopper 1 slowly the moderator is smaller than the chopper speed so use that
        if chop_width[0] > mod_eff:
            mod_width = mod_eff
        else:
            mod_width = chop_width[0]
        t_mod_chop = 252.82 * lastChopDist * lamba
        chopRes = (2 * chop_width[1] / t_mod_chop) * ((detDist + samDist + lastChopDist) / detDist)
        modRes = (2 * mod_width / t_mod_chop) * (1 + (samDist / detDist))
        res.append(np.sqrt(chopRes**2 + modRes**2) * energy)
        percent.append(np.sqrt(chopRes**2 + modRes**2))
        chwid.append(chop_width[1])
        modwid.append(mod_width)
    return res, percent, chwid, modwid


def calcFlux(Ei, freq1, percent, slot):
    """
    Looks up flux at a give Ei and resolution (frequency) from a table of measured flux
    """
    lamba = np.sqrt(81.8042 / Ei)
    # here are some constants (hahaha) relating to the instrument
    intRef = 0.885  # the flux at 5meV
    freqRef = 150.0  # the frequency this corresponds to
    refSlot = 20.0  # reference disk slot width in mm
    fluxProf = [
        0.0889,
        0.1003,
        0.1125,
        0.1213,
        0.1274,
        0.1358,
        0.1455,
        0.1562,
        0.1702,
        0.1902,
        0.2149,
        0.2496,
        0.2938,
        0.3537,
        0.4315,
        0.5244,
        0.6415,
        0.7856,
        0.9341,
        1.0551,
        1.1437,
        1.1955,
        1.2004,
        1.1903,
        1.1662,
        1.1428,
        1.1176,
        1.0875,
        1.0641,
        1.0562,
        1.0242,
        0.9876,
        0.9586,
        0.9415,
        0.924,
        0.8856,
        0.8865,
        0.8727,
        0.842,
        0.8125,
        0.7849,
        0.7596,
        0.7417,
        0.7143,
        0.6869,
        0.6608,
        0.6341,
        0.6073,
        0.581,
        0.5548,
        0.5304,
        0.507,
        0.4849,
        0.4639,
        0.4445,
        0.425,
        0.407,
        0.3902,
        0.3737,
        0.3579,
        0.3427,
        0.3274,
        0.3129,
        0.2989,
        0.2854,
        0.2724,
        0.2601,
        0.2483,
        0.2371,
        0.2267,
        0.2167,
        0.2072,
        0.1984,
        0.19,
        0.1821,
        0.1743,
        0.1669,
        0.1599,
        0.1532,
        0.1467,
        0.1404,
        0.1346,
        0.1291,
        0.1238,
        0.1189,
        0.1141,
        0.1097,
        0.1053,
        0.1014,
        0.0975,
        0.0938,
        0.0902,
        0.0866,
        0.0834,
        0.0801,
        0.077,
        0.0741,
        0.0712,
        0.0686,
        0.066,
        0.0637,
        0.0614,
        0.0593,
        0.0571,
        0.0551,
        0.0532,
        0.0512,
        0.0494,
        0.0477,
        0.0461,
        0.0445,
        0.043,
        0.0415,
        0.0401,
        0.0387,
    ]
    fluxLamba = np.linspace(0.5, 11.9, num=len(fluxProf))
    flux = []
    lamba = lamba if hasattr(lamba, "__len__") else [lamba]
    for j in range(len(lamba)):
        i = (abs(fluxLamba - lamba[j])).argmin()
        intensity = fluxProf[i]
        if percent[j] < 0.02:
            flux.append(5.6e4 * intensity / intRef * (slot / refSlot) * (freqRef / freq1) ** 2)
        else:
            flux.append(5.6e4 * intensity / intRef * (slot / refSlot) * (freqRef / freq1))
    return flux


def calcChopTimes(efocus, freq, instrumentpars, chop2Phase=5, phaseOffset=None):
    """
    A method to calculate the various possible incident energies with a given chopper setup on LET.
    The window of energy transfers plotted is 85% by default.
    efocus: The incident enrgy that all choppers are focussed on
    freq1: The frequency of the resolution choppers
    freqpr: frequency of the pulse removal chopper
    instrumentpars: a list of instrument parameters [see Instruments.py]
    chop2Phase: the second choppers phase, adjustable to take the guessing out

    Original Matlab code R. Bewley STFC
    Rewritten in Python, D Voneshen STFC 2015
    """
    # conversion factors
    lam2TOF = 252.7784  # the conversion from wavelength to TOF at 1m, multiply by distance
    uSec = 1e6  # seconds to microseconds
    lam = np.sqrt(81.8042 / efocus)  # convert from energy to wavelenth

    # extracts the instrument parameters
    dist, nslot, slots_ang_pos, slot_width, guide_width, radius, numDisk = tuple(instrumentpars[:7])
    samp_det, chop_samp, rep, tmod, frac_ei, ph_ind_v = tuple(instrumentpars[7:])

    chop_times = []  # empty list to hold each chopper opening period

    # figures out phase information
    if not hasattr(ph_ind_v, "__len__"):
        ph_ind_v = [ph_ind_v]
    ph_ind = np.array([False] * len(dist))
    if not len(ph_ind_v) == len(dist) and ph_ind_v:
        ph_ind[ph_ind_v] = True
        # For Merlin, subtract the experimental offset of 4500
        # This value can be set as phaseOffset in the input file for Merlin
        if phaseOffset is not None:
            chop2Phase[0] -= phaseOffset
        chop2Phase = phase = chop2Phase if hasattr(chop2Phase, "__len__") else [chop2Phase]
        if len(chop2Phase) != len(dist):
            if len(chop2Phase) == len(ph_ind_v):
                phase = [False] * len(dist)
                for i in range(len(ph_ind_v)):
                    phase[ph_ind_v[i]] = chop2Phase[i]
            else:
                phase = [chop2Phase[-1] if ph_ind[i] else False for i in range(len(dist))]

    # do we want multiple frames?
    source_rep, nframe = tuple(rep[:2]) if (hasattr(rep, "__len__") and len(rep) > 1) else (rep, 1)
    p_frames = source_rep / nframe

    # first we optimise on the main Ei
    for i in range(len(dist)):
        # loop over each chopper
        # checks whether this chopper should have an independently set phase / delay
        islt = int(phase[i]) if (ph_ind[i] and isinstance(phase[i], str)) else 0
        if ph_ind[i] and not isinstance(phase[i], str):
            # effective chopper velocity (if 2 disks effective velocity is double)
            chopVel = 2 * np.pi * radius[i] * numDisk[i] * freq[i]
            # full opening time
            t_full_op = uSec * (slot_width[i] + guide_width[i]) / chopVel
            realTimeOp = np.array([phase[i], phase[i] + t_full_op])
        else:
            # the opening time of the chopper so that it is open for the focus wavelength
            t_open = lam2TOF * lam * dist[i]
            # effective chopper velocity (if 2 disks effective velocity is double)
            chopVel = 2 * np.pi * radius[i] * numDisk[i] * freq[i]
            # full opening time
            t_full_op = uSec * (slot_width[i] + guide_width[i]) / chopVel
            # set the chopper phase to be as close to zero as possible
            realTimeOp = np.array([(t_open - t_full_op / 2.0), (t_open + t_full_op / 2.0)])
        chop_times.append([])
        if slots_ang_pos and nslot[i] > 1 and slots_ang_pos[i]:
            tslots = [(uSec * slots_ang_pos[i][j] / 360.0 / freq[i]) for j in range(nslot[i])]
            tslots = [[(t + r * (uSec / freq[i])) - tslots[0] for r in range(int(freq[i] / p_frames))] for t in tslots]
            realTimeOp -= np.max(tslots[islt % nslot[i]])
            islt = 0
            next_win_t = uSec / source_rep + (uSec / freq[i])
            while realTimeOp[0] < next_win_t:
                chop_times[i].append(copy.deepcopy(realTimeOp[:]))
                slt0 = islt % nslot[i]
                slt1 = (islt + 1) % nslot[i]
                angdiff = slots_ang_pos[i][slt1] - slots_ang_pos[i][slt0]
                if (slt1 - slt0) != 1:
                    angdiff += 360
                realTimeOp += uSec * (angdiff / 360.0) / freq[i]
                islt += 1
        else:
            # If angular positions of slots not defined, assumed evenly spaced (LET, MERLIN)
            next_win_t = uSec / (nslot[i] * freq[i])
            realTimeOp -= next_win_t * np.ceil(realTimeOp[0] / next_win_t)
            while realTimeOp[0] < (uSec / p_frames + next_win_t):
                chop_times[i].append(copy.deepcopy(realTimeOp[:]))
                realTimeOp += next_win_t
    # then we look for what else gets through
    # firstly calculate the bounding box for each window in final chopper
    lines_all = []
    for i in range(nframe):
        t0 = i * uSec / source_rep
        lines = findLine(chop_times[-1], dist[-1], [t0, t0 + tmod])
        lines = checkPath([np.array(ct) + t0 for ct in chop_times[0:-1]], lines, dist[:-1], dist[-1])
        if lines:
            for line in lines:
                lines_all.append(line)
    # ok, now we know the possible neutron velocities. we now need their energies
    Ei = calcEnergy(lines_all, (dist[-1] + chop_samp))

    return Ei, chop_times, [chop_times[0][0], chop_times[-1][0]], dist[-1] - dist[0], lines_all
