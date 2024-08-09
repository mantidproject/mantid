# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-arguments, redefined-builtin, too-many-locals

"""
Force for ILL backscattering raw
"""

from mantid.simpleapi import *
from mantid import config, logger, mtd, FileFinder
from mantidqt.plotting.functions import pcolormesh
import sys
import math
import os.path
import numpy as np
from IndirectCommon import extract_float, extract_int, get_efixed


#  Routines for Ascii file of raw data


def Iblock(a, first):  # read Ascii block of Integers
    line1 = a[first]
    line2 = a[first + 1]
    val = extract_int(line2)
    numb = val[0]
    lines = numb // 10
    last = numb - 10 * lines
    if line1.startswith("I"):
        error = ""
    else:
        error = "NOT an I block starting at line " + str(first)
        logger.information("ERROR *** " + error)
        sys.exit(error)
    ival = []
    for m in range(0, lines):
        mm = first + 2 + m
        val = extract_int(a[mm])
        for n in range(0, 10):
            ival.append(val[n])
    mm += 1
    val = extract_int(a[mm])
    for n in range(0, last):
        ival.append(val[n])
    mm += 1
    return mm, ival  # values as list


def Fblock(a, first):  # read Ascii block of Floats
    line1 = a[first]
    line2 = a[first + 1]
    val = extract_int(line2)
    numb = val[0]
    lines = numb // 5
    last = numb - 5 * lines
    if line1.startswith("F"):
        error = ""
    else:
        error = "NOT an F block starting at line " + str(first)
        logger.information("ERROR *** " + error)
        sys.exit(error)
    fval = []
    for m in range(0, lines):
        mm = first + 2 + m
        val = extract_float(a[mm])
        for n in range(0, 5):
            fval.append(val[n])
    mm += 1
    val = extract_float(a[mm])
    for n in range(0, last):
        fval.append(val[n])
    mm += 1
    return mm, fval  # values as list


def ReadIbackGroup(a, first):  # read Ascii block of spectrum values
    x = []
    y = []
    e = []
    next = first
    line1 = a[next]
    next += 1

    if line1.startswith("S"):
        error = ""
    else:
        error = "NOT an S block starting at line " + str(first)
        logger.information("ERROR *** " + error)
        sys.exit(error)
    next += 1
    next, Ival = Iblock(a, next)
    for m in range(0, len(Ival)):
        x.append(float(m))
        yy = float(Ival[m])
        y.append(yy)
        ee = math.sqrt(yy)
        e.append(ee)
    return next, x, y, e  # values of x,y,e as lists


# Get the path to the file
# checks if we already know the path, else
# returns searches of the file based on run number and instrument
def getFilePath(run, ext, instr):
    path = None
    fname = None
    if os.path.isfile(run):
        # using full file path
        path = run
        # base name less extension
        base = os.path.basename(path)
        fname = os.path.splitext(base)[0]
    else:
        # using run number
        path = FileFinder.getFullPath(instr + "_" + run + ext)
        fname = instr + "_" + run

    if path:
        return path, fname
    else:
        error = "ERROR *** Could not find " + ext + " file: " + fname
        sys.exit(error)


# Load an ascii/inx file
def loadFile(path):
    try:
        handle = open(path, "U")
        asc = []
        for line in handle:
            line = line.rstrip()
            asc.append(line)
        handle.close()

        return asc
    except RuntimeError:
        error = "ERROR *** Could not load " + path
        sys.exit(error)


