# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-lines, invalid-name, too-many-arguments, too-many-branches, too-many-locals
# ruff: noqa: F403   # Allow wild imports
from math import *

try:
    from mantid.simpleapi import *  # New API
except ImportError:
    pass
# import qti as qti
import numpy as n


def addRuns(runlist, wname):
    mtd.deleteWorkspace(str(wname))
    output = str(wname)
    if runlist[0] != "0":
        # nzeros=8-len(str(runlist[0]))
        # fpad=""
        # for i in range(nzeros):
        #  fpad+="0"
        # filename="offspec"+fpad+str(runlist[0])+".nxs"
        # fname=str(FileFinder.findRuns(filename))
        # fname=str.replace(fname,'\\','/')
        # fname=str.replace(fname,'[','')
        # fname=str.replace(fname,']','')
        # fname=str.replace(fname,'\'','')
        # fname=fname.lower()
        ##fname=str.replace(fname,'.nxs','.raw')
        # Load(fname,output)
        Load(str(runlist[0]), output)
    else:
        # dae="ndx"+mtd.settings['default.instrument'].lower()
        dae = "ndxoffspec"
        LoadDAE(DAEname=dae, OutputWorkspace=output, SpectrumMin="1")
        if mtd[output].isGroup():
            for k in mtd[output].getNames():
                mtd[k].setYUnit("Counts")
        else:
            mtd[output].setYUnit("Counts")

    if len(runlist) > 1:
        for i in range(1, len(runlist)):
            if runlist[i] != "0":
                # nzeros=8-len(str(runlist[i]))
                # fpad=""
                # for j in range(nzeros):
                #  fpad+="0"
                # filename="offspec"+fpad+str(runlist[i])+".nxs"
                # fname=str(FileFinder.findRuns(filename))
                # fname=str.replace(fname,'\\','/')
                # fname=str.replace(fname,'[','')
                # fname=str.replace(fname,']','')
                # fname=str.replace(fname,'\'','')
                # fname=fname.lower()
                ##fname=str.replace(fname,'.nxs','.raw')
                # Load(fname,"wtemp")
                Load(str(runlist[i]), "wtemp")
            else:
                # dae="ndx"+mtd.settings['default.instrument'].lower()
                dae = "ndxoffspec"
                LoadDAE(DAEname=dae, OutputWorkspace="wtemp", SpectrumMin="1")
                if mtd["wtemp"].isGroup():
                    for k in mtd["wtemp"].getNames():
                        mtd[k].setYUnit("Counts")
                else:
                    mtd[output].setYUnit("Counts")
            Plus(output, "wtemp", output)
            mtd.deleteWorkspace("wtemp")

    mtd.sendLogMessage("addRuns Completed")


# parse a text string of the format "1-6:2+8+9,10+11+12+13-19:3,20-24"
# to return a structure containing the separated lists [1, 3, 5, 8, 9],
# [10, 11, 12, 13, 16, 19] and [20, 21, 22, 23, 24]
# as integer lists that addRuns can handle.


def parseRunList(istring):
    rlist = []
    if len(istring) > 0:
        s1 = istring.split(",")
        rlist1 = []
        for i in range(len(s1)):
            tstr = s1[i].strip()
            if len(tstr) > 0:
                rlist1.append(tstr)
        for i in range(len(rlist1)):
            rlist2 = []
            if rlist1[i].find("+") >= 0:
                tstr = rlist1[i].split("+")
                for j in range(len(tstr)):
                    if tstr[j].find(":") >= 0 and tstr[j].find("-") >= 0:
                        tstr[j].strip()
                        tstr2 = tstr[j].split("-")
                        tstr3 = tstr2[1].split(":")
                        r1 = list(range(int(tstr2[0]), int(tstr3[0]) + 1, int(tstr3[1])))
                        for k in r1:
                            rlist2.append(str(k))
                    elif tstr[j].find("-") >= 0:
                        tstr[j].strip()
                        tstr2 = tstr[j].split("-")
                        r1 = list(range(int(tstr2[0]), int(tstr2[1]) + 1))
                        for k in r1:
                            rlist2.append(str(k))
                    else:
                        rlist2.append(tstr[j])
            else:
                if rlist1[i].find(":") >= 0 and rlist1[i].find("-") >= 0:
                    rlist1[i].strip()
                    tstr2 = rlist1[i].split("-")
                    tstr3 = tstr2[1].split(":")
                    r1 = list(range(int(tstr2[0]), int(tstr3[0]) + 1, int(tstr3[1])))
                    for k in r1:
                        rlist2.append(str(k))
                elif rlist1[i].find("-") >= 0:
                    rlist1[i].strip()
                    tstr2 = rlist1[i].split("-")
                    r1 = list(range(int(tstr2[0]), int(tstr2[1]) + 1))
                    for k in r1:
                        rlist2.append(str(k))
                else:
                    rlist2.append(rlist1[i])
            rlist.append(rlist2)
    return rlist


def parseNameList(istring):
    s1 = istring.split(",")
    namelist = []
    for i in range(len(s1)):
        tstr = s1[i].strip()
        namelist.append(tstr)
    return namelist


def floodnorm(wkspName, floodfile):
    #
    # pixel by pixel efficiency correction for the linear detector
    #
    floodloaded = 0
    a1 = mtd.getWorkspaceNames()
    for i in range(len(a1)):
        if a1[i] == "ld240flood":
            floodloaded = 1
    if floodloaded == 0:
        LoadNexusProcessed(Filename=floodfile, OutputWorkspace="ld240flood")

    Divide(wkspName, "ld240flood", wkspName)


#
# Plot a bunch of workspaces as 2D maps
# using the supplied limits and log scale settings
#
def plot2D(wkspNames, limits, logScales):
    nplot = 0
    workspace_mtx = []
    wNames = parseNameList(wkspNames)
    for i in range(len(wNames)):
        w1 = mtd[wNames[i]]
        if w1.isGroup():
            w1names = w1.getNames()
            for j in range(len(w1names)):
                # workspace_mtx.append(qti.app.mantidUI.importMatrixWorkspace(w1names[j]))
                workspace_mtx.append(mantidplot.importMatrixWorkspace(w1names[j]))
                gr2d = workspace_mtx[nplot].plotGraph2D()
                nplot = nplot + 1
                l = gr2d.activeLayer()
                if logScales[0] == "0":
                    l.setAxisScale(mantidplot.Layer.Bottom, float(limits[0]), float(limits[1]))
                elif logScales[0] == "2":
                    l.setAxisScale(mantidplot.Layer.Bottom, float(limits[0]), float(limits[1]), 1)
                if logScales[1] == "0":
                    l.setAxisScale(mantidplot.Layer.Left, float(limits[2]), float(limits[3]))
                elif logScales[1] == "2":
                    l.setAxisScale(mantidplot.Layer.Left, float(limits[2]), float(limits[3]), 1)
                if logScales[2] == "0":
                    l.setAxisScale(mantidplot.Layer.Right, float(limits[4]), float(limits[5]))
                elif logScales[2] == "2":
                    l.setAxisScale(mantidplot.Layer.Right, float(limits[4]), float(limits[5]), 1)
        else:
            workspace_mtx.append(mantidplot.importMatrixWorkspace(wNames[i]))
            gr2d = workspace_mtx[nplot].plotGraph2D()
            nplot = nplot + 1
            l = gr2d.activeLayer()
            if logScales[0] == "0":
                l.setAxisScale(mantidplot.Layer.Bottom, float(limits[0]), float(limits[1]))
            elif logScales[0] == "2":
                l.setAxisScale(mantidplot.Layer.Bottom, float(limits[0]), float(limits[1]), 1)
            if logScales[1] == "0":
                l.setAxisScale(mantidplot.Layer.Left, float(limits[2]), float(limits[3]))
            elif logScales[1] == "2":
                l.setAxisScale(mantidplot.Layer.Left, float(limits[2]), float(limits[3]), 1)
            if logScales[2] == "0":
                l.setAxisScale(mantidplot.Layer.Right, float(limits[4]), float(limits[5]))
            elif logScales[2] == "2":
                l.setAxisScale(mantidplot.Layer.Right, float(limits[4]), float(limits[5]), 1)

    mtd.sendLogMessage("plot2D finished")


#
# Plot a bunch of workspaces as 2D maps
# using the supplied limits and log scale settings
#
def XYPlot(wkspNames, spectra, limits, logScales, errors, singleFigure):
    wNames = parseNameList(wkspNames)
    spec = parseNameList(spectra)
    ploterr = 0
    xLog = 0
    yLog = 0
    if errors == "2":
        ploterr = 1
    if logScales[0] == "2":
        xLog = 1
    if logScales[1] == "2":
        yLog = 1

    if singleFigure == "2":
        p1 = plotSpectrum(wNames, spec, ploterr)
        l = p1.activeLayer()
        l.setAxisScale(mantidplot.Layer.Bottom, float(limits[0]), float(limits[1]), xLog)
        l.setAxisScale(mantidplot.Layer.Left, float(limits[2]), float(limits[3]), yLog)
    else:
        for i in range(len(wNames)):
            p1 = plotSpectrum(wNames[i], spec, ploterr)
            l = p1.activeLayer()
            l.setAxisScale(mantidplot.Layer.Bottom, float(limits[0]), float(limits[1]), xLog)
            l.setAxisScale(mantidplot.Layer.Left, float(limits[2]), float(limits[3]), yLog)

    mtd.sendLogMessage("XYPlot finished")


