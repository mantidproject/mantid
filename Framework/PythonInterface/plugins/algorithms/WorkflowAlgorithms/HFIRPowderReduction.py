# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    DataProcessorAlgorithm,
    AlgorithmFactory,
    MultipleFileProperty,
    FileAction,
    PropertyMode,
    IEventWorkspace,
    WorkspaceProperty,
    AnalysisDataService,
    WorkspaceGroup,
    FileProperty,
)

from mantid.kernel import (
    StringListValidator,
    Direction,
    Property,
    FloatBoundedValidator,
    IntArrayProperty,
    SetDefaultWhenProperty,
    Logger,
    Elastic,
    UnitConversion,
)
from mantid.dataobjects import MaskWorkspaceProperty
from mantid.simpleapi import (
    ConvertSpectrumAxis,
    ConjoinWorkspaces,
    Transpose,
    ResampleX,
    CopyInstrumentParameters,
    CylinderAbsorptionCW,
    Divide,
    DeleteWorkspace,
    Scale,
    MaskAngle,
    ExtractMask,
    Minus,
    SetSample,
    SumSpectra,
    ExtractUnmaskedSpectra,
    mtd,
    BinaryOperateMasks,
    Integration,
    GroupWorkspaces,
    RenameWorkspace,
    GroupDetectors,
    SaveAscii,
    SaveNexus,
    Load,
    LoadWAND,
    LoadInstrument,
    CreateWorkspace,
    LoadNexusLogs,
    AddSampleLog,
)
import h5py
import numpy as np
import os

logger = Logger(__name__)


