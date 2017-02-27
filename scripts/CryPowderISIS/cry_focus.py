# pylint: disable=too-many-arguments,unused-variable

from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from os.path import join
import cry_utils
import cry_load
import cry_sample
import cry_vana


# ===========
# Focusing
# ==========


def focus_all(EXPR_FILE, samplelistTexte, scale=0, NoVabs=False, NoSAC=False, Eff=True, Norm=True,
              Write_ExtV=True):
    if scale == 0:
        scale = float(EXPR_FILE.scale)
    # === Norm boolean flag used to Optionally correct to a Vana ===
    if Norm:
        print('Existing Vana Status:' + EXPR_FILE.ExistV)
        # SAC/EFF corrections loads the Vana
        load_sac_eff(EXPR_FILE, NoSAC=NoSAC, Eff=Eff)
        if EXPR_FILE.ExistV == "load":
            for i in EXPR_FILE.bankList:
                spec = i - 1
                vanfil = EXPR_FILE.CorrVanFile + "-" + str(spec) + ".nxs"
                LoadNexusProcessed(Filename=vanfil, OutputWorkspace="Vanadium-" + str(i))
                # CORRECT
        elif EXPR_FILE.ExistV == "no" and EXPR_FILE.VGrpfocus == "van":
            print("was here?")
            cry_vana.create_vana(EXPR_FILE, NoAbs=NoVabs, write_existingv=Write_ExtV)
    else:
        load_sac_eff(EXPR_FILE, NoSAC=True)
    # === Construct a list of runs, sum of runs
    sampleSumLists = cry_utils.get_sample_list(EXPR_FILE.basefile, samplelistTexte, EXPR_FILE.RawDir)
    # to loop over
    isfirst = True
    for sample2Add in sampleSumLists:
        print('--------------------------')
        print('         Start focus here        ')
        print('--------------------------')
        print(" ---> " + focus_one(EXPR_FILE, sample2Add, scale, Norm, isfirst, NoAbs=NoVabs))
        isfirst = False
    #
    # changed by WAK 8/3/2011:delete workspaces
    if not EXPR_FILE.debugMode:
        mtd.remove("Corr")
        mtd.remove("Sample")
        for i in EXPR_FILE.bankList:
            mtd.remove("Sample-" + str(i))
        for i in EXPR_FILE.bankList:
            mtd.remove("Vanadium-" + str(i))


def load_sac_eff(EXPR_FILE, NoSAC=False, Eff=True):
    # Loads SAC/Efficiency correction in wkspc "Corr" or sets it to "1"
    newCalFile = EXPR_FILE.CorrVanDir + '/' + EXPR_FILE.GrpFile
    EXPR_FILE.Path2VanGrpFile = newCalFile
    if NoSAC:
        CreateSingleValuedWorkspace(OutputWorkspace="Corr", DataValue=str(1))
        print(" => No SAC/Eff applied ")
        return
    else:
        # First try to load the vana (this won't crash if no vana run  is set)....
        (dum, uampstotal) = cry_sample.get_data_sum(EXPR_FILE.VanFile, "Vanadium", EXPR_FILE)
        uampstotal = mtd["Vanadium"].getRun().getProtonCharge()
        if uampstotal < 1e-6:
            print(" => Van NOT found : No SAC/eff correction will be applied")
            CreateSingleValuedWorkspace(OutputWorkspace="Corr", DataValue=str(1))
        else:
            print(' => Pre-calculate SAC from Vana ')
            Integration(InputWorkspace="Vanadium", OutputWorkspace="VanadiumSum")
            # Modified test equal to Zero the 17/10/2012
            MaskDetectorsIf(InputWorkspace="VanadiumSum", InputCalFile=EXPR_FILE.Path2GrpFile,
                            OutputCalFile=newCalFile,
                            Mode="DeselectIf", Operator="LessEqual", Value=10)
            if not EXPR_FILE.debugMode:
                mtd.remove("VanadiumSum")
            SolidAngle(InputWorkspace="Vanadium", OutputWorkspace="SAC")
            CreateSingleValuedWorkspace(OutputWorkspace="Sc", DataValue=str(100))
            Multiply(LHSWorkspace="SAC", RHSWorkspace="Sc", OutputWorkspace="Corr")
            if not EXPR_FILE.debugMode:
                mtd.remove("SAC")
            if Eff:
                Divide(LHSWorkspace="Vanadium", RHSWorkspace="Corr", OutputWorkspace="Eff")
                print(' => Pre-calculate Efficiency correction from Vana ')
                ConvertUnits(InputWorkspace="Eff", OutputWorkspace="Eff", Target="Wavelength")
                Integration(InputWorkspace="Eff", OutputWorkspace="Eff",
                            RangeLower=EXPR_FILE.LowerLambda, RangeUpper=EXPR_FILE.UpperLambda)
                Multiply(LHSWorkspace="Corr", RHSWorkspace="Eff", OutputWorkspace="Corr")
                #				if EXPR_FILE.instr=="polaris":
                #					CreateSingleValuedWorkspace("Sc", str(10000000))
                #				else:
                #
                CreateSingleValuedWorkspace(OutputWorkspace="Sc", DataValue=str(100000))
                Divide(LHSWorkspace="Corr", RHSWorkspace="Sc", OutputWorkspace="Corr")
                mtd.remove("Sc")
                mtd.remove("Vanadium")
                if not EXPR_FILE.debugMode:
                    mtd.remove("Eff")


