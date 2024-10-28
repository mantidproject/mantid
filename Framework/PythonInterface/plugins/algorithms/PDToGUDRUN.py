# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import mtd, AlgorithmFactory, DataProcessorAlgorithm, FileAction, FileProperty, PropertyMode, MatrixWorkspaceProperty
from mantid.kernel import ConfigService, Direction, FloatArrayProperty, StringListValidator
from mantid.simpleapi import (
    AlignAndFocusPowder,
    LoadDetectorsGroupingFile,
    LoadEventAndCompress,
    NormaliseByCurrent,
    PDDetermineCharacterizations,
    PDLoadCharacterizations,
    SaveNexusPD,
    SetUncertainties,
)

COMPRESS_TOL_TOF = 0.01
EVENT_WORKSPACE_ID = "EventWorkspace"


class PDToGUDRUN(DataProcessorAlgorithm):
    _iparmFile = None

    def category(self):
        return "Workflow\\Diffraction"

    def seeAlso(self):
        return ["PDToPDFgetN"]

    def name(self):
        return "PDToGUDRUN"

    def summary(self):
        return "The algorithm used converting raw data to gudrun input files"

    def PyInit(self):
        group = "Input"
        self.declareProperty(
            FileProperty(name="Filename", defaultValue="", action=FileAction.OptionalLoad, extensions=["_event.nxs", ".nxs.h5"]),
            "Event file",
        )
        self.declareProperty("MaxChunkSize", 0.0, "Specify maximum Gbytes of file to read in one chunk.  Default is whole file.")
        self.declareProperty(
            "FilterBadPulses", 95.0, doc="Filter out events measured while proton " + "charge is more than 5% below average"
        )

        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Handle to reduced workspace",
        )
        self.setPropertyGroup("Filename", group)
        self.setPropertyGroup("MaxChunkSize", group)
        self.setPropertyGroup("FilterBadPulses", group)
        self.setPropertyGroup("InputWorkspace", group)

        group = "Output"
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Handle to reduced workspace")
        self.declareProperty(
            FileProperty(name="GUDRUNFile", defaultValue="", action=FileAction.Save, extensions=[".getn"]), "Output filename"
        )
        self.setPropertyGroup("OutputWorkspace", group)
        self.setPropertyGroup("GUDRUNFile", group)

        self.declareProperty(
            FileProperty(name="GroupingFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".xml"]),
            "Override grouping found in CalibrationFile",
        )
        self.declareProperty(
            FileProperty(
                name="CalibrationFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".h5", ".hd5", ".hdf", ".cal"]
            )
        )
        self.declareProperty(
            FileProperty(name="CharacterizationRunsFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".txt"]),
            "File with characterization runs denoted",
        )

        self.declareProperty("RemovePromptPulseWidth", 0.0, "Width of events (in microseconds) near the prompt pulse to remove. 0 disables")
        self.declareProperty("CropWavelengthMin", 0.0, "Crop the data at this minimum wavelength.")
        self.declareProperty("CropWavelengthMax", 0.0, "Crop the data at this maximum wavelength.")

        self.declareProperty(
            FloatArrayProperty("Binning", values=[0.0, 0.0, 0.0], direction=Direction.Input),
            "Positive is linear bins, negative is logorithmic",
        )
        self.declareProperty(
            "ResampleX",
            0,
            'Number of bins in x-axis. Non-zero value overrides "Params" property. ' + "Negative value means logorithmic binning.",
        )
        self.declareProperty(
            "SetUncertainties",
            "",
            StringListValidator(["", "zero", "sqrt", "sqrtOrOne", "oneIfZero"]),
            doc="Recalculate uncertainties. Empty string is do nothing.",
        )

    def validateInputs(self):
        issues = {}

        if self.getProperty("InputWorkspace").value is None:
            filename = self.getProperty("Filename").value
            if filename is None or len(filename) <= 0:
                msg = "Must supply a Filename or InputWorkspace"
                issues["Filename"] = msg
                issues["InputWorkspace"] = msg
        else:
            if self.getProperty("InputWorkspace").value.id() == EVENT_WORKSPACE_ID:
                if self.getProperty("InputWorkspace").value.getNumberEvents() <= 0:
                    issues["InputWorkspace"] = "Workspace contains no events"

        if self.getProperty("ResampleX").value == 0:
            binning = self.getProperty("Binning").value
            if len(binning) == 1:
                binning = binning[0]
            else:
                binning = binning[1]
            if binning == 0.0:  # has to be non-zero delta
                msg = "Must supply a Binning or ResampleX"
                issues["Binning"] = msg
                issues["ResampleX"] = msg

        return issues

    def _loadCharacterizations(self):
        self._iparmFile = None

        charFilename = self.getProperty("CharacterizationRunsFile").value

        if charFilename is None or len(charFilename) <= 0:
            return

        results = PDLoadCharacterizations(Filename=charFilename, OutputWorkspace="characterizations", startProgress=0.0, endProgress=0.1)
        self._iparmFile = results[1]

    def PyExec(self):
        self._loadCharacterizations()

        wksp = self.getProperty("InputWorkspace").value
        if wksp is None:
            wksp = LoadEventAndCompress(
                Filename=self.getProperty("Filename").value,
                OutputWorkspace=self.getPropertyValue("OutputWorkspace"),
                MaxChunkSize=self.getProperty("MaxChunkSize").value,
                FilterBadPulses=self.getProperty("FilterBadPulses").value,
                CompressTOFTolerance=COMPRESS_TOL_TOF,
                startProgress=0.1,
                endProgress=0.3,
            )

            if wksp.getNumberEvents() <= 0:  # checked InputWorkspace during validateInputs
                raise RuntimeError("Workspace contains no events")
        else:
            self.log().information(
                "Using input workspace. Ignoring properties 'Filename', " + "'OutputWorkspace', 'MaxChunkSize', and 'FilterBadPulses'"
            )

        charac = ""
        if mtd.doesExist("characterizations"):
            charac = "characterizations"

        # get the correct row of the table
        PDDetermineCharacterizations(InputWorkspace=wksp, Characterizations=charac, ReductionProperties="__snspowderreduction")

        groupingFile = self.getProperty("GroupingFile").value
        if len(groupingFile) > 0:
            instrumentName = wksp.getInstrument().getName()
            instrumentName = ConfigService.getInstrument(instrumentName).shortName()
            LoadDetectorsGroupingFile(InputFile=groupingFile, OutputWorkspace=instrumentName + "_group")

        wksp = AlignAndFocusPowder(
            InputWorkspace=wksp,
            OutputWorkspace=wksp,
            CalFileName=self.getProperty("CalibrationFile").value,
            Params=self.getProperty("Binning").value,
            ResampleX=self.getProperty("ResampleX").value,
            Dspacing=True,
            PreserveEvents=False,
            RemovePromptPulseWidth=self.getProperty("RemovePromptPulseWidth").value,
            CompressTolerance=COMPRESS_TOL_TOF,
            CropWavelengthMin=self.getProperty("CropWavelengthMin").value,
            CropWavelengthMax=self.getProperty("CropWavelengthMax").value,
            ReductionProperties="__snspowderreduction",
            startProgress=0.3,
            endProgress=0.8,
        )
        wksp = NormaliseByCurrent(InputWorkspace=wksp, OutputWorkspace=wksp, startProgress=0.8, endProgress=0.9)
        wksp.getRun()["gsas_monitor"] = 1
        if self._iparmFile is not None:
            wksp.getRun()["iparm_file"] = self._iparmFile

        setUncertainties = self.getProperty("SetUncertainties").value
        if len(setUncertainties) > 0:
            wksp = SetUncertainties(InputWorkspace=wksp, OutputWorkspace=wksp, SetError=setUncertainties)

        SaveNexusPD(InputWorkspace=wksp, OutputFilename=self.getProperty("GUDRUNFile").value, startProgress=0.9, endProgress=1.0)

        self.setProperty("OutputWorkspace", wksp)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(PDToGUDRUN)