class HFIRPowderReduction(DataProcessorAlgorithm):
    MIDAS_NUM_BANKS = 7
    MIDAS_TUBES_PER_BANK = 16
    MIDAS_PIXELS_PER_TUBE = 512
    MIDAS_TOTAL_PIXELS = MIDAS_NUM_BANKS * MIDAS_TUBES_PER_BANK * MIDAS_PIXELS_PER_TUBE  # 57344

    def name(self):
        return "HFIRPowderReduction"

    def category(self):
        return "DataHandling\\Nexus"

    def summary(self):
        return "Powder reduction for HFIR instruments"

    def PyInit(self):  # noqa: C901
        # Load UI properties
        self.declareProperty(
            MultipleFileProperty(name="SampleFilename", action=FileAction.OptionalLoad, extensions=[".nxs.h5"]), "Sample files to Load"
        )
        self.declareProperty("SampleIPTS", Property.EMPTY_INT, "Sample IPTS number to load from")
        self.declareProperty(IntArrayProperty("SampleRunNumbers"), "Sample run numbers to load")

        self.declareProperty(
            MultipleFileProperty(name="VanadiumFilename", action=FileAction.OptionalLoad, extensions=[".nxs.h5"]), "Vanadium files to Load"
        )
        self.declareProperty("VanadiumIPTS", Property.EMPTY_INT, "Vanadium IPTS number to load from")
        self.declareProperty(IntArrayProperty("VanadiumRunNumbers", []), "Vanadium run numbers to load")

        self.declareProperty(
            MultipleFileProperty(name="VanadiumBackgroundFilename", action=FileAction.OptionalLoad, extensions=[".nxs.h5"]),
            "Vanadium background files to Load",
        )
        self.declareProperty("VanadiumBackgroundIPTS", Property.EMPTY_INT, "Vanadium background IPTS number to load from")
        self.declareProperty(IntArrayProperty("VanadiumBackgroundRunNumbers", []), "Vanadium background run numbers to load")

        self.declareProperty(
            MultipleFileProperty(name="SampleBackgroundFilename", action=FileAction.OptionalLoad, extensions=[".nxs.h5"]),
            "Sample background files to Load",
        )
        self.declareProperty("SampleBackgroundIPTS", Property.EMPTY_INT, "Sample background IPTS number to load from")
        self.declareProperty(IntArrayProperty("SampleBackgroundRunNumbers", []), "Sample background run numbers to load")

        self.declareProperty("ApplyMask", True, "If True standard masking will be applied to the workspace")
        self.declareProperty("Grouping", "None", StringListValidator(["None", "2x2", "4x4"]), "Group pixels")

        # Reduction UI properties
        # TODO: This field fields below will be autopopulated from the sample file in a future PR, handled by EWM item 13209
        self.declareProperty("Instrument", "", StringListValidator(["", "MIDAS", "WAND^2"]), "HB2 Instrument")
        # TODO: This field fields below will be autopopulated from the sample file in a future PR, handled by EWM item 13209
        self.declareProperty(
            "Wavelength",
            Property.EMPTY_DBL,  # A
            FloatBoundedValidator(lower=0.0),  # must be positive
            doc="Incident wavelength (A)",
        )
        # TODO: This field fields below will be autopopulated from the sample file in a future PR, handled by EWM item 13209
        self.declareProperty(
            "VanadiumDiameter",
            Property.EMPTY_DBL,  # cm
            FloatBoundedValidator(lower=0.0),  # must be positive
            doc="Vanadium rod diamter (cm)",
        )
        self.declareProperty(
            "VanadiumHeight",
            3.0,  # cm
            FloatBoundedValidator(lower=0.0),
            doc="Vanadium rod height (cm)",
        )
        self.declareProperty(
            "DoAttenuationCorrection",
            False,
            doc="If True, apply sample attenuation (absorption) correction using CylinderAbsorptionCW",
        )
        self.declareProperty(
            "DoMultipleScatteringCorrection",
            False,
            doc="If True, calculate and apply multiple scattering correction for the sample",
        )
        self.declareProperty(
            "AbsoluteIntensityUnits",
            False,
            doc="If True, output in absolute intensity units (mb/sr/f.u.)",
        )
        self.declareProperty(
            "SampleChemicalFormula",
            "",
            doc="Chemical formula of the sample (e.g. 'Fe2O3')",
        )
        self.declareProperty(
            "SampleCrystalDensity",
            Property.EMPTY_DBL,
            FloatBoundedValidator(lower=0.0),
            doc="Crystal density of the sample (g/cm3)",
        )
        self.declareProperty(
            "SamplePackingFraction",
            0.5,
            FloatBoundedValidator(lower=0.0, upper=1.0),
            doc="Packing fraction for the sample powder (default 0.5)",
        )
        self.declareProperty(
            "SampleDiameter",
            Property.EMPTY_DBL,
            FloatBoundedValidator(lower=0.0),
            doc="Sample diameter (cm)",
        )
        self.declareProperty(
            "SampleHeight",
            0.0,
            FloatBoundedValidator(lower=0.0),
            doc="Sample height (cm). Required for multiple scattering and absolute intensity units.",
        )
        self.declareProperty(
            "SampleBackgroundScaleFactor",
            1.0,
            FloatBoundedValidator(lower=0.0),
            doc="Scale factor (fB) applied to sample background before subtraction",
        )
        self.declareProperty(
            "XUnits",
            "2Theta",
            StringListValidator(["2Theta", "d-spacing", "Q"]),
            doc="The unit to which spectrum axis is converted to",
        )
        # TODO: These 2 fields below will be autopopulated from the sample file in a future PR, handled by EWM item 13209
        self.copyProperties("ResampleX", ["XMin", "XMax"])
        self.declareProperty(
            "XBinWidth",
            0.1,
            validator=FloatBoundedValidator(0.0),
            doc="Bin width for each spectrum",
        )
        self.declareProperty(
            "Scale",
            1.0,
            validator=FloatBoundedValidator(0.0),
            doc="The background will be scaled by this number before being subtracted.",
        )
        # TODO: This field below will be autopopulated from the sample file in a future PR, handled by EWM item 13209
        self.declareProperty(
            "NormaliseBy",
            "Monitor",
            StringListValidator(["None", "Time", "Monitor"]),
            "Normalise to monitor or time. ",
        )
        self.declareProperty(
            MaskWorkspaceProperty(
                "MaskWorkspace",
                "",
                optional=PropertyMode.Optional,
                direction=Direction.Input,
            ),
            doc="The mask from this workspace will be applied before reduction",
        )
        self.declareProperty(
            "MaskAngle",
            Property.EMPTY_DBL,
            "Out of plane (phi) angle above which data will be masked",
        )
        self.declareProperty(
            "Sum",
            False,
            doc="Specifies either single output workspace or output group workspace containing several workspaces.",
        )

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="Output Workspace",
        )
        self.getProperty("OutputWorkspace").setDisableReplaceWSButton(True)
        self.declareProperty(
            FileProperty(
                name="OutputDirectory", defaultValue="~/HFIRPowderReductionOutput", action=FileAction.OptionalSave, extensions=[".dat"]
            )
        )
        self.declareProperty("Overwrite", True, "If True previous file will be overwritten")

        def readIntFromFile(filename, dataset):
            intValue = 0
            with h5py.File(filename, "r") as f:
                intValue = f[dataset][0]
                intValue = intValue.decode("utf-8")
                intValue = int(intValue)
            return intValue

        def readFloatFromFile(filename, dataset):
            floatValue = 0.0
            with h5py.File(filename, "r") as f:
                floatValue = f[dataset][0]
                floatValue = floatValue.decode("utf-8")
                floatValue = float(floatValue)
            return floatValue

        def readArrayFromFile(filename, dataset):
            array = np.array([])
            with h5py.File(filename, "r") as f:
                array = f[dataset][()]
            return array

        def readStringFromFile(filename, dataset):
            with h5py.File(filename, "r") as f:
                value = f[dataset][0]
                if isinstance(value, bytes):
                    return value.decode("utf-8")
                return str(value)

        def checkFilenameforInstrument(algo, currentProp, watchedProp):
            run = watchedProp.value
            instrumentName = ""
            with h5py.File(run[0], "r") as f:
                instrumentName = f["/entry/instrument/name"][0]
                instrumentName = instrumentName.decode("utf-8")
            if "WAND" in instrumentName:
                self.setProperty("Instrument", "WAND^2")
            elif "MIDAS" in instrumentName:
                self.setProperty("Instrument", "MIDAS")
            return True

        self.setPropertySettings("Instrument", SetDefaultWhenProperty("SampleFilename", checkFilenameforInstrument))

        def checkFilenameforWavelength(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            wavelength = 0.0
            wavelength = readFloatFromFile(run[0], "/entry/wavelength")
            logger.information(f"Auto-populating Wavelength: {wavelength} from file: {run[0]}")
            currentProp.value = wavelength
            return True

        self.setPropertySettings("Wavelength", SetDefaultWhenProperty("SampleFilename", checkFilenameforWavelength))

        def checkFilenameforVanadiumDiameter(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            diameter = 0.0
            diameter = readFloatFromFile(run[0], "/entry/vanadium_diameter")
            logger.information(f"Auto-populating VanadiumDiameter: {diameter} from file: {run[0]}")
            currentProp.value = diameter
            return True

        self.setPropertySettings("VanadiumDiameter", SetDefaultWhenProperty("SampleFilename", checkFilenameforVanadiumDiameter))

        def checkFilenameforSampleChemicalFormula(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            try:
                formula = readStringFromFile(run[0], "/entry/sample_chemical_formula")
                logger.information(f"Auto-populating SampleChemicalFormula: {formula} from file: {run[0]}")
                self.setProperty("SampleChemicalFormula", formula)
                return True
            except (OSError, KeyError):
                return False

        self.setPropertySettings("SampleChemicalFormula", SetDefaultWhenProperty("SampleFilename", checkFilenameforSampleChemicalFormula))

        def checkFilenameforSampleCrystalDensity(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            try:
                density = readFloatFromFile(run[0], "/entry/sample_crystal_density")
                logger.information(f"Auto-populating SampleCrystalDensity: {density} from file: {run[0]}")
                currentProp.value = density
                return True
            except (OSError, KeyError):
                return False

        self.setPropertySettings("SampleCrystalDensity", SetDefaultWhenProperty("SampleFilename", checkFilenameforSampleCrystalDensity))

        def checkFilenameforSamplePackingFraction(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            try:
                fraction = readFloatFromFile(run[0], "/entry/sample_packing_fraction")
                logger.information(f"Auto-populating SamplePackingFraction: {fraction} from file: {run[0]}")
                currentProp.value = fraction
                return True
            except (OSError, KeyError):
                return False

        self.setPropertySettings("SamplePackingFraction", SetDefaultWhenProperty("SampleFilename", checkFilenameforSamplePackingFraction))

        def checkFilenameforSampleDiameter(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            try:
                diameter = readFloatFromFile(run[0], "/entry/sample_diameter")
                logger.information(f"Auto-populating SampleDiameter: {diameter} from file: {run[0]}")
                currentProp.value = diameter
                return True
            except (OSError, KeyError):
                return False

        self.setPropertySettings("SampleDiameter", SetDefaultWhenProperty("SampleFilename", checkFilenameforSampleDiameter))

        def checkFilenameforSampleHeight(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            try:
                height = readFloatFromFile(run[0], "/entry/sample_height")
                logger.information(f"Auto-populating SampleHeight: {height} from file: {run[0]}")
                currentProp.value = height
                return True
            except (OSError, KeyError):
                return False

        self.setPropertySettings("SampleHeight", SetDefaultWhenProperty("SampleFilename", checkFilenameforSampleHeight))

        def checkInstrumentforNormaliseBy(algo, currentProp, watchedProp):
            instrument = watchedProp.value
            if instrument == "WAND^2":
                currentProp.value = "Monitor"
            elif instrument == "MIDAS":
                currentProp.value = "Time"
            return True

        self.setPropertySettings("NormaliseBy", SetDefaultWhenProperty("Instrument", checkInstrumentforNormaliseBy))

        def checkFilenameforVanadiumRunNumbers(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            vanadiumRunNumbers = readArrayFromFile(run[0], "/entry/vanadium_run_numbers")
            logger.information(f"Auto-populating VanadiumRunNumbers: {vanadiumRunNumbers} from file: {run[0]}")
            self.setProperty("VanadiumRunNumbers", vanadiumRunNumbers)
            return True

        self.setPropertySettings("VanadiumRunNumbers", SetDefaultWhenProperty("SampleFilename", checkFilenameforVanadiumRunNumbers))

        def checkFilenameforVanadiumIPTS(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            vanadiumIPTS = readIntFromFile(run[0], "/entry/vanadium_ipts")
            logger.information(f"Auto-populating VanadiumIPTS: {vanadiumIPTS} from file: {run[0]}")
            self.setProperty("VanadiumIPTS", vanadiumIPTS)
            return True

        self.setPropertySettings("VanadiumIPTS", SetDefaultWhenProperty("SampleFilename", checkFilenameforVanadiumIPTS))

        def checkFilenameforVanadiumBackgroundIPTS(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            vanadiumBGIPTS = readIntFromFile(run[0], "/entry/vanadium_background_ipts")
            logger.information(f"Auto-populating VanadiumBackgroundIPTS: {vanadiumBGIPTS} from file: {run[0]}")
            self.setProperty("VanadiumBackgroundIPTS", vanadiumBGIPTS)
            return True

        self.setPropertySettings("VanadiumBackgroundIPTS", SetDefaultWhenProperty("SampleFilename", checkFilenameforVanadiumBackgroundIPTS))

        def checkFilenameforVanadiumBackgroundRunNumbers(algo, currentProp, watchedProp):
            run = watchedProp.value
            if not run:
                return False
            vanadiumBGRunNumbers = readArrayFromFile(run[0], "/entry/vanadium_background_run_numbers")
            logger.information(f"Auto-populating VanadiumBackgroundRunNumbers: {vanadiumBGRunNumbers} from file: {run[0]}")
            self.setProperty("VanadiumBackgroundRunNumbers", vanadiumBGRunNumbers)
            return True

        self.setPropertySettings(
            "VanadiumBackgroundRunNumbers", SetDefaultWhenProperty("SampleFilename", checkFilenameforVanadiumBackgroundRunNumbers)
        )

        # Helper function to get sample runs based on IPTS and run numbers
        # Note: this should never be refactored to use FileFinder as we want to force the user to set the IPTS number when using run numbers
        def getRuns():
            runs = []
            sampleIPTS = self.getProperty("SampleIPTS").value
            sampleRunNumbers = self.getProperty("SampleRunNumbers").value
            instrument = self.getProperty("Instrument").value
            if sampleIPTS == Property.EMPTY_INT or len(sampleRunNumbers) == 0 or instrument == "":
                return runs
            else:
                if instrument == "WAND^2":
                    runs = ["/HFIR/HB2C/IPTS-{}/nexus/HB2C_{}.nxs.h5".format(sampleIPTS, run) for run in sampleRunNumbers]
                elif instrument == "MIDAS":
                    runs = ["/HFIR/HB2A/IPTS-{}/nexus/HB2A_{}.nxs.h5".format(sampleIPTS, run) for run in sampleRunNumbers]
            return runs

        def checkRunNumbersforWavelength(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            else:
                wavelength = 0.0
                wavelength = readFloatFromFile(runs[0], "/entry/wavelength")
                logger.information(f"Auto-populating Wavelength: {wavelength} from run numbers: {runs[0]}")
                currentProp.value = wavelength
            return True

        self.setPropertySettings("Wavelength", SetDefaultWhenProperty("Instrument", checkRunNumbersforWavelength))
        self.setPropertySettings("Wavelength", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforWavelength))
        self.setPropertySettings("Wavelength", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforWavelength))

        def checkRunNumbersforVanadiumDiameter(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            else:
                diameter = 0.0
                diameter = readFloatFromFile(runs[0], "/entry/vanadium_diameter")
                logger.information(f"Auto-populating VanadiumDiameter: {diameter} from run numbers: {runs[0]}")
                currentProp.value = diameter

            return True

        self.setPropertySettings("VanadiumDiameter", SetDefaultWhenProperty("Instrument", checkRunNumbersforVanadiumDiameter))
        self.setPropertySettings("VanadiumDiameter", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforVanadiumDiameter))
        self.setPropertySettings("VanadiumDiameter", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforVanadiumDiameter))

        def checkRunNumbersforSampleChemicalFormula(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            try:
                formula = readStringFromFile(runs[0], "/entry/sample_chemical_formula")
                logger.information(f"Auto-populating SampleChemicalFormula: {formula} from run numbers: {runs[0]}")
                self.setProperty("SampleChemicalFormula", formula)
                return True
            except (OSError, KeyError):
                return False

        self.setPropertySettings("SampleChemicalFormula", SetDefaultWhenProperty("Instrument", checkRunNumbersforSampleChemicalFormula))
        self.setPropertySettings("SampleChemicalFormula", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforSampleChemicalFormula))
        self.setPropertySettings(
            "SampleChemicalFormula", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforSampleChemicalFormula)
        )

        def checkRunNumbersforSampleCrystalDensity(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            try:
                density = readFloatFromFile(runs[0], "/entry/sample_crystal_density")
                logger.information(f"Auto-populating SampleCrystalDensity: {density} from run numbers: {runs[0]}")
                currentProp.value = density
                return True
            except (OSError, KeyError):
                return False

        self.setPropertySettings("SampleCrystalDensity", SetDefaultWhenProperty("Instrument", checkRunNumbersforSampleCrystalDensity))
        self.setPropertySettings("SampleCrystalDensity", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforSampleCrystalDensity))
        self.setPropertySettings("SampleCrystalDensity", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforSampleCrystalDensity))

        def checkRunNumbersforSamplePackingFraction(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            try:
                fraction = readFloatFromFile(runs[0], "/entry/sample_packing_fraction")
                logger.information(f"Auto-populating SamplePackingFraction: {fraction} from run numbers: {runs[0]}")
                currentProp.value = fraction
                return True
            except (OSError, KeyError):
                return False

        self.setPropertySettings("SamplePackingFraction", SetDefaultWhenProperty("Instrument", checkRunNumbersforSamplePackingFraction))
        self.setPropertySettings("SamplePackingFraction", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforSamplePackingFraction))
        self.setPropertySettings(
            "SamplePackingFraction", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforSamplePackingFraction)
        )

        def checkRunNumbersforSampleDiameter(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            try:
                diameter = readFloatFromFile(runs[0], "/entry/sample_diameter")
                logger.information(f"Auto-populating SampleDiameter: {diameter} from run numbers: {runs[0]}")
                currentProp.value = diameter
                return True
            except (OSError, KeyError):
                return False

        self.setPropertySettings("SampleDiameter", SetDefaultWhenProperty("Instrument", checkRunNumbersforSampleDiameter))
        self.setPropertySettings("SampleDiameter", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforSampleDiameter))
        self.setPropertySettings("SampleDiameter", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforSampleDiameter))

        def checkRunNumbersforSampleHeight(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            try:
                height = readFloatFromFile(runs[0], "/entry/sample_height")
                logger.information(f"Auto-populating SampleHeight: {height} from run numbers: {runs[0]}")
                currentProp.value = height
                return True
            except (OSError, KeyError):
                return False

        self.setPropertySettings("SampleHeight", SetDefaultWhenProperty("Instrument", checkRunNumbersforSampleHeight))
        self.setPropertySettings("SampleHeight", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforSampleHeight))
        self.setPropertySettings("SampleHeight", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforSampleHeight))

        def checkRunNumbersforVanadiumRunNumbers(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            else:
                vanadiumRunNumbers = readArrayFromFile(runs[0], "/entry/vanadium_run_numbers")
                logger.information(f"Auto-populating VanadiumRunNumbers: {vanadiumRunNumbers} from run numbers: {runs[0]}")
                self.setProperty("VanadiumRunNumbers", vanadiumRunNumbers)

            return True

        self.setPropertySettings("VanadiumRunNumbers", SetDefaultWhenProperty("Instrument", checkRunNumbersforVanadiumRunNumbers))
        self.setPropertySettings("VanadiumRunNumbers", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforVanadiumRunNumbers))
        self.setPropertySettings("VanadiumRunNumbers", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforVanadiumRunNumbers))

        def checkRunNumbersforVanadiumIPTS(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            else:
                vanadiumIPTS = readIntFromFile(runs[0], "/entry/vanadium_ipts")
                logger.information(f"Auto-populating VanadiumIPTS: {vanadiumIPTS} from run numbers: {runs[0]}")
                self.setProperty("VanadiumIPTS", vanadiumIPTS)

            return True

        self.setPropertySettings("VanadiumIPTS", SetDefaultWhenProperty("Instrument", checkRunNumbersforVanadiumIPTS))
        self.setPropertySettings("VanadiumIPTS", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforVanadiumIPTS))
        self.setPropertySettings("VanadiumIPTS", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforVanadiumIPTS))

        def checkRunNumbersforVanadiumBackgroundRunNumbers(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            else:
                vanadiumBGRunNumbers = readArrayFromFile(runs[0], "/entry/vanadium_background_run_numbers")
                logger.information(f"Auto-populating VanadiumBackgroundRunNumbers: {vanadiumBGRunNumbers} from run numbers: {runs[0]}")
                self.setProperty("VanadiumBackgroundRunNumbers", vanadiumBGRunNumbers)

            return True

        self.setPropertySettings(
            "VanadiumBackgroundRunNumbers", SetDefaultWhenProperty("Instrument", checkRunNumbersforVanadiumBackgroundRunNumbers)
        )
        self.setPropertySettings(
            "VanadiumBackgroundRunNumbers", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforVanadiumBackgroundRunNumbers)
        )
        self.setPropertySettings(
            "VanadiumBackgroundRunNumbers", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforVanadiumBackgroundRunNumbers)
        )

        def checkRunNumbersforVanadiumBackgroundIPTS(algo, currentProp, watchedProp):
            runs = getRuns()
            if len(runs) == 0:
                return False
            else:
                vanadiumBGIPTS = readIntFromFile(runs[0], "/entry/vanadium_background_ipts")
                logger.information(f"Auto-populating VanadiumBackgroundIPTS: {vanadiumBGIPTS} from run numbers: {runs[0]}")
                self.setProperty("VanadiumBackgroundIPTS", vanadiumBGIPTS)

            return True

        self.setPropertySettings("VanadiumBackgroundIPTS", SetDefaultWhenProperty("Instrument", checkRunNumbersforVanadiumBackgroundIPTS))
        self.setPropertySettings("VanadiumBackgroundIPTS", SetDefaultWhenProperty("SampleIPTS", checkRunNumbersforVanadiumBackgroundIPTS))
        self.setPropertySettings(
            "VanadiumBackgroundIPTS", SetDefaultWhenProperty("SampleRunNumbers", checkRunNumbersforVanadiumBackgroundIPTS)
        )

    def _checkMetadataConsistency(self, files, field_name):
        """Check that metadata is consistent across multiple run files."""
        if len(files) <= 1:
            return None

        metadata_fields = [
            "/entry/wavelength",
            "/entry/vanadium_diameter",
            "/entry/vanadium_run_numbers",
            "/entry/vanadium_ipts",
            "/entry/vanadium_background_ipts",
            "/entry/vanadium_background_run_numbers",
            "/entry/sample_chemical_formula",
            "/entry/sample_crystal_density",
            "/entry/sample_packing_fraction",
            "/entry/sample_diameter",
            "/entry/sample_height",
        ]

        inconsistencies = []
        for metadata_field in metadata_fields:
            reference_value = None
            try:
                with h5py.File(files[0], "r") as f:
                    if metadata_field in f:
                        reference_value = f[metadata_field][()]
            except (OSError, KeyError):
                continue

            if reference_value is None:
                continue

            for i, file in enumerate(files[1:], start=1):
                try:
                    with h5py.File(file, "r") as f:
                        if metadata_field in f:
                            current_value = f[metadata_field][()]
                            # Compare values, handling numpy arrays
                            if isinstance(reference_value, np.ndarray) and isinstance(current_value, np.ndarray):
                                if not np.array_equal(reference_value, current_value):
                                    inconsistencies.append(f"{metadata_field}: file 0 has {reference_value}, file {i} has {current_value}")
                            elif reference_value != current_value:
                                inconsistencies.append(f"{metadata_field}: file 0 has {reference_value}, file {i} has {current_value}")
                except (OSError, KeyError):
                    continue

        if inconsistencies:
            error_msg = f"Metadata mismatch in {field_name}: " + "; ".join(inconsistencies)
            logger.warning(error_msg)
        return None

    def validateInputs(self):  # noqa: C901
        issues = dict()

        fileNameBool = self.getProperty("SampleFilename").value
        IPTSBool = self.getProperty("SampleIPTS").value
        runNumbersBool = len(self.getProperty("SampleRunNumbers").value)
        if IPTSBool == Property.EMPTY_INT and runNumbersBool == 0 and not fileNameBool:
            issues["SampleFilename"] = "Missing required field: Must specify either SampleFilename or SampleIPTS AND SampleRunNumbers"
            issues["SampleIPTS"] = "Missing required field: Must specify either SampleFilename or SampleIPTS AND SampleRunNumbers"
            issues["SampleRunNumbers"] = "Missing required field: Must specify either SampleFilename or SampleIPTS AND SampleRunNumbers"

        allFields = ["Sample", "Vanadium", "VanadiumBackground", "SampleBackground"]
        for field in allFields:
            fileNameBool = self.getProperty(f"{field}Filename").value
            IPTSBool = self.getProperty(f"{field}IPTS").value
            runNumbersBool = len(self.getProperty(f"{field}RunNumbers").value)
            if (IPTSBool != Property.EMPTY_INT or runNumbersBool != 0) and fileNameBool:
                issues[f"{field}Filename"] = (
                    f"Too many fields filled: Must specify either {field}Filename or {field}IPTS AND {field}RunNumbers"
                )
                if IPTSBool != Property.EMPTY_INT:
                    issues[f"{field}IPTS"] = (
                        f"Too many fields filled: Must specify either {field}Filename or {field}IPTS AND {field}RunNumbers"
                    )
                if runNumbersBool:
                    issues[f"{field}RunNumbers"] = (
                        f"Too many fields filled: Must specify either {field}Filename or {field}IPTS AND {field}RunNumbers"
                    )
            elif (IPTSBool == Property.EMPTY_INT) ^ (runNumbersBool == 0):
                if IPTSBool != Property.EMPTY_INT:
                    issues[f"{field}RunNumbers"] = f"{field}RunNumbers must be provided if {field}IPTS is provided"
                if runNumbersBool:
                    issues[f"{field}IPTS"] = f"{field}IPTS must be provided if {field}RunNumbers is provided"

        instrument = self.getProperty("Instrument").value
        if instrument == "":
            issues["Instrument"] = "Instrument must be provided"

        xMinBool = len(self.getProperty("XMin").value)
        if xMinBool == 0:
            issues["XMin"] = "XMin must be provided"
        xMaxBool = len(self.getProperty("XMax").value)
        if xMaxBool == 0:
            issues["XMax"] = "XMax must be provided"

        size = xMinBool
        xmins = self.getProperty("XMin").value
        xmaxs = self.getProperty("XMax").value
        if (xMinBool != xMaxBool) and (xMinBool and xMaxBool):
            msg = f"XMin and XMax do not define same number of spectra ({xMinBool} != {xMaxBool})"
            issues["XMin"] = msg
            issues["XMax"] = msg
        elif xMinBool and xMaxBool:
            for i in range(size):
                if xmins[i] >= xmaxs[i]:
                    msg = f"XMin ({xmins[i]}) cannot be greater than or equal to XMax ({xmaxs[i]})"
                    issues["XMin"] = msg
                    issues["XMax"] = msg

        wavelength = self.getProperty("Wavelength").value
        if wavelength == Property.EMPTY_DBL:
            issues["Wavelength"] = "Wavelength must be provided"
        vanadiumDiameter = self.getProperty("VanadiumDiameter").value
        if vanadiumDiameter == Property.EMPTY_DBL:
            issues["VanadiumDiameter"] = "VanadiumDiameter must be provided"

        # Validate sample absorption correction properties
        doAttenuation = self.getProperty("DoAttenuationCorrection").value
        doMS = self.getProperty("DoMultipleScatteringCorrection").value
        absoluteUnits = self.getProperty("AbsoluteIntensityUnits").value
        sampleHeight = self.getProperty("SampleHeight").value

        if doMS and sampleHeight <= 0:
            issues["SampleHeight"] = "SampleHeight must be provided when DoMultipleScatteringCorrection is True"

        if absoluteUnits:
            if vanadiumDiameter == Property.EMPTY_DBL or vanadiumDiameter <= 0:
                issues["VanadiumDiameter"] = "VanadiumDiameter must be provided and > 0 for absolute intensity units"
            vanadiumHeight = self.getProperty("VanadiumHeight").value
            if vanadiumHeight <= 0:
                issues["VanadiumHeight"] = "VanadiumHeight must be provided and > 0 for absolute intensity units"
            if sampleHeight <= 0:
                issues["SampleHeight"] = "SampleHeight must be provided for absolute intensity units"

        if doAttenuation:
            if not self.getProperty("SampleChemicalFormula").value:
                issues["SampleChemicalFormula"] = "SampleChemicalFormula is required when DoAttenuationCorrection is True"
            if self.getProperty("SampleCrystalDensity").value == Property.EMPTY_DBL:
                issues["SampleCrystalDensity"] = "SampleCrystalDensity is required when DoAttenuationCorrection is True"
            if self.getProperty("SampleDiameter").value == Property.EMPTY_DBL:
                issues["SampleDiameter"] = "SampleDiameter is required when DoAttenuationCorrection is True"

        # Check metadata consistency for multiple sample runs
        files = []
        filenames = self.getProperty("SampleFilename").value
        if filenames:
            files = list(filenames)
        else:
            # Build file list from IPTS and run numbers
            ipts = self.getProperty("SampleIPTS").value
            run_numbers = self.getProperty("SampleRunNumbers").value
            instrument = self.getProperty("Instrument").value
            if ipts != Property.EMPTY_INT and len(run_numbers) > 0 and instrument:
                if instrument == "WAND^2":
                    files = [f"/HFIR/HB2C/IPTS-{ipts}/nexus/HB2C_{run}.nxs.h5" for run in run_numbers]
                elif instrument == "MIDAS":
                    files = [f"/HFIR/HB2A/IPTS-{ipts}/nexus/HB2A_{run}.nxs.h5" for run in run_numbers]

        if len(files) > 1:
            self._checkMetadataConsistency(files, "Sample")

        if not self.getProperty("Overwrite").value and not self.getProperty("OutputDirectory").isDefault:
            output_dir = self.getProperty("OutputDirectory").value
            if os.path.exists(output_dir):
                issues["OutputDirectory"] = f"Output directory {output_dir} already exists and overwrite is set to False"

        return issues

    def _warn_unset_optional_fields(self):
        """Log warnings for optional fields that are not set (excludes fields already checked by validateInputs)."""
        if not self._has_data_to_load("Vanadium"):
            logger.warning("No vanadium run supplied. Data will not be normalized by vanadium.")
        if not self._has_data_to_load("VanadiumBackground"):
            logger.warning("VanadiumBackground is not set.")
        if not self._has_data_to_load("SampleBackground"):
            logger.warning("SampleBackground is not set.")
        if self.getProperty("MaskWorkspace").value is None:
            logger.warning("MaskWorkspace is not set.")
        if self.getProperty("MaskAngle").value == Property.EMPTY_DBL:
            logger.warning("MaskAngle is not set.")

    def _loadMIDASData(self, filename, ws):
        # Check if the file has the expected fields
        with h5py.File(filename, "r") as f:
            has_entry_fields = (
                "/entry/monitor1/total_counts" in f
                and "/entry/duration" in f
                and "/entry/run_number" in f
                and all(f"/entry/bank{b + 1}_events/event_id" in f for b in range(self.MIDAS_NUM_BANKS))
            )

        if not has_entry_fields:
            # Fall back to the generic MIDAS loader
            self._load_MIDAS(filename, ws)
            return

        data = np.zeros(self.MIDAS_TOTAL_PIXELS, dtype=np.int64)
        with h5py.File(filename, "r") as f:
            monitor_count = f["/entry/monitor1/total_counts"][0]
            duration = f["/entry/duration"][0]
            run_number = f["/entry/run_number"][0]
            for b in range(self.MIDAS_NUM_BANKS):
                data += np.bincount(
                    f["/entry/bank" + str(b + 1) + "_events/event_id"][()],
                    minlength=self.MIDAS_TOTAL_PIXELS,
                )
        CreateWorkspace(
            DataX=[0, 1],
            DataY=data,
            DataE=np.sqrt(data),
            UnitX="Empty",
            YUnitLabel="Counts",
            NSpec=self.MIDAS_TOTAL_PIXELS,
            OutputWorkspace="__tmp_load",
            EnableLogging=False,
        )
        LoadNexusLogs("__tmp_load", Filename=filename, EnableLogging=False)
        AddSampleLog(
            "__tmp_load",
            LogName="monitor_count",
            LogType="Number",
            NumberType="Double",
            LogText=str(monitor_count),
            EnableLogging=False,
        )
        AddSampleLog(
            "__tmp_load",
            LogName="gd_prtn_chrg",
            LogType="Number",
            NumberType="Double",
            LogText=str(monitor_count),
            EnableLogging=False,
        )
        AddSampleLog("__tmp_load", LogName="run_number", LogText=str(run_number), EnableLogging=False)
        AddSampleLog(
            "__tmp_load",
            LogName="duration",
            LogType="Number",
            LogText=str(duration),
            NumberType="Double",
            EnableLogging=False,
        )

        # Use the modified IDF that supports simulated data until we get real MIDAS data
        # LoadInstrument("__tmp_load", InstrumentName="MIDAS", RewriteSpectraMap=True, EnableLogging=False)
        LoadInstrument("__tmp_load", Filename="/SNS/users/nxw/mccode_fixed.xml", RewriteSpectraMap=True, EnableLogging=False)
        # Masking is not used yet, but will be added back later
        # if self.getProperty("ApplyMask").value:
        #     MaskBTP("__tmp_load", Pixel="1,2,511,512", EnableLogging=False)

        RenameWorkspace("__tmp_load", ws, EnableLogging=False)

    def PyExec(self):
        """
        Main execution method following these steps:
        1. Load sample data
        2. Load vanadium and background data
        3. Axis Conversion & Masking
        4. Resampling, Normalization, Calibration & Correction
        5. Output (Summing or Grouping)
        6. Save (optional)
        7. Cleanup
        """
        self._warn_unset_optional_fields()

        # Initialize temp workspace list for cleanup
        self.temp_workspace_list = ["_ws_cal", "_ws_cal_background"]

        outWS = self.getPropertyValue("OutputWorkspace")
        summing = self.getProperty("Sum").value
        self.instrument = self.getProperty("Instrument").value
        binWidth = self.getProperty("XBinWidth").value

        # Step 1: Load sample data
        logger.information("Step 1: Loading sample data")
        sample_workspaces = self._load_sample_data()

        # Step 2: Load vanadium and background data
        logger.information("Step 2: Loading vanadium and background data")
        self.vanadium_ws = self._load_vanadium_data()
        self.vanadium_background_ws = self._load_vanadium_background_data()
        self.sample_background_ws = self._load_sample_background_data()

        # Step 2b: Apply sample absorption correction (before axis conversion, needs detector geometry)
        if self.getProperty("DoAttenuationCorrection").value:
            logger.information("Step 2b: Applying sample absorption correction")
            self._apply_sample_corrections_pre_conversion(sample_workspaces)

        # Step 3: Axis Conversion & Masking
        logger.information("Step 3: Converting axes and applying masks")
        sample_workspaces, sample_masks = self._convert_data(sample_workspaces)

        # Step 4: Resampling, normalization, calibration & correction
        logger.information("Step 4: Resampling, normalization, calibration & correction")
        xMin, xMax = self._locate_global_xlimit(sample_workspaces)

        self._resample_inputs(sample_workspaces, sample_masks, xMin, xMax, binWidth, summing)

        # Step 5: Output (Summing or Grouping)
        logger.information("Step 5: Creating output workspace")
        if summing:
            outWS = SumSpectra(
                InputWorkspace="__ws_conjoined",
                OutputWorkspace=outWS,
                WeightedSum=True,
                MultiplyBySpectra=not bool(self.vanadium_ws),
                EnableLogging=False,
            )
        else:
            if len(sample_workspaces) == 1:
                outWS = RenameWorkspace(InputWorkspace=sample_workspaces[0], OutputWorkspace=outWS)
            else:
                outWS = GroupWorkspaces(InputWorkspaces=sample_workspaces, OutputWorkspace=outWS)

        self.setProperty("OutputWorkspace", outWS)

        if not self.getProperty("OutputDirectory").isDefault:
            # Step 6: Save
            logger.information("Step 6: Saving output to file")
            output_dir = self.getProperty("OutputDirectory").value
            # If directory ends with .dat, user has set the directoy with the browse button and no extra checking is required
            if not output_dir.endswith(".dat"):
                if not output_dir.endswith("/"):
                    output_dir += "/"
                if not output_dir.endswith(outWS.name()):
                    output_dir += outWS.name()
                output_dir += ".dat"
            SaveAscii(InputWorkspace=outWS, Filename=output_dir, EnableLogging=False)
            SaveNexus(InputWorkspace=outWS, Filename=output_dir.replace(".dat", ".nxs"))

        # Step 7: Cleanup
        logger.information("Step 7: Cleaning up temporary workspaces")
        for ws in self.temp_workspace_list:
            if mtd.doesExist(ws):
                DeleteWorkspace(ws, EnableLogging=False)

    def _resample_inputs(self, sample_workspaces, sample_masks, xMin, xMax, binWidth, summing):
        # Apply vanadium absorption correction and background subtraction on raw workspace
        self._apply_vanadium_absorption_correction()

        # Keep original raw workspace names so they can be re-used each iteration
        raw_vanadium_background_ws = self.vanadium_background_ws
        raw_sample_background_ws = self.sample_background_ws

        # Resample all sample workspaces
        for index, (ws_name, mask_name) in enumerate(zip(sample_workspaces, sample_masks)):
            ResampleX(
                InputWorkspace=ws_name,
                OutputWorkspace=ws_name,
                XMin=xMin,
                XMax=xMax,
                NumberBins=int((xMax - xMin) / binWidth),
                EnableLogging=False,
            )

            if self.vanadium_ws:
                self.vanadium_ws = self._resample_vanadium(ws_name, mask_name, xMin, xMax)

            if raw_vanadium_background_ws is not None:
                self.vanadium_background_ws = self._resample_background(
                    raw_vanadium_background_ws, ws_name, mask_name, xMin, xMax, self.vanadium_ws
                )

            Scale(
                InputWorkspace=ws_name,
                OutputWorkspace=ws_name,
                Factor=self._get_scale(self.vanadium_ws) / self._get_scale(ws_name),
                EnableLogging=False,
            )

            if raw_sample_background_ws is not None:
                self.sample_background_ws = self._resample_background(
                    raw_sample_background_ws, ws_name, mask_name, xMin, xMax, self.vanadium_ws
                )

            # Calibration & Normalization (vanadium correction)
            logger.information("Processing calibration (vanadium) data")
            vanadium_corrected = None
            if self.vanadium_ws is not None:
                vanadium_corrected = self._process_vanadium_calibration(
                    self.vanadium_ws,
                    self.vanadium_background_ws,
                )
            ws_name = self._apply_sample_absorption_correction(ws_name, vanadium_corrected, self.sample_background_ws)

            if summing:
                # conjoin
                if index < 1:
                    RenameWorkspace(
                        InputWorkspace=ws_name,
                        OutputWorkspace="__ws_conjoined",
                        EnableLogging=False,
                    )
                else:
                    # this adds to `InputWorkspace1`
                    ConjoinWorkspaces(
                        InputWorkspace1="__ws_conjoined",
                        InputWorkspace2=ws_name,
                        CheckOverlapping=False,
                        EnableLogging=False,
                    )

        # Update the list with potentially renamed workspace
        sample_workspaces[index] = ws_name

    def _load_MIDAS(self, filename, ws):
        Load(Filename=filename, OutputWorkspace=ws, EnableLogging=False)
        if not isinstance(ws, WorkspaceGroup):
            # This is a temp fix for using simulated MIDAS data
            LoadInstrument(ws, Filename="/SNS/users/nxw/mccode_fixed.xml", RewriteSpectraMap=True)

    def _load_WAND_Data(self, filename, ws):
        grouping = self.getProperty("Grouping").value
        apply_mask = self.getProperty("ApplyMask").value
        try:
            LoadWAND(Filename=filename, OutputWorkspace=ws, Grouping=grouping, ApplyMask=apply_mask, EnableLogging=False)
        except RuntimeError:
            logger.warning(f"LoadWAND failed for {filename}, falling back to generic Load")
            Load(Filename=filename, OutputWorkspace=ws, EnableLogging=False)

    def _general_load_data(self, data_type):
        instrument = self.getProperty("Instrument").value

        file_source_map = {
            "MIDAS": "/HFIR/HB2A/IPTS-{ipts}/nexus/HB2A_{run}.nxs.h5",
            "WAND^2": "/HFIR/HB2C/IPTS-{ipts}/nexus/HB2C_{run}.nxs.h5",
        }
        loader_map = {"MIDAS": self._loadMIDASData, "WAND^2": self._load_WAND_Data}
        return self._load_data(data_type, file_source_map[instrument], loader_map[instrument])

    def _load_data(self, data_type, source_template, loader):
        """
        Generic function to load data from files or IPTS/run numbers.

        Args:
            data_type: String specifying the type of data to load.
                       One of: "Sample", "Vanadium", "VanadiumBackground", "SampleBackground"

        Returns:
            list of workspace names, or empty list if nothing to load
        """

        # Get properties based on data type
        filenames = self.getProperty(f"{data_type}Filename").value
        ipts = self.getProperty(f"{data_type}IPTS").value
        run_numbers = self.getProperty(f"{data_type}RunNumbers").value

        # Build file list
        files_to_load = []
        if filenames:
            files_to_load = list(filenames)
        elif ipts != Property.EMPTY_INT and len(run_numbers) > 0:
            files_to_load = [source_template.format(run=run, ipts=ipts) for run in run_numbers]

        if not files_to_load:
            return None

        loaded_workspaces = []
        for i, filename in enumerate(files_to_load):
            temp_ws = f"{data_type.lower()}_{i}"
            self.temp_workspace_list.append(temp_ws)
            loader(filename, temp_ws)

            if self._handle_loaded_workspace(temp_ws, loaded_workspaces):
                continue

            loaded_workspaces.append(temp_ws)

        logger.information(f"Loaded {len(loaded_workspaces)} {data_type.lower()} workspace(s)")
        return loaded_workspaces

    def _handle_loaded_workspace(self, temp_ws, loaded_workspaces):
        """Track a loaded workspace and, if it is a group, expand it into
        loaded_workspaces and return True so the caller can skip further
        per-file processing.  Returns False otherwise."""
        wks = AnalysisDataService.retrieve(temp_ws)
        if isinstance(wks, WorkspaceGroup):
            self.temp_workspace_list.extend(wks.getNames())
            loaded_workspaces.extend(wks.getNames())
            AnalysisDataService.remove(temp_ws)
            return True
        else:
            self.temp_workspace_list.append(temp_ws)
        return False

    def _has_data_to_load(self, data_type):
        """Check whether filenames or IPTS+run numbers are provided for the given data type."""
        filenames = self.getProperty(f"{data_type}Filename").value
        ipts = self.getProperty(f"{data_type}IPTS").value
        run_numbers = self.getProperty(f"{data_type}RunNumbers").value
        return bool(filenames) or (ipts != Property.EMPTY_INT and len(run_numbers) > 0)

    def _load_sample_data(self):
        """Load sample data as individual workspaces."""
        if not self._has_data_to_load("Sample"):
            return []
        return self._general_load_data("Sample")

    def _load_vanadium_data(self):
        """Load vanadium calibration data."""
        if not self._has_data_to_load("Vanadium"):
            return None
        result = self._general_load_data("Vanadium")
        return result[0] if result else None

    def _load_vanadium_background_data(self):
        """Load vanadium background data."""
        if not self._has_data_to_load("VanadiumBackground"):
            return None
        result = self._general_load_data("VanadiumBackground")
        return result[0] if result else None

    def _load_sample_background_data(self):
        """Load sample background data."""
        if not self._has_data_to_load("SampleBackground"):
            return None
        result = self._general_load_data("SampleBackground")
        return result[0] if result else None

    def _apply_vanadium_absorption_correction(self):
        """
        Apply absorption correction and background subtraction to the raw vanadium workspace.

        Implements: Vcorr = (V - VB) * (1 - Δ) / A
        where V and VB are separately normalised to Time or Monitor (unless NormaliseBy=None).

        If VanadiumDiameter == 0, Δ = 0 and A = 1 so Vcorr = V - VB.

        This modifies the raw vanadium workspace in-place before resampling.
        """
        if self.vanadium_ws is None:
            return

        vanadium_ws_name = self.vanadium_ws
        vanadium_bg_ws_name = self.vanadium_background_ws
        vanadium_diameter = self.getProperty("VanadiumDiameter").value

        # Subtract normalised background: Scale VB by V_scale/VB_scale then subtract
        if vanadium_bg_ws_name is not None:
            Scale(
                InputWorkspace=vanadium_bg_ws_name,
                OutputWorkspace=vanadium_bg_ws_name,
                Factor=self._get_scale(vanadium_ws_name) / self._get_scale(vanadium_bg_ws_name),
                EnableLogging=False,
            )
            Minus(
                LHSWorkspace=vanadium_ws_name,
                RHSWorkspace=vanadium_bg_ws_name,
                OutputWorkspace=vanadium_ws_name,
                EnableLogging=False,
            )

        # Apply absorption correction if vanadium diameter is given (> 0)
        if vanadium_diameter > 0:
            wavelength = self.getProperty("Wavelength").value
            vanadium_height = self.getProperty("VanadiumHeight").value
            radius = 0.5 * vanadium_diameter  # cm

            # Set sample material for vanadium
            SetSample(
                InputWorkspace=vanadium_ws_name,
                Material={"ChemicalFormula": "V", "SampleMassDensity": 6.1172},
            )

            # Compute absorption (A) and multiple scattering (Δ) workspaces
            CylinderAbsorptionCW(
                InputWorkspace=vanadium_ws_name,
                Radius=radius,
                Height=vanadium_height,
                Wavelength=wavelength,
                AbsorptionCorrectionMethod="Sabine",
                AbsorptionWorkspace="__vanadium_absorption",
                MultipleScatteringWorkspace="__vanadium_ms",
            )

            # Extract correction values
            A_values = mtd["__vanadium_absorption"].extractY().flatten()
            delta = mtd["__vanadium_ms"].extractY()[0][0]

            logger.information(f"Vanadium absorption correction: Δ = {delta:.6f}, A range = [{A_values.min():.6f}, {A_values.max():.6f}]")

            # Apply correction: V = V * (1 - Δ) / A
            # First scale by (1 - Δ)
            Scale(
                InputWorkspace=vanadium_ws_name,
                OutputWorkspace=vanadium_ws_name,
                Factor=(1 - delta),
                EnableLogging=False,
            )
            # Then divide by absorption workspace A
            Divide(
                LHSWorkspace=vanadium_ws_name,
                RHSWorkspace="__vanadium_absorption",
                OutputWorkspace=vanadium_ws_name,
                EnableLogging=False,
            )

            # Cleanup temporary workspaces
            DeleteWorkspace("__vanadium_absorption", EnableLogging=False)
            DeleteWorkspace("__vanadium_ms", EnableLogging=False)

        # Mark background as already processed
        self.vanadium_background_ws = None

    def _apply_sample_corrections_pre_conversion(self, sample_workspaces):
        """
        Apply sample absorption correction before axis conversion.

        Implements: Scorr = (S - fB*SB) * (1 - ΔS) / AS

        This must be called before axis conversion because CylinderAbsorptionCW
        needs detector 2theta angles from the instrument geometry.
        """
        fB = self.getProperty("SampleBackgroundScaleFactor").value
        do_ms = self.getProperty("DoMultipleScatteringCorrection").value
        wavelength = self.getProperty("Wavelength").value
        sample_formula = self.getProperty("SampleChemicalFormula").value
        crystal_density = self.getProperty("SampleCrystalDensity").value
        packing_fraction = self.getProperty("SamplePackingFraction").value
        sample_diameter = self.getProperty("SampleDiameter").value
        sample_height = self.getProperty("SampleHeight").value

        sample_mass_density = crystal_density * packing_fraction
        sample_radius = 0.5 * sample_diameter  # cm

        sample_bg_ws_name = self.sample_background_ws

        for ws_name in sample_workspaces:
            # Subtract normalised background: S = S - fB * (S_scale/SB_scale) * SB
            if sample_bg_ws_name is not None:
                bg_factor = fB * self._get_scale(ws_name) / self._get_scale(sample_bg_ws_name)
                Scale(
                    InputWorkspace=sample_bg_ws_name,
                    OutputWorkspace="__sample_bg_scaled",
                    Factor=bg_factor,
                    EnableLogging=False,
                )
                self.temp_workspace_list.append("__sample_bg_scaled")
                Minus(
                    LHSWorkspace=ws_name,
                    RHSWorkspace="__sample_bg_scaled",
                    OutputWorkspace=ws_name,
                    EnableLogging=False,
                )
                if mtd.doesExist("__sample_bg_scaled"):
                    DeleteWorkspace("__sample_bg_scaled", EnableLogging=False)

            # Set sample material
            SetSample(
                InputWorkspace=ws_name,
                Material={"ChemicalFormula": sample_formula, "SampleMassDensity": sample_mass_density},
            )

            # Compute absorption (AS) and multiple scattering (ΔS) using CylinderAbsorptionCW
            CylinderAbsorptionCW(
                InputWorkspace=ws_name,
                Radius=sample_radius,
                Height=sample_height if sample_height > 0 else 3.0,
                Wavelength=wavelength,
                MultipleScattering=do_ms,
                AbsorptionCorrectionMethod="Sabine",
                AbsorptionWorkspace="__sample_absorption",
                MultipleScatteringWorkspace="__sample_ms",
            )

            delta_S = mtd["__sample_ms"].extractY()[0][0] if do_ms else 0.0

            logger.information(f"Sample absorption correction for {ws_name}: ΔS = {delta_S:.6f}")

            # Apply correction: S = S * (1 - ΔS) / AS
            Scale(
                InputWorkspace=ws_name,
                OutputWorkspace=ws_name,
                Factor=(1 - delta_S),
                EnableLogging=False,
            )
            Divide(
                LHSWorkspace=ws_name,
                RHSWorkspace="__sample_absorption",
                OutputWorkspace=ws_name,
                EnableLogging=False,
            )

            # Cleanup temporary workspaces
            DeleteWorkspace("__sample_absorption", EnableLogging=False)
            DeleteWorkspace("__sample_ms", EnableLogging=False)

        # Mark sample background as already processed
        self.sample_background_ws = None

    def _compute_fnorm(self):
        """
        Compute the absolute intensity normalization factor fnorm.

        fnorm = 1000/4π * (Ms * sigma_V_scatter * roV * hV *rV^2)/(Mv * fS * roS * hS * rS^2)

        """
        # Vanadium constants
        sigma_V_scatter = 5.08  # barn (total scattering cross-section of V)
        roV = 6.1172  # g/cm³ (mass density of V)
        Mv = 50.94  # g/mol (molar mass of V)

        vanadium_diameter = self.getProperty("VanadiumDiameter").value
        hV = self.getProperty("VanadiumHeight").value
        rV = 0.5 * vanadium_diameter  # cm

        # Sample parameters
        sample_diameter = self.getProperty("SampleDiameter").value
        hS = self.getProperty("SampleHeight").value
        sample_formula = self.getProperty("SampleChemicalFormula").value
        roS = self.getProperty("SampleCrystalDensity").value
        fS = self.getProperty("SamplePackingFraction").value
        rS = 0.5 * sample_diameter  # cm

        # Use a temporary workspace to get the material number density
        from mantid.kernel import MaterialBuilder

        builder = MaterialBuilder()
        builder.setFormula(sample_formula)
        builder.setMassDensity(roS)
        material = builder.build()
        # n_S = material.numberDensity  # formula units/Å³
        Ms = material.relativeMolecularMass()  # g/mol

        wavelength = self.getProperty("Wavelength").value
        sigma_S_scatter = material.totalScatterXSection()  # barn
        sigma_S_absorb = material.absorbXSection(wavelength)  # barn (at given λ)

        logger.information(
            f"Sample parameters: roS = {roS:.4f} g/cm^3, fS = {fS:.4f}, hS = {hS:.4f} cm, rS = {rS:.4f} cm, Ms = {Ms:.4f} g/mol"
        )
        logger.information(
            f"Sample cross-sections (barn): total scatter = {sigma_S_scatter:.4f}, "
            f"absorption (lambda = {wavelength:.4f} A) = {sigma_S_absorb:.4f}"
        )
        logger.information(
            f"Vanadium parameters: roV = {roV:.4f} g/cm^3, hV = {hV:.4f} cm, "
            f"rV = {rV:.4f} cm, Mv = {Mv:.4f} g/mol, sigma_V_scatter = {sigma_V_scatter:.4f} barn"
        )

        # fnorm in barn/sr/f.u., convert to mb/sr/f.u.
        # fnorm = (sigma_V_scatter / (4 * np.pi)) * (n_V * rV**2 * hV) / (n_S * rS**2 * hS)
        fnorm = 1000 / (4 * np.pi) * (Ms * sigma_V_scatter * roV * hV * rV**2) / (Mv * fS * roS * hS * rS**2)

        logger.information(f"Absolute intensity normalization: fnorm = {fnorm:.4f} ")

        return fnorm

    def _process_vanadium_calibration(self, vanadium_ws, vanadium_bg_ws):
        """
        Process vanadium calibration with background subtraction and absorption correction.
        Vcorr = (V - VB) * (1 - Δ) / A

        Note: If _apply_vanadium_absorption_correction was called on the raw workspace,
        the correction has already been applied and vanadium_bg_ws will be None.
        """
        if vanadium_bg_ws is not None:
            Scale(
                InputWorkspace=vanadium_bg_ws,
                OutputWorkspace=vanadium_bg_ws,
                Factor=self._get_scale(vanadium_ws) / self._get_scale(vanadium_bg_ws),
                EnableLogging=False,
            )
            Minus(
                LHSWorkspace=vanadium_ws,
                RHSWorkspace=vanadium_bg_ws,
                OutputWorkspace=vanadium_ws,
                EnableLogging=False,
            )

        return vanadium_ws

    def _apply_sample_absorption_correction(self, sample_ws, vanadium_corrected, sample_bg_ws):
        """
        Apply final corrections to sample data.

        Soutput = s * fnorm * Scorr / Vcorr,
        where s is the user-supplied Scale (default 1), and fnorm is the
        absolute-intensity normalization factor if AbsoluteIntensityUnits=True,
        otherwise fnorm = 1.

        When DoAttenuationCorrection=False, BG subtraction is applied here
        """
        s = self.getProperty("Scale").value
        do_attenuation = self.getProperty("DoAttenuationCorrection").value
        absolute_units = self.getProperty("AbsoluteIntensityUnits").value

        if not do_attenuation:
            # Legacy behavior
            if sample_bg_ws is not None:
                Minus(
                    LHSWorkspace=sample_ws,
                    RHSWorkspace=sample_bg_ws,
                    OutputWorkspace=sample_ws,
                    EnableLogging=False,
                )

        fnorm = self._compute_fnorm() if absolute_units else 1.0

        Scale(
            InputWorkspace=sample_ws,
            OutputWorkspace=sample_ws,
            Factor=s * fnorm,
            EnableLogging=False,
        )

        # Divide by corrected vanadium
        if vanadium_corrected is not None:
            Divide(
                LHSWorkspace=sample_ws,
                RHSWorkspace=vanadium_corrected,
                OutputWorkspace=sample_ws,
                EnableLogging=False,
            )

        return sample_ws

    def _get_scale(self, x):
        """return the scale factor needed during normalization"""
        normaliseBy = self.getProperty("NormaliseBy").value

        if x is None:
            return 1
        else:
            if str(normaliseBy).lower() == "none":
                return 1
            elif str(normaliseBy).lower() == "monitor":
                return mtd[str(x)].run().getProtonCharge()
            elif str(normaliseBy).lower() == "time":
                return mtd[str(x)].run().getLogData("duration").value
            else:
                raise ValueError(f"Unknown normalize type: {normaliseBy}")

    def _convert_data(self, input_workspaces):
        # NOTE:
        # Due to range difference among incoming spectra, a common bin para is needed
        # such that all data can be binned exactly the same way.
        output_workspaces = self._extract_temperatures(input_workspaces)
        mask_workspaces = self._extract_masks(input_workspaces, output_workspaces)

        return output_workspaces, mask_workspaces

    def _extract_temperatures(self, input_workspaces):
        outname = self.getProperty("OutputWorkspace").valueAsStr
        output_workspaces = list()
        for n, in_wksp in enumerate(input_workspaces):
            try:
                temp_val = 0.0
                if self.instrument == "WAND^2":
                    temp_val = 300.0
                    # temp_val = mtd[in_wksp].run().getTimeAveragedValue("HB2C:SE:SampleTemp")
                elif self.instrument == "MIDAS":
                    temp_val = 300.0
                    # temp_val = mtd[in_wksp].run().getTimeAveragedValue("HB2A:SE:SampleTemp")
            except RuntimeError:
                temp_val = 300.0

            if temp_val == 0.0:
                temp_val = 300.0
            temp_val = "{:.1F}".format(temp_val).replace(".", "p")
            out_tmp = f"{outname}{n + 1}_T{temp_val}K"
            output_workspaces.append(out_tmp)
        return output_workspaces

    def _extract_masks(self, input_workspaces, output_workspaces):
        mask = self.getProperty("MaskWorkspace").value
        mask_angle = self.getProperty("MaskAngle").value
        mask_workspaces = []
        for n, (_wksp_in, _wksp_out) in enumerate(zip(input_workspaces, output_workspaces)):
            _wksp_in = str(_wksp_in)
            if mask_angle == Property.EMPTY_DBL:
                self._to_spectrum_axis(_wksp_in, _wksp_out, mask)
                mask_workspaces.append(mask)
            else:
                _mask_n = f"__mask_{n}"  # mask for n-th
                self.temp_workspace_list.append(_mask_n)  # cleanup later

                ExtractMask(InputWorkspace=_wksp_in, OutputWorkspace=_mask_n, EnableLogging=False)
                if mask_angle != Property.EMPTY_DBL:
                    MaskAngle(
                        Workspace=_mask_n,
                        MinAngle=mask_angle,
                        Angle="Phi",
                        EnableLogging=False,
                    )
                if mask is not None:
                    # might be a bug if the mask angle isn't set
                    BinaryOperateMasks(
                        InputWorkspace1=_mask_n,
                        InputWorkspace2=mask,
                        OperationType="OR",
                        OutputWorkspace=_mask_n,
                        EnableLogging=False,
                    )

                self._to_spectrum_axis(_wksp_in, _wksp_out, _mask_n)

                # append to the list of processed workspaces
                mask_workspaces.append(_mask_n)

        return mask_workspaces

    def _to_spectrum_axis(self, workspace_in, workspace_out, mask, instrument_donor=None):
        target = self.getProperty("XUnits").value
        wavelength = self.getProperty("Wavelength").value
        e_fixed = UnitConversion.run("Wavelength", "Energy", wavelength, 0, 0, 0, Elastic, 0)
        _targetMap = {"d-spacing": "ElasticDSpacing", "2Theta": "Theta", "Q": "ElasticQ"}
        target = _targetMap[target]

        ExtractUnmaskedSpectra(
            InputWorkspace=workspace_in,
            OutputWorkspace=workspace_out,
            MaskWorkspace=mask,
            EnableLogging=False,
        )

        if instrument_donor:
            wksp_tmp = workspace_out
        else:
            wksp_tmp = workspace_in

        if isinstance(mtd[wksp_tmp], IEventWorkspace):
            Integration(
                InputWorkspace=wksp_tmp,
                OutputWorkspace=workspace_out,
                EnableLogging=False,
            )

        if instrument_donor:
            CopyInstrumentParameters(
                InputWorkspace=instrument_donor,
                OutputWorkspace=workspace_out,
                EnableLogging=False,
            )

        ConvertSpectrumAxis(
            InputWorkspace=workspace_out,
            OutputWorkspace=workspace_out,
            Target=target,
            EFixed=e_fixed,
            EnableLogging=False,
        )

        self._group_duplicates(workspace_out, target, e_fixed)

        Transpose(
            InputWorkspace=workspace_out,
            OutputWorkspace=workspace_out,
            EnableLogging=False,
        )

        return workspace_out

    def _group_duplicates(self, workspace, target, e_fixed):
        # this checks for any duplicated values in target axis, if
        # so then group them together
        axis_values = mtd[workspace].getAxis(1).extractValues()
        equal_values = axis_values == np.roll(axis_values, -1)
        if np.any(equal_values):
            operator = np.full_like(equal_values, ",", dtype="<U1")
            operator[equal_values] = "+"
            grouping_pattern = "".join(str(n) + op for n, op in enumerate(operator))
            GroupDetectors(InputWorkspace=workspace, OutputWorkspace=workspace, GroupingPattern=grouping_pattern, EnableLogging=False)
            ConvertSpectrumAxis(
                InputWorkspace=workspace,
                OutputWorkspace=workspace,
                Target=target,
                EFixed=e_fixed,
                EnableLogging=False,
            )

    def _locate_global_xlimit(self, workspaces):
        """Find the global bin from all spectrum"""
        # Due to range difference among incoming spectra, a common bin para is needed
        # such that all data can be binned exactly the same way.

        # use the supplied start value
        if self.getProperty("xMin").isDefault:
            _xMin = 1e16
        else:
            _xMin = self.getProperty("XMin").value
        if self.getProperty("xMax").isDefault:
            _xMax = -1e16
        else:
            _xMax = self.getProperty("XMax").value

        # update values based on all workspaces
        for name in workspaces:
            _ws_tmp = mtd[name]
            _xMin = max(_xMin, _ws_tmp.readX(0).min())
            _xMax = min(_xMax, _ws_tmp.readX(0).max())

        return _xMin, _xMax

    def _to_spectrum_axis_resample(self, workspace_in, workspace_out, mask, instrument_donor, x_min, x_max):
        # common part of converting axis
        self._to_spectrum_axis(workspace_in, workspace_out, mask, instrument_donor)

        # rebin the data
        return ResampleX(
            InputWorkspace=workspace_out,
            OutputWorkspace=workspace_out,
            XMin=x_min,
            XMax=x_max,
            NumberBins=int((x_max - x_min) / self.getProperty("XBinWidth").value),
            EnableLogging=False,
        )

    def _resample_vanadium(
        self,
        current_workspace,
        mask_name,
        x_min,
        x_max,
    ):
        """Perform resample on Vanadium"""
        ws_name = "vanadium_0"

        return self._to_spectrum_axis_resample(ws_name, "_ws_cal", mask_name, current_workspace, x_min, x_max)

    def _resample_background(
        self,
        current_background,
        current_workspace,
        mask_name,
        x_min,
        x_max,
        resampled_calibration,
    ):
        """Perform resample on background"""
        outname = str(current_background) + str(current_workspace)
        self.temp_workspace_list.append(outname)

        self._to_spectrum_axis_resample(current_background, outname, mask_name, current_workspace, x_min, x_max)

        return outname


AlgorithmFactory.subscribe(HFIRPowderReduction)
