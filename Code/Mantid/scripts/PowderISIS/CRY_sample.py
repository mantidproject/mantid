from mantid.simpleapi import *
import re
import CRY_load
import CRY_ini
import CRY_utils


def getDataSum(sampleAdd, samLab, expt):
    # Adds a list of data files given as a list of filenames
    # into the workspace samLab. Zero uamps runs are skipped
    # Function returns (BasenameRunno,uampstot) the first non-Zero run
    uampstotal = 0.0
    firstnonzeronotfound = 1
    for SampleFn in sampleAdd:
        if SampleFn == "none":
            return ('', 0)
        if firstnonzeronotfound:  # Load first run
            uamps, uampstot = CRY_load.Load(samLab, SampleFn, expt, add=False)
        else:  # Or add a new run
            uamps, uampstot = CRY_load.Load(samLab, SampleFn, expt, add=True)
        if uamps > 1e-6:
            uampstotal = uampstotal + uamps
            print "'w' uamps = " + str(uampstot) + 'Data uamps =' + str(uamps) + 'Manually computed sum=' + str(
                uampstotal)
            if firstnonzeronotfound:
                # Gets BasenameRunno from SampleFn using RegEx
                # try to find it as path\BasenameRunno.raw
                pseaerch = ".*(" + expt.basefile + "[0-9]+)\.raw"
                p = re.compile(pseaerch)
                m = p.match(SampleFn)
                if m <> None:
                    sampleout = m.group(1)
                else:
                    # else... try to it find as path\BasenameRunno.s*
                    pseaerch = ".*(" + expt.basefile + "[0-9]+)\.s([0-9]+)"
                    p = re.compile(pseaerch)
                    m = p.match(SampleFn)
                    sampleout = m.group(1) + "_" + m.group(2)
                firstnonzeronotfound = 0
                msg = SampleFn + " Loaded with " + str(uampstotal) + " uamp"
            else:
                msg = msg + '\n' + SampleFn + " Added with " + str(uamps) + " uamp, Total=" + str(uampstotal)
    if firstnonzeronotfound:
        return ('', 0)
    print msg
    # NormaliseByCurrent(samLab,samLab)
    CreateSingleValuedWorkspace(OutputWorkspace='totuamps', DataValue=uampstotal)
    Divide(LHSWorkspace=samLab, RHSWorkspace='totuamps', OutputWorkspace=samLab)
    return (sampleout, uampstotal)


if __name__ == '__main__':
    expt = CRY_ini.files("hrpd")
    expt.initialize('Cycle09_2', 'tests', prefFile="mtd_tst.pref")
    expt.tell()
# CorrectVana(VanFile,EmptyFile,AcalibFile,1)
# normalizeall(SampleDatFile,AcalibFile)
# rearrangeallbanks(OutputFile,"")

