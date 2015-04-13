#pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import stresstesting
from mantid.simpleapi import *
from mantid.api import FileFinder

import os

def getSaveDir():
    """determine where to save - the current working directory"""
    import os
    return os.path.abspath(os.path.curdir)

def do_cleanup():
    Files = ["PG3_9829.gsa",
    "PG3_9829.py",
    "PG3_9830.gsa",
    "PG3_9830.py",
    "PG3_4844-1.dat",
    "PG3_4844.getn",
    "PG3_4844.gsa",
    "PG3_4844.py",
    "PG3_4866.gsa"]
    for file in Files:
        absfile = FileFinder.getFullPath(file)
        if os.path.exists(absfile):
            os.remove(absfile)
    return True

class PG3Analysis(stresstesting.MantidStressTest):
    ref_file  = 'PG3_4844_reference.gsa'
    cal_file  = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2011_08_31-HR.txt"

    def cleanup(self):
        do_cleanup()
        return True

    def requiredFiles(self):
        files = [self.ref_file, self.cal_file, self.char_file]
        files.append("PG3_4844_event.nxs") # /SNS/PG3/IPTS-2767/0/
        files.append("PG3_4866_event.nxs") # /SNS/PG3/IPTS-2767/0/
        files.append("PG3_5226_event.nxs") # /SNS/PG3/IPTS-2767/0/
        return files

    def runTest(self):
        savedir = getSaveDir()

        # run the actual code
        SNSPowderReduction(Instrument="PG3", RunNumber=4844, Extension="_event.nxs",
                           PreserveEvents=True,
                           CalibrationFile=self.cal_file,
                           CharacterizationRunsFile=self.char_file,
                           LowResRef=15000, RemovePromptPulseWidth=50,
                           Binning=-0.0004, BinInDspace=True, FilterBadPulses=95,
                           SaveAs="gsas and fullprof and pdfgetn", OutputDirectory=savedir,
                           FinalDataUnits="dSpacing")


        # load output gsas file and the golden one
        LoadGSS(Filename="PG3_4844.gsa", OutputWorkspace="PG3_4844")
        LoadGSS(Filename=self.ref_file, OutputWorkspace="PG3_4844_golden")

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        self.tolerance = 1.0e-2
        return ('PG3_4844','PG3_4844_golden')

class PG3StripPeaks(stresstesting.MantidStressTest):
    ref_file = 'PG3_4866_reference.gsa'
    cal_file  = "PG3_FERNS_d4832_2011_08_24.cal"

    def cleanup(self):
        do_cleanup()
        return True

    def requiredFiles(self):
        files = [self.ref_file, self.cal_file]
        files.append("PG3_4866_event.nxs") # vanadium
        return files

    def runTest(self):
        # determine where to save
        import os
        savedir = os.path.abspath(os.path.curdir)

        LoadEventNexus(Filename="PG3_4866_event.nxs",
                       OutputWorkspace="PG3_4866",
                       Precount=True)
        FilterBadPulses(InputWorkspace="PG3_4866",
                        OutputWorkspace="PG3_4866")
        RemovePromptPulse(InputWorkspace="PG3_4866",
                          OutputWorkspace="PG3_4866",
                          Width=50)
        CompressEvents(InputWorkspace="PG3_4866",
                       OutputWorkspace="PG3_4866",
                       Tolerance=0.01)
        SortEvents(InputWorkspace="PG3_4866")
        CropWorkspace(InputWorkspace="PG3_4866",
                      OutputWorkspace="PG3_4866",
                      XMax=16666.669999999998)
        LoadCalFile(InputWorkspace="PG3_4866",
                    CalFilename=self.cal_file,
                    WorkspaceName="PG3")
        MaskDetectors(Workspace="PG3_4866",
                      MaskedWorkspace="PG3_mask")
        AlignDetectors(InputWorkspace="PG3_4866",
                       OutputWorkspace="PG3_4866",
                       OffsetsWorkspace="PG3_offsets")
        ConvertUnits(InputWorkspace="PG3_4866",
                     OutputWorkspace="PG3_4866",
                     Target="TOF")
        UnwrapSNS(InputWorkspace="PG3_4866",
                  OutputWorkspace="PG3_4866",
                  LRef=62)
        RemoveLowResTOF(InputWorkspace="PG3_4866",
                        OutputWorkspace="PG3_4866",
                        ReferenceDIFC=1500)
        ConvertUnits(InputWorkspace="PG3_4866",
                     OutputWorkspace="PG3_4866",
                     Target="dSpacing")
        Rebin(InputWorkspace="PG3_4866",
              OutputWorkspace="PG3_4866",
              Params=(0.1,-0.0004,2.2))
        SortEvents(InputWorkspace="PG3_4866")
        DiffractionFocussing(InputWorkspace="PG3_4866",
                             OutputWorkspace="PG3_4866",
                             GroupingWorkspace="PG3_group")
        EditInstrumentGeometry(Workspace="PG3_4866",
                               PrimaryFlightPath=60,
                               SpectrumIDs=[1],
                               L2=[3.2208],
                               Polar=[90.8074],
                               Azimuthal=[0])
        ConvertUnits(InputWorkspace="PG3_4866",
                     OutputWorkspace="PG3_4866",
                     Target="TOF")
        Rebin(InputWorkspace="PG3_4866",
              OutputWorkspace="PG3_4866",
              Params=[-0.0004])
        ConvertUnits(InputWorkspace="PG3_4866",
                     OutputWorkspace="PG3_4866",
                     Target="dSpacing")
        StripVanadiumPeaks(InputWorkspace="PG3_4866",
                           OutputWorkspace="PG3_4866",
                           PeakPositionTolerance=0.05,
                           FWHM=8,
                           BackgroundType="Quadratic")
        ConvertUnits(InputWorkspace="PG3_4866",
                     OutputWorkspace="PG3_4866",
                     Target="TOF")
        SaveGSS(InputWorkspace="PG3_4866",
                Filename=os.path.join(savedir, "PG3_4866.gsa"),
                SplitFiles=False,
                Append=False,
                Format="SLOG",
                MultiplyByBinWidth=False,
                ExtendedHeader=True)

        # load output gsas file and the golden one
        LoadGSS(Filename="PG3_4866.gsa", OutputWorkspace="PG3_4866")
        LoadGSS(Filename=self.ref_file, OutputWorkspace="PG3_4866_golden")

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        self.tolerance = 1.0e-2
        return ('PG3_4866','PG3_4866_golden')

