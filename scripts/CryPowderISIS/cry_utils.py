#pylint: disable=redefined-outer-name,unused-argument,anomalous-backslash-in-string,too-many-arguments

from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from types import *
import re
import os


def get_sample_list(basefile, listText, direct=""):
    raw = True
    thedir = ""
    if direct:
        thedir = direct
    samfillist = []
    p_list = re.compile("s([0-9]+)[ ]+(.+)")
    m_list = p_list.match(listText)
    if m_list is not None:
        raw = False
        runno = m_list.group(1)
        listText = m_list.group(2).rstrip()
    thelist = git_list(listText)
    for samlist in thelist:
        samfil = []
        for sam in samlist:
            if sam == 0:
                return [["none"]]
            if raw:
                fname = os.path.join(thedir, basefile) + str(sam) + ".raw"
                samfil.append(fname)
            else:
                if sam < 10:
                    sav = "0" + str(sam)
                else:
                    sav = str(sam)
                fname = os.path.join(thedir, basefile) + str(runno) + ".s" + sav
                samfil.append(fname)
        samfillist.append(samfil)
    return samfillist


def git_list(AString):
    # returns [[n]]
    # 			if Astring='n'
    #         [[n1, n2..ni]]
    #			if Astring='n-ni'
    #         [[n, n+1..n+c-1][n+c, ..n+2c-1]...[n+xc ... p]]
    # 			if Astring='n-p-c'
    AList = []
    numorrange_list = AString.rstrip().split(" ")
    for numorrange in numorrange_list:
        slist = re.compile("\+")
        m_list = slist.search(numorrange)
        p_list = re.compile('[0-9]+')
        limits = p_list.findall(numorrange)
        if m_list is not None:
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
        addlist = []
        rnum = startnum
        m_list = range(startnum, endnum)
        m_list = m_list[::sampleLen]
        for i in m_list:
            for j in range(0, sampleLen):
                rnum = i + j
                if rnum < endnum:
                    addlist.append(rnum)
                else:
                    break
            AList.append(addlist)
            addlist = []
    return AList


def get_list_int(AString):
    # returns [n]
    # 			if Astring='n'
    #         [n1, n2..ni]
    #			if Astring='n-ni'
    AList = []
    numorrange_list = AString.rstrip().split(" ")
    for numorrange in numorrange_list:
        p_list = re.compile('[0-9]+')
        limits = p_list.findall(numorrange)
        if len(limits) == 1:
            startnum = int(limits[0])
            endnum = int(limits[0]) + 1
        if len(limits) == 2:
            startnum = int(limits[0])
            endnum = int(limits[1]) + 1
        # rnum = startnum
        m_list = range(startnum, endnum)
        for i in m_list:
            AList.append(i)
    return AList


def list_of_list2_list(lol):
    alist = []
    for item in lol:
        alist.append(item[0])
    return alist


# Correct for absorption an InputWkspc(in D).
# Put its transmission in outputWkspc

def correct_abs(InputWkspc, outputWkspc, TheCylinderSampleHeight, TheCylinderSampleRadius,
                TheAttenuationXSection, TheScatteringXSection, TheSampleNumberDensity,
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
    #	AList=git_list("1-3 15-150-10 42-44")
    G_LIST = git_list("44429+44453")
    print(G_LIST)
    print(get_sample_list(EXPR_FILE.basefile, "1000 1245-1268 1308-1400-10", direct=EXPR_FILE.RawDir))
    print(get_sample_list(EXPR_FILE.basefile, "s41256 1-5 10 15-30-3", direct=EXPR_FILE.RawDir))
# print(get_list_int("1-3 15-150  42-44"))
