# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""SANSSave algorithm performs saving of SANS reduction data."""

from typing import Any

from mantid.api import (
    DataProcessorAlgorithm,
    WorkspaceProperty,
    MatrixWorkspaceProperty,
    AlgorithmFactory,
    PropertyMode,
    FileProperty,
    FileAction,
    Progress,
    WorkspaceGroup,
)
from mantid.kernel import Direction, logger, PropertyManagerProperty
from sans.algorithm_detail.save_workspace import save_to_file, get_zero_error_free_workspace, file_format_with_append
from sans.common.file_information import convert_to_shape
from sans.common.general_functions import get_detector_names_from_instrument
from sans.common.constant_containers import SANSInstrument_string_as_key_NoInstrument
from sans.common.enums import SaveType

FILE_FORMAT_DOC_SUFFIX = (
    "Note that if file formats of the same type, e.g. .xml are chosen, then the file format is appended to the file name."
)


class SANSSave(DataProcessorAlgorithm):
    def category(self):
        return "SANS\\Save"

    def summary(self):
        return "Performs saving of reduced SANS data."

    def checkGroups(self):
        return False

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input),
            doc="The workspace which is to be saved."
            " This workspace needs to be the result of a SANS reduction,"
            " i.e. it can only be 1D or 2D if the second axis is numeric.",
        )

        self.declareProperty(
            FileProperty("Filename", "", action=FileAction.Save, extensions=[]),
            doc="The name of the file which needs to be stored. Note that the actual file type is selected below.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("Transmission", defaultValue="", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The sample transmission workspace. Optional.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("TransmissionCan", defaultValue="", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="The can transmission workspace. Optional.",
        )

        self.declareProperty(
            "Nexus",
            False,
            direction=Direction.Input,
            doc="Save as nexus format. " + FILE_FORMAT_DOC_SUFFIX,
        )
        self.declareProperty(
            "CanSAS",
            False,
            direction=Direction.Input,
            doc="Save as CanSAS xml format. " + FILE_FORMAT_DOC_SUFFIX,
        )
        self.declareProperty(
            "NXcanSAS",
            False,
            direction=Direction.Input,
            doc="Save as NXcanSAS format. " + FILE_FORMAT_DOC_SUFFIX,
        )
        self.declareProperty(
            "PolarizedNXcanSAS",
            False,
            direction=Direction.Input,
            doc="Save in PolarizedNXcanSAS format. " + FILE_FORMAT_DOC_SUFFIX,
        )
        self.declareProperty(
            "NistQxy",
            False,
            direction=Direction.Input,
            doc="Save as Nist Qxy format. " + FILE_FORMAT_DOC_SUFFIX,
        )
        self.declareProperty(
            "RKH",
            False,
            direction=Direction.Input,
            doc="Save as RKH format. " + FILE_FORMAT_DOC_SUFFIX,
        )
        self.declareProperty(
            "CSV",
            False,
            direction=Direction.Input,
            doc="Save as CSV format. " + FILE_FORMAT_DOC_SUFFIX,
        )

        self.setPropertyGroup("Nexus", "FileFormats")
        self.setPropertyGroup("CanSAS", "FileFormats")
        self.setPropertyGroup("NXcanSAS", "FileFormats")
        self.setPropertyGroup("PolarizedNXcanSAS", "FileFormats")
        self.setPropertyGroup("NistQxy", "FileFormats")
        self.setPropertyGroup("RKH", "FileFormats")
        self.setPropertyGroup("CSV", "FileFormats")

        self.declareProperty(
            "UseZeroErrorFree", True, direction=Direction.Input, doc="This allows the user to artificially inflate zero error values."
        )

        self.declareProperty(
            "SampleTransmissionRunNumber",
            "",
            direction=Direction.Input,
            doc="The run number for the Sample Transmission workspace used in the reduction. Can be blank.",
        )
        self.declareProperty(
            "SampleDirectRunNumber",
            "",
            direction=Direction.Input,
            doc="The run number for the Sample Direct workspace used in the reduction. Can be blank.",
        )
        self.declareProperty(
            "CanScatterRunNumber",
            "",
            direction=Direction.Input,
            doc="The run number for the Can Scatter workspace used in the reduction. Can be blank.",
        )
        self.declareProperty(
            "CanDirectRunNumber",
            "",
            direction=Direction.Input,
            doc="The run number for the Can Direct workspace used in the reduction. Can be blank.",
        )
        self.declareProperty(
            "BackgroundSubtractionWorkspace",
            "",
            direction=Direction.Input,
            doc="The workspace used to perform a scaled background subtraction on the reduced workspace. Can be blank.",
        )
        self.declareProperty(
            "BackgroundSubtractionScaleFactor",
            0.0,
            direction=Direction.Input,
            doc="The scale factor the BackgroundSubtractionWorkspace is multiplied by before subtraction. Can be blank.",
        )
        self.declareProperty(
            PropertyManagerProperty("PolarizationProps", dict()),
            doc="A dictionary of properties to values for all of the metadata used in a polarized reduction. Supported "
            "properties are:\n"
            "InputSpinStates (str),\n"
            "PolarizerComponentName (str),\n"
            "AnalyzerComponentName (str),\n"
            "FlipperComponentNames (str, comma separated list),\n"
            "MagneticFieldStrengthLogName (str),\n"
            "MagneticFieldDirection (str, comma separated list of axes),\n"
            "PolarizationTransmissionWorkspaces (dict, component name -> workspace name),\n"
            "PolarizationEfficiencyWorkspaces (dict, component name -> workspace name),\n"
            "PolarizationEmptyCellWorkspaces (dict, component name -> workspace name),\n"
            "ElectricFieldStrengthLogName (str),\n"
            "ElectricFieldDirection (str, comma separated list of axes),\n"
            "InitialCellPolarizations (dict, component name -> value (str)),\n"
            "InitialCellPolarizationErrorWorkspaces (dict, component name -> value (str)),\n"
            "GasPressure (dict, component name -> value (float))\n",
        )

    def PyExec(self):
        use_zero_error_free = self.getProperty("UseZeroErrorFree").value
        file_formats = self._get_file_formats()
        file_name = self.getProperty("Filename").value
        workspace = self.getProperty("InputWorkspace").value

        if isinstance(workspace, WorkspaceGroup):
            additional_properties = self._extract_metadata(workspace.getItem(0))
        else:
            additional_properties = self._extract_metadata(workspace)

        transmission = self.getProperty("Transmission").value
        transmission_can = self.getProperty("TransmissionCan").value

        if use_zero_error_free:
            if isinstance(workspace, WorkspaceGroup):
                group = WorkspaceGroup()
                for i in range(0, workspace.getNumberOfEntries()):
                    group.addWorkspace(get_zero_error_free_workspace(workspace.getItem(i)))
                workspace = group
            else:
                workspace = get_zero_error_free_workspace(workspace)
            if transmission:
                transmission = get_zero_error_free_workspace(transmission)
            if transmission_can:
                transmission_can = get_zero_error_free_workspace(transmission_can)

        additional_properties["Transmission"] = transmission
        additional_properties["TransmissionCan"] = transmission_can

        additional_run_numbers = {
            "SampleTransmissionRunNumber": self.getProperty("SampleTransmissionRunNumber").value,
            "SampleDirectRunNumber": self.getProperty("SampleDirectRunNumber").value,
            "CanScatterRunNumber": self.getProperty("CanScatterRunNumber").value,
            "CanDirectRunNumber": self.getProperty("CanDirectRunNumber").value,
        }

        progress = Progress(self, start=0.0, end=1.0, nreports=len(file_formats) + 1)
        for file_format in file_formats:
            progress_message = "Saving to {0}.".format(file_format.file_format.value)
            progress.report(progress_message)
            progress.report(progress_message)
            try:
                if file_format.file_format is SaveType.POL_NX_CAN_SAS:
                    pol_props = self._fix_polarization_props_types(self.getProperty("PolarizationProps").value)
                    save_to_file(workspace, file_format, file_name, {**additional_properties, **pol_props}, additional_run_numbers)
                else:
                    save_to_file(workspace, file_format, file_name, additional_properties, additional_run_numbers)
            except (RuntimeError, ValueError) as e:
                logger.warning(
                    "Cannot save workspace using SANSSave. "
                    "This workspace needs to be the result of a SANS reduction, "
                    "and must be appropriate for saving 1D or 2D reduced data."
                )
                raise e
        progress.report("Finished saving workspace to files.")

    def _extract_metadata(self, workspace) -> dict[str:Any]:
        additional_properties = {}
        sample = workspace.sample()
        maybe_geometry = convert_to_shape(sample.getGeometryFlag())
        additional_properties["SampleHeight"] = sample.getHeight()
        additional_properties["SampleWidth"] = sample.getWidth()
        additional_properties["SampleThickness"] = sample.getThickness()

        instrument = SANSInstrument_string_as_key_NoInstrument[workspace.getInstrument().getName()]
        additional_properties["DetectorNames"] = ",".join(get_detector_names_from_instrument(instrument))

        if maybe_geometry is not None:
            additional_properties["Geometry"] = maybe_geometry.value

        additional_properties["BackgroundSubtractionWorkspace"] = self.getProperty("BackgroundSubtractionWorkspace").value
        additional_properties["BackgroundSubtractionScaleFactor"] = self.getProperty("BackgroundSubtractionScaleFactor").value

        return additional_properties

    def _fix_polarization_props_types(self, polarization_props) -> dict[str:str]:
        return dict([(key, polarization_props[key].value) for key in polarization_props.keys()])

    def validateInputs(self):
        errors = dict()
        # The workspace must be 1D or 2D if the second axis is numeric
        workspace = self.getProperty("InputWorkspace").value
        if isinstance(workspace, WorkspaceGroup):
            return self._validate_group(errors, workspace)
        if self.getProperty("PolarizedNXcanSAS").value:
            errors.update({"InputWorkspace": "Must be a workspace group to save using PolarizedNXcanSAS"})
            return errors
        return self._validate_workspace(errors, workspace)

    def _validate_group(self, errors, input_workspace):
        for i in range(0, input_workspace.getNumberOfEntries()):
            self._validate_workspace(errors, input_workspace.getItem(i))
        if self.getProperty("PolarizedNXcanSAS").value:
            self._validate_pol_props(errors)
        if (
            self.getProperty("CanSAS").value
            or self.getProperty("NistQxy").value
            or self.getProperty("RKH").value
            or self.getProperty("CSV").value
        ):
            errors.update({"InputWorkspace": "Cannot be a workspace group when saving using CanSAS, NistQxy, RKH, or CSV"})
        return errors

    def _validate_workspace(self, errors, input_workspace):
        number_of_histograms = input_workspace.getNumberHistograms()
        axis1 = input_workspace.getAxis(1)
        is_first_axis_numeric = axis1.isNumeric()
        if not is_first_axis_numeric and number_of_histograms > 1:
            errors.update({"InputWorkspace": "The input data seems to be 2D. In this case all axes need to be numeric."})

        # Make sure that at least one file format is selected
        file_formats = self._get_file_formats()
        if not file_formats:
            errors.update({"Nexus": "At least one data format needs to be specified."})
            errors.update({"CanSAS": "At least one data format needs to be specified."})
            errors.update({"NXcanSAS": "At least one data format needs to be specified."})
            errors.update({"PolarizedNXcanSAS": "At least one data format needs to be specified"})
            errors.update({"NistQxy": "At least one data format needs to be specified."})
            errors.update({"RKH": "At least one data format needs to be specified."})
            errors.update({"CSV": "At least one data format needs to be specified."})

        # NistQxy cannot be used with 1D data
        is_nistqxy_selected = self.getProperty("NistQxy").value
        if is_nistqxy_selected and number_of_histograms == 1 and not is_first_axis_numeric:
            errors.update(
                {
                    "NistQxy": "Attempting to save a 1D workspace with NistQxy. NistQxy can store 2D data"
                    " only. This requires all axes to be numeric."
                }
            )
        return errors

    def _validate_pol_props(self, errors):
        polarization_props = self.getProperty("PolarizationProps").value
        if not polarization_props:
            errors.update({"PolarizationProps": "PolarizationProps must be set to use SavePolarizedNXcanSAS."})
            return
        if "InputSpinStates" not in polarization_props.keys():
            errors.update(
                {"PolarizationProps": "Missing property for SavePolarizedNXcanSAS. These properties are missing: InputSpinStates."}
            )

    def _get_file_formats(self):
        file_types = []
        self._check_file_types(file_types, "Nexus", SaveType.NEXUS)
        self._check_file_types(file_types, "CanSAS", SaveType.CAN_SAS)
        self._check_file_types(file_types, "NXcanSAS", SaveType.NX_CAN_SAS)
        self._check_file_types(file_types, "PolarizedNXcanSAS", SaveType.POL_NX_CAN_SAS)
        self._check_file_types(file_types, "NistQxy", SaveType.NIST_QXY)
        self._check_file_types(file_types, "RKH", SaveType.RKH)
        self._check_file_types(file_types, "CSV", SaveType.CSV)

        # Now check if several file formats were selected which save into the same file
        # type, e.g. as Nexus and NXcanSAS do. If this is the case then we want to mark these file formats
        file_formats = []

        # SaveNexusProcessed clashes with SaveNXcanSAS, but we only alter NXcanSAS data
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.NEXUS, file_types, [])

        # SaveNXcanSAS clashes with SaveNexusProcessed
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.NX_CAN_SAS, file_types, [])

        # SavePolarizedNXcanSAS clashes with SaveNXcanSAS
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.POL_NX_CAN_SAS, file_types, [SaveType.NX_CAN_SAS])

        # SaveNISTDAT clashes with SaveRKH, both can save to .dat
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.NIST_QXY, file_types, [SaveType.RKH])

        # SaveRKH clashes with SaveNISTDAT, but we alter SaveNISTDAT
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.RKH, file_types, [])

        # SaveCanSAS1D does not clash with anyone
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.CAN_SAS, file_types, [])

        # SaveCSV does not clash with anyone
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.CSV, file_types, [])
        return file_formats

    def add_file_format_with_appended_name_requirement(self, file_formats, file_format, file_types, clashing_file_formats):
        """
        This function adds a file format to the file format list. It checks if there are other selected file formats
        which clash with the current file format, e.g. both SaveNexusProcessed and SaveNXcanSAS produce .nxs files

        :param file_formats: A list of file formats
        :param file_format: The file format to be added
        :param file_types: The pure file types
        :param clashing_file_formats: The other file types which might be clashing with the current file type.
        :return:
        """
        if file_format in file_types:
            append_name = False
            for clashing_file_format in clashing_file_formats:
                if clashing_file_format in file_types:
                    append_name = True
            file_formats.append(file_format_with_append(file_format=file_format, append_file_format_name=append_name))

    def _check_file_types(self, file_formats, key, to_add):
        if self.getProperty(key).value:
            file_formats.append(to_add)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSSave)