def nrtestfn(runlist, wnames):
    rlist = parseRunList(runlist)
    mtd.sendLogMessage("This is the runlist:" + str(rlist))
    namelist = parseNameList(wnames)
    mtd.sendLogMessage("This is the nameslist:" + str(namelist))
    for i in range(len(rlist)):
        addRuns(rlist[i], namelist[i])
        # Load(Filename="L:/RawData/cycle_10_1/OFFSPEC0000"+str(rlist[i][j])+".nxs",OutputWorkspace="w"+
        # str(rlist[i][j]),LoaderName="LoadISISNexus")

    mtd.sendLogMessage("nrtestfn completed")


#    output="w7503"
#    plotper=[1,2]
#    rebpars="0.5,0.025,14.5"
#    spmin=0
#    spmax=239
#    Load(Filename="L:/RawData/cycle_10_1/OFFSPEC00007503.nxs",OutputWorkspace=output,LoaderName="LoadISISNexus")
#    workspace_mtx=[]
#    nplot=0
#    for i in plotper:
#        ConvertUnits(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i),Target="Wavelength",AlignBins="1")
#        Rebin(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i),Params=rebpars)
#        CropWorkspace(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i)+"m",StartWorkspaceIndex="1",
# EndWorkspaceIndex="1")
#        CropWorkspace(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i)+"d",StartWorkspaceIndex=str(spmin+4),
# EndWorkspaceIndex=str(spmax+4))
#        Divide(output+"_"+str(i)+"d",output+"_"+str(i)+"m",output+"_"+str(i)+"n")
#        workspace_mtx.append(qti.app.mantidUI.importMatrixWorkspace(output+"_"+str(i)+"n"))
#        gr2d=workspace_mtx[nplot].plotGraph2D()
#        nplot=nplot+1
#        l=gr2d.activeLayer()

#    mtd.sendLogMessage("quickPlot Finished")


def removeoutlayer(wksp):
    # remove counts from bins where there are so few counts it makes a mess of the polarisation
    # calculation
    a1 = mtd[wksp]
    nspec = a1.getNumberHistograms()
    x = a1.readX(0)
    for i in range(nspec):
        for j in range(len(x) - 1):
            y = a1.readY(i)[j]
            if y < 2:
                a1.dataY(i)[j] = 0.0
                a1.dataE(i)[j] = 0.0


def nrSESANSFn(
    runList,
    nameList,
    P0runList,
    P0nameList,
    minSpec,
    maxSpec,
    upPeriod,
    downPeriod,
    existingP0,
    SEConstants,
    gparams,
    convertToSEL,
    lnPOverLam,
    diagnostics="0",
    removeoutlayer="0",
    floodfile="none",
):
    nlist = parseNameList(nameList)
    mtd.sendLogMessage("This is the sample nameslist:" + str(nlist))
    rlist = parseRunList(runList)
    mtd.sendLogMessage("This is the sample runlist:" + str(rlist))
    for i in range(len(rlist)):
        addRuns(rlist[i], nlist[i])

    P0nlist = parseNameList(P0nameList)
    mtd.sendLogMessage("This is the P0nameslist:" + str(P0nlist))
    if existingP0 != "2":
        P0rlist = parseRunList(P0runList)
        mtd.sendLogMessage("This is the P0runlist:" + str(P0rlist))
        for i in range(len(P0rlist)):
            addRuns(P0rlist[i], P0nlist[i])

    mon_spec = int(gparams[3]) - 1
    minSp = int(minSpec) - 1
    maxSp = int(maxSpec) - 1
    reb = gparams[0] + "," + gparams[1] + "," + gparams[2]
    if len(gparams) == 5:
        mapfile = gparams[4]

    nspec = _loop_through_name_list(
        diagnostics, downPeriod, floodfile, mapfile, maxSp, maxSpec, minSp, minSpec, mon_spec, nlist, reb, removeoutlayer, upPeriod
    )

    if existingP0 != "2":
        for i in P0nlist:
            ConvertUnits(i, i, "Wavelength", AlignBins=1)
            Rebin(i, i, reb)
            removeoutlayer(i + "_1")
            removeoutlayer(i + "_2")
            CropWorkspace(i, i + "mon", StartWorkspaceIndex=mon_spec, EndWorkspaceIndex=mon_spec)
            if int(maxSpec) > int(minSpec):
                SumSpectra(i, i + "det", StartWorkspaceIndex=minSp, EndWorkspaceIndex=maxSp)
            else:
                CropWorkspace(i, i + "det", StartWorkspaceIndex=minSp, EndWorkspaceIndex=maxSp)
            Divide(i + "det", i + "mon", i + "norm")
            DeleteWorkspace(i + "mon")
            DeleteWorkspace(i + "det")
            DeleteWorkspace(i)
            Minus(i + "norm_" + upPeriod, i + "norm_" + downPeriod, "num")
            Plus(i + "norm_2", i + "norm_1", "den")
            Divide("num", "den", i + "pol")
            ReplaceSpecialValues(i + "pol", i + "pol", 0.0, 0.0, 0.0, 0.0)
            DeleteWorkspace(i + "norm_2")
            DeleteWorkspace(i + "norm_1")
            DeleteWorkspace("num")
            DeleteWorkspace("den")

    for i in range(len(nlist)):
        if existingP0 != "2":
            Divide(nlist[i] + "pol", P0nlist[i] + "pol", nlist[i] + "SESANS")
            if nspec > 4 and minSp != 3:
                Divide(nlist[i] + "2dpol", P0nlist[i] + "pol", nlist[i] + "2dSESANS")
        else:
            Divide(nlist[i] + "pol", P0nlist[i], nlist[i] + "SESANS")
            if nspec > 4 and minSp != 3:
                Divide(nlist[i] + "2dpol", P0nlist[i], nlist[i] + "2dSESANS")
        ReplaceSpecialValues(nlist[i] + "SESANS", nlist[i] + "SESANS", 0.0, 0.0, 0.0, 0.0)

    SEConstList = parseNameList(SEConstants)
    k = 0
    _process_sesans_workspace(SEConstList, convertToSEL, k, lnPOverLam, nlist)

    if nspec > 4 and minSp != 3:
        k = 0
        _process_2dsesans_workspace(SEConstList, convertToSEL, k, lnPOverLam, nlist)


def _loop_through_name_list(
    diagnostics, downPeriod, floodfile, mapfile, maxSp, maxSpec, minSp, minSpec, mon_spec, nlist, reb, removeoutlayer, upPeriod
):
    for i in nlist:
        a1 = mtd[i + "_1"]
        nspec = a1.getNumberHistograms()
        if nspec == 1030 and minSp != 3:
            GroupDetectors(InputWorkspace=i, OutputWorkspace=i, MapFile=mapfile)
        ConvertUnits(i, i, "Wavelength", AlignBins=1)
        Rebin(i, i, reb)
        if removeoutlayer != "0":
            removeoutlayer(i + "_1")
            removeoutlayer(i + "_2")
        CropWorkspace(i, i + "mon", StartWorkspaceIndex=mon_spec, EndWorkspaceIndex=mon_spec)
        if nspec == 245:
            CropWorkspace(i, i + "2ddet", StartWorkspaceIndex=4, EndWorkspaceIndex=243)
            if floodfile != "none":
                floodnorm(i + "2ddet", floodfile)
        if nspec == 1030:
            CropWorkspace(i, i + "2ddet", StartWorkspaceIndex=3, EndWorkspaceIndex=124)
        if int(maxSpec) > int(minSpec):
            SumSpectra(i, i + "det", StartWorkspaceIndex=minSp, EndWorkspaceIndex=maxSp)
        else:
            CropWorkspace(i, i + "det", StartWorkspaceIndex=minSp, EndWorkspaceIndex=maxSp)

        Divide(i + "det", i + "mon", i + "norm")
        if nspec > 4 and minSp != 3:
            Divide(i + "2ddet", i + "mon", i + "2dnorm")
        DeleteWorkspace(i + "mon")
        if diagnostics == "0":
            DeleteWorkspace(i + "det")
        DeleteWorkspace(i)
        Minus(i + "norm_" + upPeriod, i + "norm_" + downPeriod, "num")
        Plus(i + "norm_2", i + "norm_1", "den")
        Divide("num", "den", i + "pol")
        ReplaceSpecialValues(i + "pol", i + "pol", 0.0, 0.0, 0.0, 0.0)
        if nspec > 4 and minSp != 3:
            Minus(i + "2dnorm_" + upPeriod, i + "2dnorm_" + downPeriod, "num")
            Plus(i + "2dnorm_2", i + "2dnorm_1", "den")
            Divide("num", "den", i + "2dpol")
            ReplaceSpecialValues(i + "2dpol", i + "2dpol", 0.0, 0.0, 0.0, 0.0)
        DeleteWorkspace("num")
        DeleteWorkspace("den")
    return nspec


def _process_2dsesans_workspace(SEConstList, convertToSEL, k, lnPOverLam, nlist):
    for i in nlist:
        if lnPOverLam == "2":
            CloneWorkspace(InputWorkspace=i + "2dSESANS", OutputWorkspace=i + "2dSESANS_P")
        a1 = mtd[i + "2dSESANS"]
        nspec = a1.getNumberHistograms()
        for l in range(nspec):
            x = a1.readX(l)
            for j in range(len(x) - 1):
                lam = ((a1.readX(l)[j] + a1.readX(l)[j + 1]) / 2.0) / 10.0
                p = a1.readY(l)[j]
                _e = a1.readE(l)[j]
                if lnPOverLam == "2":
                    if p > 0.0:
                        a1.dataY(l)[j] = log(p) / ((lam * 1.0e-9) ** 2)
                        a1.dataE(l)[j] = (_e / p) / ((lam * 1.0e-9) ** 2)
                    else:
                        a1.dataY(l)[j] = 0.0
                        a1.dataE(l)[j] = 0.0
            for j in range(len(x)):
                if convertToSEL == "2":
                    lam = a1.readX(l)[j]
                    a1.dataX(l)[j] = 1.0e-2 * float(SEConstList[k]) * lam * lam
        k = k + 1


