# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MultipleFileProperty, FileAction, PropertyMode
from mantid.kernel import (
    StringListValidator,
    Direction,
    Property,
    FloatBoundedValidator,
    IntArrayProperty,
    SetDefaultWhenProperty,
    Logger,
)
from mantid.dataobjects import MaskWorkspaceProperty
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
        raise NotImplementedError("HFIRPowderReduction algorithm is not yet implemented.")


AlgorithmFactory.subscribe(HFIRPowderReduction)
