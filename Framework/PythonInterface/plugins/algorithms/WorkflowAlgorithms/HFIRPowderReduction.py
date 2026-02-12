# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MultipleFileProperty, FileAction, PropertyMode, IEventWorkspace
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
    Divide,
    DeleteWorkspace,
    Scale,
    MaskAngle,
    ExtractMask,
    Minus,
    SumSpectra,
    ExtractUnmaskedSpectra,
    mtd,
    BinaryOperateMasks,
    Integration,
    GroupWorkspaces,
    RenameWorkspace,
    GroupDetectors,
)
import h5py
import numpy as np

logger = Logger(__name__)


class HFIRPowderReduction(DataProcessorAlgorithm):
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
            "XUnits",
            "2Theta",
            StringListValidator(["2Theta", "d-spacing", "Q"]),
            doc="The unit to which spectrum axis is converted to",
        )
        # TODO: These 2 fields below will be autopopulated from the sample file in a future PR, handled by EWM item 13209
        self.copyProperties("ResampleX", ["XMin", "XMax"])
        self.declareProperty(
            "XBinWidth",
            0.0,
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
            "AttenuationmuR",
            Property.EMPTY_DBL,
            doc="muR for sample attenuation correction",
        )
        self.declareProperty(
            "Sum",
            False,
            doc="Specifies either single output workspace or output group workspace containing several workspaces.",
        )

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

        return issues

    def PyExec(self):
        """
        Main execution method following these steps:
        1. Workspace Expansion
        2. Load data files if needed
        3. Axis Conversion & Masking
        4. Resampling
        5. Calibration & Normalization (vanadium processing with absorption correction)
        6. Background Subtraction (sample background)
        7. Absorption Correction (sample)
        8. Summing
        9. Cleanup
        """
        # Initialize temp workspace list for cleanup
        self.temp_workspace_list = ["_ws_cal", "_ws_cal_background"]

        outWS = "OUTPUT"  # self.getPropertyValue("OutputWorkspace")
        summing = self.getProperty("Sum").value
        self.instrument = self.getProperty("Instrument").value
        # numberBins = self.getProperty("XBinWidth").value

        # Step 1: Workspace Expansion or Load Data
        logger.information("Step 1: Loading and expanding workspaces")
        sample_workspaces = self._load_sample_data()

        # Step 2: Load vanadium and background data if provided
        logger.information("Step 2: Loading vanadium and background data")
        vanadium_ws = self._load_vanadium_data()
        vanadium_background_ws = self._load_vanadium_background_data()

        # Step 3: Axis Conversion & Masking
        logger.information("Step 3: Converting axes and applying masks")
        sample_workspaces, sample_masks = self._convert_data(sample_workspaces)

        # Step 4: Resampling - determine common x-range
        logger.information("Step 4: Resampling all data to common binning")
        xMin, xMax = self._locate_global_xlimit(sample_workspaces)

        # Resample all sample workspaces
        for n, (ws_name, mask_name) in enumerate(zip(sample_workspaces, sample_masks)):
            ResampleX(
                InputWorkspace=ws_name,
                OutputWorkspace=ws_name,
                XMin=xMin,
                XMax=xMax,
                NumberBins=1000,
                EnableLogging=False,
            )

            if vanadium_ws is not None:
                vanadium_ws = self._resample_vanadium(ws_name, mask_name, xMin, xMax)
            if vanadium_background_ws is not None:
                vanadium_background_ws = self._resample_vanadium_background(ws_name, mask_name, xMin, xMax)

            Scale(
                InputWorkspace=ws_name,
                OutputWorkspace=ws_name,
                Factor=(1),
                EnableLogging=False,
            )

            # Step 5: Calibration & Normalization (vanadium correction)
            logger.information("Step 5: Processing calibration (vanadium) data")
            if vanadium_ws is not None:
                vanadium_corrected = self._process_vanadium_calibration(
                    vanadium_ws,
                    vanadium_background_ws,
                )
                ws_name = self._apply_sample_absorption_correction(ws_name, vanadium_corrected)
            print("completed step 5")

        # Apply scale factor
        scale_factor = self.getProperty("Scale").value
        if scale_factor != 1.0:
            Scale(
                InputWorkspace=ws_name,
                OutputWorkspace=ws_name,
                Factor=scale_factor,
                EnableLogging=False,
            )

        # Normalize by monitor or time
        self._normalize_workspace(ws_name)

        # Update the list with potentially renamed workspace
        sample_workspaces[n] = ws_name
        # Step 8: Summing or Grouping
        logger.information("Step 8: Creating output workspace")
        if summing:
            # Conjoin all workspaces
            if len(sample_workspaces) > 1:
                RenameWorkspace(
                    InputWorkspace=sample_workspaces[0],
                    OutputWorkspace="__ws_conjoined",
                    EnableLogging=False,
                )
                self.temp_workspace_list.append("__ws_conjoined")

                for ws_name in sample_workspaces[1:]:
                    ConjoinWorkspaces(
                        InputWorkspace1="__ws_conjoined",
                        InputWorkspace2=ws_name,
                        CheckOverlapping=False,
                        EnableLogging=False,
                    )

                # Sum all spectra
                outWS = SumSpectra(
                    InputWorkspace="__ws_conjoined",
                    OutputWorkspace=outWS,
                    WeightedSum=True,
                    MultiplyBySpectra=vanadium_corrected is None,
                    EnableLogging=False,
                )
            else:
                # Single workspace - just sum spectra
                outWS = SumSpectra(
                    InputWorkspace=sample_workspaces[0],
                    OutputWorkspace=outWS,
                    WeightedSum=True,
                    MultiplyBySpectra=vanadium_corrected is None,
                    EnableLogging=False,
                )
        else:
            # Group workspaces
            if len(sample_workspaces) == 1:
                outWS = RenameWorkspace(
                    InputWorkspace=sample_workspaces[0],
                    OutputWorkspace=outWS,
                    EnableLogging=False,
                )
            else:
                outWS = GroupWorkspaces(
                    InputWorkspaces=sample_workspaces,
                    OutputWorkspace=outWS,
                    EnableLogging=False,
                )

        # self.setProperty("OutputWorkspace", outWS)

        # Step 9: Cleanup
        logger.information("Step 9: Cleaning up temporary workspaces")
        for ws in self.temp_workspace_list:
            if mtd.doesExist(ws):
                DeleteWorkspace(ws, EnableLogging=False)

    def _load_data(self, data_type):
        """
        Generic function to load data from files or IPTS/run numbers.

        Args:
            data_type: String specifying the type of data to load.
                       One of: "Sample", "Vanadium", "VanadiumBackground", "SampleBackground"

        Returns:
            Loaded workspace name or None if no data to load
        """
        from mantid.simpleapi import Load, Plus

        # Get properties based on data type
        filenames = self.getProperty(f"{data_type}Filename").value
        ipts = self.getProperty(f"{data_type}IPTS").value
        run_numbers = self.getProperty(f"{data_type}RunNumbers").value
        instrument = self.getProperty("Instrument").value

        # if instrument == "WAND^2":
        #     return LoadWAND(Filename=filenames[0], IPTS=ipts, RunNumbers=run_numbers)

        # Check if there's data to load
        if not filenames and (ipts == Property.EMPTY_INT or len(run_numbers) == 0):
            return None

        # Build file list
        files_to_load = []
        if filenames:
            files_to_load = list(filenames)
        elif ipts != Property.EMPTY_INT and len(run_numbers) > 0:
            if instrument == "WAND^2":
                files_to_load = [f"/HFIR/HB2C/IPTS-{ipts}/nexus/HB2C_{run}.nxs.h5" for run in run_numbers]
            elif instrument == "MIDAS":
                files_to_load = [f"/HFIR/HB2A/IPTS-{ipts}/nexus/HB2A_{run}.nxs.h5" for run in run_numbers]

        if not files_to_load:
            return None

        # Create workspace name based on data type
        ws_name = f"__{data_type.lower()}"
        self.temp_workspace_list.append(ws_name)

        # Load and sum all runs
        for i, filename in enumerate(files_to_load):
            temp_ws = f"__{data_type.lower()}_temp_{i}"
            self.temp_workspace_list.append(temp_ws)
            Load(Filename=filename, OutputWorkspace=temp_ws, EnableLogging=False)
            # if instrument == "MIDAS":
            # CropWorkspace(InputWorkspace=temp_ws, OutputWorkspace='1spectrum', EndWorkspaceIndex=0)
            # AppendSpectra(InputWorkspace1='1spectrum', InputWorkspace2=temp_ws, OutputWorkspace=temp_ws)
            # LoadInstrument(temp_ws, InstrumentName = "MIDAS", RewriteSpectraMap=True)
            # CropWorkspace(InputWorkspace=temp_ws, OutputWorkspace=temp_ws, StartWorkspaceIndex=1)
            # self.temp_workspace_list.append('1spectrum')

            if i == 0:
                RenameWorkspace(InputWorkspace=temp_ws, OutputWorkspace=ws_name, EnableLogging=False)
            else:
                Plus(LHSWorkspace=ws_name, RHSWorkspace=temp_ws, OutputWorkspace=ws_name, EnableLogging=False)

        # Apply grouping if requested (only for non-Sample data types or if implemented for Sample)
        grouping = self.getProperty("Grouping").value
        if grouping != "None":
            ws_name = self._apply_grouping(ws_name, grouping)

        logger.information(f"Loaded {data_type} data into workspace: {ws_name}")
        return ws_name

    def _load_sample_data(self):
        """Load sample data - wrapper for backward compatibility and special handling."""
        from mantid.simpleapi import Load, LoadInstrument, CropWorkspace, AppendSpectra

        filenames = self.getProperty("SampleFilename").value
        ipts = self.getProperty("SampleIPTS").value
        run_numbers = self.getProperty("SampleRunNumbers").value
        instrument = self.getProperty("Instrument").value

        # if instrument == "WAND^2":
        #     return LoadWAND(Filename=filenames[0], IPTS=ipts, RunNumbers=run_numbers)
        #     # return loaded_workspaces

        files_to_load = []

        if filenames:
            files_to_load = list(filenames)
        elif ipts != Property.EMPTY_INT and len(run_numbers) > 0:
            if instrument == "WAND^2":
                files_to_load = [f"/HFIR/HB2C/IPTS-{ipts}/nexus/HB2C_{run}.nxs.h5" for run in run_numbers]
            elif instrument == "MIDAS":
                files_to_load = [f"/HFIR/HB2A/IPTS-{ipts}/nexus/HB2A_{run}.nxs.h5" for run in run_numbers]

        # Load workspaces - sample data may need individual workspace handling
        loaded_workspaces = []
        grouping = self.getProperty("Grouping").value

        for i, filename in enumerate(files_to_load):
            ws_name = f"__sample_{i}"
            self.temp_workspace_list.append(ws_name)
            Load(Filename=filename, OutputWorkspace=ws_name, EnableLogging=False)
            if instrument == "MIDAS":
                CropWorkspace(InputWorkspace=ws_name, OutputWorkspace="1spectrum", EndWorkspaceIndex=0)
                AppendSpectra(InputWorkspace1="1spectrum", InputWorkspace2=ws_name, OutputWorkspace=ws_name)
                LoadInstrument(ws_name, InstrumentName="MIDAS", RewriteSpectraMap=True)
                CropWorkspace(InputWorkspace=ws_name, OutputWorkspace=ws_name, StartWorkspaceIndex=1)
                self.temp_workspace_list.append("1spectrum")

            # Apply grouping if requested
            if grouping != "None":
                ws_name = self._apply_grouping(ws_name, grouping)

            loaded_workspaces.append(ws_name)

        logger.information(f"Loaded {len(loaded_workspaces)} sample workspace(s)")
        return loaded_workspaces

    def _load_vanadium_data(self):
        """Load vanadium calibration data."""
        return self._load_data("Vanadium")

    def _load_vanadium_background_data(self):
        """Load vanadium background data."""
        return self._load_data("VanadiumBackground")

    def _load_sample_background_data(self):
        """Load sample background data."""
        return self._load_data("SampleBackground")

    def _apply_grouping(self, ws_name, grouping):
        """Apply 2x2 or 4x4 pixel grouping."""
        # Implementation depends on specific requirements for WAND/MIDAS
        # This is a placeholder - actual grouping logic would need to be instrument-specific
        logger.information(f"Applying {grouping} grouping to {ws_name}")
        return ws_name

    def _process_vanadium_calibration(self, vanadium_ws, vanadium_bg_ws):
        """
        Process vanadium calibration with background subtraction and absorption correction.
        VCORR = (V - V_B)/(T_0*sigma_inc + sigma_mult)
        """
        T_0 = 1.0  # Placeholder for transmission factor
        sigma_inc = 1  # Placeholder for incoherent scattering cross-section
        sigma_mult = 1  # Placeholder for multiple scattering cross-section

        if vanadium_bg_ws is not None:
            Minus(
                LHSWorkspace=vanadium_ws,
                RHSWorkspace=vanadium_bg_ws,
                OutputWorkspace=vanadium_ws,
                EnableLogging=False,
            )
        denominator = (T_0 * sigma_inc) + sigma_mult

        Scale(
            InputWorkspace=vanadium_ws,
            OutputWorkspace=vanadium_ws,
            Factor=1.0 / denominator,
            EnableLogging=False,
        )

        return vanadium_ws

    def _apply_sample_absorption_correction(self, sample_ws, vanadium_corrected):
        """
        Apply absorption correction.
        CORR = F*(SF)/(T*VCORR)
        """
        F = self.getProperty("Scale").value
        T = 1.0  # Placeholder for transmission factor

        Scale(
            InputWorkspace=sample_ws,
            OutputWorkspace=sample_ws,
            Factor=F,
            EnableLogging=False,
        )
        if vanadium_corrected is not None:
            Scale(
                InputWorkspace=vanadium_corrected,
                OutputWorkspace=vanadium_corrected,
                Factor=T,
                EnableLogging=False,
            )
            Divide(
                LHSWorkspace=sample_ws,
                RHSWorkspace=vanadium_corrected,
                OutputWorkspace=sample_ws,
                EnableLogging=False,
            )
        else:
            Divide(
                LHSWorkspace=sample_ws,
                RHSWorkspace=T,
                OutputWorkspace=sample_ws,
                EnableLogging=False,
            )

        return sample_ws

    def _normalize_workspace(self, ws_name):
        """Normalize workspace by monitor or time."""
        normaliseBy = self.getProperty("NormaliseBy").value

        if normaliseBy.lower() == "none":
            return
        elif normaliseBy.lower() == "monitor":
            norm_factor = mtd[ws_name].run().getProtonCharge()
        elif normaliseBy.lower() == "time":
            from mantid.simpleapi import AddSampleLog

            AddSampleLog(Workspace=ws_name, LogName="duration", LogText="123456", LogType="Number")
            norm_factor = mtd[ws_name].run().getLogData("duration").value
        else:
            logger.warning(f"Unknown normalization type: {normaliseBy}")
            return

        if norm_factor > 0:
            Scale(
                InputWorkspace=ws_name,
                OutputWorkspace=ws_name,
                Factor=1.0 / norm_factor,
                EnableLogging=False,
            )

    def _convert_data(self, input_workspaces):
        mask = self.getProperty("MaskWorkspace").value
        mask_angle = self.getProperty("MaskAngle").value
        outname = "OUTPUT"  # self.getProperty("OutputWorkspace").valueAsStr

        # NOTE:
        # Due to range difference among incoming spectra, a common bin para is needed
        # such that all data can be binned exactly the same way.

        # BEGIN_FOR: located_global_xMin&xMax
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

        return output_workspaces, mask_workspaces

    def _to_spectrum_axis(self, workspace_in, workspace_out, mask, instrument_donor=None):
        target = self.getProperty("XUnits").value
        wavelength = self.getProperty("Wavelength").value
        e_fixed = UnitConversion.run("Wavelength", "Energy", wavelength, 0, 0, 0, Elastic, 0)
        if target == "dSpacing":
            target = "ElasticDSpacing"
        elif target == "2Theta":
            target = "Theta"
        elif target == "Q":
            target = "ElasticQ"

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

        # this checks for any duplicated values in target axis, if
        # so then group them together
        axis_values = mtd[workspace_out].getAxis(1).extractValues()
        equal_values = axis_values == np.roll(axis_values, -1)
        if np.any(equal_values):
            operator = np.full_like(equal_values, ",", dtype="<U1")
            operator[equal_values] = "+"
            grouping_pattern = "".join(str(n) + op for n, op in enumerate(operator))
            GroupDetectors(
                InputWorkspace=workspace_out, OutputWorkspace=workspace_out, GroupingPattern=grouping_pattern, EnableLogging=False
            )
            ConvertSpectrumAxis(
                InputWorkspace=workspace_out,
                OutputWorkspace=workspace_out,
                Target=target,
                EFixed=e_fixed,
                EnableLogging=False,
            )

        Transpose(
            InputWorkspace=workspace_out,
            OutputWorkspace=workspace_out,
            EnableLogging=False,
        )

        return workspace_out

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
        # if both were set there is nothing to do
        if _xMin < _xMax and _xMin < 1e16 and _xMax > -1e16:
            return _xMin, _xMax

        # update values based on all workspaces
        for name in workspaces:
            _ws_tmp = mtd[name]
            _xMin = min(_xMin, _ws_tmp.readX(0).min())
            _xMax = max(_xMax, _ws_tmp.readX(0).max())

        return _xMin, _xMax

    def _to_spectrum_axis_resample(self, workspace_in, workspace_out, mask, instrument_donor, x_min, x_max):
        # common part of converting axis
        self._to_spectrum_axis(workspace_in, workspace_out, mask, instrument_donor)

        # rebin the data
        # number_bins = int(self.getProperty("XBinWidth").value)
        return ResampleX(
            InputWorkspace=workspace_out,
            OutputWorkspace=workspace_out,
            XMin=x_min,
            XMax=x_max,
            NumberBins=1000,
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
        ws_name = "__vanadium"
        # cal = self.getProperty("CalibrationWorkspace").valueAsStr

        return self._to_spectrum_axis_resample(ws_name, "_ws_cal", mask_name, current_workspace, x_min, x_max)

    def _resample_vanadium_background(
        self,
        current_workspace,
        mask_name,
        x_min,
        x_max,
    ):
        """Perform resample on Vanadium"""
        ws_name = "__vanadiumbackground"
        # cal = self.getProperty("CalibrationWorkspace").valueAsStr

        return self._to_spectrum_axis_resample(ws_name, "_ws_cal_background", mask_name, current_workspace, x_min, x_max)


AlgorithmFactory.subscribe(HFIRPowderReduction)
