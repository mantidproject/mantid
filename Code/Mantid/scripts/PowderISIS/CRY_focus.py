from mantid.simpleapi import *
from os.path import join
import CRY_utils
import CRY_load
import CRY_sample
import CRY_vana


# ===========
# Focussing
# ==========


def FocusAll(expt, samplelistTexte, scale=0, NoVabs=False, NoSAC=False, Eff=True, Norm=True):
    if scale == 0:
        scale = float(expt.scale)
    # === Norm boolean flag used to Optionally correct to a Vana ===
    if Norm:
        print 'Existing Vana Status:' + expt.ExistV
        # SAC/EFF corrections loads the Vana
        LoadSacEff(expt, NoSAC=NoSAC, Eff=Eff)
        if expt.ExistV == "load":
            for i in expt.bankList:
                spec = i - 1
                vanfil = expt.CorrVanFile + "-" + str(spec) + ".nxs"
                LoadNexusProcessed(Filename=vanfil, OutputWorkspace="Vanadium-" + str(i))
                # CORRECT
        elif expt.ExistV == "no" and expt.VGrpfocus == "van":
            print "was here?"
            CRY_vana.CreateVana(expt, NoAbs=NoVabs)
    else:
        LoadSacEff(expt, NoSAC=True)
    # === Construct a list of runs, sum of runs
    sampleSumLists = CRY_utils.getSampleList(expt.basefile, samplelistTexte, expt.RawDir)
    # to loop over
    isfirst = True
    for sample2Add in sampleSumLists:
        print '--------------------------'
        print '         Start focus here        '
        print '--------------------------'
        print " ---> " + Focus(expt, sample2Add, scale, Norm, isfirst, NoAbs=NoVabs)
        isfirst = False
    #
    # changed by WAK 8/3/2011:delete workspaces
    if not expt.debugMode:
        mtd.remove("Corr")
        mtd.remove("Sample")
        for i in expt.bankList:
            mtd.remove("Sample-" + str(i))
        for i in expt.bankList:
            mtd.remove("Vanadium-" + str(i))


def LoadSacEff(expt, NoSAC=False, Eff=True):
    # Loads SAC/Efficiency correction in wkspc "Corr" or sets it to "1"
    newCalFile = expt.CorrVanDir + '/' + expt.GrpFile
    expt.Path2VanGrpFile = newCalFile
    if NoSAC:
        CreateSingleValuedWorkspace(OutputWorkspace="Corr", DataValue=str(1))
        print " => No SAC/Eff applied "
        return
    else:
        # First try to load the vana (this won't crash if no vana run  is set)....
        (dum, uampstotal) = CRY_sample.getDataSum(expt.VanFile, "Vanadium", expt)
        uampstotal = mtd["Vanadium"].getRun().getProtonCharge()
        if uampstotal < 1e-6:
            print " => Van NOT found : No SAC/eff correction will be applied"
            CreateSingleValuedWorkspace(OutputWorkspace="Corr", DataValue=str(1))
        else:
            print ' => Pre-calculate SAC from Vana '
            Integration(InputWorkspace="Vanadium", OutputWorkspace="VanadiumSum")
            # Modified test equal to Zero the 17/10/2012
            MaskDetectorsIf(InputWorkspace="VanadiumSum", InputCalFile=expt.Path2GrpFile, OutputCalFile=newCalFile,
                            Mode="DeselectIf", Operator="LessEqual", Value=10)
            if not expt.debugMode:
                mtd.remove("VanadiumSum")
            SolidAngle(InputWorkspace="Vanadium", OutputWorkspace="SAC")
            CreateSingleValuedWorkspace(OutputWorkspace="Sc", DataValue=str(100))
            Multiply(LHSWorkspace="SAC", RHSWorkspace="Sc", OutputWorkspace="Corr")
            if not expt.debugMode:
                mtd.remove("SAC")
            if Eff:
                Divide(LHSWorkspace="Vanadium", RHSWorkspace="Corr", OutputWorkspace="Eff")
                print ' => Pre-calculate Efficiency correction from Vana '
                ConvertUnits(InputWorkspace="Eff", OutputWorkspace="Eff", Target="Wavelength")
                Integration(InputWorkspace="Eff", OutputWorkspace="Eff", \
                            RangeLower=expt.LowerLambda, RangeUpper=expt.UpperLambda)
                Multiply(LHSWorkspace="Corr", RHSWorkspace="Eff", OutputWorkspace="Corr")
                #				if expt.instr=="polaris":
                #					CreateSingleValuedWorkspace("Sc", str(10000000))
                #				else:
                CreateSingleValuedWorkspace(OutputWorkspace="Sc", DataValue=str(100000))
                Divide(LHSWorkspace="Corr", RHSWorkspace="Sc", OutputWorkspace="Corr")
                mtd.remove("Sc")
                mtd.remove("Vanadium")
                if not expt.debugMode:
                    mtd.remove("Eff")


