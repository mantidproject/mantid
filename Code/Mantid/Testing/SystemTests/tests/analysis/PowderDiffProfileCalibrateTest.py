########################################################################
#
# This is the system test for workflow algorithms
# 1. ExaminePowder...
# 2. SeqRefinement...
# Both of which are based on LeBailFit to do peak profile calibration
# for powder diffractometers.
#
########################################################################
import stresstesting
import mantid.simpleapi as api
from mantid.simpleapi import *

def getSaveDir():
        """determine where to save - the current working directory"""
        import os
        return os.path.abspath(os.path.curdir)

class VulcanExamineProfile(stresstesting.MantidStressTest):
    irf_file = 'arg_powder.irf'
    dat_file = 'arg_si.dat'
    bkgd_file = 'arg_si_bkgd_polynomial.nxs'

    def requiredFiles(self):
        files = [self.irf_file, self.dat_file, self.bkgd_file]
        return files

    def runTest(self):
        savedir = getSaveDir()

        LoadAscii(Filename=self.dat_file, OutputWorkspace='arg_si',Unit='TOF')

        LoadNexusProcessed(Filename=self.bkgd_file, OutputWorkspace='Arg_Si_Bkgd_Parameter')

        CreateLeBailFitInput(FullprofParameterFile=self.irf_file,
                GenerateBraggReflections='1',LatticeConstant='5.4313640',
                InstrumentParameterWorkspace='Arg_Bank1', BraggPeakParameterWorkspace='ReflectionTable')

        # run the actual code
        ExaminePowderDiffProfile(
            InputWorkspace      = 'arg_si',
            StartX              = 1990.,
            EndX                = 29100.,
            ProfileType         = 'Back-to-back exponential convoluted with PseudoVoigt',
            ProfileWorkspace    = 'Arg_Bank1',
            BraggPeakWorkspace  = 'ReflectionTable',
            BackgroundParameterWorkspace = 'Arg_Si_Bkgd_Parameter',
            BackgroundType      = 'Polynomial',
            BackgroundWorkspace = 'Arg_Si_Background',
            OutputWorkspace     = 'Arg_Si_Calculated')


        # load output gsas file and the golden one
        Load(Filename = "Arg_Si_ref.nxs", OutputWorkspace = "Arg_Si_golden")

    def validateMethod(self):
        self.tolerance=1.0e-6
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        self.tolerance=1.0e-6
        return ('Arg_Si_Calculated','Arg_Si_golden')

