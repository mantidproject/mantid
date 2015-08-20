from mantid.simpleapi import *
from os.path import join
import CRY_utils
import CRY_load
import CRY_sample
import CRY_vana


# ===========
# Focusing
# ==========


def focus_all(expt_files, samplelistTexte, scale=0, NoVabs=False, NoSAC=False, Eff=True, Norm=True):
    if scale == 0:
        scale = float(expt_files.scale)
    # === Norm boolean flag used to Optionally correct to a Vana ===
    if Norm:
        print 'Existing Vana Status:' + expt_files.ExistV
        # SAC/EFF corrections loads the Vana
        load_sac_eff(expt_files, NoSAC=NoSAC, Eff=Eff)
        if expt_files.ExistV == "load":
            for i in expt_files.bankList:
                spec = i - 1
                vanfil = expt_files.CorrVanFile + "-" + str(spec) + ".nxs"
                LoadNexusProcessed(Filename=vanfil, OutputWorkspace="Vanadium-" + str(i))
                # CORRECT
        elif expt_files.ExistV == "no" and expt_files.VGrpfocus == "van":
            print "was here?"
            CRY_vana.create_vana(expt_files, NoAbs=NoVabs)
    else:
        load_sac_eff(expt_files, NoSAC=True)
    # === Construct a list of runs, sum of runs
    sampleSumLists = CRY_utils.get_sample_list(expt_files.basefile, samplelistTexte, expt_files.RawDir)
    # to loop over
    isfirst = True
    for sample2Add in sampleSumLists:
        print '--------------------------'
        print '         Start focus here        '
        print '--------------------------'
        print " ---> " + focus(expt_files, sample2Add, scale, Norm, isfirst, NoAbs=NoVabs)
        isfirst = False
    #
    # changed by WAK 8/3/2011:delete workspaces
    if not expt_files.debugMode:
        mtd.remove("Corr")
        mtd.remove("Sample")
        for i in expt_files.bankList:
            mtd.remove("Sample-" + str(i))
        for i in expt_files.bankList:
            mtd.remove("Vanadium-" + str(i))


def load_sac_eff(expt_files, NoSAC=False, Eff=True):
    # Loads SAC/Efficiency correction in wkspc "Corr" or sets it to "1"
    newCalFile = expt_files.CorrVanDir + '/' + expt_files.GrpFile
    expt_files.Path2VanGrpFile = newCalFile
    if NoSAC:
        CreateSingleValuedWorkspace(OutputWorkspace="Corr", DataValue=str(1))
        print " => No SAC/Eff applied "
        return
    else:
        # First try to load the vana (this won't crash if no vana run  is set)....
        (dum, uampstotal) = CRY_sample.get_data_sum(expt_files.VanFile, "Vanadium", expt_files)
        uampstotal = mtd["Vanadium"].getRun().getProtonCharge()
        if uampstotal < 1e-6:
            print " => Van NOT found : No SAC/eff correction will be applied"
            CreateSingleValuedWorkspace(OutputWorkspace="Corr", DataValue=str(1))
        else:
            print ' => Pre-calculate SAC from Vana '
            Integration(InputWorkspace="Vanadium", OutputWorkspace="VanadiumSum")
            # Modified test equal to Zero the 17/10/2012
            MaskDetectorsIf(InputWorkspace="VanadiumSum", InputCalFile=expt_files.Path2GrpFile,
                            OutputCalFile=newCalFile,
                            Mode="DeselectIf", Operator="LessEqual", Value=10)
            if not expt_files.debugMode:
                mtd.remove("VanadiumSum")
            SolidAngle(InputWorkspace="Vanadium", OutputWorkspace="SAC")
            CreateSingleValuedWorkspace(OutputWorkspace="Sc", DataValue=str(100))
            Multiply(LHSWorkspace="SAC", RHSWorkspace="Sc", OutputWorkspace="Corr")
            if not expt_files.debugMode:
                mtd.remove("SAC")
            if Eff:
                Divide(LHSWorkspace="Vanadium", RHSWorkspace="Corr", OutputWorkspace="Eff")
                print ' => Pre-calculate Efficiency correction from Vana '
                ConvertUnits(InputWorkspace="Eff", OutputWorkspace="Eff", Target="Wavelength")
                Integration(InputWorkspace="Eff", OutputWorkspace="Eff", \
                            RangeLower=expt_files.LowerLambda, RangeUpper=expt_files.UpperLambda)
                Multiply(LHSWorkspace="Corr", RHSWorkspace="Eff", OutputWorkspace="Corr")
                #				if expt_files.instr=="polaris":
                #					CreateSingleValuedWorkspace("Sc", str(10000000))
                #				else:
                CreateSingleValuedWorkspace(OutputWorkspace="Sc", DataValue=str(100000))
                Divide(LHSWorkspace="Corr", RHSWorkspace="Sc", OutputWorkspace="Corr")
                mtd.remove("Sc")
                mtd.remove("Vanadium")
                if not expt_files.debugMode:
                    mtd.remove("Eff")


