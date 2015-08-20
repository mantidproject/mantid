from mantid.simpleapi import *
from types import *
import math

old_version = False


# First a function definition for the Loading algorithms which
# loads the data and immediately aligns the detectors
def load(outputArea, pathtofile, expt_files, add=False):
    total = outputArea
    if add:
        outputArea = outputArea + "add"
    # try:
    #		fin=open(pathtofile,"r")
    #	except:
    #		print 'WARNING!!!!!!!  '+pathtofile+' NOT FOUND'
    #		raise
    #	else:
    #		fin.close()
    if old_version:
        LoadRaw(Filename=pathtofile, OutputWorkspace=outputArea)
    else:
        LoadRaw(Filename=pathtofile, OutputWorkspace=outputArea, LoadLogFiles=False)
    if expt_files.instr == "hrpd":
        removeallpromptpulses(outputArea)
    uamps = mtd[outputArea].getRun().getProtonCharge()
    if add:
        Plus(LHSWorkspace=outputArea, RHSWorkspace=total, OutputWorkspace=total)
        # total=outputArea+total
        mtd.remove(outputArea)
    uampstot = mtd[total].getRun().getProtonCharge()
    return uamps, uampstot


def align_fnc(outputArea, expt_files):
    AlignDetectors(InputWorkspace=outputArea, OutputWorkspace=outputArea, CalibrationFile=expt_files.Path2OffFile)


def split_bank(InputArea, bankList, Del=True):
    for i in bankList:
        CropWorkspace(InputWorkspace=InputArea, OutputWorkspace=InputArea + "-" + str(i), StartWorkspaceIndex=i - 1,
                      EndWorkspaceIndex=i - 1)
    if Del:
        mtd.remove(InputArea)


def bin_bank(InputArea, bankList, Drange):
    for i in bankList:
        Rebin(InputWorkspace=InputArea + "-" + str(i), OutputWorkspace=InputArea + "-" + str(i), Params=Drange[i - 1])


def sets_drange(wkspc, expt_files):
    datamatrix = mtd[wkspc]
    for i in range(0, expt_files.Nbank):
        x_data = datamatrix.readX(i)
        last = len(x_data) - 1
        CropRange = expt_files.CropRange[i].rstrip().split()
        xbegin = str(x_data[0] * (1 + float(CropRange[0])))
        xend = str(x_data[last] * float(CropRange[1]))
        datbin = math.exp(math.log(x_data[last] / x_data[0]) / last) - 1
        if datbin > float(expt_files.Bining[i]):
            print 'WARNING: Rebining in *pref file ' + expt_files.Bining[
                i] + ' is lower than diffraction focusing rebining step'
            print 'WARNING: Rebining Kept to be ' + str(datbin) + ' for bank ' + str(i + 1)
            expt_files.Bining[i] = str(datbin)
        Drange = xbegin + ",-" + expt_files.Bining[i] + "," + xend
        expt_files.Drange.append(Drange)
    expt_files.dataRangeSet = True
    return setsDrange


def scale_wspc(InputArea, scale):
    CreateSingleValuedWorkspace(OutputWorkspace="scale", DataValue=str(scale))
    Multiply(LHSWorkspace=InputArea, RHSWorkspace="scale", OutputWorkspace=InputArea)
    mtd.remove("scale")


def removepromptpulse(wkspace, midle, leftcrop, rightcrop):
    mincrop = str(midle - leftcrop)
    maxcrop = str(midle + rightcrop)
    # RemoveBins(InputWorkspace=wkspace, OutputWorkspace=wkspace, XMin=mincrop, XMax=maxcrop)
    MaskBins(InputWorkspace=wkspace, OutputWorkspace=wkspace, XMin=mincrop, XMax=maxcrop)


def removeallpromptpulses(wkspace):
    raw_workspace = mtd[wkspace]
    x_data = raw_workspace.readX(1)
    ppulsemin = int(round(x_data[0]) / 20000.0) + 1
    ppulsemax = int(round(x_data[-1]) / 20000.0) + 1
    for i in range(ppulsemin, ppulsemax):
        removepromptpulse(wkspace, 20000.0 * float(i), 30.0, 140.0)


if __name__ == '__main__':
    print get_sample_list("1000 1245-1268 1308-1400-10")
# CorrectVana(VanFile,EmptyFile,AcalibFile,1)
# normalizeall(SampleDatFile,AcalibFile)
# rearrangeallbanks(OutputFile,"")