def focus_one(EXPR_FILE, sampleAdd, scale, Norm, isfirst=False, NoAbs=False):
    (outname, uampstotal) = cry_sample.get_data_sum(sampleAdd, "sample", EXPR_FILE)
    if uampstotal < 1e-6:
        return "No usable data, Raw files probably not found: cannot create " + outname + "\n"
    newCalFile = join(EXPR_FILE.user, EXPR_FILE.GrpFile)  # where user and GrpFile is appended
    Integration(InputWorkspace="sample", OutputWorkspace="sampleSum")
    MaskDetectorsIf(InputWorkspace="sampleSum", InputCalFile=EXPR_FILE.Path2GrpFile, OutputCalFile=newCalFile,
                    Mode="DeselectIf", Operator="Equal", Value=10)
    mtd.remove("sampleSum")
    EXPR_FILE.Path2DatGrpFile = newCalFile
    # isfirst always true when called from FocusAll
    if EXPR_FILE.VGrpfocus == "sam" and isfirst:
        cry_vana.create_vana(EXPR_FILE, NoAbs)
    if EXPR_FILE.SEmptyFile[0] != "none":
        # === Optionally loads Sample Empty ===
        (dum1, uamps) = cry_sample.get_data_sum(EXPR_FILE.SEmptyFile, "Sempty", EXPR_FILE)
        Minus(LHSWorkspace="sample", RHSWorkspace="Sempty", OutputWorkspace="sample")
        mtd.remove("Sempty")
    cry_load.align_fnc("sample", EXPR_FILE)
    Divide(LHSWorkspace="sample", RHSWorkspace="Corr", OutputWorkspace="sample")
    cry_load.scale_wspc("sample", scale)
    if EXPR_FILE.CorrectSampleAbs == "yes":
        if not EXPR_FILE.SampleAbsCorrected:
            cry_utils.correct_abs(InputWkspc="sample", outputWkspc="SampleTrans",
                                  TheCylinderSampleHeight=EXPR_FILE.SampleHeight,
                                  TheCylinderSampleRadius=EXPR_FILE.SampleRadius,
                                  TheAttenuationXSection=EXPR_FILE.SampleAttenuationXSection,
                                  TheScatteringXSection=EXPR_FILE.SampleScatteringXSection,
                                  TheSampleNumberDensity=EXPR_FILE.SampleNumberDensity,
                                  TheNumberOfSlices=EXPR_FILE.SampleNumberOfSlices,
                                  TheNumberOfAnnuli=EXPR_FILE.SampleNumberOfAnnuli,
                                  TheNumberOfWavelengthPoints=EXPR_FILE.SampleNumberOfWavelengthPoints,
                                  TheExpMethod=EXPR_FILE.SampleExpMethod)
            EXPR_FILE.SampleAbsCorrected = True
        else:
            ConvertUnits(InputWorkspace="sample", OutputWorkspace="sample", Target="Wavelength")
            Divide(LHSWorkspace="sample", RHSWorkspace="SampleTrans", OutputWorkspace="sample")
            ConvertUnits(InputWorkspace="sample", OutputWorkspace="sample", Target="dSpacing")
    DiffractionFocussing(InputWorkspace="sample", OutputWorkspace="sample",
                         GroupingFileName=EXPR_FILE.Path2DatGrpFile,
                         PreserveEvents=False)
    divide_samp_vana(EXPR_FILE, Norm)
    # === Cleans results in D and TOF before outputing bank by bank ===
    cry_load.bin_bank("ResultD", EXPR_FILE.bankList, EXPR_FILE.Drange)
    for i in EXPR_FILE.bankList:
        ConvertUnits(InputWorkspace="ResultD-" + str(i), OutputWorkspace="ResultTOF-" + str(i), Target="TOF")
        ReplaceSpecialValues(InputWorkspace="ResultD-" + str(i), OutputWorkspace="ResultD-" + str(i), NaNValue="0",
                             InfinityValue="0", BigNumberThreshold="99999999.99999999")
        ReplaceSpecialValues(InputWorkspace="ResultTOF-" + str(i), OutputWorkspace="ResultTOF-" + str(i), NaNValue="0",
                             InfinityValue="0", BigNumberThreshold="99999999.99999999")
    # === Output===
    # GSS
    GrpList = "ResultTOF-" + str(EXPR_FILE.bankList[0])
    if len(EXPR_FILE.bankList[1:]) > 1:
        for i in EXPR_FILE.bankList[1:]:
            GrpList = GrpList + ",ResultTOF-" + str(i)
        GroupWorkspaces(OutputWorkspace="ResultTOFgrp", InputWorkspaces=GrpList)
    if EXPR_FILE.OutSuf == "":
        OutputFile = join(EXPR_FILE.user, outname)
    else:
        OutputFile = join(EXPR_FILE.user, outname + "_" + EXPR_FILE.OutSuf)
    # Gss
    rearrang4gss(OutputFile, EXPR_FILE)
    # Nexus
    rearrange_4nex(OutputFile, EXPR_FILE)
    # XYE
    OutputFile = OutputFile + "_"
    rearrange_4xye(OutputFile, EXPR_FILE, units="TOF")
    rearrange_4xye(OutputFile, EXPR_FILE, units="D")
    return outname + "  focused with uampstotal=" + str(uampstotal)