def focus(expt_files, sampleAdd, scale, Norm, isfirst=False, NoAbs=False):
    (outname, uampstotal) = CRY_sample.get_data_sum(sampleAdd, "sample", expt_files)
    if uampstotal < 1e-6:
        return "No usable data, Raw files probably not found: cannot create " + outname + "\n"
    newCalFile = join(expt_files.user, expt_files.GrpFile)
    Integration(InputWorkspace="sample", OutputWorkspace="sampleSum")
    MaskDetectorsIf(InputWorkspace="sampleSum", InputCalFile=expt_files.Path2GrpFile, OutputCalFile=newCalFile,
                    Mode="DeselectIf", Operator="Equal", Value=10)
    mtd.remove("sampleSum")
    expt_files.Path2DatGrpFile = newCalFile
    if expt_files.VGrpfocus == "sam" and isfirst:
        CRY_vana.create_vana(expt_files, NoAbs)
    if expt_files.SEmptyFile[0] != "none":
        # === Optionally loads Sample Empty ===
        # (dum1, uamps) = CRY_sample.get_data_sum(expt_files.SEmptyFile, "Sempty", expt_files)
        Minus(LHSWorkspace="sample", RHSWorkspace="Sempty", OutputWorkspace="sample")
        mtd.remove("Sempty")
    CRY_load.align_fnc("sample", expt_files)
    Divide(LHSWorkspace="sample", RHSWorkspace="Corr", OutputWorkspace="sample")
    CRY_load.scale_wspc("sample", scale)
    if expt_files.CorrectSampleAbs == "yes":
        if expt_files.SampleAbsCorrected == False:
            CRY_utils.correct_abs(InputWkspc="sample", outputWkspc="SampleTrans", \
                                 TheCylinderSampleHeight=expt_files.SampleHeight, \
                                 TheCylinderSampleRadius=expt_files.SampleRadius, \
                                 TheAttenuationXSection=expt_files.SampleAttenuationXSection, \
                                 TheScatteringXSection=expt_files.SampleScatteringXSection, \
                                 TheSampleNumberDensity=expt_files.SampleNumberDensity, \
                                 TheNumberOfSlices=expt_files.SampleNumberOfSlices, \
                                 TheNumberOfAnnuli=expt_files.SampleNumberOfAnnuli, \
                                 TheNumberOfWavelengthPoints=expt_files.SampleNumberOfWavelengthPoints, \
                                 TheExpMethod=expt_files.SampleExpMethod)
            expt_files.SampleAbsCorrected = True
        else:
            ConvertUnits(InputWorkspace="sample", OutputWorkspace="sample", Target="Wavelength")
            Divide(LHSWorkspace="sample", RHSWorkspace="SampleTrans", OutputWorkspace="sample")
            ConvertUnits(InputWorkspace="sample", OutputWorkspace="sample", Target="dSpacing")
    DiffractionFocussing(InputWorkspace="sample", OutputWorkspace="sample", GroupingFileName=expt_files.Path2DatGrpFile,
                         PreserveEvents=False)
    divide_samp_vana(expt_files, Norm)
    # === Cleans results in D and TOF before outputing bank by bank ===
    CRY_load.bin_bank("ResultD", expt_files.bankList, expt_files.Drange)
    for i in expt_files.bankList:
        ConvertUnits(InputWorkspace="ResultD-" + str(i), OutputWorkspace="ResultTOF-" + str(i), Target="TOF")
        ReplaceSpecialValues(InputWorkspace="ResultD-" + str(i), OutputWorkspace="ResultD-" + str(i), NaNValue="0",
                             InfinityValue="0", BigNumberThreshold="99999999.99999999")
        ReplaceSpecialValues(InputWorkspace="ResultTOF-" + str(i), OutputWorkspace="ResultTOF-" + str(i), NaNValue="0",
                             InfinityValue="0", BigNumberThreshold="99999999.99999999")
    # === Output===
    # GSS
    GrpList = "ResultTOF-" + str(expt_files.bankList[0])
    if len(expt_files.bankList[1:]) > 1:
        for i in expt_files.bankList[1:]:
            GrpList = GrpList + ",ResultTOF-" + str(i)
        GroupWorkspaces(OutputWorkspace="ResultTOFgrp", InputWorkspaces=GrpList)
    if expt_files.OutSuf == "":
        OutputFile = join(expt_files.user, outname)
    else:
        OutputFile = join(expt_files.user, outname + "_" + expt_files.OutSuf)
    # Gss
    rearrang4GSS(OutputFile, expt_files)
    # Nexus
    rearrange_4nex(OutputFile, expt_files)
    # XYE
    OutputFile = OutputFile + "_"
    rearrange_4xye(OutputFile, expt_files, units="TOF")
    rearrange_4xye(OutputFile, expt_files, units="D")
    return outname + "  focused with uampstotal=" + str(uampstotal)


