from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd, AlgorithmFactory, DistributedDataProcessorAlgorithm, ITableWorkspaceProperty, \
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
    # chunkSize=0 signifies that the user wants to read the whole file
    if chunkSize == 0.:
        return [{}]

    # "small" files just get read in
    sizeGiB = os.path.getsize(filename)/1024./1024./1024.
    if 6.*sizeGiB < chunkSize:
        return [{}]

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


class AlignAndFocusPowderFromFiles(DistributedDataProcessorAlgorithm):
    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return [ "AlignAndFocusPowder" ]

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
        self.copyProperties('AlignAndFocusPowder', ['UnfocussedWorkspace'])

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
        self.log().debug('__updateAlignAndFocusArgs(%s)' % wkspname)
        # if the files are missing, there is nothing to do
        if (CAL_FILE not in self.kwargs) and (GROUP_FILE not in self.kwargs):
            self.log().debug('--> Nothing to do')
            return
        self.log().debug('--> Updating')

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

    def __determineCharacterizations(self, filename, wkspname, loadFile):
        useCharac = bool(self.charac is not None)

        # input workspace is only needed to find a row in the characterizations table
        tempname = None
        if loadFile:
            if useCharac:
                tempname = '__%s_temp' % wkspname
                Load(Filename=filename, OutputWorkspace=tempname,
                     MetaDataOnly=True)
        else:
            tempname = wkspname # assume it is already loaded

        # put together argument list
        args = dict(ReductionProperties=self.getProperty('ReductionProperties').valueAsStr)
        for name in PROPS_FOR_PD_CHARACTER:
            prop = self.getProperty(name)
            if not prop.isDefault:
                args[name] = prop.value
        if tempname is not None:
            args['InputWorkspace']=tempname
        if useCharac:
            args['Characterizations'] = self.charac

        PDDetermineCharacterizations(**args)

        if loadFile and useCharac:
            DeleteWorkspace(Workspace=tempname)

    def __getCacheName(self, wkspname):
        cachedir = self.getProperty('CacheDir').value
        if len(cachedir) <= 0:
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

    def __processFile(self, filename, wkspname, unfocusname, file_prog_start, determineCharacterizations):
        chunks = determineChunking(filename, self.chunkSize)
        numSteps = 6 # for better progress reporting - 6 steps per chunk
        if unfocusname != '':
            numSteps = 7 # one more for accumulating the unfocused workspace
        self.log().information('Processing \'{}\' in {:d} chunks'.format(filename, len(chunks)))
        prog_per_chunk_step = self.prog_per_file * 1./(numSteps*float(len(chunks)))

        # inner loop is over chunks
        for (j, chunk) in enumerate(chunks):
            prog_start = file_prog_start + float(j) * float(numSteps - 1) * prog_per_chunk_step
            chunkname = '{}_c{:d}'.format(wkspname, j)
            unfocusname_chunk = '{}_c{:d}'.format(unfocusname, j)
            Load(Filename=filename, OutputWorkspace=chunkname,
                 startProgress=prog_start, endProgress=prog_start+prog_per_chunk_step,
                 **chunk)
            if determineCharacterizations:
                self.__determineCharacterizations(filename, chunkname, False) # updates instance variable
                determineCharacterizations = False

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

            AlignAndFocusPowder(InputWorkspace=chunkname, OutputWorkspace=chunkname, UnfocussedWorkspace=unfocusname_chunk,
                                startProgress=prog_start, endProgress=prog_start+2.*prog_per_chunk_step,
                                **self.kwargs)
            prog_start += 2.*prog_per_chunk_step # AlignAndFocusPowder counts for two steps

            if j == 0:
                self.__updateAlignAndFocusArgs(chunkname)
                RenameWorkspace(InputWorkspace=chunkname, OutputWorkspace=wkspname)
                if unfocusname != '':
                    RenameWorkspace(InputWorkspace=unfocusname_chunk, OutputWorkspace=unfocusname)
            else:
                Plus(LHSWorkspace=wkspname, RHSWorkspace=chunkname, OutputWorkspace=wkspname,
                     ClearRHSWorkspace=self.kwargs['PreserveEvents'],
                     startProgress=prog_start, endProgress=prog_start+prog_per_chunk_step)
                DeleteWorkspace(Workspace=chunkname)

                if unfocusname != '':
                    Plus(LHSWorkspace=unfocusname, RHSWorkspace=unfocusname_chunk, OutputWorkspace=unfocusname,
                         ClearRHSWorkspace=self.kwargs['PreserveEvents'],
                         startProgress=prog_start, endProgress=prog_start + prog_per_chunk_step)
                    DeleteWorkspace(Workspace=unfocusname_chunk)

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
        useCaching = len(self.getProperty('CacheDir').value) > 0

        # accumulate the unfocused workspace if it was requested
        # empty string means it is not used
        unfocusname = self.getPropertyValue('UnfocussedWorkspace')
        unfocusname_file = ''
        if len(unfocusname) > 0:
            unfocusname_file = '__{}_partial'.format(unfocusname)

        if useCaching:
            # unfocus check only matters if caching is requested
            if unfocusname != '':
                self.log().warning('CacheDir is specified with "UnfocussedWorkspace" - reading cache files disabled')
        else:
            self.log().warning('CacheDir is not specified - functionality disabled')

        self.prog_per_file = 1./float(len(filenames)) # for better progress reporting

        # these are also passed into the child-algorithms
        self.kwargs = self.__getAlignAndFocusArgs()

        # outer loop creates chunks to load
        for (i, filename) in enumerate(filenames):
            # default name is based off of filename
            wkspname = os.path.split(filename)[-1].split('.')[0]

            if useCaching:
                self.__determineCharacterizations(filename,
                                                  wkspname, True) # updates instance variable
                cachefile = self.__getCacheName(wkspname)
            else:
                cachefile = None

            wkspname += '_f%d' % i # add file number to be unique

            # if the unfocussed data is requested, don't read it from disk
            # because of the extra complication of the unfocussed workspace
            if useCaching and os.path.exists(cachefile) and unfocusname == '':
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
                self.__processFile(filename, wkspname, unfocusname_file, self.prog_per_file*float(i), not useCaching)

                # write out the cachefile for the main reduced data independent of whether
                # the unfocussed workspace was requested
                if useCaching:
                    SaveNexusProcessed(InputWorkspace=wkspname, Filename=cachefile)

            # accumulate runs
            if i == 0:
                if wkspname != finalname:
                    RenameWorkspace(InputWorkspace=wkspname, OutputWorkspace=finalname)
                if unfocusname != '':
                    RenameWorkspace(InputWorkspace=unfocusname_file, OutputWorkspace=unfocusname)
            else:
                Plus(LHSWorkspace=finalname, RHSWorkspace=wkspname, OutputWorkspace=finalname,
                     ClearRHSWorkspace=self.kwargs['PreserveEvents'])
                DeleteWorkspace(Workspace=wkspname)

                if unfocusname != '':
                    Plus(LHSWorkspace=unfocusname, RHSWorkspace=unfocusname_file, OutputWorkspace=unfocusname,
                         ClearRHSWorkspace=self.kwargs['PreserveEvents'])
                    DeleteWorkspace(Workspace=unfocusname_file)

                if self.kwargs['PreserveEvents']:
                    CompressEvents(InputWorkspace=finalname, OutputWorkspace=finalname)
                    # not compressing unfocussed workspace because it is in d-spacing
                    # and is likely to be from a different part of the instrument

        # with more than one chunk or file the integrated proton charge is
        # generically wrong
        mtd[finalname].run().integrateProtonCharge()

        # set the output workspace
        self.setProperty('OutputWorkspace', mtd[finalname])
        if unfocusname != '':
            self.setProperty('UnfocussedWorkspace', mtd[unfocusname])


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(AlignAndFocusPowderFromFiles)