def _process_sesans_workspace(SEConstList, convertToSEL, k, lnPOverLam, nlist):
    for i in nlist:
        if lnPOverLam == "2":
            CloneWorkspace(InputWorkspace=i + "SESANS", OutputWorkspace=i + "SESANS_P")
        a1 = mtd[i + "SESANS"]
        x = a1.readX(0)
        for j in range(len(x) - 1):
            lam = ((a1.readX(0)[j] + a1.readX(0)[j + 1]) / 2.0) / 10.0
            p = a1.readY(0)[j]
            _e = a1.readE(0)[j]
            if lnPOverLam == "2":
                if p > 0.0:
                    a1.dataY(0)[j] = log(p) / ((lam) ** 2)
                    a1.dataE(0)[j] = (_e / p) / ((lam) ** 2)
                else:
                    a1.dataY(0)[j] = 0.0
                    a1.dataE(0)[j] = 0.0
        for j in range(len(x)):
            if convertToSEL == "2":
                lam = a1.readX(0)[j]
                a1.dataX(0)[j] = 1.0e-2 * float(SEConstList[k]) * lam * lam
        k = k + 1


def nrCalcSEConst(RFFrequency, poleShoeAngle):
    if RFFrequency == "0.5":
        B = 0.53 * 34.288
    elif RFFrequency == "1.0":
        B = 34.288
    else:
        B = 2.0 * 34.288

    h = 6.62607e-34
    m = 1.67493e-27
    L = 1.0
    Gl = 1.83247e8
    #
    # correct the angle
    # calibration of th0 using gold grating Dec 2010
    #
    th0 = float(poleShoeAngle)
    # fmt: off
    th0 = (
        -0.0000000467796 * (th0**5)
        + 0.0000195413 * (th0**4)
        - 0.00326229 * (th0**3)
        + 0.271767 * (th0**2)
        - 10.4269 * th0
        + 198.108
    )
    # fmt: on
    c1 = Gl * m * 2.0 * B * L / (2.0 * pi * h * tan(th0 * pi / 180.0) * 1.0e20)
    print(c1 * 1e8)
    return c1 * 1e8


def nrSESANSP0Fn(P0runList, P0nameList, minSpec, maxSpec, upPeriod, downPeriod, gparams, diagnostics="0"):
    P0nlist = parseNameList(P0nameList)
    mtd.sendLogMessage("This is the P0nameslist:" + str(P0nlist))
    P0rlist = parseRunList(P0runList)
    mtd.sendLogMessage("This is the P0runlist:" + str(P0rlist))
    for i in range(len(P0rlist)):
        addRuns(P0rlist[i], P0nlist[i])

    mon_spec = int(gparams[3]) - 1
    minSp = int(minSpec) - 1
    maxSp = int(maxSpec) - 1
    reb = gparams[0] + "," + gparams[1] + "," + gparams[2]
    if len(gparams) == 5:
        mapfile = gparams[4]

    for i in P0nlist:
        a1 = mtd[i + "_1"]
        nspec = a1.getNumberHistograms()
        if nspec == 1030 and minSp != 3:
            GroupDetectors(InputWorkspace=i, OutputWorkspace=i, MapFile=mapfile)
        ConvertUnits(i, i, "Wavelength", AlignBins=1)
        Rebin(i, i, reb)
        CropWorkspace(i, i + "mon", StartWorkspaceIndex=mon_spec, EndWorkspaceIndex=mon_spec)
        if int(maxSpec) > int(minSpec):
            SumSpectra(i, i + "det", StartWorkspaceIndex=minSp, EndWorkspaceIndex=maxSp)
        else:
            CropWorkspace(i, i + "det", StartWorkspaceIndex=minSp, EndWorkspaceIndex=maxSp)
        Divide(i + "det", i + "mon", i + "norm")
        if diagnostics == "0":
            DeleteWorkspace(i + "mon")
            DeleteWorkspace(i + "det")
            DeleteWorkspace(i)
        Minus(i + "norm_" + upPeriod, i + "norm_" + downPeriod, "num")
        Plus(i + "norm_2", i + "norm_1", "den")
        Divide("num", "den", i + "pol")
        ReplaceSpecialValues(i + "pol", i + "pol", 0.0, 0.0, 0.0, 0.0)
        if diagnostics == "0":
            DeleteWorkspace(i + "norm_2")
            DeleteWorkspace(i + "norm_1")
            DeleteWorkspace("num")
            DeleteWorkspace("den")


def nrSERGISFn(
    runList,
    nameList,
    P0runList,
    P0nameList,
    incidentAngles,
    SEConstants,
    specChan,
    minSpec,
    maxSpec,
    upPeriod,
    downPeriod,
    existingP0,
    gparams,
    lnPOverLam,
):
    nlist = parseNameList(nameList)
    mtd.sendLogMessage("This is the sample nameslist:" + str(nlist))
    rlist = parseRunList(runList)
    mtd.sendLogMessage("This is the sample runlist:" + str(rlist))
    incAngles = parseNameList(incidentAngles)
    mtd.sendLogMessage("This incident Angles are:" + str(incAngles))

    for i in range(len(rlist)):
        addRuns(rlist[i], nlist[i])

    P0nlist = parseNameList(P0nameList)
    mtd.sendLogMessage("This is the P0nameslist:" + str(P0nlist))
    if existingP0 != "2":
        P0rlist = parseRunList(P0runList)
        mtd.sendLogMessage("This is the P0runlist:" + str(P0rlist))
        for i in range(len(P0rlist)):
            addRuns(P0rlist[i], P0nlist[i])

    mon_spec = int(gparams[3]) - 1
    minSp = int(minSpec) - 1
    maxSp = int(maxSpec) - 1
    reb = gparams[0] + "," + gparams[1] + "," + gparams[2]

    k = 0
    for i in nlist:
        for j in range(2):
            wksp = i + "_" + pnums[j]
            ConvertUnits(wksp, wksp, "Wavelength", AlignBins=1)
            Rebin(wksp, wksp, reb)
            CropWorkspace(wksp, wksp + "mon", StartWorkspaceIndex=mon_spec, EndWorkspaceIndex=mon_spec)
            a1 = mtd[wksp]
            nspec = a1.getNumberHistograms()
            if nspec == 4:
                CropWorkspace(InputWorkspace=wksp, OutputWorkspace=wksp + "det", StartWorkspaceIndex=3, EndWorkspaceIndex=3)
                RotateInstrumentComponent(wksp + "det", "DetectorBench", X="-1.0", Angle=str(2.0 * float(incAngles[k])))
                Divide(wksp + "det", wksp + "mon", wksp + "norm")
            else:
                CropWorkspace(InputWorkspace=wksp, OutputWorkspace=wksp + "det", StartWorkspaceIndex=4, EndWorkspaceIndex=243)
                # move the first spectrum in the list onto the beam centre so that when the bench is rotated it's in the right place
                MoveInstrumentComponent(wksp + "det", "DetectorBench", Y=str((125.0 - float(minSpec)) * 1.2e-3))
                # add a bit to the angle to put the first spectrum of the group in the right place
                a1 = 2.0 * float(incAngles[k]) + atan((float(minSpec) - float(specChan)) * 1.2e-3 / 3.53) * 180.0 / pi
                # print str(2.0*float(incAngles[k]))+" "+str(atan((float(minSpec)-float(specChan))*1.2e-3/3.63)*180.0/pi)+" "+str(a1)
                RotateInstrumentComponent(wksp + "det", "DetectorBench", X="-1.0", Angle=str(a1))
                GroupDetectors(
                    wksp + "det",
                    wksp + "sum",
                    WorkspaceIndexList=list(range(int(minSpec) - 5, int(maxSpec) - 5 + 1)),
                    KeepUngroupedSpectra="0",
                )
                Divide(wksp + "sum", wksp + "mon", wksp + "norm")
                Divide(wksp + "det", wksp + "mon", wksp + "detnorm")
                floodnorm(wksp + "detnorm", floodfile)
                DeleteWorkspace(wksp + "sum")

            DeleteWorkspace(wksp + "mon")
            DeleteWorkspace(wksp + "det")
            DeleteWorkspace(wksp)

        Minus(i + "_" + upPeriod + "norm", i + "_" + downPeriod + "norm", "num")
        Plus(i + "_1norm", i + "_2norm", "den")
        Divide("num", "den", i + "pol")
        ReplaceSpecialValues(i + "pol", i + "pol", 0.0, 0.0, 0.0, 0.0)
        DeleteWorkspace(i + "_1norm")
        DeleteWorkspace(i + "_2norm")
        DeleteWorkspace("num")
        DeleteWorkspace("den")

        if nspec != 4:
            Minus(i + "_" + upPeriod + "detnorm", i + "_" + downPeriod + "detnorm", "num")
            Plus(i + "_1detnorm", i + "_2detnorm", "den")
            Divide("num", "den", i + "2dpol")
            ReplaceSpecialValues(i + "2dpol", i + "2dpol", 0.0, 0.0, 0.0, 0.0)
            DeleteWorkspace(i + "_1detnorm")
            DeleteWorkspace(i + "_2detnorm")
            DeleteWorkspace("num")
            DeleteWorkspace("den")

    if existingP0 != "2":
        for i in P0nlist:
            ConvertUnits(i, i, "Wavelength", AlignBins=1)
            Rebin(i, i, reb)
            CropWorkspace(i, i + "mon", StartWorkspaceIndex=mon_spec, EndWorkspaceIndex=mon_spec)
            if int(maxSpec) > int(minSpec):
                SumSpectra(i, i + "det", StartWorkspaceIndex=minSp, EndWorkspaceIndex=maxSp)
            else:
                CropWorkspace(i, i + "det", StartWorkspaceIndex=minSp, EndWorkspaceIndex=maxSp)
            Divide(i + "det", i + "mon", i + "norm")
            DeleteWorkspace(i + "mon")
            DeleteWorkspace(i + "det")
            DeleteWorkspace(i)
            Minus(i + "norm_" + upPeriod, i + "norm_" + downPeriod, "num")
            Plus(i + "norm_2", i + "norm_1", "den")
            Divide("num", "den", i + "pol")
            ReplaceSpecialValues(i + "pol", i + "pol", 0.0, 0.0, 0.0, 0.0)
            DeleteWorkspace(i + "norm_2")
            DeleteWorkspace(i + "norm_1")
            DeleteWorkspace("num")
            DeleteWorkspace("den")

    for i in range(len(nlist)):
        if existingP0 != "2":
            Divide(nlist[i] + "pol", P0nlist[i] + "pol", nlist[i] + "SESANS")
        else:
            Divide(nlist[i] + "pol", P0nlist[i] + "pol", nlist[i] + "SESANS")
        ReplaceSpecialValues(nlist[i] + "SESANS", nlist[i] + "SESANS", 0.0, 0.0, 0.0, 0.0)

    SEConstList = parseNameList(SEConstants)
    k = 0
    for i in nlist:
        a1 = mtd[i + "SESANS"]
        x = a1.readX(0)
        for j in range(len(x) - 1):
            lam = ((a1.readX(0)[j] + a1.readX(0)[j + 1]) / 2.0) / 10.0
            p = a1.readY(0)[j]
            a1.dataY(0)[j] = log(p) / ((lam * 1.0e-8) ** 2)
        for j in range(len(x)):
            lam = a1.readX(0)[j]
            a1.dataX(0)[j] = 1.0e-2 * float(SEConstList[k]) * lam * lam
            # print str(lam)+" "+str(1.0e-2*float(SEConstList[k])*lam*lam)
        k = k + 1


