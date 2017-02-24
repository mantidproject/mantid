#pylint: disable=attribute-defined-outside-init,undefined-loop-variable,too-many-arguments,too-many-branches,
#pylint: disable=too-many-instance-attributes,old-style-class,global-variable-not-assigned

from __future__ import (absolute_import, division, print_function)
import cry_utils
import re
import os.path
from os.path import abspath, join, dirname

ANALYSIS_DIR = ''
#print('--------------------- ')
#print('Using the B:\\MantidPowderFocus\\\scripts2\\CRY_ini.py scripts... ')
#print('--------------------- ')


class Files:
    def __init__(self, instr, RawDir="", Analysisdir="", UnitTest=False, debugMode=False, forceRootDirFromScripts=False, inputInstDir=''):
        global ANALYSIS_DIR
        self.debugMode = False
        if debugMode:
            self.debugMode = True
        self.instr = instr.lower()
        if forceRootDirFromScripts:
            self.InsDir = join(dirname(abspath(__file__)), instr)
        else:
            self.InsDir = join(inputInstDir, instr)
        print('--------------------- ')
        print('INSTRUMENT DIRECTORY: ' + self.InsDir)
        print('--------------------- ')
        self.RawDir = RawDir
        self.user = ""
        self.basefile = instr[0:3]
        self.LstPrefDir = ""
        self.LstPreffil = ""
        self.VanFile = []
        self.VEmptyFile = []
        self.SEmptyFile = []
        self.CorrVanFile = ""
        self.ExistV = "no"
        self.VGrpfocus = "van"
        self.Path2DatGrpFile = ""
        self.VHeight = "2"
        self.VRadius = "0.4"
        self.VAttenuationXSection = "5.1"
        self.VScatteringXSection = "5.08"
        self.VanaNumberDensity = "0.072"
        self.VNumberOfSlices = "10"
        self.VNumberOfAnnuli = "10"
        self.VNumberOfWavelengthPoints = "100"
        self.VExpMethod = "Normal"
        self.VanSmooth = "100"
        self.VanSplineCoef = "15"
        # Peak removal, default, interpolation
        self.VanPeakRemove = "interpol"
        self.VanPeakFile = 'VanaPeaks.dat'
        self.VanPeakList = []
        # Peak removal, in the future, stripeaks with params....
        # self.VanPeakRemove           = "strip"
        self.VanPeakWdt = [20, 50, 0]
        self.VanPeakTol = [4, 4, 0]
        #
        self.CorrectSampleAbs = "no"
        self.SampleAbsCorrected = False
        self.SampleHeight = "0.0"
        self.SampleRadius = "0.0"
        self.SampleAttenuationXSection = "0.0"
        self.SampleScatteringXSection = "0.0"
        self.SampleNumberDensity = "1"
        self.SampleNumberOfSlices = "10"
        self.SampleNumberOfAnnuli = "10"
        self.SampleNumberOfWavelengthPoints = "100"
        self.SampleExpMethod = "Normal"
        # Efficiency correction : Hard Coded for HRPD
        self.LowerLambda = 1.4
        self.UpperLambda = 3.0
        #
        self.OutSuf = ""
        self.XYEDspc = "yes"
        self.XYEtof = "yes"
        self.GSS = "yes"
        self.Nex = "yes"
        self.NBank = 3
        self.bankList = [1, 2, 3]
        self.Drange = []
        # Default values if none given in Arguments
        os.chdir(self.InsDir)
        if UnitTest:
            self.Analysisdir = join(self.InsDir, 'UnitTest')
            self.RawDir = join(self.InsDir, 'UnitTest', 'RAW')
        else:
            if RawDir != "":
                self.RawDir = RawDir
            else:
                self.RawDir = ''
            if Analysisdir != "":
                self.Analysisdir = abspath(Analysisdir)
            else:
                self.Analysisdir = abspath(ANALYSIS_DIR)
        self.OffDir = join(self.Analysisdir, "GrpOff")
        self.GrpDir = join(self.Analysisdir, "GrpOff")

    def initialize(self, Cycle, user, prefFile="", prefDir="", Verbose=False):
        msg = ""
        ierr = 0
        self.cycle = join(self.Analysisdir, Cycle)
        if not os.path.exists(self.cycle):
            os.makedirs(self.cycle)

        self.calib = join(self.Analysisdir, Cycle, "Calibration")
        if not os.path.exists(self.calib):
            os.makedirs(self.calib)

        self.user = join(self.Analysisdir, Cycle, user)
        if not os.path.exists(self.user):
            os.makedirs(self.user)

        if prefFile != "":
            self.LstPreffil = prefFile
        else:
            self.LstPreffil = "mtd.pref"

        if prefDir != "":
            self.LstPrefDir = prefDir
        else:
            self.LstPrefDir = self.user

        # Check existence of preference file
        os.chdir(self.LstPrefDir)

        print('look for file --->')
        print(self.LstPrefDir + self.LstPreffil)
        prefFilePath = join(self.LstPrefDir, self.LstPreffil)
        fexist = os.path.exists(prefFilePath)
        if not fexist:
            msg = msg + "Abort: Required Preference file :" + self.LstPreffil
            msg = msg + "\n NOT FOUND in the directory :" + self.LstPrefDir + '\n'
            raise RuntimeError(msg)
        else:
            msg = "Preference file :" + self.LstPreffil + " FOUND in the directory :" + self.LstPrefDir + "\n"
        print(msg)

        # Read preference file: default, read_prefline-label ALWAYS REQUIRED in pref-file
        # Read preference file: optional params read with updatePrefVal
        self.OffDir = self.updatePrefVal("OffDir", self.OffDir)
        self.GrpDir = self.updatePrefVal("GrpDir", self.GrpDir)
        self.OffFile = self.read_prefline("Offsets")
        self.GrpFile = self.read_prefline("Grouping")
        if self.RawDir == '':
            self.RawDir = join(r'\\isis\inst$\ndx%s' % self.instr, 'instrument', 'data', Cycle)
        else:
            self.RawDir = self.updatePrefVal("RawDir", self.RawDir)
        self.VanDir = self.RawDir
        self.VEmptyDir = self.RawDir
        self.SEmptyDir = self.RawDir
        self.VanDir = self.updatePrefVal("VanDir", self.VanDir)
        self.VEmptyDir = self.updatePrefVal("VEmptyDir", self.VEmptyDir)
        self.SEmptyDir = self.updatePrefVal("SEmptyDir", self.SEmptyDir)
        self.VrunnoList = self.read_prefline("Vanadium")
        self.VErunnoList = self.read_prefline("V-Empty")
        self.SErunnoList = self.read_prefline("S-Empty")
        # ===== Vanadium file handling =====
        AutoVan = False
        self.ExistV = self.read_prefline("ExistingV")
        self.CorrVanFile = self.updatePrefVal("CorrVanFile", self.CorrVanFile)
        if self.CorrVanFile == "":
            AutoVan = True
        self.CorrVanDir = join(self.Analysisdir, Cycle, "Calibration")
        self.CorrVanDir = self.updatePrefVal("CorrVanDir", self.CorrVanDir)
        #
        self.VHeight = self.updatePrefVal("VHeight", self.VHeight)
        self.VRadius = self.updatePrefVal("VRadius", self.VRadius)
        self.VAttenuationXSection = self.updatePrefVal("VAttenuationXSection", self.VAttenuationXSection)
        self.VScatteringXSection = self.updatePrefVal("VScatteringXSection", self.VScatteringXSection)
        self.VanaNumberDensity = self.updatePrefVal("VanaNumberDensity", self.VanaNumberDensity)
        self.VNumberOfSlices = self.updatePrefVal("VNumberOfSlices", self.VNumberOfSlices)
        self.VNumberOfAnnuli = self.updatePrefVal("VNumberOfAnnuli", self.VNumberOfAnnuli)
        self.VNumberOfWavelengthPoints = self.updatePrefVal("VNumberOfWavelengthPoints", self.VNumberOfWavelengthPoints)
        self.VExpMethod = self.updatePrefVal("VExpMethod", self.VExpMethod)
        self.VanPeakRemove = self.updatePrefVal("VanPeakRemove", self.VanPeakRemove)
        self.VanPeakFile = self.updatePrefVal("VanPeakFile", self.VanPeakFile)
        self.VanSmooth = self.updatePrefVal("VanSmooth", self.VanSmooth)
        self.VanSplineCoef = self.updatePrefVal("VanSplineCoef", self.VanSplineCoef)
        # ===== Sac and Efficcinecy correction file handling =====
        self.LowerLambda = self.updatePrefVal("LowerLambda", self.LowerLambda)
        self.UpperLambda = self.updatePrefVal("UpperLambda", self.UpperLambda)
        self.SacEffDir = join(self.Analysisdir, Cycle, "Calibration")
        self.SacEffDir = self.updatePrefVal("SacEffDir", self.SacEffDir)
        self.SacEffFile = self.CorrVanFile + "_detcorr"
        if AutoVan:
            self.CorrVanFile = "van_" + self.VrunnoList + "_" + self.VErunnoList
            self.SacEffFile = "corr_" + self.VrunnoList + "_" + self.VErunnoList
        else:
            self.SacEffFile = self.updatePrefVal("SacEffFile", self.SacEffFile)
        # ===== Set all Calibration file access =====
        self.updateCalib()
        self.VanCorrFileLookup()
        #
        # ===== output params =====
        self.CorrectSampleAbs = self.updatePrefVal("CorrectSampleAbs", self.CorrectSampleAbs)
        self.SampleHeight = self.updatePrefVal("SampleHeight", self.SampleHeight)
        self.SampleHeight = self.updatePrefVal("SampleHeight", self.SampleHeight)
        self.SampleRadius = self.updatePrefVal("SampleRadius", self.SampleRadius)
        self.SampleAttenuationXSection = self.updatePrefVal("SampleAttenuationXSection", self.SampleAttenuationXSection)
        self.SampleScatteringXSection = self.updatePrefVal("SampleScatteringXSection", self.SampleScatteringXSection)
        self.SampleNumberDensity = self.updatePrefVal("SampleNumberDensity", self.SampleNumberDensity)
        self.SampleNumberOfSlices = self.updatePrefVal("SampleNumberOfSlices", self.SampleNumberOfSlices)
        self.SampleNumberOfAnnuli = self.updatePrefVal("SampleNumberOfAnnuli", self.SampleNumberOfAnnuli)
        self.SampleNumberOfWavelengthPoints = self.updatePrefVal("SampleNumberOfWavelengthPoints",
                                                                 self.SampleNumberOfWavelengthPoints)
        self.SampleExpMethod = self.updatePrefVal("SampleExpMethod", self.SampleExpMethod)
        self.scale = self.read_prefline("Scale")
        if self.scale == "":
            self.scale = "1"
        self.OutSuf = self.updatePrefVal("Output", self.OutSuf)
        self.XYEtof = self.read_prefline("XYE-TOF")
        self.XYEdspc = self.read_prefline("XYE-D")
        self.GSS = self.read_prefline("GSS")
        self.Nex = self.read_prefline("Nexus")
        self.saveXYEtof = (self.XYEtof == "yes")
        self.saveXYEd = (self.XYEdspc == "yes")
        Nbank = self.read_prefline("Nbank")
        self.Nbank = int(Nbank)
        self.CropRange = self.read_multprefline("CropRange", self.Nbank)
        self.Bining = self.read_multprefline("Bining", self.Nbank)
        self.dataRangeSet = False
        if self.saveXYEtof or self.saveXYEd:
            bankList = self.read_prefline("BankList")
            self.bankList = cry_utils.get_list_int(bankList)
        print("Done")
        if Verbose:
            self.tell()
        return ierr

    def VanCorrFileLookup(self):
        msg = ''
        VanOK = 0
        for i in self.bankList:
            spec = i - 1
            fexist = os.path.exists(self.CorrVanFile + "-" + str(spec) + ".nxs")
            if not fexist:
                VanOK = i
        if self.ExistV == "yes" or self.ExistV == "sam":
            if VanOK != 0:
                msg = msg + "At least the corrected vanadium file " + self.CorrVanFile + "-" + str(
                    i) + ".nxs is  missing \n"
                if self.ExistV == "sam":
                    msg = msg + "Vanadium runs will be created in due time according to the grouping deduced from 1st \
                    sample dead detector list\n"
                    msg = msg + "Using as parameters: \n"
                    self.VGrpfocus = "sam"
                if self.ExistV == "yes":
                    msg = msg + "I'm re-creating the van nexus files using as parameters: \n"
                self.ExistV = "no"
            else:
                msg = msg + "All the corrected vanadium file " + self.CorrVanFile + "-i.nxs exists and will be used \n"
                self.ExistV = "load"
                print('got Here!!!')
        else:
            if self.ExistV == "over":
                msg = msg + "A corrected Van file will be created with base name " + self.CorrVanFile + " overriding \
                any previous one created before\n"
            else:
                msg = msg + "At least the corrected Van file " + self.CorrVanFile + "-" + str(
                    i) + ".nxs given in the pref file already exists,\n"
                self.CorrVanFile = self.CorrVanFile + "_new"
                msg = msg + "New ones will be created changing the base name to : "
            self.ExistV = "no"
        if self.ExistV == "no":
            msg = msg + "File : " + self.CorrVanFile + ".nxs, using as parameters:\n"
            msg = msg + "VHeight                 = " + self.VHeight + "\n"
            msg = msg + "VRadius                 = " + self.VRadius + "\n"
            msg = msg + "AttenuationXSection     = " + self.VAttenuationXSection + "\n"
            msg = msg + "ScatteringXSection      = " + self.VScatteringXSection + "\n"
            msg = msg + "SampleNumberDensity     = " + self.VanaNumberDensity + "\n"
            msg = msg + "NumberOfSlices          = " + self.VNumberOfSlices + "\n"
            msg = msg + "NumberOfAnnuli          = " + self.VNumberOfAnnuli + "\n"
            msg = msg + "NumberOfWavelengthPoints= " + self.VNumberOfWavelengthPoints + "\n"
            msg = msg + "ExpMethod               = " + self.VExpMethod
        print('after Lookup: ' + self.ExistV)
        #
        if self.VanPeakRemove == "interpol" or self.VanPeakRemove == "spline":
            fexist = os.path.exists(self.InsDir + "/" + self.VanPeakFile)
            if not fexist:
                msg = msg + "Vanadium Peak file " + self.InsDir + "/" + self.VanPeakFile + " not found, \n"
                msg = msg + "I will not strip any peak...! \n"
            else:
                fvan = open(self.InsDir + "/" + self.VanPeakFile, 'r')
                b_int = -1
                for line in fvan:
                    p_lst = re.compile("bank")
                    m_lst = p_lst.search(line)
                    if m_lst is not None:
                        self.VanPeakList.append([])
                        b_int = b_int + 1
                        continue
                    numbers = line.rstrip().split()
                    self.VanPeakList[b_int].append([float(numbers[0]), float(numbers[1])])
                print("Vana Peaks will be removed by interpolation in ranges given in")
                print(self.InsDir + "/" + self.VanPeakFile + '\n')
                fvan.close()
        else:
            self.VanPeakWdt = self.updatePrefVal("VanPeakWdt", self.VanPeakWdt)
            self.VanPeakTol = self.updatePrefVal("VanPeakTol", self.VanPeakTol)
        print(msg)

    def setCommonRawDir(self, RawDir=""):
        if RawDir != "":
            self.RawDir = RawDir
        self.VanDir = self.RawDir
        self.VEmptyDir = self.RawDir
        self.SEmptyDir = self.RawDir

    def updateCalib(self):
        self.VanFile = cry_utils.list_of_list2_list(cry_utils.get_sample_list(self.basefile, self.VrunnoList, self.VanDir))
        self.VEmptyFile = cry_utils.list_of_list2_list(
            cry_utils.get_sample_list(self.basefile, self.VErunnoList, self.VEmptyDir))
        self.SEmptyFile = cry_utils.list_of_list2_list(
            cry_utils.get_sample_list(self.basefile, self.SErunnoList, self.SEmptyDir))
        self.Path2OffFile = self.OffDir + "/" + self.OffFile
        self.Path2GrpFile = self.GrpDir + "/" + self.GrpFile
        self.CorrVanFile = self.CorrVanDir + "/" + self.CorrVanFile
        self.SacEffFile = self.SacEffDir + "/" + self.SacEffFile

    def updatePrefVal(self, Label, updated):
        Field2Update = self.read_prefline(Label, optional=True)
        if Field2Update == "":
            return updated
        else:
            return Field2Update

    def setRawFile(self, runno):
        if runno == 0:
            return "none"
        else:
            return self.RawDir + "/" + self.basefile + str(runno) + ".raw"

    def read_prefline(self, label, optional=False):
        os.chdir(self.LstPrefDir)
        fin = open(self.LstPreffil, "r")
        found = False
        for line in fin:
            pattern2match = label + '[ ]*(.*)'
            p_lst = re.compile(pattern2match)
            m_lst = p_lst.match(line)
            if m_lst is not None:
                found = True
                break
        fin.close()
        if found:
            return m_lst.group(1).rstrip()
        else:
            if optional:
                return ""
            else:
                print("Severe Warning: Field " + label + " required in pref file")
                print("PROGRAM MIGHT CRASH!...")
                return ""

    def write_prefline(self, label, value):
        os.chdir(self.LstPrefDir)
        fin = open(self.LstPreffil, "r")
        fileline = []
        for line in fin:
            pattern2match = '(' + label + '[ ]+)'
            p_lst = re.compile(pattern2match)
            m_lst = p_lst.match(line)
            if m_lst is not None:
                line = m_lst.group() + value + "\n"
            fileline.append(line)
        fin.close()
        fin = open(self.LstPreffil, "w")
        for line in fileline:
            fin.write(line)
        fin.close()

    def read_multprefline(self, label, n_int):
        fin = open(self.LstPreffil, "r")
        # found = False
        output = []
        i = 0
        for line in fin:
            pattern2match = label + '[ ]+(.+)'
            p_lst = re.compile(pattern2match)
            m_lst = p_lst.match(line)
            if m_lst is not None:
                # found = True
                i += 1
                output.append(m_lst.group(1).rstrip())
            if i == n_int:
                break
        fin.close()
        return output

    def read_dir(self, fin, col):
        for line in fin:
            if line[0] != "#":
                break
        alist = line.rstrip().split()
        return alist[col - 1]

    def tell(self):
        msg = ""
        msg = msg + "InsDir                  " + self.InsDir + "\n"
        msg = msg + "RawDir                  " + self.RawDir + "\n"
        msg = msg + "Analysisdir             " + self.Analysisdir + "\n"
        msg = msg + "user                    " + self.user + "\n"
        msg = msg + "basefile                " + self.basefile + "\n"
        msg = msg + "OffFile                 " + self.Path2OffFile + "\n"
        msg = msg + "GrpFile                 " + self.Path2GrpFile + "\n"
        msg = msg + "LstPrefDir              " + self.LstPrefDir + "\n"
        msg = msg + "LstPreffil              " + self.LstPreffil + "\n"
        msg = msg + "Analysisdir             " + self.Analysisdir + "\n"
        for afile in self.VanFile:
            msg = msg + "VanFile                 " + afile + "\n"
        for afile in self.VEmptyFile:
            msg = msg + "VEmptyFile              " + afile + "\n"
        for afile in self.SEmptyFile:
            msg = msg + "SEmptyFile              " + afile + "\n"
        msg = msg + "ExistingV               " + self.ExistV + "\n"
        msg = msg + "CorrVanFile             " + self.CorrVanFile + ".nxs" + "\n"
        msg = msg + "VHeight                 " + self.VHeight + "\n"
        msg = msg + "VRadius                 " + self.VRadius + "\n"
        msg = msg + "AttenuationXSection     " + self.VAttenuationXSection + "\n"
        msg = msg + "ScatteringXSection      " + self.VScatteringXSection + "\n"
        msg = msg + "SampleNumberDensity     " + self.VanaNumberDensity + "\n"
        msg = msg + "NumberOfSlices          " + self.VNumberOfSlices + "\n"
        msg = msg + "NumberOfAnnuli          " + self.VNumberOfAnnuli + "\n"
        msg = msg + "NumberOfWavelengthPoints" + self.VNumberOfWavelengthPoints + "\n"
        msg = msg + "ExpMethod               " + self.VExpMethod + "\n"
        msg = msg + "VanSmooth               " + self.VanSmooth + "\n"
        msg = msg + "VanPeakRemove           " + self.VanPeakRemove + "\n"
        msg = msg + "SacEffFile              " + self.SacEffFile + "\n"
        msg = msg + "SampleHeight                  " + self.SampleHeight + "\n"
        msg = msg + "SampleRadius                  " + self.SampleRadius + "\n"
        msg = msg + "SampleAttenuationXSection     " + self.SampleAttenuationXSection + "\n"
        msg = msg + "SampleScatteringXSection      " + self.SampleScatteringXSection + "\n"
        msg = msg + "SampleSampleNumberDensity     " + self.SampleNumberDensity + "\n"
        msg = msg + "SampleNumberOfSlices          " + self.SampleNumberOfSlices + "\n"
        msg = msg + "SampleNumberOfAnnuli          " + self.SampleNumberOfAnnuli + "\n"
        msg = msg + "SampleNumberOfWavelengthPoints" + self.SampleNumberOfWavelengthPoints + "\n"
        msg = msg + "SampleExpMethod               " + self.SampleExpMethod + "\n"
        msg = msg + "OutSuf                  " + self.OutSuf + "\n"
        msg = msg + "XYEDspc                 " + self.XYEDspc + "\n"
        msg = msg + "XYEtof                  " + self.XYEtof + "\n"
        msg = msg + "GSS                     " + self.GSS + "\n"
        msg = msg + "Nbank                   " + str(self.Nbank) + "\n"
        for i in range(0, self.Nbank):
            msg = msg + "Bank:                   " + str(i) + "\n"
            msg = msg + "  CropRange             " + self.CropRange[i] + "\n"
            msg = msg + "  Bining                " + self.Bining[i] + "\n"
        for i in self.bankList:
            msg = msg + "bankUsed                " + str(i) + "\n"
        print(msg)


if __name__ == '__main__':
    #Opt : EXPR_FILE=Files("hrpd",RawDir="",Analysisdir="" )
    EXPR_FILE = Files("hrpd")
    # default
    EXPR_FILE.initialize('Cycle08_2', 'Si')
# Opt : EXPR_FILE.initialize('Cycle09_2','tests',prefFile='mtd.pref', prefDir="")
