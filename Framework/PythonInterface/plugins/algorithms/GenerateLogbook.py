# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid import config
from mantid.api import AlgorithmFactory, FileAction, FileProperty, ITableWorkspaceProperty, Progress, PythonAlgorithm
from mantid.kernel import Direction, IntArrayBoundedValidator, StringListValidator, StringMandatoryValidator
from mantid.simpleapi import *

import fnmatch
import h5py
import numpy as np
import os
import re


class GenerateLogbook(PythonAlgorithm):
    _data_directory = None
    _facility = None
    _instrument = None
    _numor_range = None
    _metadata_headers = None
    _metadata_entries = None

    def category(self):
        return "Utility"

    def summary(self):
        return "Generates logbook containing meta-data specific to the instrument and technique used to obtain the raw data"

    def name(self):
        return "GenerateLogbook"

    def validateInputs(self):
        issues = dict()

        instrument = self.getPropertyValue("Instrument")
        ws_tmp = CreateSingleValuedWorkspace()
        try:
            LoadParameterFile(Workspace=ws_tmp, Filename=instrument + "_Parameters.xml")
        except Exception as e:
            self.log().error(str(e))
            issues["Instrument"] = "There is no parameter file for {} instrument.".format(instrument)
        DeleteWorkspace(Workspace=ws_tmp)

        if not self.getProperty("NumorRange").isDefault:
            numor_range = self.getProperty("NumorRange").value
            if len(numor_range) < 2:
                issues["NumorRange"] = "Please provide both bottom and upper numor limits."
            if numor_range[0] > numor_range[-1]:
                issues["NumorRange"] = "The upper limit must be larger than the bottom one."

        if not self.getProperty("CustomEntries").isDefault:
            custom_entries = self.getPropertyValue("CustomEntries")
            custom_entries = custom_entries.split(",")
            if not self.getProperty("CustomHeaders").isDefault:
                custom_headers = self.getPropertyValue("CustomHeaders")
                custom_headers = custom_headers.split(",")
                if len(custom_entries) != len(custom_headers):
                    issues["CustomHeaders"] = "Provide none or as many headers as custom entries."

        return issues

    def PyInit(self):
        self.declareProperty(
            FileProperty("Directory", "", action=FileAction.Directory), doc="Path to directory containing data files for logging."
        )

        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The output table workspace.")

        self.declareProperty(
            "NumorRange",
            [0, 0],
            direction=Direction.Input,
            validator=IntArrayBoundedValidator(lower=0),
            doc="Numor range or a list of numors to be analysed in the directory.",
        )

        facilities = StringListValidator(list(config.getFacilityNames()))
        self.declareProperty(
            name="Facility", defaultValue="ILL", validator=facilities, direction=Direction.Input, doc="Facility the data belongs to."
        )

        self.declareProperty(
            "Instrument",
            "",
            validator=StringMandatoryValidator(),
            direction=Direction.Input,
            doc="Instrument the data has been collected with.",
        )

        self.declareProperty(
            FileProperty("OutputFile", "", extensions=".csv", action=FileAction.OptionalSave), doc="Comma-separated output file."
        )

        self.declareProperty(
            "OptionalHeaders",
            "",
            doc="Names of optional metadata to be included in the logbook. Entries need to be specified" "in the instrument IPF.",
        )

        self.declareProperty("CustomEntries", "", doc="Custom NeXus paths for additional metadata to be included in the logbook.")

        self.declareProperty("CustomHeaders", "", doc="Names of those additional custom entries.")

    def _prepare_file_array(self):
        """Prepares a list containing the NeXus files in the specified directory."""
        instrument_name_len = 0
        if self._facility != "ILL":
            instrument_name_len = len(self._instrument)

        file_list = []
        for file in sorted(fnmatch.filter(os.listdir(self._data_directory), "*.nxs")):
            try:
                numor = int(os.path.splitext(file[instrument_name_len:])[0])
                if self._numor_range is None or numor in self._numor_range:
                    file_list.append(os.path.splitext(file)[0])
            except (ValueError, OverflowError):
                self.log().debug("File {} cannot be cast into an integer numor".format(file))
                continue
        if file_list == list():
            raise RuntimeError("There are no files in {} with specified numors.".format(self._data_directory))
        return file_list

    def _get_optional_entries(self, parameters):
        try:
            logbook_optional_parameters = parameters.getStringParameter("logbook_optional_parameters")[0]
        except IndexError:
            self.log().warning("Optional headers are requested but are not defined for {}.".format(self._instrument))
            return
        else:
            logbook_optional_parameters = logbook_optional_parameters.split(",")
            # create tmp dictionary with headers and paths read from IPF with whitespaces removed from the header
            optional_entries = dict()
            for entry in logbook_optional_parameters:
                optional_entry = entry.split(":")
                if len(optional_entry) == 1:
                    self.log().warning("Optional header {} is requested but is not properly defined.".format(optional_entry[0]))
                    continue
                if len(optional_entry) < 3:
                    optional_entry.append("s")
                optional_entries[(optional_entry[2], str(optional_entry[0]).strip())] = optional_entry[1]
            requested_headers = self.getPropertyValue("OptionalHeaders")
            if str(requested_headers).casefold() == "all":
                for type, header in optional_entries:
                    self._metadata_headers.append((type, header))
                    self._metadata_entries.append(optional_entries[(type, header)])
            else:
                for header in requested_headers.split(","):
                    for type in ["s", "d", "f"]:
                        if (type, header) in optional_entries:
                            self._metadata_headers.append((type, header))
                            self._metadata_entries.append(optional_entries[(type, header)])
                            break
                    if (
                        ("s", header) not in optional_entries
                        and ("d", header) not in optional_entries
                        and ("f", header) not in optional_entries
                    ):
                        self.log().warning("Header {} requested, but not defined for {}.".format(header, self._instrument))

    def _get_custom_entries(self):
        logbook_custom_entries = self.getPropertyValue("CustomEntries")
        logbook_custom_entries = logbook_custom_entries.split(",")
        for entry in logbook_custom_entries:
            self._metadata_entries.append(entry.split(":")[0])
        logbook_custom_headers = [""] * len(logbook_custom_entries)
        operators = ["+", "-", "*", "//"]
        columnType = "s"
        if self.getProperty("CustomHeaders").isDefault:
            # derive headers from custom entries:
            for entry_no, entry in enumerate(logbook_custom_entries):
                entry_content = entry.split(":")
                if len(entry_content) > 1:
                    columnType = entry_content[1]
                if any(op in entry_content[0] for op in operators):
                    list_entries, binary_operations = self._process_regex(entry_content[0])
                    header = ""
                    for split_entry_no, split_entry in enumerate(list_entries):
                        # always use two strings around the final '/' for more informative header
                        partial_header = split_entry[split_entry.rfind("/", 0, split_entry.rfind("/") - 1) + 1 :]
                        header += partial_header
                        header += binary_operations[split_entry_no] if split_entry_no < len(binary_operations) else ""
                    logbook_custom_headers[entry_no] = (columnType, header)
                else:
                    # always use two strings around the final '/' for more informative header
                    logbook_custom_headers[entry_no] = (
                        columnType,
                        (entry_content[0])[entry_content[0].rfind("/", 0, entry_content[0].rfind("/") - 1) + 1 :],
                    )
        else:
            logbook_custom_headers = self.getPropertyValue("CustomHeaders")
            logbook_custom_headers = [(columnType, header) for header in logbook_custom_headers.split(",")]
        return logbook_custom_headers

    def _get_entries(self):
        """Gets default and optional metadata entries using the specified instrument IPF."""
        self._metadata_entries = []
        self._metadata_headers = [("s", "file_name")]
        tmp_instr = self._instrument + "_tmp"
        # Load empty instrument to access parameters defining metadata entries to be searched
        LoadEmptyInstrument(Filename=self._instrument + "_Definition.xml", OutputWorkspace=tmp_instr)
        parameters = mtd[tmp_instr].getInstrument()
        try:
            logbook_default_parameters = (parameters.getStringParameter("logbook_default_parameters")[0]).split(",")
            for parameter in logbook_default_parameters:
                parameter = parameter.split(":")
                if len(parameter) < 3:
                    parameter.append("s")
                # type, header, strip removes whitespaces
                self._metadata_headers.append((parameter[2], str(parameter[0]).strip()))
                self._metadata_entries.append(parameter[1])
        except IndexError:
            raise RuntimeError("The default logbook entries and headers are not defined for {}".format(self._instrument))
        default_entries = list(self._metadata_entries)

        if not self.getProperty("OptionalHeaders").isDefault:
            self._get_optional_entries(parameters)

        if not self.getProperty("CustomEntries").isDefault:
            logbook_custom_headers = self._get_custom_entries()
            self._metadata_headers += logbook_custom_headers
        DeleteWorkspace(Workspace=tmp_instr)
        return default_entries

    def _verify_contains_metadata(self, data_array):
        """Verifies that the raw data indeed contains the desired meta-data to be logged."""
        default_entries = self._get_entries()
        data_path = os.path.join(self._data_directory, data_array[0] + ".nxs")
        # check only if default entries exist in the first file in the directory
        with h5py.File(data_path, "r") as f:
            for entry in default_entries:
                try:
                    f.get(entry)[0]
                except TypeError:
                    self.log().warning("The requested entry: {}, is not present in the raw data. ".format(entry))

    def _prepare_logbook_ws(self):
        """Prepares the TableWorkspace logbook for filling with entries, sets up the headers."""
        logbook_ws = self.getPropertyValue("OutputWorkspace")
        CreateEmptyTableWorkspace(OutputWorkspace=logbook_ws)
        type_dict = {"s": "str", "d": "int", "f": "float"}
        for header_type, headline in self._metadata_headers:
            mtd[logbook_ws].addColumn(type_dict[header_type], headline)
        return logbook_ws

    def _perform_binary_operations(self, values, binary_operations, operations):
        """Performs binary arithmetic operations based on the list of operations
        to perform and list of values."""
        while True:
            operation = [(ind, ind + 1, op) for ind, op in enumerate(binary_operations) if op in operations]
            if operation == list():
                break
            ind1, ind2, op = operation[0]
            if values[ind2] is None or values[ind2] is str():
                if op != "+" or op == "+" and values[ind1] in [None, str()]:
                    values[ind1] = "N/A"
                values.pop(ind2)
                binary_operations.pop(ind1)
                continue
            if op == "+":
                padding = 0
                if isinstance(values[ind1], str):
                    padding = " "
                new_val = values[ind1] + padding + values[ind2]
            elif op == "-":
                new_val = values[ind1] - values[ind2]
            elif op == "*":
                new_val = values[ind1] * values[ind2]
            elif op == "//":
                try:
                    new_val = values[ind1] / values[ind2]
                except (RuntimeWarning, TypeError) as e:
                    self.log().warning("Division error: {}".format(str(e)))
                    new_val = "N/A"
            else:
                raise RuntimeError("Unknown operation: {}".format(operation))
            if type(new_val) is str:
                new_val = new_val.strip()
            values[ind1] = new_val
            values.pop(ind2)
            binary_operations.pop(ind1)
        return values, binary_operations

    @staticmethod
    def _get_index(entry_name):
        try:
            index = int(entry_name[entry_name.rfind("/") + 1 :])
        except ValueError:
            index = 0
            new_name = entry_name
        else:
            new_name = entry_name[: entry_name.rfind("/")]
        return new_name, index

    @staticmethod
    def _process_regex(entry):
        regex_all = r"(\*)|(//)|(\+)|(\-)"
        p = re.compile(regex_all)
        list_entries = []
        binary_operations = []
        prev_pos = 0
        for obj in p.finditer(entry):
            list_entries.append(entry[prev_pos : obj.span()[0]])
            prev_pos = obj.span()[1]
            binary_operations.append(obj.group())
        list_entries.append(entry[prev_pos:])  # add the last remaining file
        return list_entries, binary_operations

    @staticmethod
    def _perform_cast(data, type):
        if type == "f":
            try:
                data = float(data)
            except ValueError:
                data = np.nan
        elif type == "d":
            try:
                data = int(data)
            except ValueError:
                data = -99999
        elif type == "s":
            data = str(data)
        return data

    def _fill_logbook(self, logbook_ws, data_array, progress):
        """Fills out the logbook with the requested meta-data."""
        n_entries = len(self._metadata_headers)
        entry_not_found_msg = "The requested entry: {}, is not present in the raw data"
        operators = ["+", "-", "*", "//"]
        cache_entries_ops = {}

        for file_no, file_name in enumerate(data_array):
            # reporting progress each 10% of the data
            if file_no % (len(data_array) / 10) == 0:
                progress.report("Filling logbook table...")
            file_path = os.path.join(self._data_directory, file_name + ".nxs")

            with h5py.File(file_path, "r") as f:
                rowData = np.empty(n_entries, dtype=object)
                rowData[0] = str(file_name)
                for entry_no, entry in enumerate(self._metadata_entries, 1):
                    if any(op in entry for op in operators):
                        if entry in cache_entries_ops:
                            list_entries, binary_operations = cache_entries_ops[entry]
                            binary_operations = binary_operations.copy()
                        else:
                            list_entries, binary_operations = self._process_regex(entry)
                            cache_entries_ops[entry] = (list_entries, list(binary_operations))
                        # load all entries from the file
                        values = [0] * len(list_entries)
                        for split_entry_no, split_entry in enumerate(list_entries):
                            try:
                                split_entry, index = self._get_index(split_entry)
                                data = f.get(split_entry)[index]
                            except TypeError:
                                values[split_entry_no] = ""
                                self.log().warning(entry_not_found_msg.format(entry))
                                continue
                            else:
                                if isinstance(data, np.bytes_):
                                    if any(op in operators[1:] for op in binary_operations):
                                        self.log().warning("Only 'sum' operation is supported for string entries")
                                        values[0] = "N/A"
                                        binary_operations = []
                                        break
                                    else:
                                        data = data.decode("utf-8")
                                        data = data.replace(",", ";")  # needed for CSV output
                            values[split_entry_no] = data
                        values, binary_operations = self._perform_binary_operations(values, binary_operations, operations=["*", "//"])
                        values, _ = self._perform_binary_operations(values, binary_operations, operations=["+", "-"])
                        if isinstance(values, np.ndarray):
                            tmp_data = ""
                            for value in values[0]:
                                tmp_data += str(value) + ","
                            rowData[entry_no] = tmp_data[:-1]
                        else:
                            data = self._perform_cast(values[0], self._metadata_headers[entry_no][0])
                            rowData[entry_no] = data
                    else:
                        try:
                            entry, index = self._get_index(entry)
                            data = f.get(entry)[index]
                        except TypeError:
                            data = "Not found"
                            self.log().warning(entry_not_found_msg.format(entry))

                        if isinstance(data, np.ndarray):
                            tmp_data = ""
                            for array in data:
                                tmp_data += ",".join(array)
                            data = tmp_data
                        elif isinstance(data, np.bytes_):
                            data = data.decode("utf-8")
                            data = str(data.replace(",", ";")).strip()  # needed for CSV output
                        data = self._perform_cast(data, self._metadata_headers[entry_no][0])
                        rowData[entry_no] = data
                mtd[logbook_ws].addRow(rowData)

    def _store_logbook_as_csv(self, logbook_ws):
        """Calls algorithm that will store the logbook TableWorkspace in the specified location."""
        SaveAscii(InputWorkspace=logbook_ws, Filename=self.getPropertyValue("OutputFile"), Separator="CSV")

    def PyExec(self):
        self._data_directory = self.getPropertyValue("Directory")
        self._facility = self.getPropertyValue("Facility")
        self._instrument = self.getPropertyValue("Instrument")
        if not self.getProperty("NumorRange").isDefault:
            self._numor_range = self.getProperty("NumorRange").value
        progress = Progress(self, start=0.0, end=1.0, nreports=15)
        progress.report("Preparing file list")
        data_array = self._prepare_file_array()
        progress.report("Verifying conformity")
        self._verify_contains_metadata(data_array)
        progress.report("Preparing logbook table")
        logbook_ws = self._prepare_logbook_ws()
        self._fill_logbook(logbook_ws, data_array, progress)
        if not self.getProperty("OutputFile").isDefault:
            progress.report("Saving logbook as CSV")
            self._store_logbook_as_csv(logbook_ws)
        progress.report("Done")
        self.setProperty("OutputWorkspace", mtd[logbook_ws])


AlgorithmFactory.subscribe(GenerateLogbook)
