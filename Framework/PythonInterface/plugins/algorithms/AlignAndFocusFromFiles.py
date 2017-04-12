from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd, AlgorithmFactory, DataProcessorAlgorithm, ITableWorkspaceProperty, MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode
from mantid.kernel import Direction
from mantid.simpleapi import AlignAndFocusPowder, CompressEvents, ConvertUnits, DeleteWorkspace, DetermineChunking, Divide, FilterBadPulses, Load, PDDetermineCharacterizations, Plus, RenameWorkspace
import os

EXTENSIONS_NXS = ["_event.nxs", ".nxs.h5"]

PROPS_FOR_ALIGN = ["CalFileName", "GroupFilename", "GroupingWorkspace",
                   "CalibrationWorkspace", "OffsetsWorkspace",
                   "MaskWorkspace", "MaskBinTable",
                   "Params", "ResampleX", "Dspacing", "DMin", "DMax",
                   "TMin", "TMax", "PreserveEvents",
                   "RemovePromptPulseWidth", "CompressTolerance",
                   "UnwrapRef", "LowResRef",
                   "CropWavelengthMin", "CropWavelengthMax",
                   "PrimaryFlightPath", "SpectrumIDs", "L2", "Polar", "Azimuthal",
                   "LowResSpectrumOffset", "ReductionProperties"]

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

class AlignAndFocusFromFiles(DataProcessorAlgorithm):
    def category(self):
        return "Diffraction\\Reduction"

    def name(self):
        return "AlignAndFocusFromFiles"

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

    def getAlignAndFocusArgs(self):
        args = {}
        for name in PROPS_FOR_ALIGN:
            prop = self.getProperty(name)
            if name == 'PreserveEvents' or not prop.isDefault:
                args[name] = prop.value
        return args

    def PyExec(self):
        filenames = self._getLinearizedFilenames('Filename')
        filterBadPulses = self.getProperty('FilterBadPulses').value
        chunkSize = self.getProperty('MaxChunkSize').value
        absorption = self.getProperty('AbsorptionWorkspace').value
        charac = self.getProperty('Characterizations').value
        finalname = self.getProperty('OutputWorkspace').valueAsStr

        # these are also passed into the child-algorithms
        kwargs = self.getAlignAndFocusArgs()

        # outer loop creates chunks to load
        for (i, filename) in enumerate(filenames):
            wkspname = os.path.split(filename)[-1].split('.')[0] + '_' + str(i)
            chunks = determineChunking(filename, chunkSize)

            # inner loop is over chunks
            for (j, chunk) in enumerate(chunks):
                chunkname = "%s_chunk%d" % (wkspname, j)
                Load(Filename=filename, OutputWorkspace=chunkname, **chunk)
                if filterBadPulses > 0.:
                    FilterBadPulses(InputWorkspace=chunkname, OutputWorkspace=chunkname,
                                    LowerCutoff=filterBadPulses)

                # TODO only do once per file
                if j == 0 and charac is not None:
                    PDDetermineCharacterizations(InputWorkspace=chunkname,
                                                 Characterizations=charac,
                                                 ReductionProperties=self.getProperty('ReductionProperties').valueAsStr)

                # absorption correction workspace
                if absorption is not None and len(str(absorption)) > 0:
                    ConvertUnits(InputWorkspace=chunkname, OutputWorkspace=chunkname,
                                 Target='Wavelength', EMode='Elastic')
                    Divide(LHSWorkspace=chunkname, RHSWorkspace=absorption, OutputWorkspace=chunkname)
                    ConvertUnits(InputWorkspace=chunkname, OutputWorkspace=chunkname,
                                 Target='TOF', EMode='Elastic')

                AlignAndFocusPowder(InputWorkspace=chunkname, OutputWorkspace=chunkname, **kwargs)


                if j == 0:
                    RenameWorkspace(InputWorkspace=chunkname, OutputWorkspace=wkspname)
                else:
                    Plus(LHSWorkspace=wkspname, RHSWorkspace=chunkname, OutputWorkspace=wkspname,
                         ClearRHSWorkspace=kwargs['PreserveEvents'])
                    DeleteWorkspace(Workspace=chunkname)
            # end of inner loop

            # accumulate runs
            if i == 0:
                if wkspname != finalname:
                    RenameWorkspace(InputWorkspace=wkspname, OutputWorkspace=finalname)
            else:
                Plus(LHSWorkspace=finalname, RHSWorkspace=wkspname, OutputWorkspace=finalname,
                     ClearRHSWorkspace=kwargs['PreserveEvents'])
                if kwargs['PreserveEvents']:
                    CompressEvents(InputWorkspace=finalname, OutputWorkspace=finalname)
                DeleteWorkspace(Workspace=wkspname)

        # set the output workspace
        self.setProperty('OutputWorkspace', mtd[finalname])

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(AlignAndFocusFromFiles)