def nrNRFn(runList, nameList, incidentAngles, DBList, specChan, minSpec, maxSpec, gparams, floodfile, diagnostics=0):
    nlist = parseNameList(nameList)
    mtd.sendLogMessage("This is the sample nameslist:" + str(nlist))
    rlist = parseRunList(runList)
    mtd.sendLogMessage("This is the sample runlist:" + str(rlist))
    dlist = parseNameList(DBList)
    mtd.sendLogMessage("This is the Direct Beam nameslist:" + str(dlist))
    incAngles = parseNameList(incidentAngles)
    mtd.sendLogMessage("This incident Angles are:" + str(incAngles))

    for i in range(len(rlist)):
        addRuns(rlist[i], nlist[i])

    mon_spec = int(gparams[3]) - 1
    reb = gparams[0] + "," + gparams[1] + "," + gparams[2]

    k = 0
    for i in nlist:
        if mtd[i].isGroup():
            # RenameWorkspace(i+"_1",i)
            snames = mtd[i].getNames()
            Plus(i + "_1", i + "_2", "wtemp")
            if len(snames) > 2:
                for j in range(2, len(snames) - 1):
                    Plus("wtemp", snames[j], "wtemp")
            for j in snames:
                DeleteWorkspace(j)
            RenameWorkspace("wtemp", i)
        ConvertUnits(InputWorkspace=i, OutputWorkspace=i, Target="Wavelength", AlignBins="1")
        a1 = mtd[i]
        nspec = a1.getNumberHistograms()
        if nspec == 4:
            Rebin(InputWorkspace=i, OutputWorkspace=i, Params=reb)
            CropWorkspace(InputWorkspace=i, OutputWorkspace=i + "mon", StartWorkspaceIndex=mon_spec, EndWorkspaceIndex=mon_spec)
            CropWorkspace(InputWorkspace=i, OutputWorkspace=i + "det", StartWorkspaceIndex=3, EndWorkspaceIndex=3)
            RotateInstrumentComponent(i + "det", "DetectorBench", X="-1.0", Angle=str(2.0 * float(incAngles[k])))
            Divide(i + "det", i + "mon", i + "norm")
            if dlist[k] != "none":
                Divide(i + "norm", dlist[k], i + "norm")
                ReplaceSpecialValues(i + "norm", i + "norm", "0.0", "0.0", "0.0", "0.0")
            ConvertUnits(i + "norm", i + "RvQ", Target="MomentumTransfer")
        else:
            # Rebin using internal parameters to avoid problems with summing in Q
            # internalreb=gparams[0]+",0.01,"+gparams[2]
            # Rebin(InputWorkspace=i,OutputWorkspace=i,Params=internalreb)
            CropWorkspace(InputWorkspace=i, OutputWorkspace=i + "det", StartWorkspaceIndex=4, EndWorkspaceIndex=243)
            floodnorm(i + "det", floodfile)
            # move the first spectrum in the list onto the beam centre so that when the bench is rotated it's in the right place
            MoveInstrumentComponent(i + "det", "DetectorBench", Y=str((125.0 - float(minSpec)) * 1.2e-3))
            # add a bit to the angle to put the first spectrum of the group in the right place
            a1 = 2.0 * float(incAngles[k]) + atan((float(minSpec) - float(specChan)) * 1.2e-3 / 3.53) * 180.0 / pi
            # print str(2.0*float(incAngles[k]))+" "+str(atan((float(minSpec)-float(specChan))*1.2e-3/3.63)*180.0/pi)+" "+str(a1)
            RotateInstrumentComponent(i + "det", "DetectorBench", X="-1.0", Angle=str(a1))
            # Experimental convert to Q before summing
            # CropWorkspace(InputWorkspace=i+"det",OutputWorkspace=i+"detQ",StartWorkspaceIndex=int(minSpec)-5,
            # EndWorkspaceIndex=int(maxSpec)-5+1)
            # ConvertUnits(InputWorkspace=i+"detQ",OutputWorkspace=i+"detQ",Target="MomentumTransfer",AlignBins="1")
            # GroupDetectors(i+"detQ",i+"sum",WorkspaceIndexList=range(0,int(maxSpec)-int(minSpec)),KeepUngroupedSpectra="0")
            # ConvertUnits(InputWorkspace=i+"sum",OutputWorkspace=i+"sum",Target="Wavelength",AlignBins="1")
            # Rebin(InputWorkspace=i+"sum",OutputWorkspace=i+"sum",Params=reb)
            # CropWorkspace(InputWorkspace=i,OutputWorkspace=i+"mon",StartWorkspaceIndex=mon_spec,EndWorkspaceIndex=mon_spec)
            # Rebin(InputWorkspace=i+"mon",OutputWorkspace=i+"mon",Params=reb)
            # Rebin(InputWorkspace=i+"det",OutputWorkspace=i+"det",Params=reb)
            CropWorkspace(InputWorkspace=i, OutputWorkspace=i + "mon", StartWorkspaceIndex=mon_spec, EndWorkspaceIndex=mon_spec)
            Rebin(InputWorkspace=i + "mon", OutputWorkspace=i + "mon", Params=reb)
            Rebin(InputWorkspace=i + "det", OutputWorkspace=i + "det", Params=reb)
            GroupDetectors(
                i + "det", i + "sum", WorkspaceIndexList=list(range(int(minSpec) - 5, int(maxSpec) - 5 + 1)), KeepUngroupedSpectra="0"
            )
            Divide(i + "sum", i + "mon", i + "norm")
            Divide(i + "det", i + "mon", i + "detnorm")
            if dlist[k] == "none":
                a1 = 0
            elif dlist[k] == "function":
                # polynomial + power law corrections based on Run numbers 8291 and 8292
                Divide(i + "norm", i + "norm", i + "normt1")
                PolynomialCorrection(i + "normt1", i + "normPC", "-0.0177398,0.00101695,0.0", Operation="Multiply")
                PowerLawCorrection(i + "normt1", i + "normPLC", "2.01332", "-1.8188")
                Plus(i + "normPC", i + "normPLC", i + "normt1")
                Divide(i + "norm", i + "normt1", i + "norm")
                ReplaceSpecialValues(i + "norm", i + "norm", 0.0, 0.0, 0.0, 0.0)
                DeleteWorkspace(i + "normPC")
                DeleteWorkspace(i + "normPLC")
                DeleteWorkspace(i + "normt1")
            else:
                Divide(i + "norm", dlist[k], i + "norm")
                ReplaceSpecialValues(i + "norm", i + "norm", "0.0", "0.0", "0.0", "0.0")
                Divide(i + "detnorm", dlist[k], i + "detnorm")
                ReplaceSpecialValues(i + "detnorm", i + "detnorm", "0.0", "0.0", "0.0", "0.0")
            ConvertUnits(i + "norm", i + "RvQ", Target="MomentumTransfer")
            # floodnorm(i+"detnorm",floodfile)
            DeleteWorkspace(i + "sum")

        k = k + 1
        DeleteWorkspace(i)
        if diagnostics == 0:
            DeleteWorkspace(i + "mon")
            DeleteWorkspace(i + "det")


def findbin(wksp, val):
    a1 = mtd[wksp]
    x1 = a1.readX(0)

    i = None
    for i in range(len(x1) - 1):
        if x1[i] > val:
            break
    return i - 1


