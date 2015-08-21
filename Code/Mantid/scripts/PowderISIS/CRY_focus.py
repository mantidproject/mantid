#pylint: disable=too-many-arguments,unused-variable

from mantid.simpleapi import *
from os.path import join
import CRY_utils
import CRY_load
import CRY_sample
import CRY_vana


# ===========
# Focusing
# ==========


def focus_all(experimentf, samplelistTexte, scale=0, NoVabs=False, NoSAC=False, Eff=True, Norm=True):
    if scale == 0:
        scale = float(experimentf.scale)
    # === Norm boolean flag used to Optionally correct to a Vana ===
    if Norm:
        print 'Existing Vana Status:' + experimentf.ExistV
        # SAC/EFF corrections loads the Vana
        load_sac_eff(experimentf, NoSAC=NoSAC, Eff=Eff)
        if experimentf.ExistV == "load":
            for i in experimentf.bankList:
                spec = i - 1
                vanfil = experimentf.CorrVanFile + "-" + str(spec) + ".nxs"
                LoadNexusProcessed(Filename=vanfil, OutputWorkspace="Vanadium-" + str(i))
                # CORRECT
        elif experimentf.ExistV == "no" and experimentf.VGrpfocus == "van":
            print "was here?"
            CRY_vana.create_vana(experimentf, NoAbs=NoVabs)
    else:
        load_sac_eff(experimentf, NoSAC=True)
    # === Construct a list of runs, sum of runs
    sampleSumLists = CRY_utils.get_sample_list(experimentf.basefile, samplelistTexte, experimentf.RawDir)
    # to loop over
    isfirst = True
    for sample2Add in sampleSumLists:
        print '--------------------------'
        print '         Start focus here        '
        print '--------------------------'
        print " ---> " + Focus(experimentf, sample2Add, scale, Norm, isfirst, NoAbs=NoVabs)
        isfirst = False
    #
    # changed by WAK 8/3/2011:delete workspaces
    if not experimentf.debugMode:
        mtd.remove("Corr")
        mtd.remove("Sample")
        for i in experimentf.bankList:
            mtd.remove("Sample-" + str(i))
        for i in experimentf.bankList:
            mtd.remove("Vanadium-" + str(i))


def load_sac_eff(experimentf, NoSAC=False, Eff=True):
    # Loads SAC/Efficiency correction in wkspc "Corr" or sets it to "1"
    newCalFile = experimentf.CorrVanDir + '/' + experimentf.GrpFile
    experimentf.Path2VanGrpFile = newCalFile
    if NoSAC:
        CreateSingleValuedWorkspace(OutputWorkspace="Corr", DataValue=str(1))
        print " => No SAC/Eff applied "
        return
    else:
        # First try to load the vana (this won't crash if no vana run  is set)....
        (dum, uampstotal) = CRY_sample.get_data_sum(experimentf.VanFile, "Vanadium", experimentf)
        uampstotal = mtd["Vanadium"].getRun().getProtonCharge()
        if uampstotal < 1e-6:
            print " => Van NOT found : No SAC/eff correction will be applied"
            CreateSingleValuedWorkspace(OutputWorkspace="Corr", DataValue=str(1))
        else:
            print ' => Pre-calculate SAC from Vana '
            Integration(InputWorkspace="Vanadium", OutputWorkspace="VanadiumSum")
            # Modified test equal to Zero the 17/10/2012
            MaskDetectorsIf(InputWorkspace="VanadiumSum", InputCalFile=experimentf.Path2GrpFile,
                            OutputCalFile=newCalFile,
                            Mode="DeselectIf", Operator="LessEqual", Value=10)
            if not experimentf.debugMode:
                mtd.remove("VanadiumSum")
            SolidAngle(InputWorkspace="Vanadium", OutputWorkspace="SAC")
            CreateSingleValuedWorkspace(OutputWorkspace="Sc", DataValue=str(100))
            Multiply(LHSWorkspace="SAC", RHSWorkspace="Sc", OutputWorkspace="Corr")
            if not experimentf.debugMode:
                mtd.remove("SAC")
            if Eff:
                Divide(LHSWorkspace="Vanadium", RHSWorkspace="Corr", OutputWorkspace="Eff")
                print ' => Pre-calculate Efficiency correction from Vana '
                ConvertUnits(InputWorkspace="Eff", OutputWorkspace="Eff", Target="Wavelength")
                Integration(InputWorkspace="Eff", OutputWorkspace="Eff", \
                            RangeLower=experimentf.LowerLambda, RangeUpper=experimentf.UpperLambda)
                Multiply(LHSWorkspace="Corr", RHSWorkspace="Eff", OutputWorkspace="Corr")
                #				if experimentf.instr=="polaris":
                #					CreateSingleValuedWorkspace("Sc", str(10000000))
                #				else:
                CreateSingleValuedWorkspace(OutputWorkspace="Sc", DataValue=str(100000))
                Divide(LHSWorkspace="Corr", RHSWorkspace="Sc", OutputWorkspace="Corr")
                mtd.remove("Sc")
                mtd.remove("Vanadium")
                if not experimentf.debugMode:
                    mtd.remove("Eff")


