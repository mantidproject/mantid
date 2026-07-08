# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    mtd,
    AlgorithmFactory,
    DataProcessorAlgorithm,
    ITableWorkspaceProperty,
    MatrixWorkspaceProperty,
    MultipleFileProperty,
    PropertyMode,
)
from mantid.kernel import Direction, PropertyManagerDataService, StringArrayProperty
from mantid.simpleapi import (
    AlignAndFocusPowder,
    AlignAndFocusPowderSlim,
    CompressEvents,
    ConvertDiffCal,
    ConvertUnits,
    CopyLogs,
    CopySample,
    CreateCacheFilename,
    DeleteWorkspace,
    DetermineChunking,
    Divide,
    EditInstrumentGeometry,
    FilterBadPulses,
    LoadDiffCal,
    Load,
    LoadIDFFromNexus,
    LoadNexusProcessed,
    PDDetermineCharacterizations,
    Plus,
    RebinToWorkspace,
    RemoveLogs,
    RenameWorkspace,
    SaveNexusProcessed,
)
import os
import numpy as np
import h5py
import math

EXTENSIONS_NXS = ["_event.nxs", ".nxs.h5"]
PROPS_FOR_INSTR = ["PrimaryFlightPath", "SpectrumIDs", "L2", "Polar", "Azimuthal"]
CAL_FILE, GROUP_FILE = "CalFileName", "GroupFilename"
CAL_WKSP, GRP_WKSP, MASK_WKSP = "CalibrationWorkspace", "GroupingWorkspace", "MaskWorkspace"
# AlignAndFocusPowder only uses the ranges
PROPS_IN_PD_CHARACTER = ["DMin", "DMax", "DeltaRagged", "TMin", "TMax", "CropWavelengthMin", "CropWavelengthMax"]
PROPS_FOR_ALIGN = [
    CAL_FILE,
    GROUP_FILE,
    GRP_WKSP,
    CAL_WKSP,
    "OffsetsWorkspace",
    MASK_WKSP,
    "MaskBinTable",
    "Params",
    "ResampleX",
    "Dspacing",
    "PreserveEvents",
    "RemovePromptPulseWidth",
    "ResonanceFilterUnits",
    "ResonanceFilterLowerLimits",
    "ResonanceFilterUpperLimits",
    "CompressTolerance",
    "CompressWallClockTolerance",
    "CompressStartTime",
    "CompressBinningMode",
    "LorentzCorrection",
    "ReductionProperties",
]
PROPS_FOR_ALIGN.extend(PROPS_IN_PD_CHARACTER)
PROPS_FOR_ALIGN.extend(PROPS_FOR_INSTR)
PROPS_FOR_PD_CHARACTER = ["FrequencyLogNames", "WaveLengthLogNames"]

# Properties mandatory in AAFPSlim but not in FromFiles
_SLIM_REQUIRED_PROPS = ["PrimaryFlightPath", "L2", "Polar"]
# Properties that block use of AAFPSlim entirely
_SLIM_BLOCKING_PROPS = [
    "AbsorptionWorkspace",
    "OffsetsWorkspace",
    "MaskBinTable",
    "ResampleX",
    "RemovePromptPulseWidth",
    "ResonanceFilterUnits",
    "ResonanceFilterLowerLimits",
    "ResonanceFilterUpperLimits",
    "CompressWallClockTolerance",
    "CompressStartTime",
    "LorentzCorrection",
    "TMin",
    "TMax",
    "CropWavelengthMin",
    "CropWavelengthMax",
    "UnfocussedWorkspace",
]

# Bytes per event (neutron event record size) used for MaxChunkSize → ReadSizeFromDisk conversion
_BYTES_PER_EVENT = 24


def determineCompression(filename, compression, chunking, absorption):
    if compression == 0.0 or chunking > 0.0 or absorption:
        return False
    sizeGiB = os.path.getsize(filename) / 1024.0 / 1024.0 / 1024.0
    if sizeGiB > compression:
        return True


def determineChunking(filename, chunkSize):
    # chunkSize=0 signifies that the user wants to read the whole file
    if chunkSize == 0.0:
        return [{}]

    # "small" files just get read in
    sizeGiB = os.path.getsize(filename) / 1024.0 / 1024.0 / 1024.0
    if 6.0 * sizeGiB < chunkSize:
        return [{}]

    chunks = DetermineChunking(Filename=filename, MaxChunkSize=chunkSize, OutputWorkspace="chunks")

    strategy = []
    for row in chunks:
        strategy.append(row)

    # For table with no rows
    if len(strategy) == 0:
        strategy.append({})

    # delete chunks workspace
    chunks = str(chunks)  # release the handle to the workspace object
    DeleteWorkspace(Workspace="chunks")

    return strategy


def uniqueDescription(name, wksp):
    wksp = str(wksp)
    if name == "AbsorptionWorkspace":
        sample = mtd[wksp].sample()
        materialname = sample.getMaterial().name()
        shapeXML = sample.getShape().getShapeXML()
        density = str(sample.getMaterial().numberDensityEffective)
        wavelength = mtd[wksp].readX(0)
        wavelength = "{} to {} with {} bins".format(wavelength[0], wavelength[-1], mtd[wksp].readY(0).size)
        value = ";".join((materialname, density, shapeXML, wavelength))
    elif name == CAL_WKSP:
        value = str(np.sum(mtd[wksp].column("difc")))  # less false collisions than the workspace name
    elif name == GRP_WKSP:
        value = ",".join(mtd[wksp].extractY().astype(int).astype(str).ravel())
    elif name == MASK_WKSP:
        value = ",".join([str(item) for item in mtd[wksp].getMaskedDetectors()])
    else:
        raise RuntimeError('Do not know how to create unique description for Property "{}"'.format(name))

    return "{}={}".format(name, value)