def nrDBFn(
    runListShort,
    nameListShort,
    runListLong,
    nameListLong,
    nameListComb,
    minSpec,
    maxSpec,
    minWavelength,
    gparams,
    floodfile="",
    diagnostics="0",
):
    nlistS = parseNameList(nameListShort)
    rlistS = parseRunList(runListShort)
    nlistL = parseNameList(nameListLong)
    rlistL = parseRunList(runListLong)
    nlistComb = parseNameList(nameListComb)

    for i in range(len(rlistS)):
        addRuns(rlistS[i], nlistS[i])
    for i in range(len(rlistL)):
        addRuns(rlistL[i], nlistL[i])

    mon_spec = int(gparams[3]) - 1
    reb = gparams[0] + "," + gparams[1] + "," + gparams[2]

    for i in nlistS:
        ConvertUnits(InputWorkspace=i, OutputWorkspace=i, Target="Wavelength", AlignBins="1")
        Rebin(InputWorkspace=i, OutputWorkspace=i, Params=reb)
        CropWorkspace(InputWorkspace=i, OutputWorkspace=i + "mon", StartWorkspaceIndex=mon_spec, EndWorkspaceIndex=mon_spec)
        if mtd[i].isGroup():
            snames = mtd[i].getNames()
            a1 = mtd[snames[0]]
        else:
            a1 = mtd[i]

        nspec = a1.getNumberHistograms()

        if nspec == 4:
            CropWorkspace(InputWorkspace=i, OutputWorkspace=i + "det", StartWorkspaceIndex=3, EndWorkspaceIndex=3)
            Divide(i + "det", i + "mon", i + "norm")
            ReplaceSpecialValues(i + "norm", i + "norm", "0.0", "0.0", "0.0", "0.0")
        else:
            CropWorkspace(InputWorkspace=i, OutputWorkspace=i + "det", StartWorkspaceIndex=4, EndWorkspaceIndex=243)
            floodnorm(i + "det", floodfile)
            GroupDetectors(
                i + "det", i + "sum", WorkspaceIndexList=list(range(int(minSpec) - 5, int(maxSpec) - 5 + 1)), KeepUngroupedSpectra="0"
            )
            Divide(i + "sum", i + "mon", i + "norm")
            ReplaceSpecialValues(i + "norm", i + "norm", "0.0", "0.0", "0.0", "0.0")

    for i in nlistL:
        ConvertUnits(InputWorkspace=i, OutputWorkspace=i, Target="Wavelength", AlignBins="1")
        Rebin(InputWorkspace=i, OutputWorkspace=i, Params=reb)
        CropWorkspace(InputWorkspace=i, OutputWorkspace=i + "mon", StartWorkspaceIndex=mon_spec, EndWorkspaceIndex=mon_spec)
        if mtd[i].isGroup():
            lnames = mtd[i].getNames()
            a1 = mtd[lnames[0]]
        else:
            a1 = mtd[i]

        nspec = a1.getNumberHistograms()

        if nspec == 4:
            CropWorkspace(InputWorkspace=i, OutputWorkspace=i + "det", StartWorkspaceIndex=3, EndWorkspaceIndex=3)
            Divide(i + "det", i + "mon", i + "norm")
            ReplaceSpecialValues(i + "norm", i + "norm", "0.0", "0.0", "0.0", "0.0")
        else:
            CropWorkspace(InputWorkspace=i, OutputWorkspace=i + "det", StartWorkspaceIndex=4, EndWorkspaceIndex=243)
            floodnorm(i + "det", floodfile)
            GroupDetectors(
                i + "det", i + "sum", WorkspaceIndexList=list(range(int(minSpec) - 5, int(maxSpec) - 5 + 1)), KeepUngroupedSpectra="0"
            )
            Divide(i + "sum", i + "mon", i + "norm")
            ReplaceSpecialValues(i + "norm", i + "norm", "0.0", "0.0", "0.0", "0.0")

    for i in range(len(nlistS)):
        if mtd[nlistS[i] + "norm"].isGroup():
            snames = mtd[nlistS[i] + "norm"].getNames()
            lnames = mtd[nlistL[i] + "norm"].getNames()
            for k in range(len(snames)):
                Integration(snames[k], snames[k] + "int", minWavelength, gparams[2])
                Integration(lnames[k], lnames[k] + "int", minWavelength, gparams[2])
                Multiply(snames[k], lnames[k] + "int", snames[k])
                Divide(snames[k], snames[k] + "int", snames[k])
                a1 = findbin(lnames[k], float(minWavelength))
                MultiplyRange(lnames[k], lnames[k], "0", str(a1), "0.0")
                WeightedMean(snames[k], lnames[k], nlistComb[i] + "_" + str(k + 1))
                if diagnostics == "0":
                    DeleteWorkspace(snames[k] + "int")
                    DeleteWorkspace(lnames[k] + "int")
        else:
            Integration(nlistS[i] + "norm", nlistS[i] + "int", minWavelength, gparams[2])
            Integration(nlistL[i] + "norm", nlistL[i] + "int", minWavelength, gparams[2])
            Multiply(nlistS[i] + "norm", nlistL[i] + "int", nlistS[i] + "norm")
            Divide(nlistS[i] + "norm", nlistS[i] + "int", nlistS[i] + "norm")
            a1 = findbin(nlistL[i] + "norm", float(minWavelength))
            MultiplyRange(nlistL[i] + "norm", nlistL[i] + "norm", "0", str(a1), "0.0")
            WeightedMean(nlistS[i] + "norm", nlistL[i] + "norm", nlistComb[i])
            if diagnostics == "0":
                DeleteWorkspace(nlistS[i] + "int")
                DeleteWorkspace(nlistL[i] + "int")

            if diagnostics == "0":
                DeleteWorkspace(nlistS[i] + "mon")
                DeleteWorkspace(nlistS[i] + "det")
                if nspec != 4:
                    DeleteWorkspace(nlistS[i] + "sum")
                DeleteWorkspace(nlistS[i] + "norm")
                DeleteWorkspace(nlistS[i])
                DeleteWorkspace(nlistL[i] + "mon")
                DeleteWorkspace(nlistL[i] + "det")
                if nspec != 4:
                    DeleteWorkspace(nlistL[i] + "sum")
                DeleteWorkspace(nlistL[i] + "norm")
                DeleteWorkspace(nlistL[i])


def numberofbins(wksp):
    a1 = mtd[wksp]
    y1 = a1.readY(0)
    return len(y1) - 1


def maskbin(wksp, val):
    a1 = mtd[wksp]
    x1 = a1.readX(0)
    i = None
    for i in range(len(x1) - 1):
        if x1[i] > val:
            break
    a1.dataY(0)[i - 1] = 0.0
    a1.dataE(0)[i - 1] = 0.0


def arr2list(iarray):
    # convert array of strings to a single string with commas
    res = ""
    for i in range(len(iarray) - 1):
        res = res + iarray[i] + ","
    res = res + iarray[len(iarray) - 1]
    return res


def NRCombineDatafn(RunsNameList, CombNameList, applySFs, SFList, SFError, scaleOption, bparams, globalSF, applyGlobalSF, diagnostics=0):
    qmin = bparams[0]
    _bin = bparams[1]
    qmax = bparams[2]
    rlist = parseNameList(RunsNameList)
    listsfs = parseNameList(SFList)
    listsfserr = parseNameList(SFError)
    sfs = []
    sferrs = []
    for i in rlist:
        Rebin(i, i + "reb", qmin + "," + _bin + "," + qmax)
    # find the overlap ranges
    bol = []  # beginning of overlaps
    eol = []  # end of overlaps
    for i in range(len(rlist) - 1):
        a1 = mtd[rlist[i + 1]]
        x = a1.readX(0)
        bol.append(x[0])
        a1 = mtd[rlist[i]]
        x = a1.readX(0)
        eol.append(x[len(x) - 1])
    # set the edges of the rebinned data to 0.0 to avoid partial bin problems
    maskbin(rlist[0] + "reb", eol[0])
    if len(rlist) > 2:
        for i in range(1, len(rlist) - 1):
            maskbin(rlist[i] + "reb", bol[i - 1])
            maskbin(rlist[i] + "reb", eol[i])
    maskbin(rlist[len(rlist) - 1] + "reb", bol[len(rlist) - 2])
    # Now find the various scale factors and store in temp workspaces
    for i in range(len(rlist) - 1):
        Integration(rlist[i] + "reb", "i" + str(i) + "1temp", str(bol[i]), str(eol[i]))
        Integration(rlist[i + 1] + "reb", "i" + str(i) + "2temp", str(bol[i]), str(eol[i]))
        if scaleOption != "2":
            Divide("i" + str(i) + "1temp", "i" + str(i) + "2temp", "sf" + str(i))
            a1 = mtd["sf" + str(i)]
            print("sf" + str(i) + "=" + str(a1.readY(0)) + " +/- " + str(a1.readE(0)))
            sfs.append(str(a1.readY(0)[0]))
            sferrs.append(str(a1.readE(0)[0]))
        else:
            Divide("i" + str(i) + "2temp", "i" + str(i) + "1temp", "sf" + str(i))
            print("sf" + str(i) + "=" + str(a1.readY(0)) + " +/- " + str(a1.readE(0)))
            sfs.append(str(a1.readY(0)[0]))
            sferrs.append(str(a1.readE(0)[0]))
        mtd.deleteWorkspace("i" + str(i) + "1temp")
        mtd.deleteWorkspace("i" + str(i) + "2temp")
    # if applying pre-defined scale factors substitute the given values now
    # Note the errors are now set to 0
    if applySFs == "2":
        for i in range(len(rlist) - 1):
            a1 = mtd["sf" + str(i)]
            a1.dataY(0)[0] = float(listsfs[i])
            a1.dataE(0)[0] = float(listsfserr[i])
    # Now scale the various data sets in the correct order
    if scaleOption != "2":
        for i in range(len(rlist) - 1):
            for j in range(i + 1, len(rlist)):
                Multiply(rlist[j] + "reb", "sf" + str(i), rlist[j] + "reb")
    else:
        for i in range(len(rlist) - 1, 0, -1):
            for j in range(i, 0, -1):
                Multiply(rlist[j] + "reb", "sf" + str(i - 1), rlist[j] + "reb")

    WeightedMean(rlist[0] + "reb", rlist[1] + "reb", "currentSum")
    if len(rlist) > 2:
        for i in range(2, len(rlist)):
            WeightedMean("currentSum", rlist[i] + "reb", "currentSum")

    # if applying a global scale factor do it here
    if applyGlobalSF == "2":
        RenameWorkspace("scaledData", CombNameList)
        mtd.deleteWorkspace("currentSum")
    else:
        RenameWorkspace("currentSum", CombNameList)
    for i in range(len(rlist) - 1):
        mtd.deleteWorkspace("sf" + str(i))
    if diagnostics == 0:
        for i in range(len(rlist)):
            mtd.deleteWorkspace(rlist[i] + "reb")
    return [arr2list(sfs), arr2list(sferrs)]