def Focus(experimentf, sampleAdd, scale, Norm, isfirst=False, NoAbs=False):
    (outname, uampstotal) = CRY_sample.get_data_sum(sampleAdd, "sample", experimentf)
    if uampstotal < 1e-6:
        return "No usable data, Raw files probably not found: cannot create " + outname + "\n"
    newCalFile = join(experimentf.user, experimentf.GrpFile)
    Integration(InputWorkspace="sample", OutputWorkspace="sampleSum")
    MaskDetectorsIf(InputWorkspace="sampleSum", InputCalFile=experimentf.Path2GrpFile, OutputCalFile=newCalFile,
                    Mode="DeselectIf", Operator="Equal", Value=10)
    mtd.remove("sampleSum")
    experimentf.Path2DatGrpFile = newCalFile
    if experimentf.VGrpfocus == "sam" and isfirst:
        CRY_vana.create_vana(experimentf, NoAbs)
    if experimentf.SEmptyFile[0] != "none":
        # === Optionally loads Sample Empty ===
        # (dum1, uamps) = CRY_sample.get_data_sum(experimentf.SEmptyFile, "Sempty", experimentf)
        Minus(LHSWorkspace="sample", RHSWorkspace="Sempty", OutputWorkspace="sample")
        mtd.remove("Sempty")
    CRY_load.align_fnc("sample", experimentf)
    Divide(LHSWorkspace="sample", RHSWorkspace="Corr", OutputWorkspace="sample")
    CRY_load.scale_wspc("sample", scale)
    if experimentf.CorrectSampleAbs == "yes":
        if experimentf.SampleAbsCorrected == False:
            CRY_utils.correct_abs(InputWkspc="sample", outputWkspc="SampleTrans", \
                                 TheCylinderSampleHeight=experimentf.SampleHeight, \
                                 TheCylinderSampleRadius=experimentf.SampleRadius, \
                                 TheAttenuationXSection=experimentf.SampleAttenuationXSection, \
                                 TheScatteringXSection=experimentf.SampleScatteringXSection, \
                                 TheSampleNumberDensity=experimentf.SampleNumberDensity, \
                                 TheNumberOfSlices=experimentf.SampleNumberOfSlices, \
                                 TheNumberOfAnnuli=experimentf.SampleNumberOfAnnuli, \
                                 TheNumberOfWavelengthPoints=experimentf.SampleNumberOfWavelengthPoints, \
                                 TheExpMethod=experimentf.SampleExpMethod)
            experimentf.SampleAbsCorrected = True
        else:
            ConvertUnits(InputWorkspace="sample", OutputWorkspace="sample", Target="Wavelength")
            Divide(LHSWorkspace="sample", RHSWorkspace="SampleTrans", OutputWorkspace="sample")
            ConvertUnits(InputWorkspace="sample", OutputWorkspace="sample", Target="dSpacing")
    DiffractionFocussing(InputWorkspace="sample", OutputWorkspace="sample", GroupingFileName=experimentf.Path2DatGrpFile,
                         PreserveEvents=False)
    divide_samp_vana(experimentf, Norm)
    # === Cleans results in D and TOF before outputing bank by bank ===
    CRY_load.bin_bank("ResultD", experimentf.bankList, experimentf.Drange)
    for i in experimentf.bankList:
        ConvertUnits(InputWorkspace="ResultD-" + str(i), OutputWorkspace="ResultTOF-" + str(i), Target="TOF")
        ReplaceSpecialValues(InputWorkspace="ResultD-" + str(i), OutputWorkspace="ResultD-" + str(i), NaNValue="0",
                             InfinityValue="0", BigNumberThreshold="99999999.99999999")
        ReplaceSpecialValues(InputWorkspace="ResultTOF-" + str(i), OutputWorkspace="ResultTOF-" + str(i), NaNValue="0",
                             InfinityValue="0", BigNumberThreshold="99999999.99999999")
    # === Output===
    # GSS
    GrpList = "ResultTOF-" + str(experimentf.bankList[0])
    if len(experimentf.bankList[1:]) > 1:
        for i in experimentf.bankList[1:]:
            GrpList = GrpList + ",ResultTOF-" + str(i)
        GroupWorkspaces(OutputWorkspace="ResultTOFgrp", InputWorkspaces=GrpList)
    if experimentf.OutSuf == "":
        OutputFile = join(experimentf.user, outname)
    else:
        OutputFile = join(experimentf.user, outname + "_" + experimentf.OutSuf)
    # Gss
    rearrang4gss(OutputFile, experimentf)
    # Nexus
    rearrange_4nex(OutputFile, experimentf)
    # XYE
    OutputFile = OutputFile + "_"
    rearrange_4xye(OutputFile, experimentf, units="TOF")
    rearrange_4xye(OutputFile, experimentf, units="D")
    return outname + "  focused with uampstotal=" + str(uampstotal)