def IbackStart(instr, run, ana, refl, rejectZ, useM, mapPath, Plot, Save):  # Ascii start routine
    workdir = config["defaultsave.directory"]

    path, fname = getFilePath(run, ".asc", instr)

    logger.information("Reading file : " + path)

    asc = loadFile(path)

    # raw head
    text = asc[1]
    run = text[:8]
    first = 5
    next, _Ival = Iblock(asc, first)
    next += 2
    title = asc[next]  # title line
    next += 1
    text = asc[next]  # user line
    next += 6  # 5 lines of text
    # back head1
    next, Fval = Fblock(asc, next)
    if instr == "IN10":
        freq = Fval[89]
    if instr == "IN16":
        freq = Fval[2]
        amp = Fval[3]
        wave = Fval[69]
        npt = int(Fval[6])
        nsp = int(Fval[7])
    # back head2
    next, Fval = Fblock(asc, next)
    theta = []
    for m in range(0, nsp):
        theta.append(Fval[m])
    # raw spectra
    val = extract_int(asc[next + 3])
    npt = val[0]
    lgrp = 5 + npt // 10
    val = extract_int(asc[next + 1])
    if instr == "IN10":
        nsp = int(val[2])
    logger.information("Number of spectra : " + str(nsp))
    # read monitor
    nmon = next + nsp * lgrp
    _nm, _xm, ym, em = ReadIbackGroup(asc, nmon)
    # monitor calcs
    imin = 0
    ymax = 0
    for m in range(0, 20):
        if ym[m] > ymax:
            imin = m
            ymax = ym[m]
    npt = len(ym)
    imax = npt
    ymax = 0
    for m in range(npt - 1, npt - 20, -1):
        if ym[m] > ymax:
            imax = m
            ymax = ym[m]
    new = imax - imin
    imid = new // 2 + 1
    if instr == "IN10":
        DRV = 18.706  # fast drive
        vmax = freq * DRV
    if instr == "IN16":
        vmax = 1.2992581918414711e-4 * freq * amp * 2.0 / wave  # max energy
    dele = 2.0 * vmax / new
    xMon = []
    yOut = []
    eOut = []
    for m in range(0, new + 1):
        xe = (m - imid) * dele
        mm = m + imin
        xMon.append(xe)
        yOut.append(ym[mm] / 100.0)
        eOut.append(em[mm] / 10.0)
    xMon.append(2 * xMon[new - 1] - xMon[new - 2])
    monWS = "__Mon"
    CreateWorkspace(OutputWorkspace=monWS, DataX=xMon, DataY=yOut, DataE=eOut, Nspec=1, UnitX="DeltaE")
    #
    _Qaxis = ""
    xDat = []
    yDat = []
    eDat = []
    tot = []
    for n in range(0, nsp):
        next, _xd, yd, ed = ReadIbackGroup(asc, next)
        tot.append(sum(yd))
        logger.information("Spectrum " + str(n + 1) + " at angle " + str(theta[n]) + " ; Total counts = " + str(sum(yd)))
        for m in range(0, new + 1):
            mm = m + imin
            xDat.append(xMon[m])
            yDat.append(yd[mm])
            eDat.append(ed[mm])
        if n != 0:
            _Qaxis += ","
        xDat.append(2 * xDat[new - 1] - xDat[new - 2])
        _Qaxis += str(theta[n])
    ascWS = fname + "_" + ana + refl + "_asc"
    outWS = fname + "_" + ana + refl + "_red"
    CreateWorkspace(OutputWorkspace=ascWS, DataX=xDat, DataY=yDat, DataE=eDat, Nspec=nsp, UnitX="DeltaE")
    Divide(LHSWorkspace=ascWS, RHSWorkspace=monWS, OutputWorkspace=ascWS, AllowDifferentNumberSpectra=True)
    DeleteWorkspace(monWS)  # delete monitor WS
    InstrParas(ascWS, instr, ana, refl)
    RunParas(ascWS, instr, run, title)
    ChangeAngles(ascWS, instr, theta)
    if useM:
        map = ReadMap(mapPath)
        UseMap(ascWS, map)
    if rejectZ:
        RejectZero(ascWS, tot)
    if not useM and not rejectZ:
        CloneWorkspace(InputWorkspace=ascWS, OutputWorkspace=outWS)
    if Save:
        opath = os.path.join(workdir, outWS + ".nxs")
        SaveNexusProcessed(InputWorkspace=outWS, Filename=opath)
        logger.information("Output file : " + opath)
    if Plot:
        plotForce(outWS, Plot)


# Routines for Inx ascii file


def ReadInxGroup(asc, n, lgrp):  # read ascii x,y,e
    x = []
    y = []
    e = []
    first = n * lgrp
    last = (n + 1) * lgrp
    val = extract_float(asc[first + 2])
    Q = val[0]
    for m in range(first + 4, last):
        val = extract_float(asc[m])
        x.append(val[0] / 1000.0)
        y.append(val[1])
        e.append(val[2])
    npt = len(x)
    return Q, npt, x, y, e  # values of x,y,e as lists