def nrWriteXYE(wksp, fname):
    a1 = mtd[wksp]
    x1 = a1.readX(0)
    X1 = n.zeros((len(x1) - 1))
    for i in range(0, len(x1) - 1):
        X1[i] = (x1[i] + x1[i + 1]) / 2.0
    y1 = a1.readY(0)
    e1 = a1.readE(0)
    f = open(fname, "w")
    for i in range(len(X1)):
        s = ""
        s += "%f," % X1[i]
        s += "%f," % y1[i]
        s += "%f\n" % e1[i]
        f.write(s)
    f.close()


def nrPNRCorrection(UpWksp, DownWksp):
    #   crho=[0.941893,0.0234006,-0.00210536,0.0]
    #   calpha=[0.945088,0.0242861,-0.00213624,0.0]
    #   cAp=[1.00079,-0.0186778,0.00131546,0.0]
    #   cPp=[1.01649,-0.0228172,0.00214626,0.0]
    # Constants Based on Runs 18350+18355 and 18351+18356 analyser theta at -0.1deg
    # 2 RF Flippers as the polarising system
    crho = [1.006831, -0.011467, 0.002244, -0.000095]
    calpha = [1.017526, -0.017183, 0.003136, -0.000140]
    cAp = [0.917940, 0.038265, -0.006645, 0.000282]
    cPp = [0.972762, 0.001828, -0.000261, 0.0]
    Ip = mtd[UpWksp]
    Ia = mtd[DownWksp]
    CloneWorkspace(Ip, "PCalpha")
    CropWorkspace(InputWorkspace="PCalpha", OutputWorkspace="PCalpha", StartWorkspaceIndex="0", EndWorkspaceIndex="0")
    # a1=alpha.readY(0)
    # for i in range(0,len(a1)):
    # alpha.dataY(0)[i]=0.0
    # alpha.dataE(0)[i]=0.0
    CloneWorkspace("PCalpha", "PCrho")
    CloneWorkspace("PCalpha", "PCAp")
    CloneWorkspace("PCalpha", "PCPp")
    rho = mtd["PCrho"]
    Pp = mtd["PCPp"]
    # for i in range(0,len(a1)):
    # x=(alpha.dataX(0)[i]+alpha.dataX(0)[i])/2.0
    # for j in range(0,4):
    # alpha.dataY(0)[i]=alpha.dataY(0)[i]+calpha[j]*x**j
    # rho.dataY(0)[i]=rho.dataY(0)[i]+crho[j]*x**j
    # Ap.dataY(0)[i]=Ap.dataY(0)[i]+cAp[j]*x**j
    # Pp.dataY(0)[i]=Pp.dataY(0)[i]+cPp[j]*x**j
    PolynomialCorrection(InputWorkspace="PCalpha", OutputWorkspace="PCalpha", Coefficients=calpha, Operation="Multiply")
    PolynomialCorrection(InputWorkspace="PCrho", OutputWorkspace="PCrho", Coefficients=crho, Operation="Multiply")
    PolynomialCorrection(InputWorkspace="PCAp", OutputWorkspace="PCAp", Coefficients=cAp, Operation="Multiply")
    PolynomialCorrection(InputWorkspace="PCPp", OutputWorkspace="PCPp", Coefficients=cPp, Operation="Multiply")
    D = Pp * (1.0 + rho)
    nIp = (Ip * (rho * Pp + 1.0) + Ia * (Pp - 1.0)) / D
    nIa = (Ip * (rho * Pp - 1.0) + Ia * (Pp + 1.0)) / D
    RenameWorkspace(nIp, str(Ip) + "corr")
    RenameWorkspace(nIa, str(Ia) + "corr")
    iwksp = mtd.getWorkspaceNames()
    _list = [str(Ip), str(Ia), "PCalpha", "PCrho", "PCAp", "PCPp", "1_p"]
    for i in range(len(iwksp)):
        for j in _list:
            lname = len(j)
            if iwksp[i][0 : lname + 1] == j + "_":
                mtd.deleteWorkspace(iwksp[i])
    mtd.deleteWorkspace("PCalpha")
    mtd.deleteWorkspace("PCrho")
    mtd.deleteWorkspace("PCAp")
    mtd.deleteWorkspace("PCPp")
    mtd.deleteWorkspace("D")


def nrPACorrection(UpUpWksp, UpDownWksp, DownUpWksp, DownDownWksp):
    #   crho=[0.941893,0.0234006,-0.00210536,0.0]
    #   calpha=[0.945088,0.0242861,-0.00213624,0.0]
    #   cAp=[1.00079,-0.0186778,0.00131546,0.0]
    #   cPp=[1.01649,-0.0228172,0.00214626,0.0]
    # Constants Based on Runs 18350+18355 and 18351+18356 analyser theta at -0.1deg
    # 2 RF Flippers as the polarising system
    crho = [1.006831, -0.011467, 0.002244, -0.000095]
    calpha = [1.017526, -0.017183, 0.003136, -0.000140]
    cAp = [0.917940, 0.038265, -0.006645, 0.000282]
    cPp = [0.972762, 0.001828, -0.000261, 0.0]
    Ipp = mtd[UpUpWksp]
    Ipa = mtd[UpDownWksp]
    Iap = mtd[DownUpWksp]
    Iaa = mtd[DownDownWksp]
    CloneWorkspace(Ipp, "PCalpha")
    CropWorkspace(InputWorkspace="PCalpha", OutputWorkspace="PCalpha", StartWorkspaceIndex="0", EndWorkspaceIndex="0")
    alpha = mtd["PCalpha"]
    # a1=alpha.readY(0)
    # for i in range(0,len(a1)):
    # alpha.dataY(0)[i]=0.0
    # alpha.dataE(0)[i]=0.0
    CloneWorkspace("PCalpha", "PCrho")
    CloneWorkspace("PCalpha", "PCAp")
    CloneWorkspace("PCalpha", "PCPp")
    rho = mtd["PCrho"]
    Ap = mtd["PCAp"]
    Pp = mtd["PCPp"]
    # for i in range(0,len(a1)):
    # x=(alpha.dataX(0)[i]+alpha.dataX(0)[i])/2.0
    # for j in range(0,4):
    # alpha.dataY(0)[i]=alpha.dataY(0)[i]+calpha[j]*x**j
    # rho.dataY(0)[i]=rho.dataY(0)[i]+crho[j]*x**j
    # Ap.dataY(0)[i]=Ap.dataY(0)[i]+cAp[j]*x**j
    # Pp.dataY(0)[i]=Pp.dataY(0)[i]+cPp[j]*x**j
    # Use the polynomial corretion fn instead
    PolynomialCorrection(InputWorkspace="PCalpha", OutputWorkspace="PCalpha", Coefficients=calpha, Operation="Multiply")
    PolynomialCorrection(InputWorkspace="PCrho", OutputWorkspace="PCrho", Coefficients=crho, Operation="Multiply")
    PolynomialCorrection(InputWorkspace="PCAp", OutputWorkspace="PCAp", Coefficients=cAp, Operation="Multiply")
    PolynomialCorrection(InputWorkspace="PCPp", OutputWorkspace="PCPp", Coefficients=cPp, Operation="Multiply")

    A0 = (Iaa * Pp * Ap) + (Ap * Ipa * rho * Pp) + (Ap * Iap * Pp * alpha) + (Ipp * Ap * alpha * rho * Pp)
    A1 = Pp * Iaa
    A2 = Pp * Iap
    A3 = Ap * Iaa
    A4 = Ap * Ipa
    A5 = Ap * alpha * Ipp
    A6 = Ap * alpha * Iap
    A7 = Pp * rho * Ipp
    A8 = Pp * rho * Ipa
    D = Pp * Ap * (1.0 + rho + alpha + (rho * alpha))
    nIpp = (A0 - A1 + A2 - A3 + A4 + A5 - A6 + A7 - A8 + Ipp + Iaa - Ipa - Iap) / D
    nIaa = (A0 + A1 - A2 + A3 - A4 - A5 + A6 - A7 + A8 + Ipp + Iaa - Ipa - Iap) / D
    nIpa = (A0 - A1 + A2 + A3 - A4 - A5 + A6 + A7 - A8 - Ipp - Iaa + Ipa + Iap) / D
    nIap = (A0 + A1 - A2 - A3 + A4 + A5 - A6 - A7 + A8 - Ipp - Iaa + Ipa + Iap) / D
    RenameWorkspace(nIpp, str(Ipp) + "corr")
    RenameWorkspace(nIpa, str(Ipa) + "corr")
    RenameWorkspace(nIap, str(Iap) + "corr")
    RenameWorkspace(nIaa, str(Iaa) + "corr")
    ReplaceSpecialValues(str(Ipp) + "corr", str(Ipp) + "corr", NaNValue="0.0", NaNError="0.0", InfinityValue="0.0", InfinityError="0.0")
    ReplaceSpecialValues(str(Ipp) + "corr", str(Ipp) + "corr", NaNValue="0.0", NaNError="0.0", InfinityValue="0.0", InfinityError="0.0")
    ReplaceSpecialValues(str(Ipp) + "corr", str(Ipp) + "corr", NaNValue="0.0", NaNError="0.0", InfinityValue="0.0", InfinityError="0.0")
    ReplaceSpecialValues(str(Ipp) + "corr", str(Ipp) + "corr", NaNValue="0.0", NaNError="0.0", InfinityValue="0.0", InfinityError="0.0")
    iwksp = mtd.getWorkspaceNames()
    _list = [str(Ipp), str(Ipa), str(Iap), str(Iaa), "PCalpha", "PCrho", "PCAp", "PCPp", "1_p"]
    for i in range(len(iwksp)):
        for j in _list:
            lname = len(j)
            if iwksp[i][0 : lname + 1] == j + "_":
                mtd.deleteWorkspace(iwksp[i])
    mtd.deleteWorkspace("PCalpha")
    mtd.deleteWorkspace("PCrho")
    mtd.deleteWorkspace("PCAp")
    mtd.deleteWorkspace("PCPp")
    mtd.deleteWorkspace("A0")
    mtd.deleteWorkspace("A1")
    mtd.deleteWorkspace("A2")
    mtd.deleteWorkspace("A3")
    mtd.deleteWorkspace("A4")
    mtd.deleteWorkspace("A5")
    mtd.deleteWorkspace("A6")
    mtd.deleteWorkspace("A7")
    mtd.deleteWorkspace("A8")
    mtd.deleteWorkspace("D")


