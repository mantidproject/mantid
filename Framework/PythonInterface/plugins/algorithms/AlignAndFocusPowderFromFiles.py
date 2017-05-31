from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd, AlgorithmFactory, DataProcessorAlgorithm, ITableWorkspaceProperty, \
    MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode
from mantid.kernel import ConfigService, Direction
from mantid.simpleapi import AlignAndFocusPowder, CompressEvents, ConvertUnits, CreateCacheFilename, \
    DeleteWorkspace, DetermineChunking, Divide, EditInstrumentGeometry, FilterBadPulses, Load, \
    LoadNexusProcessed, PDDetermineCharacterizations, Plus, RenameWorkspace, SaveNexusProcessed
import os

EXTENSIONS_NXS = ["_event.nxs", ".nxs.h5"]
PROPS_FOR_INSTR = ["PrimaryFlightPath", "SpectrumIDs", "L2", "Polar", "Azimuthal"]
CAL_FILE, GROUP_FILE = "CalFileName", "GroupFilename"
CAL_WKSP, GRP_WKSP, MASK_WKSP = "CalibrationWorkspace", "GroupingWorkspace", "MaskWorkspace"
PROPS_FOR_ALIGN = [CAL_FILE, GROUP_FILE,
                   GRP_WKSP,CAL_WKSP, "OffsetsWorkspace",
                   MASK_WKSP, "MaskBinTable",
                   "Params", "ResampleX", "Dspacing", "DMin", "DMax",
                   "TMin", "TMax", "PreserveEvents",
                   "RemovePromptPulseWidth", "CompressTolerance",
                   "UnwrapRef", "LowResRef",
                   "CropWavelengthMin", "CropWavelengthMax",
                   "LowResSpectrumOffset", "ReductionProperties"]
PROPS_FOR_ALIGN.extend(PROPS_FOR_INSTR)
PROPS_FOR_PD_CHARACTER = ['FrequencyLogNames', 'WaveLengthLogNames']


def determineChunking(filename, chunkSize):
    chunks = DetermineChunking(Filename=filename, MaxChunkSize=chunkSize, OutputWorkspace='chunks')

    strategy = []
    for row in chunks:
        strategy.append(row)

    # For table with no rows
    if len(strategy) == 0:
        strategy.append({})

    # delete chunks workspace
    chunks = str(chunks)
    DeleteWorkspace(Workspace='chunks')

    return strategy


