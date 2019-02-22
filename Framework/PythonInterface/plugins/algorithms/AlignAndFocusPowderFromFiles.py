# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd, AlgorithmFactory, DistributedDataProcessorAlgorithm, ITableWorkspaceProperty, \
    MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode
from mantid.kernel import Direction
from mantid.simpleapi import AlignAndFocusPowder, CompressEvents, ConvertDiffCal, ConvertUnits, CopyLogs, \
    CreateCacheFilename, DeleteWorkspace, DetermineChunking, Divide, EditInstrumentGeometry, FilterBadPulses, \
    LoadDiffCal, LoadNexusProcessed, PDDetermineCharacterizations, Plus, RemoveLogs, RenameWorkspace, SaveNexusProcessed
import os
import numpy as np

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

    def __getLinearizedFilenames(self, propertyName):
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
        useCal = (not self.getProperty('CalFileName').isDefault) or (not self.getProperty('CalFileName').isDefault)

        # input workspace is only needed to find a row in the characterizations table
        tempname = None
        if loadFile:
            if useCharac or useCal:
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

        # some bit of data has been loaded so use it to get the characterizations
        self.__setupCalibration(tempname)

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

        if loadFile and (useCharac or useCal):
            DeleteWorkspace(Workspace=tempname)

    def __getCacheName(self, wkspname, additional_props=None):
        """additional_props: list. additional properties to be hashed
        """
        cachedir = self.getProperty('CacheDir').value
        if len(cachedir) <= 0:
            return None

        # fix up the workspace name
        prefix = wkspname.replace('__', '')

        propman_properties = ['bank', 'd_min', 'd_max', 'tof_min', 'tof_max', 'wavelength_min', 'wavelength_max']
        alignandfocusargs = []
        # calculate general properties
        for name in PROPS_FOR_ALIGN:
            # skip these because this has been reworked to only worry about information in workspaces
            if name in (CAL_FILE, GROUP_FILE, CAL_WKSP, GRP_WKSP, MASK_WKSP):
                continue
            prop = self.getProperty(name)
            if name == 'PreserveEvents' or not prop.isDefault:
                value = prop.valueAsStr  # default representation for everything
                if name == 'AbsorptionWorkspace':  # TODO need better unique identifier for absorption workspace
                    value = str(mtd[self.__grpWksp].extractY().sum())
                alignandfocusargs.append('%s=%s' % (name, value))
        # special calculations for group and mask - calibration hasn't been customized yet
        if self.__calWksp:
            value = str(np.sum(mtd[self.__calWksp].column('difc')))  # less false collisions than the workspace name
            alignandfocusargs.append('%s=%s' % (CAL_WKSP, value))
        if self.__grpWksp:
            value = ','.join(mtd[self.__grpWksp].extractY().astype(int).astype(str).ravel())
            alignandfocusargs.append('%s=%s' % (GRP_WKSP, value))
        if self.__mskWksp:
            value = ','.join([str(item) for item in mtd[self.__mskWksp].getMaskedDetectors()])
            alignandfocusargs.append('%s=%s' % (MASK_WKSP, value))

        alignandfocusargs += additional_props or []
        return CreateCacheFilename(Prefix=prefix,
                                   PropertyManager=self.getProperty('ReductionProperties').valueAsStr,
                                   Properties=propman_properties,
                                   OtherProperties=alignandfocusargs,
                                   CacheDir=cachedir).OutputFilename

    def __getGroupCacheName(self, group):
        wsname = self.__getGroupWkspName(group)
        filenames_str = ','.join(group)
        newprop = 'files_to_sum={}'.format(filenames_str)
        return self.__getCacheName('summed_'+wsname, additional_props=[newprop])

    def __processFile(self, filename, file_prog_start, determineCharacterizations, createUnfocused):
        chunks = determineChunking(filename, self.chunkSize)
        numSteps = 6  # for better progress reporting - 6 steps per chunk
        if createUnfocused:
            numSteps = 7  # one more for accumulating the unfocused workspace
        self.log().information('Processing \'{}\' in {:d} chunks'.format(filename, len(chunks)))
        prog_per_chunk_step = self.prog_per_file * 1./(numSteps*float(len(chunks)))

        # create a unique name for the workspace
        wkspname = '__' + self.__wkspNameFromFile(filename)
        wkspname += '_f%d' % self._filenames.index(filename)  # add file number to be unique
        unfocusname = ''
        if createUnfocused:
            unfocusname = wkspname + '_unfocused'

        # check for a cachefilename
        cachefile = self.__getCacheName(self.__wkspNameFromFile(filename))
        if (not createUnfocused) and self.useCaching and os.path.exists(cachefile):
            if self.__loadCacheFile(cachefile, wkspname):
                return wkspname, ''

        unfocusname_chunk = ''
        canSkipLoadingLogs = False

        # inner loop is over chunks
        for (j, chunk) in enumerate(chunks):
            prog_start = file_prog_start + float(j) * float(numSteps - 1) * prog_per_chunk_step
            chunkname = '{}_c{:d}'.format(wkspname, j)
            if unfocusname :  # only create unfocus chunk if needed
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
            if j == 0:
                self.__setupCalibration(chunkname)

            # copy the necessary logs onto the workspace
            if canSkipLoadingLogs:
                CopyLogs(InputWorkspace=wkspname, OutputWorkspace=chunkname, MergeStrategy='WipeExisting')

            # get the underlying loader name if we used the generic one
            if self.__loaderName == 'Load':
                self.__loaderName = loader.getPropertyValue('LoaderName')
            # only LoadEventNexus can turn off loading logs, but FilterBadPulses requires them to be loaded from the file
            canSkipLoadingLogs = self.__loaderName == 'LoadEventNexus' and self.filterBadPulses <= 0.

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

            self.__accumulate(chunkname, wkspname, unfocusname_chunk, unfocusname, j == 0, removelogs=canSkipLoadingLogs)
        # end of inner loop

        # write out the cachefile for the main reduced data independent of whether
        # the unfocussed workspace was requested
        if self.useCaching and not os.path.exists(cachefile):
            SaveNexusProcessed(InputWorkspace=wkspname, Filename=cachefile)

        return wkspname, unfocusname

    def __compressEvents(self, wkspname):
        if self.kwargs['PreserveEvents'] and self.kwargs['CompressTolerance'] > 0.:
            CompressEvents(InputWorkspace=wkspname, OutputWorkspace=wkspname,
                           WallClockTolerance=self.kwargs['CompressWallClockTolerance'],
                           Tolerance=self.kwargs['CompressTolerance'],
                           StartTime=self.kwargs['CompressStartTime'])

    def __accumulate(self, chunkname, sumname, chunkunfocusname, sumuunfocusname, firstrun, removelogs=False):
        """accumulate newdata `wkspname` into sum `sumwkspname` and delete `wkspname`"""
        # the first call to accumulate to a specific target should be a simple rename
        self.log().debug('__accumulate({}, {}, {}, {}, {})'.format(chunkname, sumname, chunkunfocusname,
                                                                   sumuunfocusname, firstrun))

        if not firstrun:
            # if the sum workspace doesn't already exist, just rename
            if not mtd.doesExist(sumname):
                firstrun = True

        if firstrun:
            if chunkname != sumname:
                RenameWorkspace(InputWorkspace=chunkname, OutputWorkspace=sumname)
            if chunkunfocusname and chunkunfocusname != sumuunfocusname:
                RenameWorkspace(InputWorkspace=chunkunfocusname, OutputWorkspace=sumuunfocusname)
        else:
            if removelogs:
                RemoveLogs(Workspace=chunkname)  # accumulation has them already
            Plus(LHSWorkspace=sumname, RHSWorkspace=chunkname, OutputWorkspace=sumname,
                 ClearRHSWorkspace=self.kwargs['PreserveEvents'])
            DeleteWorkspace(Workspace=chunkname)
            self.__compressEvents(sumname)  # could be smarter about when to run

            if chunkunfocusname and chunkunfocusname != sumuunfocusname:
                if removelogs:
                    RemoveLogs(Workspace=chunkunfocusname)  # accumulation has them already
                Plus(LHSWorkspace=sumuunfocusname, RHSWorkspace=chunkunfocusname, OutputWorkspace=sumuunfocusname,
                     ClearRHSWorkspace=self.kwargs['PreserveEvents'])
                DeleteWorkspace(Workspace=chunkunfocusname)
                self.__compressEvents(sumuunfocusname)  # could be smarter about when to run

    def __setupCalibration(self, wksp):
        '''Convert whatever calibration/grouping/masking into workspaces that will be passed down'''
        if self.haveDeterminedCalibration:
            return  # nothing to do
        self.haveDeterminedCalibration = True

        self.__calWksp = ''
        self.__grpWksp = ''
        self.__mskWksp = ''
        # first see if the workspaces have been specified
        # check that the canonical names don't already exist as a backup
        if not self.getProperty('CalibrationWorkspace').isDefault:
            self.__calWksp = self.getPropertyValue('CalibrationWorkspace')
        elif not self.getProperty('OffsetsWorkspace').isDefault:
            self.__calWksp = self.getPropertyValue('OffsetsWorkspace') + '_cal'
            ConvertDiffCal(OffsetsWorkspace=self.getPropertyValue('OffsetsWorkspace'),
                           OutputWorkspace=self.instr + '_cal')
            self.setProperty('CalibrationWorkspace', self.__calWksp)
        elif mtd.doesExist(self.instr + '_cal'):
            self.__calWksp = self.instr + '_cal'

        if not self.getProperty('GroupingWorkspace').isDefault:
            self.__grpWksp = self.getPropertyValue('GroupingWorkspace')
        elif mtd.doesExist(self.instr + '_group'):
            self.__grpWksp = self.instr + '_group'

        if not self.getProperty('MaskWorkspace').isDefault:
            self.__mskWksp = self.getPropertyValue('MaskWorkspace')
        elif mtd.doesExist(self.instr + '_mask'):
            self.__mskWksp = self.instr + '_mask'

        # check that anything was specified
        if self.getProperty('CalFileName').isDefault and self.getProperty('GroupFilename').isDefault:
            return

        # decide what to load
        loadCalibration = not bool(self.__calWksp)
        loadGrouping = not bool(self.__grpWksp)
        loadMask = not bool(self.__mskWksp)

        # load and update
        if loadCalibration or loadGrouping or loadMask:
            if not wksp:
                raise RuntimeError('Trying to load calibration without a donor workspace')
            LoadDiffCal(InputWorkspace=wksp,
                        Filename=self.getPropertyValue('CalFileName'),
                        GroupFilename=self.getPropertyValue('GroupFilename'),
                        MakeCalWorkspace=loadCalibration,
                        MakeGroupingWorkspace=loadGrouping,
                        MakeMaskWorkspace=loadMask,
                        WorkspaceName=self.instr)
        if loadCalibration:
            self.__calWksp = self.instr + '_cal'
            self.setPropertyValue('CalibrationWorkspace', self.instr + '_cal')
        if loadGrouping:
            self.__grpWksp = self.instr + '_group'
            self.setPropertyValue('GroupingWorkspace', self.instr + '_group')
        if loadMask:
            self.__mskWksp = self.instr + '_mask'
            self.setPropertyValue('MaskWorkspace', self.instr + '_mask')

    def PyExec(self):
        self._filenames = sorted(self.__getLinearizedFilenames('Filename'))
        self.instr = os.path.basename(self._filenames[0]).split('_')[0]  # wrong for instrument's with `_` in the name
        self.haveDeterminedCalibration = False  # setup variables for loading calibration into workspaces
        self.__loaderName = 'Load'   # set the loader to be generic on first load
        self.filterBadPulses = self.getProperty('FilterBadPulses').value
        self.chunkSize = self.getProperty('MaxChunkSize').value
        self.absorption = self.getProperty('AbsorptionWorkspace').value
        self.charac = self.getProperty('Characterizations').value
        self.useCaching = len(self.getProperty('CacheDir').value) > 0
        finalname = self.getPropertyValue('OutputWorkspace')

        # accumulate the unfocused workspace if it was requested
        # empty string means it is not used
        finalunfocusname = self.getPropertyValue('UnfocussedWorkspace')

        if self.useCaching:
            # unfocus check only matters if caching is requested
            if finalunfocusname != '':
                self.useCaching = False
                self.log().warning('CacheDir is specified with "UnfocussedWorkspace" - reading cache files disabled')
        else:
            self.log().warning('CacheDir is not specified - functionality disabled')

        assert len(self._filenames), "No files specified"
        self.prog_per_file = 1./float(len(self._filenames))  # for better progress reporting

        # these are also passed into the child-algorithms
        self.kwargs = self.__getAlignAndFocusArgs()

        # initialization for caching mechanism
        if self.useCaching:
            filename = self._filenames[0]
            wkspname = os.path.split(filename)[-1].split('.')[0]
            self.__determineCharacterizations(filename, wkspname)
            del filename
            del wkspname

        # find caches and sum them. return files without cache - cannot be used with UnfocussedWorkspace
        if self.useCaching:
            notcached = self.__findAndLoadCachefiles(self._filenames, finalname, firstTime=True)
        else:
            notcached = self._filenames[:]  # make a copy
        self.log().notice('Files to process: {}'.format(notcached,))

        # process not-cached files
        if notcached:
            self.__processFiles(notcached, finalname, finalunfocusname, len(notcached) != len(self._filenames))

        # create cache of everything summed together
        if self.useCaching and len(self._filenames) > 1:
            self.__saveSummedGroupToCache(self._filenames, wkspname=finalname)

        # with more than one chunk or file the integrated proton charge is
        # generically wrong
        mtd[finalname].run().integrateProtonCharge()

        # set the output workspace
        self.setProperty('OutputWorkspace', mtd[finalname])
        if finalunfocusname:
            self.setProperty('UnfocussedWorkspace', mtd[finalunfocusname])

    def __loadCacheFile(self, filename, wkspname):
        '''@returns True if a file was loaded'''
        if os.path.exists(filename):
            self.log().notice('Loading cache from {}'.format(filename))
        else:
            return False

        LoadNexusProcessed(Filename=filename, OutputWorkspace=wkspname)
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

        return True

    def __findAndLoadCachefiles(self, filenames, finalname, firstTime):
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
                fileSubset = filenames[start:end+1]
                summed_cache_file = self.__getGroupCacheName(fileSubset)
                wkspname = self.__getGroupWkspName(fileSubset)
                if self.__loadCacheFile(summed_cache_file, wkspname):
                    self.__accumulate(wkspname, finalname, '', '', firstTime)
                    found = True
                    break
            if found:
                break
            continue
        if not found:
            return filenames
        remained = filenames[:start] + filenames[end+1:]
        if remained:
            return self.__findAndLoadCachefiles(remained, finalname, False)
        return []

    def __wkspNameFromFile(self, filename):
        name = os.path.basename(filename).split('.')[0]
        name = name.replace('_event', '')  # for old SNS files
        return name

    def __getGroupWkspName(self, group):
        group = [self.__wkspNameFromFile(filename) for filename in group]
        group = [name.replace('_event', '') for name in group]
        if len(group) == 1:
            return group[0]
        else:
            return '{}-{}'.format(group[0], group[-1])

    def __processFiles(self, files, finalname, finalunfocusname, hasAccumulated):
        """process given files, separate them to "grains". Sum each grain, and add the grain sum to final sum
        """
        numberFilesToProcess = len(files)
        import math
        if numberFilesToProcess > 3:
            grain_size = int(math.sqrt(numberFilesToProcess))  # grain size
        else:
            grain_size = numberFilesToProcess
        for (i, filename) in enumerate(files):
            self.__loaderName = 'Load'  # reset to generic load with each file
            wkspname, unfocusname = self.__processFile(filename, self.prog_per_file * float(i), not self.useCaching,
                                                       bool(finalunfocusname))
            # accumulate into partial sum
            grain_start = i//grain_size*grain_size
            grain_end = min((i//grain_size+1)*grain_size, numberFilesToProcess)
            grain = files[grain_start:grain_end]
            partialsum_wkspname = '__' + self.__getGroupWkspName(grain)
            if finalunfocusname:
                partialsum_unfocusname = partialsum_wkspname + '_unfocused'
            else:
                partialsum_unfocusname = ''
            self.__accumulate(wkspname, partialsum_wkspname, unfocusname, partialsum_unfocusname, (not hasAccumulated) or i == 0)
            hasAccumulated = True
            if i == grain_end - 1:
                if self.useCaching and len(grain) > 1:
                    # save partial cache
                    self.__saveSummedGroupToCache(grain, partialsum_wkspname)
                # accumulate into final sum
                self.__accumulate(partialsum_wkspname, finalname, partialsum_unfocusname, finalunfocusname, (not hasAccumulated) or i==0)

    def __saveSummedGroupToCache(self, group, wkspname):
        cache_file = self.__getGroupCacheName(group)
        if not os.path.exists(cache_file):
            SaveNexusProcessed(InputWorkspace=wkspname, Filename=cache_file)
        return


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(AlignAndFocusPowderFromFiles)
