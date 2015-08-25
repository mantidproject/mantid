#pylint: disable = too-few-public-methods,old-style-class,redefined-outer-name

from mantid.simpleapi import *
import os.path
import cry_load

NUM_BANK_DIC = { \
    'hrp': 3, \
    'gem': 5, \
    'pol': 6 \
    }

GRP_OF_DIC = { \
    'hrp': 'hrpd_new_072_01_corr.cal'
    }


def rawpath(runno, inst='hrp', Verbose=False):
    ofile = open('//filepath.isis.rl.ac.uk/' + inst + str(runno) + '.raw/windir.txt', 'r')
    line = os.path.abspath(ofile.readline())
    if Verbose:
        print '//filepath.isis.rl.ac.uk/' + inst + str(runno) + '.raw/windir.txt'
        print line
    return line


class Instrument:
    def __init__(self, instr):
        self.name = instr
        self.sname = instr[0:3]
        self.nbank = NUM_BANK_DIC[self.sname]
        self.pc_comp = 'Y:'
        self.grpOfffile = CRY_ini.ANALYSIS_DIR + '/GrpOff/' + GRP_OF_DIC[self.sname]


class Focus(Instrument):
    def __init__(self, instr='hrpd'):
        Instrument.__init__(self, instr)
        self.Instr = Instrument(instr)
        self.isave = False
        self.wkspaces = {}

    def savemode_on(self):
        self.isave = True

    def savemode_off(self):
        self.isave = False

    def load(self, runno, norm=True, path=False, focus=True):
        if path:
            wkspname = runno.split('|')[0]
            FileLoc = runno.split('|')[1]
        elif self.isave:
            ofile = open('//isis/inst$/NDX' + self.Instr.name + '/Instrument/logs/lastrun.txt', 'r')
            lstrun = str(int(ofile.readlines()[0].split()[1]) + 1)
            if runno < 10:
                FileLoc = self.Instr.pc_comp + '/' + self.Instr.sname + lstrun + '.s' + '0' + str(runno)
            else:
                FileLoc = self.Instr.pc_comp + '/' + self.Instr.sname + lstrun + '.s' + str(runno)
            print FileLoc
            wkspname = str(lstrun) + '_' + str(runno)
            tempwkspc = LoadRaw(Filename=FileLoc, OutputWorkspace=wkspname)
            self.wkspaces[wkspname] = tempwkspc
        else:
            FileLoc = rawpath(runno, inst=self.Instr.sname) + '\\' + self.Instr.sname + str(runno) + '.raw'
            if not os.path.exists(FileLoc):
                print 'File not found expected in directory ' + FileLoc + '\n'
                print 'use a.load("wksp|filepath", path=True) instead'
                return
            wkspname = str(runno)
        print FileLoc
        tempwkspc = LoadRaw(Filename=FileLoc, OutputWorkspace=wkspname)
        self.wkspaces[wkspname] = tempwkspc
        if norm:
            NormaliseByCurrent(InputWorkspace=wkspname, OutputWorkspace=wkspname)
        if focus:
            AlignDetectors(InputWorkspace=wkspname, OutputWorkspace=wkspname, CalibrationFile=self.Instr.grpOfffile)
            DiffractionFocussing(InputWorkspace=wkspname, OutputWorkspace=wkspname,
                                 GroupingFileName=self.Instr.grpOfffile)
            blist = range(1, self.Instr.nbank + 1)
            cry_load.split_bank(wkspname, blist, Del=True)


if __name__ == '__main__':
    from SET_env_scripts2_migrated import *

    A_FOCUS = Focus()
    # a_focus.savemode_on() .
    # a_focus.load('TEST|//isis/inst$/ndxhrpd/instrument/data/cycle_11_5/hrp51683.raw', path=True)
    print rawpath(35493, inst='gem', Verbose=False)
    # a_focus.load('TEST|//isis/inst$/ndxhrpd/instrument/data/cycle_11_5/hrp51683.raw', path=True)
    A_FOCUS.load(35493)

