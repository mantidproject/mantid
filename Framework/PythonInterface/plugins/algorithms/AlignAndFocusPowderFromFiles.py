# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd, AlgorithmFactory, DistributedDataProcessorAlgorithm, ITableWorkspaceProperty, \
    MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode, AnalysisDataService
from mantid.kernel import Direction
from mantid.simpleapi import AlignAndFocusPowder, CompressEvents, ConvertUnits, CopyLogs, CreateCacheFilename, \
    DeleteWorkspace, DetermineChunking, Divide, EditInstrumentGeometry, FilterBadPulses, LoadNexusProcessed, \
    PDDetermineCharacterizations, Plus, RemoveLogs, RenameWorkspace, SaveNexusProcessed, LoadDiffCal, \
    MaskDetectors, LoadDetectorsGroupingFile
import os

EXTENSIONS_NXS = ["_event.nxs", ".nxs.h5"]
PROPS_FOR_INSTR = ["PrimaryFlightPath", "SpectrumIDs", "L2", "Polar", "Azimuthal"]
CAL_FILE, GROUP_FILE = "CalFileName", "GroupFilename"
CAL_WKSP, GRP_WKSP, MASK_WKSP = "CalibrationWorkspace", "GroupingWorkspace", "MaskWorkspace"
PROPS_FOR_ALIGN = [CAL_FILE, GROUP_FILE,
                   GRP_WKSP, CAL_WKSP, "OffsetsWorkspace",
                   MASK_WKSP, "MaskBinTable",
                   "Params", "ResampleX", "Dspacing", "DMin", "DMax",
                   "TMin", "TMax", "PreserveEvents",
                   "RemovePromptPulseWidth", "CompressTolerance", "CompressWallClockTolerance",
                   "CompressStartTime", "UnwrapRef", "LowResRef",
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
        return ["AlignAndFocusPowder"]

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

    def validateInputs(self):
        errors = dict()

        unfocusname = self.getPropertyValue('UnfocussedWorkspace')
        if len(unfocusname) > 0:
            finalname = self.getPropertyValue('OutputWorkspace')
            if unfocusname == finalname:
                errors["OutputWorkspace"] = "Cannot be the same as UnfocussedWorkspace"
                errors["UnfocussedWorkspace"] = "Cannot be the same as OutputWorkspace"

        return errors

    def _getLinearizedFilenames(self, propertyName):
        runnumbers = self.getProperty(propertyName).value
        linearizedRuns = []
        for item in runnumbers:
            if type(item) == list:
                linearizedRuns.extend(item)
            else:
                linearizedRuns.append(item)
        return linearizedRuns

    def __createLoader(self, filename, wkspname, progstart=None, progstop=None):
        # load a chunk - this is a bit crazy long because we need to get an output property from `Load` when it
        # is run and the algorithm history doesn't exist until the parent algorithm (this) has finished
        if progstart is None or progstop is None:
            loader = self.createChildAlgorithm(self.__loaderName)
        else:
            loader = self.createChildAlgorithm(self.__loaderName,
                                               startProgress=progstart, endProgress=progstop)
        loader.setAlwaysStoreInADS(True)
        loader.setLogging(True)
        loader.initialize()
        loader.setPropertyValue('Filename', filename)
        loader.setPropertyValue('OutputWorkspace', wkspname)
        return loader

    def __getAlignAndFocusArgs(self):
        args = {}
        for name in PROPS_FOR_ALIGN:
            prop = self.getProperty(name)
            name_list = ['PreserveEvents', 'CompressTolerance',
                         'CompressWallClockTolerance', 'CompressStartTime']
            if name in name_list or not prop.isDefault:
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

        # use the canonical names if they weren't specifed
        for key, ext in zip((CAL_WKSP, GRP_WKSP, MASK_WKSP),
                            ('_cal', '_group', '_mask')):
            if key not in self.kwargs:
                self.kwargs[key] = self.instr + ext

    def __determineCharacterizations(self, filename, wkspname):
        useCharac = bool(self.charac is not None)
        loadFile = not mtd.doesExist(wkspname)

        # input workspace is only needed to find a row in the characterizations table
        tempname = None
        if loadFile:
            if useCharac:
                tempname = '__%s_temp' % wkspname
                # set the loader for this file
                loader = self.__createLoader(filename, tempname)
                loader.setProperty('MetaDataOnly', True)  # this is only supported by LoadEventNexus
                loader.execute()

                # get the underlying loader name if we used the generic one
                if self.__loaderName == 'Load':
                    self.__loaderName = loader.getPropertyValue('LoaderName')
        else:
            tempname = wkspname  # assume it is already loaded

        # put together argument list
        args = dict(ReductionProperties=self.getProperty('ReductionProperties').valueAsStr)
        for name in PROPS_FOR_PD_CHARACTER:
            prop = self.getProperty(name)
            if not prop.isDefault:
                args[name] = prop.value
        if tempname is not None:
            args['InputWorkspace'] = tempname
        if useCharac:
            args['Characterizations'] = self.charac

        PDDetermineCharacterizations(**args)

        if loadFile and useCharac:
            DeleteWorkspace(Workspace=tempname)

    def __getCacheName(self, wkspname, additional_props=None):
        """additional_props: list. additional properties to be hashed
        """
        cachedir = self.getProperty('CacheDir').value
        if len(cachedir) <= 0:
            return None

        propman_properties = ['bank', 'd_min', 'd_max', 'tof_min', 'tof_max', 'wavelength_min', 'wavelength_max']
        alignandfocusargs = []
        for name in PROPS_FOR_ALIGN:
            prop = self.getProperty(name)
            if name == 'PreserveEvents' or not prop.isDefault:
                # TODO need unique identifier for absorption workspace
                value = prop.valueAsStr  # default representation for everything
                if name == 'GroupFilename':
                    if not self.getProperty('GroupingWorkspace').isDefault:
                        # do not add this property to the cache calculation
                        continue
                    if prop.valueAsStr.split('.')[1] == 'cal':
                        __groups = LoadDiffCal(InstrumentName=self.instr, Filename=prop.valueAsStr,
                                               MakeCalWorkspace=False,
                                               WorkspaceName="__groups", MakeMaskWorkspace=False)
                    else:
                        __groups = LoadDetectorsGroupingFile(InputFile=prop.valueAsStr)
                    value = ','.join(__groups.extractY().astype(int).astype(str).ravel())
                    DeleteWorkspace(Workspace=__groups)
                elif name == 'GroupingWorkspace':
                    __groups = mtd[prop.valueAsStr]
                    value = ','.join(__groups.extractY().astype(int).astype(str).ravel())
                elif name == 'MaskWorkspace':
                    __mask = mtd[prop.valueAsStr]
                    value = ','.join([str(item) for item in __mask.getMaskedDetectors()])
                # add the calculated value
                alignandfocusargs.append('%s=%s' % (name, value))

        alignandfocusargs += additional_props or []
        return CreateCacheFilename(Prefix=wkspname,
                                   PropertyManager=self.getProperty('ReductionProperties').valueAsStr,
                                   Properties=propman_properties,
                                   OtherProperties=alignandfocusargs,
                                   CacheDir=cachedir).OutputFilename

    def __processFile(self, filename, wkspname, unfocusname, file_prog_start, determineCharacterizations):
        chunks = determineChunking(filename, self.chunkSize)
        numSteps = 6  # for better progress reporting - 6 steps per chunk
        if unfocusname != '':
            numSteps = 7  # one more for accumulating the unfocused workspace
        self.log().information('Processing \'{}\' in {:d} chunks'.format(filename, len(chunks)))
        prog_per_chunk_step = self.prog_per_file * 1./(numSteps*float(len(chunks)))
        unfocusname_chunk = ''
        canSkipLoadingLogs = False

        # inner loop is over chunks
        for (j, chunk) in enumerate(chunks):
            prog_start = file_prog_start + float(j) * float(numSteps - 1) * prog_per_chunk_step
            chunkname = '{}_c{:d}'.format(wkspname, j)
            if unfocusname != '':  # only create unfocus chunk if needed
                unfocusname_chunk = '{}_c{:d}'.format(unfocusname, j)

            # load a chunk - this is a bit crazy long because we need to get an output property from `Load` when it
            # is run and the algorithm history doesn't exist until the parent algorithm (this) has finished
            loader = self.__createLoader(filename, chunkname,
                                         progstart=prog_start, progstop=prog_start + prog_per_chunk_step)
            if canSkipLoadingLogs:
                loader.setProperty('LoadLogs', False)
            for key, value in chunk.items():
                if isinstance(value, str):
                    loader.setPropertyValue(key, value)
                else:
                    loader.setProperty(key, value)
            loader.execute()

            # copy the necessary logs onto the workspace
            if canSkipLoadingLogs:
                CopyLogs(InputWorkspace=wkspname, OutputWorkspace=chunkname, MergeStrategy='WipeExisting')

            # get the underlying loader name if we used the generic one
            if self.__loaderName == 'Load':
                self.__loaderName = loader.getPropertyValue('LoaderName')
            canSkipLoadingLogs = self.__loaderName == 'LoadEventNexus'

            if determineCharacterizations and j == 0:
                self.__determineCharacterizations(filename, chunkname)  # updates instance variable
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
            prog_start += 2. * prog_per_chunk_step  # AlignAndFocusPowder counts for two steps

            if j == 0:
                self.__updateAlignAndFocusArgs(chunkname)
                RenameWorkspace(InputWorkspace=chunkname, OutputWorkspace=wkspname)
                if unfocusname != '':
                    RenameWorkspace(InputWorkspace=unfocusname_chunk, OutputWorkspace=unfocusname)
            else:
                RemoveLogs(Workspace=chunkname)  # accumulation has them already
                Plus(LHSWorkspace=wkspname, RHSWorkspace=chunkname, OutputWorkspace=wkspname,
                     ClearRHSWorkspace=self.kwargs['PreserveEvents'],
                     startProgress=prog_start, endProgress=prog_start+prog_per_chunk_step)
                DeleteWorkspace(Workspace=chunkname)

                if unfocusname != '':
                    RemoveLogs(Workspace=unfocusname_chunk)  # accumulation has them already
                    Plus(LHSWorkspace=unfocusname, RHSWorkspace=unfocusname_chunk, OutputWorkspace=unfocusname,
                         ClearRHSWorkspace=self.kwargs['PreserveEvents'],
                         startProgress=prog_start, endProgress=prog_start + prog_per_chunk_step)
                    DeleteWorkspace(Workspace=unfocusname_chunk)

                if self.kwargs['PreserveEvents'] and self.kwargs['CompressTolerance'] > 0.:
                    CompressEvents(InputWorkspace=wkspname, OutputWorkspace=wkspname,
                                   WallClockTolerance=self.kwargs['CompressWallClockTolerance'],
                                   Tolerance=self.kwargs['CompressTolerance'],
                                   StartTime=self.kwargs['CompressStartTime'])
        # end of inner loop

    def __processFile_withcache(self, filename, useCaching, unfocusname, unfocusname_file):
        """process the given file and save the result in `wkspname`
        the difference between this and __processFile is this function takes cached into account
        """
        # default name is based off of filename
        wkspname = os.path.split(filename)[-1].split('.')[0]

        if useCaching:
            self.__determineCharacterizations(filename, wkspname)  # updates instance variable
            cachefile = self.__getCacheName(wkspname)
        else:
            cachefile = None

        i = self._filenames.index(filename)
        wkspname += '_f%d' % i  # add file number to be unique

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
            self.__processFile(filename, wkspname, unfocusname_file, self.prog_per_file * float(i), not useCaching)

            # write out the cachefile for the main reduced data independent of whether
            # the unfocussed workspace was requested
            if useCaching:
                SaveNexusProcessed(InputWorkspace=wkspname, Filename=cachefile)
        return wkspname

    def __accumulate(self, wkspname, sumwkspname, unfocusname, unfocusname_file):
        """accumulate newdata `wkspname` into sum `sumwkspname` and delete `wkspname`"""
        # the first call to accumulate to a specific target should be a simple rename
        self._accumulate_calls[sumwkspname] += 1
        firstrun = self._accumulate_calls[sumwkspname] == 1
        if firstrun:
            if wkspname != sumwkspname:
                RenameWorkspace(InputWorkspace=wkspname, OutputWorkspace=sumwkspname)
        else:
            Plus(LHSWorkspace=sumwkspname, RHSWorkspace=wkspname, OutputWorkspace=sumwkspname,
                 ClearRHSWorkspace=self.kwargs['PreserveEvents'])
            DeleteWorkspace(Workspace=wkspname)
            if self.kwargs['PreserveEvents'] and self.kwargs['CompressTolerance'] > 0.:
                finalname = self.getPropertyValue('OutputWorkspace')
                # only compress when adding individual files
                if sumwkspname != finalname:
                    CompressEvents(InputWorkspace=sumwkspname, OutputWorkspace=sumwkspname,
                                   WallClockTolerance=self.kwargs['CompressWallClockTolerance'],
                                   Tolerance=self.kwargs['CompressTolerance'],
                                   StartTime=self.kwargs['CompressStartTime'])
        #
        if unfocusname == '':
            return
        self._accumulate_calls[unfocusname] += 1
        firstrun = self._accumulate_calls[unfocusname] == 1
        if firstrun:
            RenameWorkspace(InputWorkspace=unfocusname_file, OutputWorkspace=unfocusname)
        else:
            Plus(LHSWorkspace=unfocusname, RHSWorkspace=unfocusname_file, OutputWorkspace=unfocusname,
                 ClearRHSWorkspace=self.kwargs['PreserveEvents'])
            DeleteWorkspace(Workspace=unfocusname_file)
            # not compressing unfocussed workspace because it is in d-spacing
            # and is likely to be from a different part of the instrument

    def PyExec(self):
        self._filenames = filenames = sorted(self._getLinearizedFilenames('Filename'))
        import collections
        self._accumulate_calls = collections.defaultdict(int) # bookkeeping for __accumulate
        # get the instrument name - will not work if the instrument has a '_' in its name
        self.instr = os.path.basename(filenames[0]).split('_')[0]
        self.__loaderName = 'Load'   # set the loader to be generic on first load
        for ext in ['_cal', '_group',' _mask']:
            if AnalysisDataService.doesExist(self.instr+ext):
                DeleteWorkspace(Workspace=self.instr+ext)
        self.filterBadPulses = self.getProperty('FilterBadPulses').value
        self.chunkSize = self.getProperty('MaxChunkSize').value
        self.absorption = self.getProperty('AbsorptionWorkspace').value
        self.charac = self.getProperty('Characterizations').value
        finalname = self.getPropertyValue('OutputWorkspace')
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

        assert len(filenames), "No files specified"
        self.prog_per_file = 1./float(len(filenames))  # for better progress reporting

        # these are also passed into the child-algorithms
        self.kwargs = self.__getAlignAndFocusArgs()

        # initialization for caching mechanism
        if useCaching:
            filename = filenames[0]
            wkspname = os.path.split(filename)[-1].split('.')[0]
            self.__determineCharacterizations(filename, wkspname)

        if useCaching:
            # find caches and sum them. return files without cache
            nocache = self.__find_caches(filenames, finalname, unfocusname, unfocusname_file)
        else:
            nocache = filenames
        self.log().notice('Files remained to add: {}'.format(nocache,))
        #
        # process not-cached files
        if nocache:
            self.__processFiles(nocache, useCaching, unfocusname, unfocusname_file)
        #
        # create cache of everything summed together
        if useCaching:
            self.__saveSummedGroupToCache(filenames, wkspname=finalname)

        # with more than one chunk or file the integrated proton charge is
        # generically wrong
        mtd[finalname].run().integrateProtonCharge()

        # set the output workspace
        self.setProperty('OutputWorkspace', mtd[finalname])
        if unfocusname != '':
            self.setProperty('UnfocussedWorkspace', mtd[unfocusname])

    def __find_caches(self, filenames, finalname, unfocusname, unfocusname_file, firstcall=True):
        """find caches and load them using a greedy algorithm

        Find cache for the longest partial sum of the given filenames.
        If no cache found for any partial sum, return the filename list.
        If a cache found for a partial sum, continue to work on the remained files by calling this function.
        When a cache is found, it will be loaded and accumulated, and then deleted.
        """
        N = len(filenames)
        found = False
        for length in range(N, 1, -1):
            for start in range(N-length+1):
                end = start+length-1
                files1 = filenames[start:end+1]
                summed_cache_file = self.__get_grp_cache_fn(files1)
                if os.path.exists(summed_cache_file):
                    self.log().notice('Found cache for {}'.format(files1))
                    wkspname = self.__get_grp_ws_name(files1)
                    self.log().notice('Loading cache from {}'.format(summed_cache_file))
                    LoadNexusProcessed(Filename=summed_cache_file, OutputWorkspace=wkspname)
                    self.__accumulate(wkspname, finalname, unfocusname, unfocusname_file)
                    found = True
                    break
            if found:
                break
            continue
        if not found:
            return filenames
        remained = filenames[:start] + filenames[end+1:]
        if remained:
            return self.__find_caches(remained, finalname, unfocusname, unfocusname_file, firstcall=False)
        return []

    def __get_grp_ws_name(self, group):
        def _(filename):
            return os.path.split(filename)[-1].split('.')[0]
        return _(group[0]) + "-" + _(group[-1])

    def __get_grp_cache_fn(self, group):
        wsname = self.__get_grp_ws_name(group)
        filenames_str = ','.join(group)
        newprop = 'files_to_sum={}'.format(filenames_str)
        return self.__getCacheName('summed_'+wsname, additional_props=[newprop])

    def __processFiles(self, files, useCaching, unfocusname, unfocusname_file):
        """process given files, separate them to "grains". Sum each grain, and add the grain sum to final sum
        """
        finalname = self.getPropertyValue('OutputWorkspace')
        N = len(files)
        import math
        if N>3: #9
            n = int(math.sqrt(N)) # grain size
        else:
            n = N
        for (i, f) in enumerate(files):
            self.__loaderName = 'Load'  # reset to generic load with each file
            wkspname = self.__processFile_withcache(f, useCaching, unfocusname, unfocusname_file)
            # accumulate into partial sum
            grain_start = i//n*n
            grain_end = min((i//n+1)*n, N)
            grain = files[grain_start:grain_end]
            partialsum_wkspname = self.__get_grp_ws_name(grain)
            self.__accumulate(
                wkspname, partialsum_wkspname, unfocusname, unfocusname_file)
            if i==grain_end-1:
                if useCaching and len(grain)>1:
                    # save partial cache
                    self.__saveSummedGroupToCache(grain)
                # accumulate into final sum
                self.__accumulate(partialsum_wkspname, finalname, unfocusname, unfocusname_file)
        return

    def __saveSummedGroupToCache(self, group, wkspname=None):
        cache_file = self.__get_grp_cache_fn(group)
        wksp = wkspname or self.__get_grp_ws_name(group)
        if not os.path.exists(cache_file):
            SaveNexusProcessed(InputWorkspace=wksp, Filename=cache_file)
        return



# Register algorithm with Mantid.
AlgorithmFactory.subscribe(AlignAndFocusPowderFromFiles)