def nrPNRFn(  # noqa: C901
    runList,
    nameList,
    incidentAngles,
    DBList,
    specChan,
    minSpec,
    maxSpec,
    gparams,
    floodfile,
    PNRwithPA,
    pnums,
    doCorrs,
    doLDCorrs="0",
    subbgd=0,
    diagnostics=0,
):
    nlist = parseNameList(nameList)
    mtd.sendLogMessage("This is the sample nameslist:" + str(nlist))
    rlist = parseRunList(runList)
    mtd.sendLogMessage("This is the sample runlist:" + str(rlist))
    dlist = parseNameList(DBList)
    mtd.sendLogMessage("This is the Direct Beam nameslist:" + str(dlist))
    incAngles = parseNameList(incidentAngles)
    mtd.sendLogMessage("This incident Angles are:" + str(incAngles))

    if PNRwithPA == "2":
        nper = 4
        mtd.sendLogMessage("PNRwithPA=" + str(PNRwithPA))
        mtd.sendLogMessage(str(pnums))
    else:
        nper = 2

    for i in range(len(rlist)):
        addRuns(rlist[i], nlist[i])

    mon_spec = int(gparams[3]) - 1
    reb = gparams[0] + "," + gparams[1] + "," + gparams[2]

    k = 0
    for i in nlist:
        a1 = mtd[i + "_1"]
        nspec = a1.getNumberHistograms()
        if subbgd == 1 and nspec != 4:
            # If a background subtraction is required sum the bgd outside the
            # area of the detector that is visible through the analyser over all periods and average
            CloneWorkspace(i, "bgdtemp")
            ConvertUnits(InputWorkspace="bgdtemp", OutputWorkspace="bgdtemp", Target="Wavelength", AlignBins="1")
            Rebin(InputWorkspace="bgdtemp", OutputWorkspace="bgdtemp", Params=reb)
            CropWorkspace(InputWorkspace="bgdtemp", OutputWorkspace="bgdtemp", StartWorkspaceIndex=4, EndWorkspaceIndex=243)
            Plus("bgdtemp" + "_" + pnums[0], "bgdtemp" + "_" + pnums[1], OutputWorkspace="wbgdsum")
            if nper > 2:
                for j in range(2, nper):
                    Plus("wbgdsum", "bgdtemp" + "_" + pnums[j], OutputWorkspace="wbgdsum")
            GroupDetectors("wbgdsum", "bgd2", WorkspaceIndexList=list(range(0, 50)), KeepUngroupedSpectra="0")
            GroupDetectors("wbgdsum", "bgd1", WorkspaceIndexList=list(range(160, 240)), KeepUngroupedSpectra="0")
            Plus("bgd1", "bgd2", OutputWorkspace="bgd")
            wbgdtemp = mtd["bgd"] / (130.0 * nper)
            mtd.deleteWorkspace("bgdtemp")
            mtd.deleteWorkspace("wbgdsum")
            mtd.deleteWorkspace("bgd1")
            mtd.deleteWorkspace("bgd2")
            mtd.deleteWorkspace("bgd")

        wksp = i
        ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp, Target="Wavelength", AlignBins="1")
        Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=reb)
        CropWorkspace(InputWorkspace=wksp, OutputWorkspace=wksp + "mon", StartWorkspaceIndex=mon_spec, EndWorkspaceIndex=mon_spec)
        if nspec == 4:
            CropWorkspace(InputWorkspace=wksp, OutputWorkspace=wksp + "det", StartWorkspaceIndex=3, EndWorkspaceIndex=3)
            RotateInstrumentComponent(wksp + "det", "DetectorBench", X="-1.0", Angle=str(2.0 * float(incAngles[k])))
            Divide(LHSWorkspace=wksp + "det", RHSWorkspace=wksp + "mon", OutputWorkspace=wksp + "norm")
            if dlist[k] != "none":
                Divide(LHSWorkspace=wksp + "norm", RHSWorkspace=dlist[k], OutputWorkspace=wksp + "norm")
                ReplaceSpecialValues(wksp + "norm", wksp + "norm", "0.0", "0.0", "0.0", "0.0")
                ConvertUnits(wksp + "norm", wksp + "RvQ", Target="MomentumTransfer")
        else:
            CropWorkspace(InputWorkspace=wksp, OutputWorkspace=wksp + "det", StartWorkspaceIndex=4, EndWorkspaceIndex=243)
            # move the first spectrum in the list onto the beam centre so that when the bench is rotated it's in the right place
            MoveInstrumentComponent(wksp + "det", "DetectorBench", Y=str((125.0 - float(minSpec)) * 1.2e-3))
            # add a bit to the angle to put the first spectrum of the group in the right place
            a1 = 2.0 * float(incAngles[k]) + atan((float(minSpec) - float(specChan)) * 1.2e-3 / 3.53) * 180.0 / pi
            RotateInstrumentComponent(wksp + "det", "DetectorBench", X="-1.0", Angle=str(a1))
            floodnorm(wksp + "det", floodfile)
            if subbgd == 1:
                # Subract a per spectrum background
                Minus(wksp + "det", wbgdtemp, OutputWorkspace=wksp + "det")
                ResetNegatives(InputWorkspace=wksp + "det", OutputWorkspace=wksp + "det", AddMinimum="0", ResetValue="0.0")
                GroupDetectors(
                    wksp + "det",
                    wksp + "sum",
                    WorkspaceIndexList=list(range(int(minSpec) - 5, int(maxSpec) - 5 + 1)),
                    KeepUngroupedSpectra="0",
                )
            else:
                GroupDetectors(
                    wksp + "det",
                    wksp + "sum",
                    WorkspaceIndexList=list(range(int(minSpec) - 5, int(maxSpec) - 5 + 1)),
                    KeepUngroupedSpectra="0",
                )
            RebinToWorkspace(WorkspaceToRebin=wksp + "sum", WorkspaceToMatch=wksp + "mon", OutputWorkspace=wksp + "sum")
            Divide(LHSWorkspace=wksp + "sum", RHSWorkspace=wksp + "mon", OutputWorkspace=wksp + "norm")
            RebinToWorkspace(WorkspaceToRebin=wksp + "det", WorkspaceToMatch=wksp + "mon", OutputWorkspace=wksp + "det")
            Divide(LHSWorkspace=wksp + "det", RHSWorkspace=wksp + "mon", OutputWorkspace=wksp + "detnorm")
            if dlist[k] != "none":
                Divide(LHSWorkspace=wksp + "norm", RHSWorkspace=dlist[k], OutputWorkspace=wksp + "norm")
                ReplaceSpecialValues(wksp + "norm", wksp + "norm", "0.0", "0.0", "0.0", "0.0")
                Divide(LHSWorkspace=wksp + "detnorm", RHSWorkspace=dlist[k], OutputWorkspace=wksp + "detnorm")
                ReplaceSpecialValues(wksp + "detnorm", wksp + "detnorm", "0.0", "0.0", "0.0", "0.0")
            ConvertUnits(wksp + "norm", wksp + "RvQ", Target="MomentumTransfer")
            DeleteWorkspace(wksp + "sum")

            DeleteWorkspace(wksp + "mon")
            DeleteWorkspace(wksp + "det")
        if doCorrs != "0":
            if nper == 2:
                nrPNRCorrection(i + "norm_" + pnums[0], i + "norm_" + pnums[1])
                for j in range(2):
                    RenameWorkspace(InputWorkspace=i + "norm" + "_" + pnums[j] + "corr", OutputWorkspace=i + "normcorr" + "_" + pnums[j])
                GroupWorkspaces(
                    InputWorkspaces=i + "normcorr_" + pnums[0] + "," + i + "normcorr_" + pnums[1], OutputWorkspace=i + "normcorr"
                )
                ConvertUnits(InputWorkspace=i + "normcorr", OutputWorkspace=i + "normcorrRvQ", Target="MomentumTransfer")
                if nspec > 4 and doLDCorrs != "0":
                    nrPNRCorrection(i + "detnorm_" + pnums[0], i + "detnorm_" + pnums[1])
                    for j in range(4):
                        RenameWorkspace(
                            InputWorkspace=i + "detnorm" + "_" + pnums[j] + "corr", OutputWorkspace=i + "detnormcorr" + "_" + pnums[j]
                        )
                    GroupWorkspaces(
                        InputWorkspaces=i + "detnormcorr_" + pnums[0] + "," + i + "detnormcorr_" + pnums[1],
                        OutputWorkspace=i + "detnormcorr",
                    )
            else:
                nrPACorrection(
                    i + "norm" + "_" + pnums[0], i + "norm" + "_" + pnums[1], i + "norm" + "_" + pnums[2], i + "norm" + "_" + pnums[3]
                )
                for j in range(4):
                    RenameWorkspace(InputWorkspace=i + "norm" + "_" + pnums[j] + "corr", OutputWorkspace=i + "normcorr" + "_" + pnums[j])
                GroupWorkspaces(
                    InputWorkspaces=i
                    + "normcorr_"
                    + pnums[0]
                    + ","
                    + i
                    + "normcorr_"
                    + pnums[1]
                    + ","
                    + i
                    + "normcorr_"
                    + pnums[2]
                    + ","
                    + i
                    + "normcorr_"
                    + pnums[3]
                    + "",
                    OutputWorkspace=i + "normcorr",
                )
                ConvertUnits(InputWorkspace=i + "normcorr", OutputWorkspace=i + "normcorrRvQ", Target="MomentumTransfer")
                if nspec > 4 and doLDCorrs != "0":
                    nrPACorrection(
                        i + "detnorm_" + pnums[0], i + "detnorm_" + pnums[1], i + "detnorm_" + pnums[2], i + "detnorm_" + pnums[3]
                    )
                    for j in range(4):
                        RenameWorkspace(
                            InputWorkspace=i + "detnorm" + "_" + pnums[j] + "corr", OutputWorkspace=i + "detnormcorr" + "_" + pnums[j]
                        )
                    GroupWorkspaces(
                        InputWorkspaces=i
                        + "detnormcorr_"
                        + pnums[0]
                        + ","
                        + i
                        + "detnormcorr_"
                        + pnums[1]
                        + ","
                        + i
                        + "detnormcorr_"
                        + pnums[2]
                        + ","
                        + i
                        + "detnormcorr_"
                        + pnums[3]
                        + "",
                        OutputWorkspace=i + "detnormcorr",
                    )
            if diagnostics == 0 and doCorrs != "0":
                DeleteWorkspace(i + "norm")
                DeleteWorkspace(i + "RvQ")
            if diagnostics == 0 and doLDCorrs != "0":
                DeleteWorkspace(i + "detnorm")
        k = k + 1
        DeleteWorkspace(i)
        if subbgd == 1:
            DeleteWorkspace("wbgdtemp")