class SeriesAndConjoinFilesTest(stresstesting.MantidStressTest):
    cal_file   = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file  = "PG3_characterization_2012_02_23-HR-ILL.txt"
    ref_files  = ['PG3_9829_reference.gsa', 'PG3_9830_reference.gsa']
    data_files = ['PG3_9829_event.nxs', 'PG3_9830_event.nxs']

    def cleanup(self):
        do_cleanup()
        return True

    def requiredMemoryMB(self):
        """Requires 3Gb"""
        return 3000

    def requiredFiles(self):
        files = [self.cal_file, self.char_file]
        files.extend(self.ref_files)
        files.extend(self.data_files)
        return files

    def runTest(self):
        savedir = getSaveDir()

        # reduce a sum of runs - and drop it
        SNSPowderReduction(Instrument="PG3", RunNumber=[9829,9830], Extension="_event.nxs",
                           Sum=True, # This is the difference with the next call
                           PreserveEvents=True, VanadiumNumber=-1,
                           CalibrationFile=self.cal_file,
                           CharacterizationRunsFile=self.char_file,
                           LowResRef=15000, RemovePromptPulseWidth=50,
                           Binning=-0.0004, BinInDspace=True, FilterBadPulses=True,
                           SaveAs="gsas", OutputDirectory=savedir,
                           FinalDataUnits="dSpacing")

        # reduce a series of runs
        SNSPowderReduction(Instrument="PG3", RunNumber=[9829,9830], Extension="_event.nxs",
                           PreserveEvents=True, VanadiumNumber=-1,
                           CalibrationFile=self.cal_file,
                           CharacterizationRunsFile=self.char_file,
                           LowResRef=15000, RemovePromptPulseWidth=50,
                           Binning=-0.0004, BinInDspace=True, FilterBadPulses=True,
                           SaveAs="gsas", OutputDirectory=savedir,
                           FinalDataUnits="dSpacing")

        # needs to be set for ConjoinFiles to work
        config['default.facility'] = 'SNS'
        config['default.instrument'] = 'POWGEN'

        # load back in the resulting gsas files
        ConjoinFiles(RunNumbers=[9829,9830], OutputWorkspace='ConjoinFilesTest', Directory=savedir)
        # convert units makes sure the geometry was picked up
        ConvertUnits(InputWorkspace='ConjoinFilesTest', OutputWorkspace='ConjoinFilesTest',
                     Target="dSpacing")

        # prepare for validation
        LoadGSS(Filename="PG3_9829.gsa", OutputWorkspace="PG3_9829")
        LoadGSS(Filename=self.ref_files[0], OutputWorkspace="PG3_4844_golden")
        #LoadGSS("PG3_9830.gsa", "PG3_9830") # can only validate one workspace
        #LoadGSS(self.ref_file[1], "PG3_9830_golden")

    def validateMethod(self):
        return None # it running is all that we need

    def validate(self):
        self.tolerance = 1.0e-2
        return ('PG3_9829','PG3_9829_golden')
        #return ('PG3_9830','PG3_9830_golden') # can only validate one workspace
