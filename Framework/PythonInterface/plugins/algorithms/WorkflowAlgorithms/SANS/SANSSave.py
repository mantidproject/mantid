

# pylint: disable=invalid-name

""" SANSSave algorithm performs saving of SANS reduction data."""

from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode,
                        FileProperty, FileAction, Progress)
from sans.common.enums import (SaveType)
from sans.algorithm_detail.save_workspace import (save_to_file, get_zero_error_free_workspace, file_format_with_append)


class SANSSave(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Save'

    def summary(self):
        return 'Performs saving of reduced SANS data.'

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The workspace which is to be saved.'
                                 ' This workspace needs to be the result of a SANS reduction,'
                                 ' i.e. it can only be 1D or 2D if the second axis is numeric.')

        self.declareProperty(FileProperty("Filename", '', action=FileAction.Save,
                                          extensions=[]),
                             doc="The name of the file which needs to be stored. Note that "
                                 "the actual file type is selected below.")

        self.declareProperty("Nexus", False, direction=Direction.Input,
                             doc="Save as nexus format. "
                                 "Note that if file formats of the same type, e.g. .xml are chosen, then the "
                                 "file format is appended to the file name.")
        self.declareProperty("CanSAS", False, direction=Direction.Input,
                             doc="Save as CanSAS xml format."
                                 "Note that if file formats of the same type, e.g. .xml are chosen, then the "
                                 "file format is appended to the file name.")
        self.declareProperty("NXcanSAS", False, direction=Direction.Input,
                             doc="Save as NXcanSAS format."
                                 "Note that if file formats of the same type, e.g. .xml are chosen, then the "
                                 "file format is appended to the file name.")
        self.declareProperty("NistQxy", False, direction=Direction.Input,
                             doc="Save as Nist Qxy format."
                                 "Note that if file formats of the same type, e.g. .xml are chosen, then the "
                                 "file format is appended to the file name.")
        self.declareProperty("RKH", False, direction=Direction.Input,
                             doc="Save as RKH format."
                                 "Note that if file formats of the same type, e.g. .xml are chosen, then the "
                                 "file format is appended to the file name.")
        self.declareProperty("CSV", False, direction=Direction.Input,
                             doc="Save as CSV format."
                                 "Note that if file formats of the same type, e.g. .xml are chosen, then the "
                                 "file format is appended to the file name.")

        self.setPropertyGroup("Nexus", 'FileFormats')
        self.setPropertyGroup("CanSAS", 'FileFormats')
        self.setPropertyGroup("NXCanSAS", 'FileFormats')
        self.setPropertyGroup("NistQxy", 'FileFormats')
        self.setPropertyGroup("RKH", 'FileFormats')
        self.setPropertyGroup("CSV", 'FileFormats')

        self.declareProperty("UseZeroErrorFree", True, direction=Direction.Input,
                             doc="This allows the user to artificially inflate zero error values.")

    def PyExec(self):
        use_zero_error_free = self.getProperty("UseZeroErrorFree").value
        file_formats = self._get_file_formats()
        file_name = self.getProperty("Filename").value
        workspace = self.getProperty("InputWorkspace").value

        if use_zero_error_free:
            workspace = get_zero_error_free_workspace(workspace)
        progress = Progress(self, start=0.0, end=1.0, nreports=len(file_formats) + 1)
        for file_format in file_formats:
            progress_message = "Saving to {0}.".format(SaveType.to_string(file_format.file_format))
            progress.report(progress_message)
            save_to_file(workspace, file_format, file_name)
        progress.report("Finished saving workspace to files.")

    def validateInputs(self):
        errors = dict()
        # The workspace must be 1D or 2D if the second axis is numeric
        workspace = self.getProperty("InputWorkspace").value
        number_of_histograms = workspace.getNumberHistograms()
        axis1 = workspace.getAxis(1)
        is_first_axis_numeric = axis1.isNumeric()
        if not is_first_axis_numeric and number_of_histograms > 1:
            errors.update({"InputWorkspace": "The input data seems to be 2D. In this case all "
                                             "axes need to be numeric."})

        # Make sure that at least one file format is selected
        file_formats = self._get_file_formats()
        if not file_formats:
            errors.update({"Nexus": "At least one data format needs to be specified."})
            errors.update({"CanSAS": "At least one data format needs to be specified."})
            errors.update({"NXcanSAS": "At least one data format needs to be specified."})
            errors.update({"NistQxy": "At least one data format needs to be specified."})
            errors.update({"RKH": "At least one data format needs to be specified."})
            errors.update({"CSV": "At least one data format needs to be specified."})

        # NistQxy cannot be used with 1D data
        is_nistqxy_selected = self.getProperty("NistQxy").value
        if is_nistqxy_selected and number_of_histograms == 1 and not is_first_axis_numeric:
            errors.update({"NistQxy": "Attempting to save a 1D workspace with NistQxy. NistQxy can store 2D data"
                                      " only. This requires all axes to be numeric."})
        return errors

    def _get_file_formats(self):
        file_types = []
        self._check_file_types(file_types, "Nexus", SaveType.Nexus)
        self._check_file_types(file_types, "CanSAS", SaveType.CanSAS)
        self._check_file_types(file_types, "NXcanSAS", SaveType.NXcanSAS)
        self._check_file_types(file_types, "NistQxy", SaveType.NistQxy)
        self._check_file_types(file_types, "RKH", SaveType.RKH)
        self._check_file_types(file_types, "CSV", SaveType.CSV)

        # Now check if several file formats were selected which save into the same file
        # type, e.g. as Nexus and NXcanSAS do. If this is the case then we want to mark these file formats
        file_formats = []

        # SaveNexusProcessed clashes with SaveNXcanSAS, but we only alter NXcanSAS data
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.Nexus, file_types, [])

        # SaveNXcanSAS clashes with SaveNexusProcessed
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.NXcanSAS, file_types,
                                                            [])

        # SaveNISTDAT clashes with SaveRKH, both can save to .dat
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.NistQxy, file_types, [SaveType.RKH])

        # SaveRKH clashes with SaveNISTDAT, but we alter SaveNISTDAT
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.RKH, file_types, [])

        # SaveCanSAS1D does not clash with anyone
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.CanSAS, file_types, [])

        # SaveCSV does not clash with anyone
        self.add_file_format_with_appended_name_requirement(file_formats, SaveType.CSV, file_types, [])
        return file_formats

    def add_file_format_with_appended_name_requirement(self, file_formats, file_format, file_types,
                                                       clashing_file_formats):
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
            file_formats.append(file_format_with_append(file_format=file_format,
                                                        append_file_format_name=append_name))

    def _check_file_types(self, file_formats, key, to_add):
        if self.getProperty(key).value:
            file_formats.append(to_add)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSSave)