def tl(wksp, th0, schan):
    pixel = 1.2
    dist = 3630
    ThetaInc = th0 * pi / 180.0
    a1 = mtd[wksp]
    y = a1.readY(0)
    ntc = len(y)
    nspec = a1.getNumberHistograms()
    x1 = n.zeros((nspec + 1, ntc + 1))
    theta = n.zeros((nspec + 1, ntc + 1))
    y1 = n.zeros((nspec, ntc))
    e1 = n.zeros((nspec, ntc))
    for i in range(0, nspec):
        x = a1.readX(i)
        y = a1.readY(i)
        _e = a1.readE(i)
        x1[i, 0 : ntc + 1] = x[0 : ntc + 1]
        theta[i, :] = atan2((i - schan - 0.5) * pixel + dist * tan(ThetaInc), dist) * 180 / pi
        y1[i, 0:ntc] = y[0:ntc]
        e1[i, 0:ntc] = _e[0:ntc]
    x1[nspec, :] = x1[nspec - 1, :]
    theta[nspec, :] = atan2((nspec - schan - 0.5) * pixel + dist * tan(ThetaInc), dist) * 180 / pi
    d1 = [x1, theta, y1, e1]
    return d1


def writemap_tab(dat, th0, spchan, fname):
    a1 = tl(dat, th0, spchan)
    f = open(fname, "w")
    x = a1[0]
    y = a1[1]
    z = a1[2]
    _e = a1[3]
    s = "\t"
    for i in range(0, n.shape(z)[1] - 1):
        s += "%g\t" % ((x[0][i] + x[0][i + 1]) / 2.0)
        s += "%g\t" % ((x[0][i] + x[0][i + 1]) / 2.0)
    s += "\n"
    f.write(s)
    for i in range(0, n.shape(y)[0] - 1):
        s = ""
        s += "%g\t" % ((y[i][0] + y[i + 1][0]) / 2.0)
        for j in range(0, n.shape(z)[1] - 1):
            s += "%g\t" % z[i][j]
            s += "%g\t" % _e[i][j]
        s += "\n"
        f.write(s)
    f.close()


def xye(wksp):
    a1 = mtd[wksp]
    x1 = a1.readX(0)
    X1 = n.zeros((len(x1) - 1))
    for i in range(0, len(x1) - 1):
        X1[i] = (x1[i] + x1[i + 1]) / 2.0
    y1 = a1.readY(0)
    e1 = a1.readE(0)
    d1 = [X1, y1, e1]
    return d1


def writeXYE_tab(dat, fname):
    a1 = xye(dat)
    f = open(fname, "w")
    x = a1[0]
    y = a1[1]
    _e = a1[2]
    s = ""
    s += "x\ty\te\n"
    f.write(s)
    for i in range(len(x)):
        s = ""
        s += "%f\t" % x[i]
        s += "%f\t" % y[i]
        s += "%f\n" % _e[i]
        f.write(s)
    f.close()


# def quickPlot(runlist,dataDir,lmin,reb,lmax,spmin,spmax,output,plotper,polper,zmin,zmax,zlog):

#    isisDataDir=dataDir
#    mtd.sendLogMessage("setting dataDir="+dataDir+" "+isisDataDir)
#    deleteFromRootName(output)
#    addruns(runlist,output)
#    nper=nperFromList(output)
#    rebpars=str(lmin)+","+str(reb)+","+str(lmax)

#    if nper == 1:
#        ConvertUnits(InputWorkspace=output,OutputWorkspace=output,Target="Wavelength",AlignBins="1")
#        Rebin(InputWorkspace=output,OutputWorkspace=output,Params=rebpars)
#        CropWorkspace(InputWorkspace=output,OutputWorkspace=output+"m",StartWorkspaceIndex="1",EndWorkspaceIndex="1")
#        CropWorkspace(InputWorkspace=output,OutputWorkspace=output+"d",StartWorkspaceIndex=str(spmin+4),EndWorkspaceIndex=str(spmax+4))
#        Divide(output+"d",output+"m",output+"n")
#        workspace_mtx=qti.app.mantidUI.importMatrixWorkspace(output+"n")
#        gr2d=workspace_mtx.plotGraph2D()
#        l=gr2d.activeLayer()
#        if zlog == 0:
#            l.setAxisScale(1,zmin,zmax,0)
#        else:
#            l.setAxisScale(1,zmin,zmax,1)
#    else:
#        workspace_mtx=[]
#        nplot=0
#        for i in plotper:
#            ConvertUnits(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i),Target="Wavelength",AlignBins="1")
#            Rebin(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i),Params=rebpars)
#            CropWorkspace(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i)+"m",StartWorkspaceIndex="1",
# EndWorkspaceIndex="1")
#            CropWorkspace(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i)+"d",StartWorkspaceIndex=str(spmin+4),
# EndWorkspaceIndex=str(spmax+4))
#            Divide(output+"_"+str(i)+"d",output+"_"+str(i)+"m",output+"_"+str(i)+"n")
#            workspace_mtx.append(qti.app.mantidUI.importMatrixWorkspace(output+"_"+str(i)+"n"))
#            gr2d=workspace_mtx[nplot].plotGraph2D()
#            nplot=nplot+1
#            l=gr2d.activeLayer()
#            if zlog == 0:
#                l.setAxisScale(1,zmin,zmax,0)
#            else:
#                l.setAxisScale(1,zmin,zmax,1)

#        up=mtd[output+"_2d"]
#        down=mtd[output+"_1d"]
#        asym=(up-down)/(down+up)
#        RenameWorkspace(asym,output+"_asym")
#        ReplaceSpecialValues(output+"_asym",output+"_asym","0.0","0.0","0.0","0.0")
#        workspace_mtx.append(qti.app.mantidUI.importMatrixWorkspace(output+"_asym"))
#        gr2d=workspace_mtx[nplot].plotGraph2D()
#        l=gr2d.activeLayer()
#        l.setAxisScale(1,-1.0,1.0,0)


#    mtd.sendLogMessage("quickPlot Finished")