class AlignAndFocusPowderFromFiles(DataProcessorAlgorithm):
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
        self.declareProperty(MultipleFileProperty(name="Filename", extensions=EXTENSIONS_NXS), "Files to combine in reduction")
        self.declareProperty("MaxChunkSize", 0.0, "Specify maximum Gbytes of file to read in one chunk.  Default is whole file.")
        self.declareProperty("MinSizeCompressOnLoad", 0.0, "Specify the file size in GB to use compression")
        self.declareProperty("FilterBadPulses", 0.0, doc="Filter out events measured while proton charge is more than 5% below average")

        self.declareProperty(
            MatrixWorkspaceProperty("AbsorptionWorkspace", "", Direction.Input, PropertyMode.Optional),
            doc="Divide data by this Pixel-by-pixel workspace",
        )

        self.copyProperties("CreateCacheFilename", "CacheDir")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Combined output workspace")
        self.copyProperties("AlignAndFocusPowder", ["UnfocussedWorkspace"])

        self.declareProperty(
            ITableWorkspaceProperty("Characterizations", "", Direction.Input, PropertyMode.Optional), "Characterizations table"
        )
        self.declareProperty(
            StringArrayProperty(name="LogAllowList"),
            "If specified, only these logs will be loaded from the file. This is passed to LoadEventNexus",
        )
        self.declareProperty(
            StringArrayProperty(name="LogBlockList"),
            "If specified, these logs will not be loaded from the file. This is passed to LoadEventNexus",
        )

        self.declareProperty(
            "AllowSlimProcess", True, doc="Allow using the AlignAndFocusPowderSlim algorithm if the file and properties are compatible"
        )

        self.copyProperties("AlignAndFocusPowder", PROPS_FOR_ALIGN)
        self.copyProperties("PDDetermineCharacterizations", PROPS_FOR_PD_CHARACTER)

    def validateInputs(self):
        errors = dict()

        unfocusname = self.getPropertyValue("UnfocussedWorkspace")
        if len(unfocusname) > 0:
            finalname = self.getPropertyValue("OutputWorkspace")
            if unfocusname == finalname:
                errors["OutputWorkspace"] = "Cannot be the same as UnfocussedWorkspace"
                errors["UnfocussedWorkspace"] = "Cannot be the same as OutputWorkspace"
        if (not self.getProperty("LogAllowList").isDefault) and (not self.getProperty("LogBlockList").isDefault):
            errors["LogAllowList"] = "Cannot specify with LogBlockList"
            errors["LogBlockList"] = "Cannot specify with LogAllowList"

        return errors

    def __getLinearizedFilenames(self, propertyName):
        runnumbers = self.getProperty(propertyName).value
        linearizedRuns = []
        for item in runnumbers:
            if isinstance(item, list):
                linearizedRuns.extend(item)
            else:
                linearizedRuns.append(item)
        return linearizedRuns

    def __createLoader(self, filename, wkspname, progstart=None, progstop=None, skipLoadingLogs=False, filterBadPulses=0, **kwargs):
        # load a chunk - this is a bit crazy long because we need to get an output property from `Load` when it
        # is run and the algorithm history doesn't exist until the parent algorithm (this) has finished
        # the kwargs are extra things to be supplied to the loader
        if progstart is None or progstop is None:
            loader = self.createChildAlgorithm(self.__loaderName)
        else:
            loader = self.createChildAlgorithm(self.__loaderName, startProgress=progstart, endProgress=progstop)
        loader.setAlwaysStoreInADS(True)
        loader.setLogging(True)
        loader.initialize()
        loader.setPropertyValue("Filename", filename)
        loader.setPropertyValue("OutputWorkspace", wkspname)

        if self.do_compression:
            self.kwargs = self.__getAlignAndFocusArgs()
            loader.setProperty("CompressTolerance", self.kwargs["CompressTolerance"])
            loader.setPropertyValue("CompressBinningMode", self.kwargs["CompressBinningMode"])

        if skipLoadingLogs:
            if self.__loaderName != "LoadEventNexus":
                raise RuntimeError("Cannot set LoadLogs=False in {}".format(self.__loaderName))
            loader.setProperty("LoadLogs", False)
        elif "AllowList" not in kwargs:
            # this only works for LoadEventNexus
            # i.e. LoadNexusProcessed doesn't have these properties
            try:
                if not self.getProperty("LogAllowList").isDefault:
                    loader.setProperty("AllowList", self.getProperty("LogAllowList").value)
                if not self.getProperty("LogBlockList").isDefault:
                    loader.setProperty("BlockList", self.getProperty("LogBlockList").value)
            except RuntimeError:
                pass  # let it drop on the floor

        # don't automatically bin the data to start
        try:
            loader.setProperty("NumberOfBins", 1)
        except RuntimeError:
            pass  # let it drop on the floor

        try:
            if filterBadPulses > 0:
                loader.setProperty("FilterBadPulsesLowerCutoff", filterBadPulses)
        except RuntimeError:
            # only works for LoadEventNexus, instead run FilterBadPulses separately
            pass

        for key, value in kwargs.items():
            if isinstance(value, str):
                loader.setPropertyValue(key, value)
            else:
                loader.setProperty(key, value)
        return loader

    def __getAlignAndFocusArgs(self):
        # always put these in since they are loaded in __setupCalibration
        # this requires that function to be called before this one
        args = {CAL_WKSP: self.__calWksp, GRP_WKSP: self.__grpWksp, MASK_WKSP: self.__mskWksp}

        for name in PROPS_FOR_ALIGN + PROPS_IN_PD_CHARACTER:
            prop = self.getProperty(name)
            name_list = ["PreserveEvents", "CompressTolerance", "CompressWallClockTolerance", "CompressStartTime", "CompressBinningMode"]

            if name in name_list or not prop.isDefault:
                if "Workspace" in name:
                    args[name] = prop.valueAsStr
                elif name in [CAL_FILE, GROUP_FILE]:
                    pass  # these were loaded into workspaces already
                else:
                    args[name] = prop.value
        return args

    def __isCharacterizationsNeeded(self):
        """Determine if the characterization file is needed by checking if
        all the properties it would set are already specified"""
        if not self.charac:
            return False

        for name in PROPS_IN_PD_CHARACTER:
            if self.getProperty(name).isDefault:
                return True

        return False

    def __needToLoadCal(self):
        if (not self.getProperty(CAL_FILE).isDefault) or (not self.getProperty(GROUP_FILE).isDefault):
            return not bool(self.__calWksp and self.__grpWksp and self.__mskWksp)
        return False

    def __determineCharacterizations(self, filename, wkspname):
        useCharTable = self.__isCharacterizationsNeeded()
        needToLoadCal = self.__needToLoadCal()

        # something needs to use the workspace and it needs to not already be in memory
        loadFile = (useCharTable or needToLoadCal) and (not mtd.doesExist(wkspname))

        # input workspace is only needed to find a row in the characterizations table
        tempname = None
        if loadFile:
            if useCharTable or needToLoadCal:
                tempname = "__%s_temp" % wkspname
                # set the loader for this file
                try:
                    # put together list of logs that matter, all the others will be skipped
                    allowedLogs = []
                    allowedLogs.extend(self.getProperty("FrequencyLogNames").value)
                    allowedLogs.extend(self.getProperty("WaveLengthLogNames").value)
                    # MetaDataOnly=True is only supported by LoadEventNexus
                    loader = self.__createLoader(filename, tempname, MetaDataOnly=True, AllowList=allowedLogs)
                    loader.execute()

                    # get the underlying loader name if we used the generic one
                    if self.__loaderName == "Load":
                        self.__loaderName = loader.getPropertyValue("LoaderName")
                except RuntimeError:
                    # give up and load the whole file - this can be expensive
                    Load(OutputWorkspace=tempname, Filename=filename)
        else:
            tempname = wkspname  # assume it is already loaded

        # some bit of data has been loaded so use it to get the characterizations
        self.__setupCalibration(tempname)

        # put together argument list for determining characterizations
        args = dict(ReductionProperties=self.getProperty("ReductionProperties").valueAsStr)
        for name in PROPS_FOR_PD_CHARACTER:
            prop = self.getProperty(name)
            if not prop.isDefault:
                args[name] = prop.value
        if tempname is not None:
            args["InputWorkspace"] = tempname
        if useCharTable:
            args["Characterizations"] = self.charac

        if useCharTable:
            PDDetermineCharacterizations(**args)

        if loadFile and (useCharTable or needToLoadCal):
            DeleteWorkspace(Workspace=tempname)

    def __getCacheName(self, wkspname, additional_props=None):
        """additional_props: list. additional properties to be hashed"""
        cachedir = self.getProperty("CacheDir").value
        if len(cachedir) <= 0:
            return None

        # fix up the workspace name
        prefix = wkspname.replace("__", "")

        propman_properties = ["bank", "d_min", "d_max", "tof_min", "tof_max", "wavelength_min", "wavelength_max"]
        alignandfocusargs = []

        # calculate general properties
        for name in PROPS_FOR_ALIGN + PROPS_IN_PD_CHARACTER:
            # skip these because this has been reworked to only worry about information in workspaces
            if name in (CAL_FILE, GROUP_FILE, CAL_WKSP, GRP_WKSP, MASK_WKSP):
                continue
            prop = self.getProperty(name)
            if name == "PreserveEvents" or not prop.isDefault:
                value = prop.valueAsStr  # default representation for everything
                alignandfocusargs.append("%s=%s" % (name, value))

        # special calculations for workspaces
        if self.absorption:
            alignandfocusargs.append(uniqueDescription("AbsorptionWorkspace", self.absorption))
        if self.__calWksp:
            alignandfocusargs.append(uniqueDescription(CAL_WKSP, self.__calWksp))
        if self.__grpWksp:
            alignandfocusargs.append(uniqueDescription(GRP_WKSP, self.__grpWksp))
        if self.__mskWksp:
            alignandfocusargs.append(uniqueDescription(MASK_WKSP, self.__mskWksp))

        alignandfocusargs += additional_props or []
        reductionPropertiesName = self.getProperty("ReductionProperties").valueAsStr
        if reductionPropertiesName not in PropertyManagerDataService:
            reductionPropertiesName = ""  # do not specify non-existant manager

        return CreateCacheFilename(
            Prefix=prefix,
            PropertyManager=reductionPropertiesName,
            Properties=propman_properties,
            OtherProperties=alignandfocusargs,
            CacheDir=cachedir,
        ).OutputFilename

    def __getGroupCacheName(self, group):
        wsname = self.__getGroupWkspName(group)
        filenames_str = ",".join(group)
        newprop = "files_to_sum={}".format(filenames_str)
        return self.__getCacheName("summed_" + wsname, additional_props=[newprop])

    def __processFile(self, filename, file_prog_start, determineCharacterizations, createUnfocused):  # noqa: C901
        # create a unique name for the workspace
        wkspname = "__" + self.__wkspNameFromFile(filename)
        wkspname += "_f%d" % self._filenames.index(filename)  # add file number to be unique
        unfocusname = ""
        if createUnfocused:
            unfocusname = wkspname + "_unfocused"

        # check for a cachefilename
        cachefile = self.__getCacheName(self.__wkspNameFromFile(filename))
        self.log().information('looking for cachefile "{}"'.format(cachefile))
        if (not createUnfocused) and self.useCaching and os.path.exists(cachefile):
            try:
                if self.__loadCacheFile(cachefile, wkspname):
                    return wkspname, ""
            except RuntimeError as e:
                # log as a warning and carry on as though the cache file didn't exist
                self.log().warning('Failed to load cache file "{}": {}'.format(cachefile, e))
        else:
            self.log().information("not using cache")

        chunks = determineChunking(filename, self.chunkSize)
        numSteps = 6  # for better progress reporting - 6 steps per chunk
        if createUnfocused:
            numSteps = 7  # one more for accumulating the unfocused workspace
        self.log().information("Processing '{}' in {:d} chunks".format(filename, len(chunks)))
        prog_per_chunk_step = self.prog_per_file * 1.0 / (numSteps * float(len(chunks)))

        unfocusname_chunk = ""
        canSkipLoadingLogs = False

        # inner loop is over chunks
        haveAccumulationForFile = False
        for j, chunk in enumerate(chunks):
            prog_start = file_prog_start + float(j) * float(numSteps - 1) * prog_per_chunk_step

            # if reading all at once, put the data into the final name directly
            if len(chunks) == 1:
                chunkname = wkspname
                unfocusname_chunk = unfocusname
            else:
                chunkname = "{}_c{:d}".format(wkspname, j)
                if unfocusname:  # only create unfocus chunk if needed
                    unfocusname_chunk = "{}_c{:d}".format(unfocusname, j)

            # load a chunk - this is a bit crazy long because we need to get an output property from `Load` when it
            # is run and the algorithm history doesn't exist until the parent algorithm (this) has finished
            loader = self.__createLoader(
                filename,
                chunkname,
                skipLoadingLogs=(len(chunks) > 1 and canSkipLoadingLogs and haveAccumulationForFile),
                progstart=prog_start,
                progstop=prog_start + prog_per_chunk_step,
                filterBadPulses=self.filterBadPulses,
                **chunk,
            )
            loader.execute()
            if j == 0:
                self.__setupCalibration(chunkname)

            # copy the necessary logs onto the workspace
            if len(chunks) > 1 and canSkipLoadingLogs and haveAccumulationForFile:
                CopyLogs(InputWorkspace=wkspname, OutputWorkspace=chunkname, MergeStrategy="WipeExisting")
                # re-load instrument so detector positions that depend on logs get initialized
                try:
                    LoadIDFFromNexus(Workspace=chunkname, Filename=filename, InstrumentParentPath="/entry")
                except RuntimeError as e:
                    self.log().warning('Reloading instrument using "LoadIDFFromNexus" failed: {}'.format(e))

            # get the underlying loader name if we used the generic one
            if self.__loaderName == "Load":
                self.__loaderName = loader.getPropertyValue("LoaderName")
            # only LoadEventNexus can turn off loading logs, but FilterBadPulses
            # requires them to be loaded from the file
            canSkipLoadingLogs = self.__loaderName == "LoadEventNexus" and self.filterBadPulses <= 0.0 and haveAccumulationForFile

            if determineCharacterizations and j == 0:
                self.__determineCharacterizations(filename, chunkname)  # updates instance variable
                determineCharacterizations = False

            if self.__loaderName == "LoadEventNexus" and mtd[chunkname].getNumberEvents() == 0:
                self.log().notice("Chunk {} of {} contained no events. Skipping to next chunk.".format(j + 1, len(chunks)))
                continue

            prog_start += prog_per_chunk_step

            # if LoadEventNexus was used then FilterBadPulses happen during loading
            if self.filterBadPulses > 0.0 and self.__loaderName != "LoadEventNexus":
                FilterBadPulses(
                    InputWorkspace=chunkname,
                    OutputWorkspace=chunkname,
                    LowerCutoff=self.filterBadPulses,
                    startProgress=prog_start,
                    endProgress=prog_start + prog_per_chunk_step,
                )
                if mtd[chunkname].getNumberEvents() == 0:
                    msg = "FilterBadPulses removed all events from "
                    if len(chunks) == 1:
                        raise RuntimeError(msg + filename)
                    else:
                        raise RuntimeError(msg + "chunk {} of {} in {}".format(j, len(chunks), filename))

            prog_start += prog_per_chunk_step

            # absorption correction workspace
            if self.absorption is not None and len(str(self.absorption)) > 0:
                ConvertUnits(InputWorkspace=chunkname, OutputWorkspace=chunkname, Target="Wavelength", EMode="Elastic")
                # rebin the absorption correction to match the binning of the inputs if in histogram mode
                # EventWorkspace will compare the wavelength of each individual event
                absWksp = self.absorption
                if mtd[chunkname].id() != "EventWorkspace":
                    absWksp = "__absWkspRebinned"
                    RebinToWorkspace(WorkspaceToRebin=self.absorption, WorkspaceToMatch=chunkname, OutputWorkspace=absWksp)
                Divide(
                    LHSWorkspace=chunkname,
                    RHSWorkspace=absWksp,
                    OutputWorkspace=chunkname,
                    startProgress=prog_start,
                    endProgress=prog_start + prog_per_chunk_step,
                )
                if absWksp != self.absorption:  # clean up
                    DeleteWorkspace(Workspace=absWksp)
                ConvertUnits(InputWorkspace=chunkname, OutputWorkspace=chunkname, Target="TOF", EMode="Elastic")
            prog_start += prog_per_chunk_step

            if self.kwargs is None:
                raise RuntimeError('Somehow arguments for "AlignAndFocusPowder" aren\'t set')

            AlignAndFocusPowder(
                InputWorkspace=chunkname,
                OutputWorkspace=chunkname,
                UnfocussedWorkspace=unfocusname_chunk,
                startProgress=prog_start,
                endProgress=prog_start + 2.0 * prog_per_chunk_step,
                **self.kwargs,
            )
            prog_start += 2.0 * prog_per_chunk_step  # AlignAndFocusPowder counts for two steps

            self.__accumulate(
                chunkname, wkspname, unfocusname_chunk, unfocusname, not haveAccumulationForFile, removelogs=canSkipLoadingLogs
            )

            haveAccumulationForFile = True
        # end of inner loop
        if not mtd.doesExist(wkspname):
            raise RuntimeError('Failed to process any data from file "{}"'.format(filename))

        # copy the sample object from the absorption workspace
        if self.absorption is not None and len(str(self.absorption)) > 0:
            CopySample(InputWorkspace=self.absorption, OutputWorkspace=wkspname, CopyEnvironment=False)

        # write out the cachefile for the main reduced data independent of whether
        # the unfocussed workspace was requested
        if self.useCaching and not os.path.exists(cachefile):
            self.log().information('Saving data to cachefile "{}"'.format(cachefile))
            SaveNexusProcessed(InputWorkspace=wkspname, Filename=cachefile)

        return wkspname, unfocusname

    def __compressEvents(self, wkspname):
        if self.kwargs["PreserveEvents"] and self.kwargs["CompressTolerance"] != 0.0:
            CompressEvents(
                InputWorkspace=wkspname,
                OutputWorkspace=wkspname,
                WallClockTolerance=self.kwargs["CompressWallClockTolerance"],
                Tolerance=self.kwargs["CompressTolerance"],
                StartTime=self.kwargs["CompressStartTime"],
                BinningMode=self.kwargs["CompressBinningMode"],
            )

    def __accumulate(self, chunkname, sumname, chunkunfocusname, sumuunfocusname, firstrun, removelogs=False):
        """accumulate newdata `wkspname` into sum `sumwkspname` and delete `wkspname`"""
        # the first call to accumulate to a specific target should be a simple rename
        self.log().debug("__accumulate({}, {}, {}, {}, {})".format(chunkname, sumname, chunkunfocusname, sumuunfocusname, firstrun))
        if chunkname == sumname:
            return  # there is nothing to be done

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
            RebinToWorkspace(WorkspaceToRebin=chunkname, WorkspaceToMatch=sumname, OutputWorkspace=chunkname)
            Plus(LHSWorkspace=sumname, RHSWorkspace=chunkname, OutputWorkspace=sumname, ClearRHSWorkspace=self.kwargs["PreserveEvents"])
            DeleteWorkspace(Workspace=chunkname)
            self.__compressEvents(sumname)  # could be smarter about when to run

            if chunkunfocusname and chunkunfocusname != sumuunfocusname:
                if removelogs:
                    RemoveLogs(Workspace=chunkunfocusname)  # accumulation has them already
                Plus(
                    LHSWorkspace=sumuunfocusname,
                    RHSWorkspace=chunkunfocusname,
                    OutputWorkspace=sumuunfocusname,
                    ClearRHSWorkspace=self.kwargs["PreserveEvents"],
                )
                DeleteWorkspace(Workspace=chunkunfocusname)
                self.__compressEvents(sumuunfocusname)  # could be smarter about when to run

    def __setupCalibration(self, wksp):
        """Convert whatever calibration/grouping/masking into workspaces that will be passed down"""
        if self.haveDeterminedCalibration:
            return  # nothing to do
        self.haveDeterminedCalibration = True

        # first see if the workspaces have been specified
        # check that the canonical names don't already exist as a backup
        if not self.getProperty("CalibrationWorkspace").isDefault:
            self.__calWksp = self.getPropertyValue("CalibrationWorkspace")
        elif not self.getProperty("OffsetsWorkspace").isDefault:
            self.__calWksp = self.getPropertyValue("OffsetsWorkspace") + "_cal"
            ConvertDiffCal(OffsetsWorkspace=self.getPropertyValue("OffsetsWorkspace"), OutputWorkspace=self.instr + "_cal")
            self.setProperty("CalibrationWorkspace", self.__calWksp)
        elif mtd.doesExist(self.instr + "_cal"):
            self.__calWksp = self.instr + "_cal"

        if not self.getProperty("GroupingWorkspace").isDefault:
            self.__grpWksp = self.getPropertyValue("GroupingWorkspace")
        elif mtd.doesExist(self.instr + "_group"):
            self.__grpWksp = self.instr + "_group"

        if not self.getProperty("MaskWorkspace").isDefault:
            self.__mskWksp = self.getPropertyValue("MaskWorkspace")
        elif mtd.doesExist(self.instr + "_mask"):
            self.__mskWksp = self.instr + "_mask"

        # check that anything was specified
        if self.getProperty(CAL_FILE).isDefault and self.getProperty(GROUP_FILE).isDefault:
            self.kwargs = self.__getAlignAndFocusArgs()
            return

        # decide what to load
        loadCalibration = not bool(self.__calWksp)
        loadGrouping = not bool(self.__grpWksp)
        loadMask = not bool(self.__mskWksp)

        # load and update
        if loadCalibration or loadGrouping or loadMask:
            if not wksp:
                raise RuntimeError("Trying to load calibration without a donor workspace")

            tmin_val = self.__getAlignAndFocusArgs().get("TMin", 0.0)

            calfile = self.getPropertyValue(CAL_FILE)
            has_calfile = bool(calfile)
            if not calfile:
                # LoadDiffCal needs a filename; fall back to grouping file
                calfile = self.getPropertyValue(GROUP_FILE)

            diffcal_kwargs = dict(
                InputWorkspace=wksp,
                Filename=calfile,
                GroupFilename=self.getPropertyValue(GROUP_FILE),
                # Grouping-only HDF files (no DIFC columns) must not request a cal workspace;
                # LoadDiffCal also requires InputWorkspace only when extracting DIFC calibration.
                MakeCalWorkspace=loadCalibration and has_calfile,
                MakeGroupingWorkspace=loadGrouping,
                MakeMaskWorkspace=loadMask,
                WorkspaceName=self.instr,
                TofMin=tmin_val,
            )

            LoadDiffCal(**diffcal_kwargs)
        if loadCalibration and has_calfile:
            self.__calWksp = self.instr + "_cal"
            self.setPropertyValue("CalibrationWorkspace", self.instr + "_cal")
        if loadGrouping:
            self.__grpWksp = self.instr + "_group"
            self.setPropertyValue("GroupingWorkspace", self.instr + "_group")
        if loadMask:
            self.__mskWksp = self.instr + "_mask"
            self.setPropertyValue("MaskWorkspace", self.instr + "_mask")
        self.kwargs = self.__getAlignAndFocusArgs()

    def __canUseSlim(self):
        """Check whether AlignAndFocusPowderSlim can replace the normal processing path."""

        if not self.getProperty("AllowSlimProcess").value:
            return False

        # Check that all required properties are set
        for prop in _SLIM_REQUIRED_PROPS:
            if self.getProperty(prop).isDefault:
                self.log().debug("AlignAndFocusPowderFromFiles: cannot use slim path - required property '%s' is not set" % prop)
                return False

        # Check hard blockers
        for prop in _SLIM_BLOCKING_PROPS:
            if not self.getProperty(prop).isDefault:
                self.log().debug("AlignAndFocusPowderFromFiles: cannot use slim path - property '%s' is set" % prop)
                return False

        # AAFPSlim does not preserve events; require the caller to explicitly opt in to that behaviour
        if self.getProperty("PreserveEvents").value:
            self.log().debug(
                "AlignAndFocusPowderFromFiles: cannot use slim path - PreserveEvents=True "
                "(AlignAndFocusPowderSlim always produces histograms; set PreserveEvents=False to allow the slim path)"
            )
            return False

        # AAFPSlim only accepts a single file
        if len(self._filenames) != 1:
            self.log().debug("AlignAndFocusPowderFromFiles: cannot use slim path - more than one file provided")
            return False

        # Params (TOF rebinning) must be either unset or a simple [xmin, delta, xmax] triple;
        # multi-range Params (>3 values) cannot be expressed in Slim's per-group arrays simply
        if not self.getProperty("Params").isDefault:
            if len(self.getProperty("Params").value) != 3:
                self.log().debug("AlignAndFocusPowderFromFiles: cannot use slim path - Params has more than 3 values")
                return False

        # AlignAndFocusPowderSlim requires NXevent_data; reject non-event nexus files early
        # to avoid the unhelpful "No NXevent_data entries found in file" error at run time.
        try:
            found = []

            def _check_event_data(name, obj):
                if isinstance(obj, h5py.Group):
                    nx_class = obj.attrs.get("NX_class", b"")
                    if isinstance(nx_class, bytes):
                        nx_class = nx_class.decode("ascii", errors="ignore")
                    if nx_class == "NXevent_data":
                        found.append(True)
                        return True  # returning True stops visitation

            with h5py.File(self._filenames[0], "r") as f:
                f.visititems(_check_event_data)
            if not found:
                self.log().debug("AlignAndFocusPowderFromFiles: cannot use slim path - file has no NXevent_data entries")
                return False
        except Exception as e:
            self.log().debug("AlignAndFocusPowderFromFiles: could not inspect file for NXevent_data (%s); letting Slim try" % str(e))

        return True

    def __getSlimArgsFromPropManager(self):
        """Read d_min/d_max/delta from the ReductionProperties property manager.

        Returns a dict of extra kwargs for AlignAndFocusPowderSlim, or None if the
        property manager contains settings that cannot be expressed in Slim
        (non-zero tof_min/tof_max/wavelength_min/wavelength_max)."""
        pm_name = self.getProperty("ReductionProperties").valueAsStr
        if pm_name not in PropertyManagerDataService:
            return {}

        pm = PropertyManagerDataService[pm_name]

        # Select PM keys that match the requested binning units (driven by the Dspacing property)
        # to avoid a unit mismatch when the property manager contains both tof and d-spacing keys.
        if self.getProperty("Dspacing").value:
            units = "dSpacing"
            key_combo = ("d_min", "d_max", "delta")
        else:
            units = "tof"
            key_combo = ("tof_min", "tof_max", "delta")

        extra = {}
        keys = [pm.existsProperty(key) for key in key_combo]
        if all(keys):
            extra["BinningUnits"] = units
            extra["XMin"] = list(pm.getProperty(key_combo[0]).value)
            extra["XMax"] = list(pm.getProperty(key_combo[1]).value)
            extra["XDelta"] = list(pm.getProperty(key_combo[2]).value)

        return extra

    def __runCharacterizationsForSlim(self):
        """Load file metadata and run PDDetermineCharacterizations to populate the
        ReductionProperties property manager for use by the Slim path."""
        filename = self._filenames[0]
        tempname = "__{}_slim_char".format(self.__wkspNameFromFile(filename))

        allowedLogs = list(self.getProperty("FrequencyLogNames").value)
        allowedLogs.extend(self.getProperty("WaveLengthLogNames").value)
        try:
            loader = self.__createLoader(filename, tempname, MetaDataOnly=True, AllowList=allowedLogs)
            loader.execute()
            if self.__loaderName == "Load":
                self.__loaderName = loader.getPropertyValue("LoaderName")
        except RuntimeError:
            Load(OutputWorkspace=tempname, Filename=filename)

        args = {
            "ReductionProperties": self.getProperty("ReductionProperties").valueAsStr,
            "InputWorkspace": tempname,
            "Characterizations": self.charac,
        }
        for name in PROPS_FOR_PD_CHARACTER:
            prop = self.getProperty(name)
            if not prop.isDefault:
                args[name] = prop.value

        PDDetermineCharacterizations(**args)
        DeleteWorkspace(Workspace=tempname)

    def __runSlim(self, outputname, pm_args=None):
        """Call AlignAndFocusPowderSlim and store the result under *outputname*."""

        # All properties that are simply mapped
        kwargs = {
            "Filename": self._filenames[0],
            "OutputWorkspace": outputname,
        }
        propertyMap = {
            "CalFileName": CAL_FILE,
            "L1": "PrimaryFlightPath",
            "L2": "L2",
            "Polar": "Polar",
            "Azimuthal": "Azimuthal",
            "LogAllowList": "LogAllowList",
            "LogBlockList": "LogBlockList",
        }
        for slimProp, thisProp in propertyMap.items():
            kwargs[slimProp] = self.getPropertyValue(thisProp)

        # Properties requiring special mapping

        # whether or not to use dSpacing
        kwargs["BinningUnits"] = "dSpacing" if self.getProperty("Dspacing").value else "TOF"

        # binning ranges: direct DMin/DMax win, then ReductionProperties PM, then Params
        if (
            not self.getProperty("DMin").isDefault
            or not self.getProperty("DMax").isDefault
            or not self.getProperty("DeltaRagged").isDefault
        ):
            kwargs["BinningUnits"] = "dSpacing"
            if not self.getProperty("DMin").isDefault:
                kwargs["XMin"] = list(self.getProperty("DMin").value)
            if not self.getProperty("DMax").isDefault:
                kwargs["XMax"] = list(self.getProperty("DMax").value)
            if not self.getProperty("DeltaRagged").isDefault:
                kwargs["XDelta"] = list(self.getProperty("DeltaRagged").value)

        # Fill any unset binning keys from the ReductionProperties property manager
        if pm_args:
            for key, val in pm_args.items():
                if key not in kwargs:
                    kwargs[key] = val

        # Fall back to Params if no d-spacing ranges have been set
        if "XMin" not in kwargs and not self.getProperty("Params").isDefault:
            params = self.getProperty("Params").value
            kwargs["XMin"] = [float(params[0])]
            kwargs["XDelta"] = [float(params[1])]
            kwargs["XMax"] = [float(params[2])]

        # grouping: workspace takes priority, then pass file directly to AAFPSlim
        if not self.getProperty(GRP_WKSP).isDefault:
            kwargs["GroupingWorkspace"] = self.getPropertyValue(GRP_WKSP)
        elif not self.getProperty(GROUP_FILE).isDefault:
            kwargs["GroupFilename"] = self.getPropertyValue(GROUP_FILE)

        # calibration and mask workspaces take precedence over the contents of CalFileName in AAFPSlim
        if not self.getProperty(CAL_WKSP).isDefault:
            kwargs["CalibrationWorkspace"] = self.getPropertyValue(CAL_WKSP)
        if not self.getProperty(MASK_WKSP).isDefault:
            kwargs["MaskWorkspace"] = self.getPropertyValue(MASK_WKSP)

        # chunk size
        if not self.getProperty("MaxChunkSize").isDefault and self.chunkSize > 0.0:
            bytes_total = self.chunkSize * 1024**3
            kwargs["ReadSizeFromDisk"] = int(bytes_total / _BYTES_PER_EVENT)

        # bad pulse filtering
        if self.filterBadPulses > 0.0:
            kwargs["FilterBadPulses"] = True
            kwargs["BadPulsesLowerCutoff"] = self.filterBadPulses

        try:
            AlignAndFocusPowderSlim(**kwargs)
        except RuntimeError as e:
            if "No NXevent_data entries found in file" in str(e):
                self.log().warning(
                    "AlignAndFocusPowderFromFiles: file does not contain NXevent_data entries; falling back to standard processing."
                )
                return
            raise

        # post-processing: apply SpectrumIDs renaming if requested ---
        if not self.getProperty("SpectrumIDs").isDefault:
            EditInstrumentGeometry(
                Workspace=outputname,
                SpectrumIDs=self.getProperty("SpectrumIDs").value,
            )

        # TODO CacheDir?

    def PyExec(self):
        self._filenames = sorted(self.__getLinearizedFilenames("Filename"))
        self.instr = os.path.basename(self._filenames[0]).split("_")[0]  # wrong for instrument's with `_` in the name
        self.haveDeterminedCalibration = False  # setup variables for loading calibration into workspaces
        self.__loaderName = "Load"  # set the loader to be generic on first load
        self.filterBadPulses = self.getProperty("FilterBadPulses").value
        self.chunkSize = self.getProperty("MaxChunkSize").value
        self.compression_threshold = self.getProperty("MinSizeCompressOnLoad").value
        self.absorption = self.getProperty("AbsorptionWorkspace").value
        self.charac = self.getProperty("Characterizations").value
        self.useCaching = len(self.getProperty("CacheDir").value) > 0
        self.__calWksp = ""
        self.__grpWksp = ""
        self.__mskWksp = ""
        self.kwargs = None
        finalname = self.getPropertyValue("OutputWorkspace")

        # accumulate the unfocused workspace if it was requested
        # empty string means it is not used
        finalunfocusname = self.getPropertyValue("UnfocussedWorkspace")

        # determining compression (needed by __createLoader, called from __runCharacterizationsForSlim)
        self.do_compression = determineCompression(
            filename=self._filenames[0], compression=self.compression_threshold, chunking=self.chunkSize, absorption=self.absorption
        )

        # ------------------------------------------------------------------ #
        # Fast path: delegate entirely to AlignAndFocusPowderSlim when the   #
        # inputs are compatible with its more restricted interface.           #
        # ------------------------------------------------------------------ #
        if self.__canUseSlim():
            # Populate ReductionProperties from Characterizations if provided.
            if self.charac:
                self.__runCharacterizationsForSlim()

            # Try to extract d_min/d_max from the ReductionProperties PM.
            # Returns None if the PM contains tof/wavelength ranges Slim cannot handle;
            # returns {} (or a dict with XMin/XMax) otherwise.
            # Binning may also be set via DMin/DMax/DeltaRagged or Params directly —
            # those are handled inside __runSlim regardless.
            slim_pm_args = self.__getSlimArgsFromPropManager()

            if slim_pm_args is None:
                # PM has settings (tof_min/tof_max/wavelength) that Slim cannot handle;
                # fall through to the normal processing path.
                self.log().warning(
                    "AlignAndFocusPowderFromFiles: characterizations/ReductionProperties set "
                    "TOF or wavelength ranges not supported by AlignAndFocusPowderSlim; "
                    "using standard processing."
                )
            else:
                self.log().notice("AlignAndFocusPowderFromFiles: delegating to AlignAndFocusPowderSlim for faster single-file processing.")
                self.__runSlim(finalname, slim_pm_args)
                if finalname in mtd:
                    self.setProperty("OutputWorkspace", mtd[finalname])

                    # TODO CacheDir?
                else:
                    # __runSlim returned without output (e.g. non-event nexus); fall through to standard processing
                    self.__slowPath(finalname, finalunfocusname)
        # ------------------------------------------------------------------ #
        # Slower path: load needed workspaces and call AlignAndFocusPowder   #
        # ------------------------------------------------------------------ #
        else:
            self.__slowPath(finalname, finalunfocusname)

    def __slowPath(self, finalname, finalunfocusname):
        if self.useCaching:
            # unfocus check only matters if caching is requested
            if finalunfocusname != "":
                self.useCaching = False
                self.log().warning('CacheDir is specified with "UnfocussedWorkspace" - reading cache files disabled')
        else:
            self.log().warning("CacheDir is not specified - functionality disabled")

        assert len(self._filenames), "No files specified"
        self.prog_per_file = 1.0 / float(len(self._filenames))  # for better progress reporting

        # initialization for caching mechanism
        if self.useCaching:
            filename = self._filenames[0]
            wkspname = os.path.split(filename)[-1].split(".")[0]
            self.__determineCharacterizations(filename, wkspname)
            del filename
            del wkspname

        # find caches and sum them. return files without cache - cannot be used with UnfocussedWorkspace
        if self.useCaching:
            notcached = self.__findAndLoadCachefiles(self._filenames, finalname, firstTime=True)
        else:
            notcached = self._filenames[:]  # make a copy
        self.log().notice("Files to process: {}".format(notcached))

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
        self.setProperty("OutputWorkspace", mtd[finalname])
        if finalunfocusname:
            self.setProperty("UnfocussedWorkspace", mtd[finalunfocusname])

    def __loadCacheFile(self, filename, wkspname):
        """@returns True if a file was loaded"""
        if os.path.exists(filename):
            self.log().notice("Loading cache from {}".format(filename))
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
            try:
                EditInstrumentGeometry(Workspace=wkspname, **editinstrargs)
            except RuntimeError as e:
                # treat this as a non-fatal error
                self.log().warning("Failed to update instrument geometry in cache file: {}".format(e))

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
            for start in range(N - length + 1):
                end = start + length - 1
                fileSubset = filenames[start : end + 1]
                summed_cache_file = self.__getGroupCacheName(fileSubset)
                wkspname = self.__getGroupWkspName(fileSubset)
                try:
                    if self.__loadCacheFile(summed_cache_file, wkspname):
                        self.__accumulate(wkspname, finalname, "", "", firstTime)
                        found = True
                        break
                except RuntimeError as e:
                    # log as a warning and carry on as though the cache file didn't exist
                    self.log().warning('Failed to load cache file "{}": {}'.format(summed_cache_file, e))
            if found:
                break
            continue
        if not found:
            return filenames
        remained = filenames[:start] + filenames[end + 1 :]
        if remained:
            return self.__findAndLoadCachefiles(remained, finalname, False)
        return []

    def __wkspNameFromFile(self, filename):
        name = os.path.basename(filename).split(".")[0]
        name = name.replace("_event", "")  # for old SNS files
        return name

    def __getGroupWkspName(self, group):
        group = [self.__wkspNameFromFile(filename) for filename in group]
        group = [name.replace("_event", "") for name in group]
        if len(group) == 1:
            return group[0]
        else:
            return "{}-{}".format(group[0], group[-1])

    def __processFiles(self, files, finalname, finalunfocusname, hasAccumulated):
        """process given files, separate them to "grains". Sum each grain, and add the grain sum to final sum"""
        numberFilesToProcess = len(files)
        if numberFilesToProcess > 3:
            grain_size = int(math.sqrt(numberFilesToProcess))  # grain size
        else:
            grain_size = numberFilesToProcess
        for i, filename in enumerate(files):
            self.__loaderName = "Load"  # reset to generic load with each file
            wkspname, unfocusname = self.__processFile(filename, self.prog_per_file * float(i), not self.useCaching, bool(finalunfocusname))
            # accumulate into partial sum
            grain_start = i // grain_size * grain_size
            grain_end = min((i // grain_size + 1) * grain_size, numberFilesToProcess)
            grain = files[grain_start:grain_end]
            partialsum_wkspname = "__" + self.__getGroupWkspName(grain)
            if finalunfocusname:
                partialsum_unfocusname = partialsum_wkspname + "_unfocused"
            else:
                partialsum_unfocusname = ""
            self.__accumulate(wkspname, partialsum_wkspname, unfocusname, partialsum_unfocusname, (not hasAccumulated) or i == 0)
            hasAccumulated = True
            if i == grain_end - 1:
                if self.useCaching and len(grain) > 1:
                    # save partial cache
                    self.__saveSummedGroupToCache(grain, partialsum_wkspname)
                # accumulate into final sum
                self.__accumulate(partialsum_wkspname, finalname, partialsum_unfocusname, finalunfocusname, (not hasAccumulated) or i == 0)

    def __saveSummedGroupToCache(self, group, wkspname):
        cache_file = self.__getGroupCacheName(group)
        if not os.path.exists(cache_file):
            SaveNexusProcessed(InputWorkspace=wkspname, Filename=cache_file)
        return


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(AlignAndFocusPowderFromFiles)