def Focus(expt, sampleAdd, scale, Norm, isfirst=False, NoAbs=False):
    (outname, uampstotal) = CRY_sample.getDataSum(sampleAdd, "sample", expt)
    if uampstotal < 1e-6:
        return "No usable data, Raw files probably not found: cannot create " + outname + "\n"
    newCalFile = join(expt.user, expt.GrpFile)
    Integration(InputWorkspace="sample", OutputWorkspace="sampleSum")
    MaskDetectorsIf(InputWorkspace="sampleSum", InputCalFile=expt.Path2GrpFile, OutputCalFile=newCalFile,
                    Mode="DeselectIf", Operator="Equal", Value=10)
    mtd.remove("sampleSum")
    expt.Path2DatGrpFile = newCalFile
    if expt.VGrpfocus == "sam" and isfirst:
        CRY_vana.CreateVana(expt, NoAbs)
    if expt.SEmptyFile[0] <> "none":
        # === Optionally loads Sample Empty ===
        (dum1, uamps) = CRY_sample.getDataSum(expt.SEmptyFile, "Sempty", expt)
        Minus(LHSWorkspace="sample", RHSWorkspace="Sempty", OutputWorkspace="sample")
        mtd.remove("Sempty")
    CRY_load.Align("sample", expt)
    Divide(LHSWorkspace="sample", RHSWorkspace="Corr", OutputWorkspace="sample")
    CRY_load.ScaleWspc("sample", scale)
    if expt.CorrectSampleAbs == "yes":
        if expt.SampleAbsCorrected == False:
            CRY_utils.CorrectAbs(InputWkspc="sample", outputWkspc="SampleTrans", \
                                 TheCylinderSampleHeight=expt.SampleHeight, \
                                 TheCylinderSampleRadius=expt.SampleRadius, \
                                 TheAttenuationXSection=expt.SampleAttenuationXSection, \
                                 TheScatteringXSection=expt.SampleScatteringXSection, \
                                 TheSampleNumberDensity=expt.SampleNumberDensity, \
                                 TheNumberOfSlices=expt.SampleNumberOfSlices, \
                                 TheNumberOfAnnuli=expt.SampleNumberOfAnnuli, \
                                 TheNumberOfWavelengthPoints=expt.SampleNumberOfWavelengthPoints, \
                                 TheExpMethod=expt.SampleExpMethod)
            expt.SampleAbsCorrected = True
        else:
            ConvertUnits(InputWorkspace="sample", OutputWorkspace="sample", Target="Wavelength")
            Divide(LHSWorkspace="sample", RHSWorkspace="SampleTrans", OutputWorkspace="sample")
            ConvertUnits(InputWorkspace="sample", OutputWorkspace="sample", Target="dSpacing")
    DiffractionFocussing(InputWorkspace="sample", OutputWorkspace="sample", GroupingFileName=expt.Path2DatGrpFile,
                         PreserveEvents=False)
    DivideSampVana(expt, Norm)
    # === Cleans results in D and TOF before outputing bank by bank ===
    CRY_load.BinBank("ResultD", expt.bankList, expt.Drange)
    for i in expt.bankList:
        ConvertUnits(InputWorkspace="ResultD-" + str(i), OutputWorkspace="ResultTOF-" + str(i), Target="TOF")
        ReplaceSpecialValues(InputWorkspace="ResultD-" + str(i), OutputWorkspace="ResultD-" + str(i), NaNValue="0",
                             InfinityValue="0", BigNumberThreshold="99999999.99999999")
        ReplaceSpecialValues(InputWorkspace="ResultTOF-" + str(i), OutputWorkspace="ResultTOF-" + str(i), NaNValue="0",
                             InfinityValue="0", BigNumberThreshold="99999999.99999999")
    # === Output===
    # GSS
    GrpList = "ResultTOF-" + str(expt.bankList[0])
    if len(expt.bankList[1:]) > 1:
        for i in expt.bankList[1:]:
            GrpList = GrpList + ",ResultTOF-" + str(i)
        GroupWorkspaces(OutputWorkspace="ResultTOFgrp", InputWorkspaces=GrpList)
    if expt.OutSuf == "":
        OutputFile = join(expt.user, outname)
    else:
        OutputFile = join(expt.user, outname + "_" + expt.OutSuf)
    # Gss
    rearrang4GSS(OutputFile, expt)
    # Nexus
    rearrang4Nex(OutputFile, expt)
    # XYE
    OutputFile = OutputFile + "_"
    rearrange4XYE(OutputFile, expt, units="TOF")
    rearrange4XYE(OutputFile, expt, units="D")
    return outname + "  focused with uampstotal=" + str(uampstotal)


