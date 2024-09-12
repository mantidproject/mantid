# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.simpleapi import (
    AlignAndFocusPowder,
    AlignAndFocusPowderFromFiles,
    NormaliseByCurrent,
    PDDetermineCharacterizations,
    PDLoadCharacterizations,
    SaveGSS,
    SetUncertainties,
)
from mantid.api import mtd, AlgorithmFactory, DataProcessorAlgorithm, FileAction, FileProperty, MatrixWorkspaceProperty, PropertyMode
from mantid.kernel import Direction, FloatArrayProperty
import mantid

COMPRESS_TOL_TOF = 0.01
EVENT_WORKSPACE_ID = "EventWorkspace"


class PDToPDFgetN(DataProcessorAlgorithm):
    _focusPos = {}
    _iparmFile = None

    def category(self):
        return "Workflow\\Diffraction"

    def seeAlso(self):
        return ["PDToGUDRUN"]

    def name(self):
        return "PDToPDFgetN"

    def summary(self):
        return "The algorithm used converting raw data to pdfgetn input files"

    def PyInit(self):
        group = "Input"
        self.declareProperty(
            FileProperty(name="Filename", defaultValue="", action=FileAction.OptionalLoad, extensions=["_event.nxs", ".nxs.h5"]),
            "Event file",
        )
        self.copyProperties("AlignAndFocusPowderFromFiles", "MaxChunkSize")
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
        self.copyProperties("AlignAndFocusPowderFromFiles", "CacheDir")
        self.declareProperty(
            FileProperty(name="PDFgetNFile", defaultValue="", action=FileAction.Save, extensions=[".getn"]), "Output filename"
        )
        self.setPropertyGroup("OutputWorkspace", group)
        self.setPropertyGroup("PDFgetNFile", group)

        self.declareProperty(
            FileProperty(
                name="CalibrationFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".h5", ".hd5", ".hdf", ".cal"]
            )
        )
        self.declareProperty(
            FileProperty(name="CharacterizationRunsFile", defaultValue="", action=FileAction.OptionalLoad, extensions=["txt"]),
            "File with characterization runs denoted",
        )
        self.copyProperties(
            "AlignAndFocusPowderFromFiles",
            ["FrequencyLogNames", "WaveLengthLogNames", "RemovePromptPulseWidth", "CropWavelengthMin", "CropWavelengthMax"],
        )

        self.declareProperty(
            FloatArrayProperty("Binning", values=[0.0, 0.0, 0.0], direction=Direction.Input),
            "Positive is linear bins, negative is logorithmic",
        )
        self.declareProperty(
            "ResampleX",
            0,
            'Number of bins in x-axis. Non-zero value overrides "Params" property. ' + "Negative value means logorithmic binning.",
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
        self._alignArgs = {}
        self._iparmFile = None

        charFilename = self.getProperty("CharacterizationRunsFile").value

        if charFilename is None or len(charFilename) <= 0:
            return

        results = PDLoadCharacterizations(Filename=charFilename, OutputWorkspace="characterizations")
        self._iparmFile = results[1]
        self._alignArgs["PrimaryFlightPath"] = results[2]
        self._alignArgs["SpectrumIDs"] = results[3]
        self._alignArgs["L2"] = results[4]
        self._alignArgs["Polar"] = results[5]
        self._alignArgs["Azimuthal"] = results[6]

    def PyExec(self):
        self._loadCharacterizations()
        charac = ""
        if mtd.doesExist("characterizations"):
            charac = "characterizations"

        # arguments for both AlignAndFocusPowder and AlignAndFocusPowderFromFiles
        self._alignArgs["OutputWorkspace"] = self.getPropertyValue("OutputWorkspace")
        self._alignArgs["RemovePromptPulseWidth"] = self.getProperty("RemovePromptPulseWidth").value
        self._alignArgs["CompressTolerance"] = COMPRESS_TOL_TOF
        self._alignArgs["PreserveEvents"] = True
        self._alignArgs["CalFileName"] = self.getProperty("CalibrationFile").value
        self._alignArgs["Params"] = self.getProperty("Binning").value
        self._alignArgs["ResampleX"] = self.getProperty("ResampleX").value
        self._alignArgs["Dspacing"] = True
        self._alignArgs["CropWavelengthMin"] = self.getProperty("CropWavelengthMin").value
        self._alignArgs["CropWavelengthMax"] = self.getProperty("CropWavelengthMax").value
        self._alignArgs["ReductionProperties"] = "__snspowderreduction"

        wksp = self.getProperty("InputWorkspace").value
        if wksp is None:  # run from file with caching
            wksp = AlignAndFocusPowderFromFiles(
                Filename=self.getProperty("Filename").value,
                CacheDir=self.getProperty("CacheDir").value,
                MaxChunkSize=self.getProperty("MaxChunkSize").value,
                FilterBadPulses=self.getProperty("FilterBadPulses").value,
                Characterizations=charac,
                FrequencyLogNames=self.getProperty("FrequencyLogNames").value,
                WaveLengthLogNames=self.getProperty("WaveLengthLogNames").value,
                **(self._alignArgs),
            )
        else:  # process the input workspace
            self.log().information(
                "Using input workspace. Ignoring properties 'Filename', " + "'OutputWorkspace', 'MaxChunkSize', and 'FilterBadPulses'"
            )

            # get the correct row of the table
            PDDetermineCharacterizations(
                InputWorkspace=wksp,
                Characterizations=charac,
                ReductionProperties="__snspowderreduction",
                FrequencyLogNames=self.getProperty("FrequencyLogNames").value,
                WaveLengthLogNames=self.getProperty("WaveLengthLogNames").value,
            )

            wksp = AlignAndFocusPowder(InputWorkspace=wksp, **(self._alignArgs))

        wksp = NormaliseByCurrent(InputWorkspace=wksp, OutputWorkspace=wksp)
        wksp.getRun()["gsas_monitor"] = 1
        if self._iparmFile is not None:
            wksp.getRun()["iparm_file"] = self._iparmFile

        wksp = SetUncertainties(InputWorkspace=wksp, OutputWorkspace=wksp, SetError="sqrtOrOne")
        SaveGSS(
            InputWorkspace=wksp,
            Filename=self.getProperty("PDFgetNFile").value,
            SplitFiles=False,
            Append=False,
            MultiplyByBinWidth=False,
            Bank=mantid.pmds["__snspowderreduction"]["bank"].value,
            Format="SLOG",
            ExtendedHeader=True,
        )

        self.setProperty("OutputWorkspace", wksp)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(PDToPDFgetN)