class VulcanSeqRefineProfileFromScratch(stresstesting.MantidStressTest):
    """ System test for sequential refinement 
    """
    irf_file = 'VULCAN_SNS_1.irf'
    dat_file = 'VULCAN_22946_NOM.dat'

    def requiredFiles(self):
        files = [self.irf_file, self.dat_file]
        return files

    def runTest(self):
        savedir = getSaveDir()

        # Data 
        LoadAscii(Filename=self.dat_file, OutputWorkspace='VULCAN_22946_NOM',Unit='TOF') 
        
        # Reflections and starting profile parameters 
        CreateLeBailFitInput(FullprofParameterFile=self.irf_file, 
                GenerateBraggReflections='1',LatticeConstant='5.431364000',
                InstrumentParameterWorkspace='Vulcan_B270_Profile', 
                BraggPeakParameterWorkspace='GeneralReflectionTable')

        # Pre-refined background
        paramnames = ["Bkpos", "A0", "A1", "A2", "A3", "A4", "A5"] 
        paramvalues = [11000.000, 0.034, 0.027, -0.129, 0.161, -0.083, .015] 
        bkgdtablewsname = "VULCAN_22946_Bkgd_Parameter" 
        api.CreateEmptyTableWorkspace(OutputWorkspace=bkgdtablewsname) 
        ws = mtd[bkgdtablewsname] 
        ws.addColumn("str", "Name") 
        ws.addColumn("double", "Value") 
        for i in xrange(len(paramnames)): 
            ws.addRow([paramnames[i], paramvalues[i]])
        
        # Examine profile
        ExaminePowderDiffProfile(
                InputWorkspace      = "VULCAN_22946_NOM",
                LoadData            = False,
                StartX              = 7000.,
                EndX                = 33000.,
                ProfileType         = "Back-to-back exponential convoluted with PseudoVoigt",
                ProfileWorkspace    = "Vulcan_B270_Profile",
                BraggPeakWorkspace  = "GeneralReflectionTable",
                GenerateInformationWS   = False,
                BackgroundParameterWorkspace    = "VULCAN_22946_Bkgd_Parameter",
                ProcessBackground   = False,
                BackgroundType      = "FullprofPolynomial",
                BackgroundWorkspace = "Dummy",
                OutputWorkspace     = "VULCAN_22946_Calculated")

        # Set up sequential refinement
        api.RefinePowderDiffProfileSeq(
                InputWorkspace      = "VULCAN_22946_NOM",
                SeqControlInfoWorkspace = "",
                InputProfileWorkspace = "Vulcan_B270_Profile",
                InputBraggPeaksWorkspace = "GeneralReflectionTable",
                InputBackgroundParameterWorkspace = "VULCAN_22946_Bkgd_Parameter",
                StartX = 7000.,
                EndX = 33000.,
                FunctionOption = "Setup", # or "Refine"
                RefinementOption = "Random Walk",
                ParametersToRefine = "Alph0",
                NumRefineCycles = 1000,
                ProfileType = "Neutron Back-to-back exponential convoluted with pseudo-voigt",
                BackgroundType = "FullprofPolynomial",
                ProjectID = "IDx890")

        # Refine step 1
        api.RefinePowderDiffProfileSeq(
                InputWorkspace      = "VULCAN_22946_NOM",
                SeqControlInfoWorkspace = "RecordIDx890Table",
                InputProfileWorkspace = "Vulcan_B270_Profile",
                InputBraggPeaksWorkspace = "GeneralReflectionTable",
                InputBackgroundParameterWorkspace = "VULCAN_22946_Bkgd_Parameter",
                StartX = 7000.,
                EndX = 33000.,
                FunctionOption = "Refine", # or "Refine"
                RefinementOption = "Random Walk",
                ParametersToRefine = "Alph0",
                NumRefineCycles = 1000,
                ProfileType = "Neutron Back-to-back exponential convoluted with pseudo-voigt",
                BackgroundType = "FullprofPolynomial",
                ProjectID = "IDx890")


        # Refine step 2
        api.RefinePowderDiffProfileSeq(
                InputWorkspace      = "VULCAN_22946_NOM",
                SeqControlInfoWorkspace = "RecordIDx890Table",
                # InputProfileWorkspace = "Vulcan_B270_Profile",
                # InputBraggPeaksWorkspace = "GeneralReflectionTable",
                # InputBackgroundParameterWorkspace = "VULCAN_22946_Bkgd_Parameter",
                StartX = 7000.,
                EndX = 33000.,
                FunctionOption = "Refine", # or "Refine"
                RefinementOption = "Random Walk",
                ParametersToRefine = "Beta0, Beta1",
                NumRefineCycles = 100,
                # ProfileType = "Neutron Back-to-back exponential convoluted with psuedo-voigt",
                # BackgroundType = "FullprofPolynomial"
                ProjectID = "IDx890")


        # Refine step 3 (not from previous cycle)
        api.RefinePowderDiffProfileSeq(
                InputWorkspace      = "VULCAN_22946_NOM",
                SeqControlInfoWorkspace = "RecordIDx890Table",
                StartX = 7000.,
                EndX = 33000.,
                FunctionOption = "Refine", # or "Refine"
                RefinementOption = "Random Walk",
                ParametersToRefine = "Beta0, Beta1",
                NumRefineCycles = 100,
                FromStep = 1,
                ProjectID = "IDx890")

        # Save
        api.RefinePowderDiffProfileSeq(
                InputWorkspace      = "VULCAN_22946_NOM",
                SeqControlInfoWorkspace = "RecordIDx890Table",
                FunctionOption = "Save", 
                OutputProjectFilename = "temp991.nxs",
                ProjectID = "IDx890")

        return
        
    def validateMethod(self):
        """ Return None as running is all that we want at this moment. 
        """
        return None

    def validate(self):
        self.tolerance=1.0e-6
        return ('VULCAN_22946_Calculated', 'VULCAN_22946_Calculated')

class VulcanSeqRefineProfileLoadPlus(stresstesting.MantidStressTest):
    """ System test for sequential refinement 
    """
    seqfile = "VULCAN_Calibrate_Seq.nxs"

    def requiredFiles(self):
        files = [self.seqfile]
        return files

    def runTest(self):
        savedir = getSaveDir()

        # Load
        api.RefinePowderDiffProfileSeq(
                FunctionOption = "Load", 
                InputProjectFilename = self.seqfile,
                ProjectID = "IDx890")

        # Refine step 4
        api.RefinePowderDiffProfileSeq(
                InputWorkspace      = "VULCAN_22946_NOM",
                SeqControlInfoWorkspace = "RecordIDx890Table",
                startx = 7000.,
                EndX = 33000.,
                FunctionOption = "Refine", # or "Refine"
                RefinementOption = "Random Walk",
                ParametersToRefine = "Alph1",
                NumRefineCycles = 200,
                ProjectID = "IDx890")


    def validateMethod(self):
        """ Return None as running is all that we want at this moment. 
        """
        return None

    def validate(self):
        self.tolerance=1.0e-6
        return ('VULCAN_22946_Calculated', 'VULCAN_22946_Calculated')