def divide_samp_vana(EXPR_FILE, Norm):
    print(" => SAMPLE FOCUSED")
    if not EXPR_FILE.dataRangeSet:
        cry_load.sets_drange("sample", EXPR_FILE)
    cry_load.split_bank("sample", EXPR_FILE.bankList, Del=False)
    if Norm:
        # === Optional normalization ===
        for i in EXPR_FILE.bankList:
            RebinToWorkspace(WorkspaceToRebin="Vanadium-" + str(i), WorkspaceToMatch="sample-" + str(i),
                             OutputWorkspace="Vanadium-" + str(i))
            Divide(LHSWorkspace="sample-" + str(i), RHSWorkspace="Vanadium-" + str(i),
                   OutputWorkspace="ResultD-" + str(i))
    else:
        for i in EXPR_FILE.bankList:
            RenameWorkspace(InputWorkspace="sample-" + str(i), OutputWorkspace="ResultD-" + str(i))


# ===========
# Output in XYE, GSS...
# ==========
def rearrange_4xye(OutputFile, EXPR_FILE, units="TOF"):
    if (units == "D" and EXPR_FILE.saveXYEd) or (units == "TOF" and EXPR_FILE.saveXYEtof):
        for i in EXPR_FILE.bankList:
            inwkpsc = "Result" + units + "-" + str(i)
            SaveFocusedXYE(InputWorkspace=inwkpsc, Filename=OutputFile + "b" + str(i) + "_" + units + ".dat",
                           SplitFiles=False, IncludeHeader=False)


# SaveFocusedXYE(InputWorkspace=inwkpsc, Filename=OutputFile+"b"+str(i)+"_"+units+".dat", SplitFiles=False)

def rearrang4gss(OutputFile, EXPR_FILE):
    if EXPR_FILE.GSS == "no":
        return
    if len(EXPR_FILE.bankList[1:]) > 1:
        SaveGSS(InputWorkspace="ResultTOFgrp", Filename=OutputFile + ".gss", SplitFiles=False, Append=False)
    else:
        SaveGSS(InputWorkspace="ResultTOF-1", Filename=OutputFile + ".gss", SplitFiles=False, Append=False)


# changed by WAK 3/3/2011 following Martyn Gigg's advice on group save

def rearrange_4nex(OutputFile, EXPR_FILE):
    if EXPR_FILE.Nex == "no":
        return
    SaveNexusProcessed(Filename=OutputFile + ".nxs", InputWorkspace="ResultTOFgrp")

# SaveNexusProcessed(Filename=OutputFile+".nxs", InputWorkspace="ResultTOFgrp")
# groupList = mtd["ResultTOFgrp"].getNames()
# for name in groupList[1:]:
#	SaveNexusProcessed(InputWorkspace=name,Filename=OutputFile+'.nxs', Append=False)
# end change


# if __name__ == '__main__': :
#    rearrange_4xye("ResultTOFtmp", 0, "11200,-0.0003,104000", "tst1.dat", units="TOF")
#    rearrange_4xye("ResultTOFtmp", 1, "11200,-0.0008,104000", "tst2.dat", units="TOF")
#    rearrange_4xye("ResultTOFtmp", 2, "11200,-0.0012,104000", "tst3.dat", units="TOF")
# CorrectVana(VanFile,EmptyFile,AcalibFile,1)
# normalizeall(SampleDatFile,AcalibFile)
# rearrangeallbanks(OutputFile,"")
