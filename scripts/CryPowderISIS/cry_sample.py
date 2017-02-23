#pylint: disable=anomalous-backslash-in-string,redefined-outer-name

from __future__ import (absolute_import, division, print_function)
import re
from mantid.simpleapi import *
import cry_load


def get_data_sum(sampleAdd, samLab, EXPR_FILE):
    # Adds a list of data files given as a list of filenames.
    # into the workspace samLab. Zero uamps runs are skipped
    # Function returns (BasenameRunno,uampstot) the first non-Zero run
    uampstotal = 0.0
    firstnonzeronotfound = 1
    for SampleFn in sampleAdd:
        if SampleFn == "none":
            return ('', 0)
        if firstnonzeronotfound:  # load first run
            uamps, uampstot = cry_load.load(samLab, SampleFn, EXPR_FILE, add=False)
        else:  # Or add a new run
            uamps, uampstot = cry_load.load(samLab, SampleFn, EXPR_FILE, add=True)
        if uamps > 1e-6:
            uampstotal = uampstotal + uamps
            print("'w' uamps = " + str(uampstot) + 'Data uamps =' + str(uamps) + 'Manually computed sum=' + str(
                uampstotal))
            if firstnonzeronotfound:
                # Gets BasenameRunno from SampleFn using RegEx
                # try to find it as path\BasenameRunno.raw
                pseaerch = ".*(" + EXPR_FILE.basefile + "[0-9]+)\.raw"
                p_wrd = re.compile(pseaerch)
                m_wrd = p_wrd.match(SampleFn)
                if m_wrd is not None:
                    sampleout = m_wrd.group(1)
                else:
                    # else... try to it find as path\BasenameRunno.s*
                    pseaerch = ".*(" + EXPR_FILE.basefile + "[0-9]+)\.s([0-9]+)"
                    p_wrd = re.compile(pseaerch)
                    m_wrd = p_wrd.match(SampleFn)
                    sampleout = m_wrd.group(1) + "_" + m_wrd.group(2)
                firstnonzeronotfound = 0
                msg = SampleFn + " Loaded with " + str(uampstotal) + " uamp"
            else:
                msg = msg + '\n' + SampleFn + " Added with " + str(uamps) + " uamp, Total=" + str(uampstotal)
    if firstnonzeronotfound:
        return ('', 0)
    print(msg)
    # NormaliseByCurrent(samLab,samLab)
    CreateSingleValuedWorkspace(OutputWorkspace='totuamps', DataValue=uampstotal)
    Divide(LHSWorkspace=samLab, RHSWorkspace='totuamps', OutputWorkspace=samLab)
    return (sampleout, uampstotal)


if __name__ == '__main__':
    EXPR_FILE = CRY_ini.File("hrpd")
    EXPR_FILE.initialize('Cycle09_2', 'tests', prefFile="mtd_tst.pref")
    EXPR_FILE.tell()
# CorrectVana(VanFile,EmptyFile,AcalibFile,1)
# normalizeall(SampleDatFile,AcalibFile)
# rearrangeallbanks(OutputFile,"")