def divide_samp_vana(experimentf, Norm):
    print " => SAMPLE FOCUSED"
    if not experimentf.dataRangeSet:
        CRY_load.sets_drange("sample", experimentf)
    CRY_load.split_bank("sample", experimentf.bankList, Del=False)
    if Norm:
        # === Optional normalization ===
        for i in experimentf.bankList:
            RebinToWorkspace(WorkspaceToRebin="Vanadium-" + str(i), WorkspaceToMatch="sample-" + str(i),
                             OutputWorkspace="Vanadium-" + str(i))
            Divide(LHSWorkspace="sample-" + str(i), RHSWorkspace="Vanadium-" + str(i),
                   OutputWorkspace="ResultD-" + str(i))
    else:
        for i in experimentf.bankList:
            RenameWorkspace(InputWorkspace="sample-" + str(i), OutputWorkspace="ResultD-" + str(i))


# ===========
# Output in XYE, GSS...
# ==========
def rearrange_4xye(OutputFile, experimentf, units="TOF"):
    if (units == "D" and experimentf.saveXYEd) or (units == "TOF" and experimentf.saveXYEtof):
        for i in experimentf.bankList:
            inwkpsc = "Result" + units + "-" + str(i)
            SaveFocusedXYE(InputWorkspace=inwkpsc, Filename=OutputFile + "b" + str(i) + "_" + units + ".dat",
                           SplitFiles=False, IncludeHeader=False)


# SaveFocusedXYE(InputWorkspace=inwkpsc, Filename=OutputFile+"b"+str(i)+"_"+units+".dat", SplitFiles=False)

def rearrang4gss(OutputFile, experimentf):
    if experimentf.GSS == "no":
        return
    if len(experimentf.bankList[1:]) > 1:
        SaveGSS(InputWorkspace="ResultTOFgrp", Filename=OutputFile + ".gss", SplitFiles=False, Append=False)
    else:
        SaveGSS(InputWorkspace="ResultTOF-1", Filename=OutputFile + ".gss", SplitFiles=False, Append=False)


# changed by WAK 3/3/2011 following Martyn Gigg's advice on group save

def rearrange_4nex(OutputFile, experimentf):
    if experimentf.Nex == "no":
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
