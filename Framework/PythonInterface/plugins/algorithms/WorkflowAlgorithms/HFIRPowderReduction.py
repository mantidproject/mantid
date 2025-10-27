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
)
from mantid.dataobjects import MaskWorkspaceProperty


class HFIRPowderReduction(DataProcessorAlgorithm):
    def name(self):
        return "HFIRPowderReduction"

    def category(self):
        return "DataHandling\\Nexus"

    def summary(self):
        return "Powder reduction for HFIR instruments"

    def PyInit(self):
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

    def validateInputs(self):
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

        return issues

    def PyExec(self):
        raise NotImplementedError("HFIRPowderReduction algorithm is not yet implemented.")


AlgorithmFactory.subscribe(HFIRPowderReduction)
