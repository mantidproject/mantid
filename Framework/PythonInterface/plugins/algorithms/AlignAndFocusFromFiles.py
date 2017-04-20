from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd, AlgorithmFactory, DataProcessorAlgorithm, ITableWorkspaceProperty, MatrixWorkspaceProperty, FileAction, FileProperty, MultipleFileProperty, Progress, PropertyMode
from mantid.kernel import Direction
from mantid.simpleapi import AlignAndFocusPowder, CompressEvents, ConvertUnits, DeleteWorkspace, DetermineChunking, Divide, FilterBadPulses, Load, PDDetermineCharacterizations, Plus, RenameWorkspace
import os

EXTENSIONS_NXS = ["_event.nxs", ".nxs.h5"]
PROPS_FOR_INSTR = ["PrimaryFlightPath", "SpectrumIDs", "L2", "Polar", "Azimuthal"]
PROPS_FOR_ALIGN = ["CalFileName", "GroupFilename", "GroupingWorkspace",
                   "CalibrationWorkspace", "OffsetsWorkspace",
                   "MaskWorkspace", "MaskBinTable",
                   "Params", "ResampleX", "Dspacing", "DMin", "DMax",
                   "TMin", "TMax", "PreserveEvents",
                   "RemovePromptPulseWidth", "CompressTolerance",
                   "UnwrapRef", "LowResRef",
                   "CropWavelengthMin", "CropWavelengthMax",
                   "LowResSpectrumOffset", "ReductionProperties"]
PROPS_FOR_ALIGN.extend(PROPS_FOR_INSTR)

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

        self.declareProperty(FileProperty(name="CacheDirectory",defaultValue="",
                                          action=FileAction.OptionalDirectory))

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     Direction.Output),
                             doc='Combined output workspace')

        self.declareProperty(ITableWorkspaceProperty('Characterizations', '',
                                                     Direction.Input, PropertyMode.Optional),
                             'Characterizations table')

        self.copyProperties("AlignAndFocusPowder", PROPS_FOR_ALIGN)

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
                args[name] = prop.value
        return args

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

            # TODO only do once per file
            if j == 0 and self.charac is not None:
                PDDetermineCharacterizations(InputWorkspace=chunkname,
                                             Characterizations=self.charac,
                                             ReductionProperties=self.getProperty('ReductionProperties').valueAsStr)

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
            wkspname = os.path.split(filename)[-1].split('.')[0] + '_f' + str(i)

            self.__processFile(filename, wkspname, self.prog_per_file*float(i))

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

        # set the output workspace
        self.setProperty('OutputWorkspace', mtd[finalname])

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(AlignAndFocusPowderFromFiles)