def InxStart(instr, run, ana, refl, rejectZ, useM, mapPath, Plot, Save):
    workdir = config["defaultsave.directory"]

    path, fname = getFilePath(run, ".inx", instr)

    logger.information("Reading file : " + path)

    asc = loadFile(path)
    lasc = len(asc)

    val = extract_int(asc[0])
    lgrp = int(val[0])
    ngrp = int(val[2])
    title = asc[1]
    ltot = ngrp * lgrp
    logger.information("Number of spectra : " + str(ngrp))
    if ltot == lasc:
        error = ""
    else:
        error = "file " + filext + " should be " + str(ltot) + " lines"
        logger.information("ERROR *** " + error)
        sys.exit(error)
    _Qaxis = ""
    xDat = []
    yDat = []
    eDat = []
    ns = 0
    Q = []
    tot = []
    for m in range(0, ngrp):
        Qq, nd, xd, yd, ed = ReadInxGroup(asc, m, lgrp)
        tot.append(sum(yd))
        logger.information("Spectrum " + str(m + 1) + " at Q= " + str(Qq) + " ; Total counts = " + str(tot))
        if ns != 0:
            _Qaxis += ","
        _Qaxis += str(Qq)
        Q.append(Qq)
        for n in range(0, nd):
            xDat.append(xd[n])
            yDat.append(yd[n])
            eDat.append(ed[n])
        xDat.append(2 * xd[nd - 1] - xd[nd - 2])
        ns += 1
    ascWS = fname + "_" + ana + refl + "_inx"
    outWS = fname + "_" + ana + refl + "_red"
    CreateWorkspace(OutputWorkspace=ascWS, DataX=xDat, DataY=yDat, DataE=eDat, Nspec=ns, UnitX="DeltaE")
    InstrParas(ascWS, instr, ana, refl)
    efixed = RunParas(ascWS, instr, 0, title)
    pi4 = 4.0 * math.pi
    wave = 1.8 * math.sqrt(25.2429 / efixed)
    theta = []
    for n in range(0, ngrp):
        qw = wave * Q[n] / pi4
        ang = 2.0 * math.degrees(math.asin(qw))
        theta.append(ang)
    ChangeAngles(ascWS, instr, theta)
    if useM:
        map = ReadMap(mapPath)
        UseMap(ascWS, map)
    if rejectZ:
        RejectZero(ascWS, tot)
    if not useM and not rejectZ:
        CloneWorkspace(InputWorkspace=ascWS, OutputWorkspace=outWS)
    if Save:
        opath = os.path.join(workdir, outWS + ".nxs")
        SaveNexusProcessed(InputWorkspace=outWS, Filename=opath)
        logger.information("Output file : " + opath)
    if Plot:
        plotForce(outWS, Plot)


# General routines


def RejectZero(inWS, tot):
    nin = mtd[inWS].getNumberHistograms()  # no. of hist/groups in sam
    nout = 0
    outWS = inWS[:-3] + "red"
    for n in range(0, nin):
        if tot[n] > 0:
            ExtractSingleSpectrum(InputWorkspace=inWS, OutputWorkspace="__tmp", WorkspaceIndex=n)
            if nout == 0:
                RenameWorkspace(InputWorkspace="__tmp", OutputWorkspace=outWS)
            else:
                ConjoinWorkspaces(InputWorkspace1=outWS, InputWorkspace2="__tmp", CheckOverlapping=False)
            nout += 1
        else:
            logger.information("** spectrum " + str(n + 1) + " rejected")


def ReadMap(path):
    asc = loadFile(path)

    lasc = len(asc)
    logger.information("Map file : " + path + " ; spectra = " + str(lasc - 1))
    val = extract_int(asc[0])
    numb = val[0]
    if numb != (lasc - 1):
        error = "Number of lines  not equal to number of spectra"
        logger.error(error)
        sys.exit(error)
    map = []
    for n in range(1, lasc):
        val = extract_int(asc[n])
        map.append(val[1])
    return map