def divide_samp_vana(expt_files, Norm):
    print " => SAMPLE FOCUSED"
    if not expt_files.dataRangeSet:
        CRY_load.sets_drange("sample", expt_files)
    CRY_load.split_bank("sample", expt_files.bankList, Del=False)
    if Norm:
        # === Optional normalization ===
        for i in expt_files.bankList:
            RebinToWorkspace(WorkspaceToRebin="Vanadium-" + str(i), WorkspaceToMatch="sample-" + str(i),
                             OutputWorkspace="Vanadium-" + str(i))
            Divide(LHSWorkspace="sample-" + str(i), RHSWorkspace="Vanadium-" + str(i),
                   OutputWorkspace="ResultD-" + str(i))
    else:
        for i in expt_files.bankList:
            RenameWorkspace(InputWorkspace="sample-" + str(i), OutputWorkspace="ResultD-" + str(i))


# ===========
# Output in XYE, GSS...
# ==========
def rearrange_4xye(OutputFile, expt_files, units="TOF"):
    if (units == "D" and expt_files.saveXYEd) or (units == "TOF" and expt_files.saveXYEtof):
        for i in expt_files.bankList:
            inwkpsc = "Result" + units + "-" + str(i)
            SaveFocusedXYE(InputWorkspace=inwkpsc, Filename=OutputFile + "b" + str(i) + "_" + units + ".dat",
                           SplitFiles=False, IncludeHeader=False)


# SaveFocusedXYE(InputWorkspace=inwkpsc, Filename=OutputFile+"b"+str(i)+"_"+units+".dat", SplitFiles=False)

def rearrang4GSS(OutputFile, expt_files):
    if expt_files.GSS == "no":
        return
    if len(expt_files.bankList[1:]) > 1:
        SaveGSS(InputWorkspace="ResultTOFgrp", Filename=OutputFile + ".gss", SplitFiles=False, Append=False)
    else:
        SaveGSS(InputWorkspace="ResultTOF-1", Filename=OutputFile + ".gss", SplitFiles=False, Append=False)


# changed by WAK 3/3/2011 following Martyn Gigg's advice on group save

def rearrange_4nex(OutputFile, expt_files):
    if expt_files.Nex == "no":
        return
    SaveNexusProcessed(Filename=OutputFile + ".nxs", InputWorkspace="ResultTOFgrp")

# SaveNexusProcessed(Filename=OutputFile+".nxs", InputWorkspace="ResultTOFgrp")
# groupList = mtd["ResultTOFgrp"].getNames()
# for name in groupList[1:]:
#	SaveNexusProcessed(InputWorkspace=name,Filename=OutputFile+'.nxs', Append=False)
# end change


# if __name__ == '__main__':
#    rearrange_4xye("ResultTOFtmp", 0, "11200,-0.0003,104000", "tst1.dat", units="TOF")
#    rearrange_4xye("ResultTOFtmp", 1, "11200,-0.0008,104000", "tst2.dat", units="TOF")
#    rearrange_4xye("ResultTOFtmp", 2, "11200,-0.0012,104000", "tst3.dat", units="TOF")
# CorrectVana(VanFile,EmptyFile,AcalibFile,1)
# normalizeall(SampleDatFile,AcalibFile)
# rearrangeallbanks(OutputFile,"")