def DivideSampVana(expt, Norm):
    print " => SAMPLE FOCUSED"
    if not expt.dataRangeSet:
        CRY_load.setsDrange("sample", expt)
    CRY_load.SplitBank("sample", expt.bankList, Del=False)
    if Norm:
        # === Optional normalization ===
        for i in expt.bankList:
            RebinToWorkspace(WorkspaceToRebin="Vanadium-" + str(i), WorkspaceToMatch="sample-" + str(i),
                             OutputWorkspace="Vanadium-" + str(i))
            Divide(LHSWorkspace="sample-" + str(i), RHSWorkspace="Vanadium-" + str(i),
                   OutputWorkspace="ResultD-" + str(i))
    else:
        for i in expt.bankList:
            RenameWorkspace(InputWorkspace="sample-" + str(i), OutputWorkspace="ResultD-" + str(i))


# ===========
# Output in XYE, GSS...
# ==========
def rearrange4XYE(OutputFile, expt, units="TOF"):
    if (units == "D" and expt.saveXYEd) or (units == "TOF" and expt.saveXYEtof):
        for i in expt.bankList:
            inwkpsc = "Result" + units + "-" + str(i)
            SaveFocusedXYE(InputWorkspace=inwkpsc, Filename=OutputFile + "b" + str(i) + "_" + units + ".dat",
                           SplitFiles=False, IncludeHeader=False)


# SaveFocusedXYE(InputWorkspace=inwkpsc, Filename=OutputFile+"b"+str(i)+"_"+units+".dat", SplitFiles=False)

def rearrang4GSS(OutputFile, expt):
    if expt.GSS == "no":
        return
    if len(expt.bankList[1:]) > 1:
        SaveGSS(InputWorkspace="ResultTOFgrp", Filename=OutputFile + ".gss", SplitFiles=False, Append=False)
    else:
        SaveGSS(InputWorkspace="ResultTOF-1", Filename=OutputFile + ".gss", SplitFiles=False, Append=False)


# changed by WAK 3/3/2011 following Martyn Gigg's advice on group save 
def rearrang4Nex(OutputFile, expt):
    if expt.Nex == "no":
        return
    SaveNexusProcessed(Filename=OutputFile + ".nxs", InputWorkspace="ResultTOFgrp")


# SaveNexusProcessed(Filename=OutputFile+".nxs", InputWorkspace="ResultTOFgrp")
# groupList = mtd["ResultTOFgrp"].getNames()
# for name in groupList[1:]:
#	SaveNexusProcessed(InputWorkspace=name,Filename=OutputFile+'.nxs', Append=False)
# end change


if __name__ == '__main__':
    rearrange4XYE("ResultTOFtmp", 0, "11200,-0.0003,104000", "tst1.dat", units="TOF")
    rearrange4XYE("ResultTOFtmp", 1, "11200,-0.0008,104000", "tst2.dat", units="TOF")
    rearrange4XYE("ResultTOFtmp", 2, "11200,-0.0012,104000", "tst3.dat", units="TOF")
# CorrectVana(VanFile,EmptyFile,AcalibFile,1)
# normalizeall(SampleDatFile,AcalibFile)
# rearrangeallbanks(OutputFile,"")