def UseMap(inWS, map):
    nin = mtd[inWS].getNumberHistograms()  # no. of hist/groups in sam
    nout = 0
    outWS = inWS[:-3] + "red"
    for n in range(0, nin):
        if map[n] == 1:
            ExtractSingleSpectrum(InputWorkspace=inWS, OutputWorkspace="__tmp", WorkspaceIndex=n)
            if nout == 0:
                RenameWorkspace(InputWorkspace="__tmp", OutputWorkspace=outWS)
            else:
                ConjoinWorkspaces(InputWorkspace1=outWS, InputWorkspace2="__tmp", CheckOverlapping=False)
            nout += 1
            logger.information("** spectrum " + str(n + 1) + " mapped")
        else:
            logger.information("** spectrum " + str(n + 1) + " skipped")


def plotForce(inWS, Plot):
    if Plot == "Spectrum" or Plot == "Both":
        nHist = mtd[inWS].getNumberHistograms()
        if nHist > 10:
            nHist = 10
        plot_list = []
        for i in range(0, nHist):
            plot_list.append(i)
        plotSpectrum(inWS, plot_list)
    if Plot == "Contour" or Plot == "Both":
        pcolormesh(inWS)


def ChangeAngles(inWS, instr, theta):
    workdir = config["defaultsave.directory"]
    filename = instr + "_angles.txt"
    path = os.path.join(workdir, filename)
    logger.information("Creating angles file : " + path)
    handle = open(path, "w")
    head = "spectrum,theta"
    handle.write(head + " \n")
    for n in range(0, len(theta)):
        handle.write(str(n + 1) + "   " + str(theta[n]) + "\n")
        logger.information("Spectrum " + str(n + 1) + " = " + str(theta[n]))
    handle.close()
    UpdateInstrumentFromFile(Workspace=inWS, Filename=path, MoveMonitors=False, IgnorePhi=False, AsciiHeader=head)


def InstrParas(ws, instr, ana, refl):
    idf_dir = config["instrumentDefinition.directory"]
    idf = idf_dir + instr + "_Definition.xml"
    LoadInstrument(Workspace=ws, Filename=idf, RewriteSpectraMap=True)
    ipf = idf_dir + instr + "_" + ana + "_" + refl + "_Parameters.xml"
    LoadParameterFile(Workspace=ws, Filename=ipf)


def RunParas(ascWS, _instr, run, title):
    ws = mtd[ascWS]
    inst = ws.getInstrument()
    AddSampleLog(Workspace=ascWS, LogName="facility", LogType="String", LogText="ILL")
    ws.getRun()["run_number"] = run
    ws.getRun()["run_title"] = title
    efixed = get_efixed(ascWS)

    facility = ws.getRun().getLogData("facility").value
    logger.information("Facility is " + facility)
    runNo = ws.getRun()["run_number"].value
    runTitle = ws.getRun()["run_title"].value.strip()
    logger.information("Run : " + str(runNo) + " ; Title : " + runTitle)
    an = inst.getStringParameter("analyser")[0]
    ref = inst.getStringParameter("reflection")[0]
    logger.information("Analyser : " + an + ref + " with energy = " + str(efixed))

    return efixed


# IN13 routines
# These routines are specific to loading data for the ILL IN13 instrument


def IN13Start(instr, run, ana, refl, _rejectZ, _useM, _mapPath, Plot, Save):  # Ascii start routine
    IN13Read(instr, run, ana, refl, Plot, Save)