class AlignAndFocusPowderFromFiles(DataProcessorAlgorithm):
    def category(self):
        return "Diffraction\\Reduction"

    def name(self):
        return "AlignAndFocusPowderFromFiles"

    def summary(self):
        """
        summary of the algorithm
        :return:
        """
        return "The algorithm used for reduction of powder diffraction data"

    def PyInit(self):
        self.declareProperty(MultipleFileProperty(name="Filename",
                                                  extensions=EXTENSIONS_NXS),
                             "Files to combine in reduction")
        self.declareProperty("MaxChunkSize", 0.,
                             "Specify maximum Gbytes of file to read in one chunk.  Default is whole file.")
        self.declareProperty("FilterBadPulses", 0.,
                             doc="Filter out events measured while proton charge is more than 5% below average")

        self.declareProperty(MatrixWorkspaceProperty('AbsorptionWorkspace', '',
                                                     Direction.Input, PropertyMode.Optional),
                             doc='Divide data by this Pixel-by-pixel workspace')

        self.copyProperties('CreateCacheFilename', 'CacheDir')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     Direction.Output),
                             doc='Combined output workspace')

        self.declareProperty(ITableWorkspaceProperty('Characterizations', '',
                                                     Direction.Input, PropertyMode.Optional),
                             'Characterizations table')

        self.copyProperties("AlignAndFocusPowder", PROPS_FOR_ALIGN)
        self.copyProperties('PDDetermineCharacterizations', PROPS_FOR_PD_CHARACTER)

    def _getLinearizedFilenames(self, propertyName):
        runnumbers = self.getProperty(propertyName).value
        linearizedRuns = []
        for item in runnumbers:
            if type(item) == list:
                linearizedRuns.extend(item)
            else:
                linearizedRuns.append(item)
        return linearizedRuns

    def __getAlignAndFocusArgs(self):
        args = {}
        for name in PROPS_FOR_ALIGN:
            prop = self.getProperty(name)
            if name == 'PreserveEvents' or not prop.isDefault:
                if 'Workspace' in name:
                    args[name] = prop.valueAsStr
                else:
                    args[name] = prop.value
        return args

    def __updateAlignAndFocusArgs(self, wkspname):
        self.log().warning('__updateAlignAndFocusArgs(%s)!!!!!!!' % wkspname)
        # if the files are missing, there is nothing to do
        if (CAL_FILE not in self.kwargs) and (GROUP_FILE not in self.kwargs):
            self.log().warning('--> Nothing to do')
        self.log().warning('--> Updating')

        # delete the files from the list of kwargs
        if CAL_FILE in self.kwargs:
            del self.kwargs[CAL_FILE]
        if CAL_FILE in self.kwargs:
            del self.kwargs[GROUP_FILE]

        # get the instrument name
        instr = mtd[wkspname].getInstrument().getName()
        instr = ConfigService.getInstrument(instr).shortName()

        # use the canonical names if they weren't specifed
        for key, ext in zip((CAL_WKSP, GRP_WKSP, MASK_WKSP),
                            ('_cal', '_group', '_mask')):
            if key not in self.kwargs:
                self.kwargs[key] = instr + ext

    def __determineCharacterizations(self, filename, wkspname):
        tempname = '__%s_temp' % wkspname
        Load(Filename=filename, OutputWorkspace=tempname,
             MetaDataOnly=True)

        # put together argument list
        args = dict(InputWorkspace=tempname,
                    ReductionProperties=self.getProperty('ReductionProperties').valueAsStr)
        for name in PROPS_FOR_PD_CHARACTER:
            prop = self.getProperty(name)
            if not prop.isDefault:
                args[name] = prop.value
        if self.charac is not None:
            args['Characterizations'] = self.charac

        PDDetermineCharacterizations(**args)
        DeleteWorkspace(Workspace=tempname)

    def __getCacheName(self, wkspname):
        cachedir = self.getProperty('CacheDir').value
        if len(cachedir) <= 0:
            self.log().warning('CacheDir is not specified - functionality disabled')
            return None

        propman_properties = ['bank', 'd_min', 'd_max', 'tof_min', 'tof_max', 'wavelength_min', 'wavelength_max']
        alignandfocusargs = []
        for name in PROPS_FOR_ALIGN:
            prop = self.getProperty(name)
            if name == 'PreserveEvents' or not prop.isDefault:
                # TODO need unique identifier for absorption workspace
                alignandfocusargs.append('%s=%s' % (name, prop.valueAsStr))

        return CreateCacheFilename(Prefix=wkspname,
                                   PropertyManager=self.getProperty('ReductionProperties').valueAsStr,
                                   Properties=propman_properties,
                                   OtherProperties=alignandfocusargs,
                                   CacheDir=cachedir).OutputFilename

    def __processFile(self, filename, wkspname, file_prog_start):
        chunks = determineChunking(filename, self.chunkSize)
        self.log().information('Processing \'%s\' in %d chunks' % (filename, len(chunks)))
        prog_per_chunk_step = self.prog_per_file * 1./(6.*float(len(chunks))) # for better progress reporting - 6 steps per chunk

        # inner loop is over chunks
        for (j, chunk) in enumerate(chunks):
            prog_start = file_prog_start + float(j) * 5. * prog_per_chunk_step
            chunkname = "%s_c%d" % (wkspname, j)
            Load(Filename=filename, OutputWorkspace=chunkname,
                 startProgress=prog_start, endProgress=prog_start+prog_per_chunk_step,
                 **chunk)
            prog_start += prog_per_chunk_step
            if self.filterBadPulses > 0.:
                FilterBadPulses(InputWorkspace=chunkname, OutputWorkspace=chunkname,
                                LowerCutoff=self.filterBadPulses,
                                startProgress=prog_start, endProgress=prog_start+prog_per_chunk_step)
            prog_start += prog_per_chunk_step

            # absorption correction workspace
            if self.absorption is not None and len(str(self.absorption)) > 0:
                ConvertUnits(InputWorkspace=chunkname, OutputWorkspace=chunkname,
                             Target='Wavelength', EMode='Elastic')
                Divide(LHSWorkspace=chunkname, RHSWorkspace=self.absorption, OutputWorkspace=chunkname,
                       startProgress=prog_start, endProgress=prog_start+prog_per_chunk_step)
                ConvertUnits(InputWorkspace=chunkname, OutputWorkspace=chunkname,
                             Target='TOF', EMode='Elastic')
            prog_start += prog_per_chunk_step

            AlignAndFocusPowder(InputWorkspace=chunkname, OutputWorkspace=chunkname,
                                startProgress=prog_start, endProgress=prog_start+2.*prog_per_chunk_step,
                                **self.kwargs)
            prog_start += 2.*prog_per_chunk_step # AlignAndFocusPowder counts for two steps

            if j == 0:
                self.__updateAlignAndFocusArgs(chunkname)
                RenameWorkspace(InputWorkspace=chunkname, OutputWorkspace=wkspname)
            else:
                Plus(LHSWorkspace=wkspname, RHSWorkspace=chunkname, OutputWorkspace=wkspname,
                     ClearRHSWorkspace=self.kwargs['PreserveEvents'],
                     startProgress=prog_start, endProgress=prog_start+prog_per_chunk_step)
                DeleteWorkspace(Workspace=chunkname)
                if self.kwargs['PreserveEvents']:
                    CompressEvents(InputWorkspace=wkspname, OutputWorkspace=wkspname)
        # end of inner loop

    def PyExec(self):
        filenames = self._getLinearizedFilenames('Filename')
        self.filterBadPulses = self.getProperty('FilterBadPulses').value
        self.chunkSize = self.getProperty('MaxChunkSize').value
        self.absorption = self.getProperty('AbsorptionWorkspace').value
        self.charac = self.getProperty('Characterizations').value
        finalname = self.getProperty('OutputWorkspace').valueAsStr

        self.prog_per_file = 1./float(len(filenames)) # for better progress reporting

        # these are also passed into the child-algorithms
        self.kwargs = self.__getAlignAndFocusArgs()

        # outer loop creates chunks to load
        for (i, filename) in enumerate(filenames):
            # default name is based off of filename
            wkspname = os.path.split(filename)[-1].split('.')[0]
            self.__determineCharacterizations(filename,
                                              wkspname) # updates instance variable
            cachefile = self.__getCacheName(wkspname)
            wkspname += '_f%d' % i # add file number to be unique

            if cachefile is not None and os.path.exists(cachefile):
                LoadNexusProcessed(Filename=cachefile, OutputWorkspace=wkspname)
                # TODO LoadNexusProcessed has a bug. When it finds the
                # instrument name without xml it reads in from an IDF
                # in the instrument directory.
                editinstrargs = {}
                for name in PROPS_FOR_INSTR:
                    prop = self.getProperty(name)
                    if not prop.isDefault:
                        editinstrargs[name] = prop.value
                if editinstrargs:
                    EditInstrumentGeometry(Workspace=wkspname, **editinstrargs)
            else:
                self.__processFile(filename, wkspname, self.prog_per_file*float(i))
                if cachefile is not None:
                    SaveNexusProcessed(InputWorkspace=wkspname, Filename=cachefile)

            # accumulate runs
            if i == 0:
                if wkspname != finalname:
                    RenameWorkspace(InputWorkspace=wkspname, OutputWorkspace=finalname)
            else:
                Plus(LHSWorkspace=finalname, RHSWorkspace=wkspname, OutputWorkspace=finalname,
                     ClearRHSWorkspace=self.kwargs['PreserveEvents'])
                DeleteWorkspace(Workspace=wkspname)
                if self.kwargs['PreserveEvents']:
                    CompressEvents(InputWorkspace=finalname, OutputWorkspace=finalname)

        # with more than one chunk or file the integrated proton charge is
        # generically wrong
        mtd[finalname].run().integrateProtonCharge()

        # set the output workspace
        self.setProperty('OutputWorkspace', mtd[finalname])

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(AlignAndFocusPowderFromFiles)
