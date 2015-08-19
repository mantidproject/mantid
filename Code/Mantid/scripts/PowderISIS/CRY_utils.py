from mantid.simpleapi import *
from types import *
import re


def getSampleList(basefile, listText, directory=""):
    raw = True
    thedir = ""
    # if directory<>"":
    #	thedir=directory+"/"
    samfillist = []
    p = re.compile("s([0-9]+)[ ]+(.+)")
    m = p.match(listText)
    if m <> None:
        raw = False
        runno = m.group(1)
        listText = m.group(2).rstrip()
    thelist = getList(listText)
    for samlist in thelist:
        samfil = []
        for sam in samlist:
            if sam == 0:
                return [["none"]]
            if raw:
                fname = thedir + basefile + str(sam) + ".raw"
                samfil.append(fname)
            else:
                if sam < 10:
                    sav = "0" + str(sam)
                else:
                    sav = str(sam)
                fname = thedir + basefile + str(runno) + ".s" + sav
                samfil.append(fname)
        samfillist.append(samfil)
    return samfillist


def getList(AString):
    # returns [[n]]
    # 			if Astring='n'
    #         [[n1, n2..ni]]
    #			if Astring='n-ni'
    #         [[n, n+1..n+c-1][n+c, ..n+2c-1]...[n+xc ... p]]
    # 			if Astring='n-p-c'
    AList = []
    numorrange_list = AString.rstrip().split(" ")
    for numorrange in numorrange_list:
        s = re.compile("\+")
        m = s.search(numorrange)
        p = re.compile('[0-9]+')
        limits = p.findall(numorrange)
        if m <> None:
            # '+' found, other cases skipped
            AList.append(limits)
            continue
        if len(limits) == 1:
            # Single run
            startnum = int(limits[0])
            endnum = int(limits[0]) + 1
            sampleLen = 1
        if len(limits) == 2:
            # range 'nnn-ppp'
            startnum = int(limits[0])
            endnum = int(limits[1]) + 1
            sampleLen = 1
        if len(limits) == 3:
            # merge 'nnn-ppp-nmerge'
            startnum = int(limits[0])
            endnum = int(limits[1]) + 1
            sampleLen = int(limits[2])
        addlist = [];
        rnum = startnum
        m = range(startnum, endnum)
        m = m[::sampleLen]
        for i in m:
            for j in range(0, sampleLen):
                rnum = i + j
                if rnum < endnum:
                    addlist.append(rnum)
                else:
                    break
            AList.append(addlist)
            addlist = [];
    return AList


def getListInt(AString):
    # returns [n]
    # 			if Astring='n'
    #         [n1, n2..ni]
    #			if Astring='n-ni'
    AList = []
    numorrange_list = AString.rstrip().split(" ")
    for numorrange in numorrange_list:
        p = re.compile('[0-9]+')
        limits = p.findall(numorrange)
        if len(limits) == 1:
            startnum = int(limits[0])
            endnum = int(limits[0]) + 1
        if len(limits) == 2:
            startnum = int(limits[0])
            endnum = int(limits[1]) + 1
        rnum = startnum
        m = range(startnum, endnum)
        for i in m:
            AList.append(i)
    return AList


def ListOfList2List(lol):
    alist = []
    for item in lol:
        alist.append(item[0])
    return alist


# Correct for absorption an InputWkspc(in D)
# Put its transmission in outputWkspc 
def CorrectAbs(InputWkspc, outputWkspc, TheCylinderSampleHeight, TheCylinderSampleRadius, \
               TheAttenuationXSection, TheScatteringXSection, TheSampleNumberDensity, \
               TheNumberOfSlices, TheNumberOfAnnuli, TheNumberOfWavelengthPoints, TheExpMethod):
    # The input workspace needs to be in units of wavelength for the CylinderAbsorption algorithm
    ConvertUnits(InputWorkspace=InputWkspc, OutputWorkspace=InputWkspc, Target="Wavelength")
    CylinderAbsorption(InputWorkspace=InputWkspc, OutputWorkspace=outputWkspc,
                       CylinderSampleHeight=TheCylinderSampleHeight,
                       CylinderSampleRadius=TheCylinderSampleRadius,
                       AttenuationXSection=TheAttenuationXSection,
                       ScatteringXSection=TheScatteringXSection,
                       SampleNumberDensity=TheSampleNumberDensity,
                       NumberOfSlices=TheNumberOfSlices,
                       NumberOfAnnuli=TheNumberOfAnnuli,
                       NumberOfWavelengthPoints=TheNumberOfWavelengthPoints,
                       ExpMethod=TheExpMethod)
    Divide(LHSWorkspace=InputWkspc, RHSWorkspace=outputWkspc, OutputWorkspace=InputWkspc)
    ConvertUnits(InputWorkspace=InputWkspc, OutputWorkspace=InputWkspc, Target="dSpacing")


if __name__ == '__main__':
    #	AList=getList("1-3 15-150-10 42-44")
    AList = getList("44429+44453")
    print AList
    print getSampleList(expt.basefile, "1000 1245-1268 1308-1400-10", directory=expt.RawDir)
    print getSampleList(expt.basefile, "s41256 1-5 10 15-30-3", directory=expt.RawDir)
# print getListInt("1-3 15-150  42-44")