def IN13Read(instr, run, ana, refl, Plot, Save):  # Ascii start routine
    workdir = config["defaultsave.directory"]

    path, fname = getFilePath(run, ".asc", instr)

    logger.information("Reading file : " + path)

    asc = loadFile(path)

    # header block
    text = asc[1]
    run = text[:8]
    text = asc[4]  # run line
    instr = text[:4]
    next, Ival = Iblock(asc, 5)
    nsubsp = Ival[0]
    nspec = Ival[153] - 2
    # text block
    text = asc[25]
    title = text[:20]
    # para1 block
    next, Fval = Fblock(asc, 32)
    ntemp = int(Fval[6])
    f1 = ntemp / 10
    ltemp = int(f1)
    f2 = f1 - 10 * ltemp
    if f2 >= 0.0:
        ltemp = ltemp + 1
    wave = 2.0 * Fval[81]

    logger.information("No. sub-spectra : " + str(nsubsp))
    logger.information("No. spectra : " + str(nspec))
    logger.information("Scan type : " + str(int(Fval[8])) + " ; Average energy : " + str(Fval[9]))
    logger.information("CaF2 lattice : " + str(Fval[81]) + " ; Graphite lattice : " + str(Fval[82]))
    logger.information("Wavelength : " + str(wave))
    logger.information("No. temperatures : " + str(ntemp))
    logger.information("No. temperature lines : " + str(ltemp))

    # para2 block
    next, Fval = Fblock(asc, next)
    angles = Fval[:nspec]
    logger.information("Angles : " + str(angles))
    lspec = 4 + ltemp
    # monitors
    psd = next + (nspec + 2048) * lspec
    l1m1 = psd + 1
    l2m1 = l1m1 + 3
    mon1 = extract_float(asc[l2m1])
    logger.information("Mon1 : Line " + str(l2m1) + " : " + asc[l2m1])
    # raw spectra
    first = next
    xDat = angles
    y2D = []
    e2D = []
    for n in range(0, nspec):
        ylist = []
        elist = []
        l1 = first + lspec * n + 1
        logger.information("Line " + str(l1) + " : " + asc[l1])
        for l in range(0, ltemp):
            l2 = l1 + 3 + l
            val = extract_float(asc[l2])
            nval = len(val)
            for m in range(0, nval):
                ylist.append(val[m] / mon1[m])
                elist.append(math.sqrt(val[m]) / mon1[m])
        y2D.append(ylist)
        e2D.append(elist)
    # create WS
    npt = len(xDat)
    xDat.append(2 * xDat[npt - 1] - xDat[npt - 2])
    ascWS = fname + "_" + ana + refl + "_ang"
    outWS = fname + "_" + ana + refl + "_q"
    xD = np.array(xDat)
    k0 = 4 * math.pi / wave
    Q = []
    for n in range(0, nspec):
        if angles[n] >= 180.0:
            angles[n] = 360.0 - angles[n]
        theta = math.radians(angles[n] / 2.0)
        qq = k0 * math.sin(theta)
        Q.append(qq)
    Qa = np.array(Q)
    sorted_indices = Qa.argsort()
    sorted_Q = Qa[sorted_indices]
    lxdq = len(sorted_Q)
    xlast = 2 * sorted_Q[lxdq - 1] - sorted_Q[lxdq - 2]
    sorted_Q = np.append(sorted_Q, xlast)
    xDq = sorted_Q
    for m in range(0, nval):
        ylist = []
        elist = []
        for n in range(0, nspec):
            ylist.append(y2D[n][m])
            elist.append(e2D[n][m])
        y1D = np.array(ylist)
        e1D = np.array(elist)
        y1Dq = y1D[sorted_indices]
        e1Dq = e1D[sorted_indices]
        if m == 0:
            xData = xD
            yData = y1D
            eData = e1D
            xDq = sorted_Q
            yDq = y1Dq
            eDq = e1Dq
        else:
            xData = np.append(xData, xD)
            yData = np.append(yData, y1D)
            eData = np.append(eData, e1D)
            xDq = np.append(xDq, sorted_Q)
            yDq = np.append(yDq, y1Dq)
            eDq = np.append(eDq, e1Dq)
    CreateWorkspace(OutputWorkspace=ascWS, DataX=xData, DataY=yData, DataE=eData, Nspec=3, UnitX="MomentumTransfer")
    IN13Paras(ascWS, run, title, wave)
    CreateWorkspace(OutputWorkspace=outWS, DataX=xDq, DataY=yDq, DataE=eDq, Nspec=3, UnitX="MomentumTransfer")
    IN13Paras(outWS, run, title, wave)
    if Save:
        opath = os.path.join(workdir, outWS + ".nxs")
        SaveNexusProcessed(InputWorkspace=outWS, Filename=opath)
        logger.information("Output file : " + opath)
    if Plot != "None":
        plotForce(outWS, Plot)
    return outWS


def IN13Paras(ascWS, run, title, wave):
    ws = mtd[ascWS]
    AddSampleLog(Workspace=ascWS, LogName="facility", LogType="String", LogText="ILL")
    ws.getRun()["run_number"] = run
    ws.getRun()["run_title"] = title

    facility = ws.getRun().getLogData("facility").value
    logger.information("Facility is " + facility)
    runNo = ws.getRun()["run_number"].value
    runTitle = ws.getRun()["run_title"].value.strip()
    logger.information("Run : " + runNo + " ; Title : " + runTitle)
    logger.information("Wavelength : " + str(wave))
