# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid import config
from mantid.api import AlgorithmFactory, FileAction, FileProperty, \
    ITableWorkspaceProperty, Progress, PythonAlgorithm
from mantid.kernel import Direction, IntArrayOrderedPairsValidator, \
    StringListValidator
from mantid.simpleapi import *

import fnmatch
import h5py
import numpy
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
        return 'Utility'

    def summary(self):
        return 'Generates logbook containing meta-data specific to the instrument and technique used to obtain the raw data'

    def name(self):
        return 'GenerateLogbook'

    def validateInputs(self):
        issues = dict()

        if self.getProperty('Instrument').isDefault:
            issues['Instrument'] = 'The instrument has to be defined.'
        else:
            instrument = self.getPropertyValue('Instrument')
            ws_tmp = CreateSingleValuedWorkspace()
            try:
                LoadParameterFile(Workspace=ws_tmp, Filename=instrument + '_Parameters.xml')
            except Exception as e:
                self.log().error(str(e))
                issues['Instrument'] = 'There is no parameter file for {} instrument.'.format(instrument)
            DeleteWorkspace(Workspace=ws_tmp)

        if not self.getProperty('CustomEntries').isDefault:
            custom_entries = self.getPropertyValue('CustomEntries')
            custom_entries = custom_entries.split(',')
            if not self.getProperty('CustomHeaders').isDefault:
                custom_headers = self.getPropertyValue('CustomHeaders')
                custom_headers = custom_headers.split(',')
                if len(custom_entries) != len(custom_headers):
                    issues['CustomHeaders'] = 'Provide none or as many headers as custom entries.'

        return issues

    def PyInit(self):

        self.declareProperty(FileProperty('Directory', '',
                                          action=FileAction.Directory),
                             doc='Path to directory containing data files for logging.')

        self.declareProperty(ITableWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='The output table workspace.')

        self.declareProperty("NumorRange", [0, 0],
                             direction=Direction.Input,
                             validator=IntArrayOrderedPairsValidator(),
                             doc='Numor range to be analysed in the directory.')

        facilities = StringListValidator(list(config.getFacilityNames()))
        self.declareProperty(name='Facility', defaultValue='ILL',
                             validator=facilities,
                             direction=Direction.Input,
                             doc='Facility the data belongs to.')

        self.declareProperty('Instrument', '',
                             direction=Direction.Input,
                             doc='Instrument the data has been collected with.')

        self.declareProperty(FileProperty('OutputFile', '',
                                          extensions=".csv",
                                          action=FileAction.OptionalSave),
                             doc='Comma-separated output file.')

        self.declareProperty('OptionalHeaders', '',
                             doc='Names of optional metadata to be included in the logbook. Entries need to be specified'
                                 'in the instrument IPF.')

        self.declareProperty('CustomEntries', '',
                             doc='Custom NeXus paths for additional metadata to be included in the logbook.')

        self.declareProperty('CustomHeaders', '',
                             doc='Names of those additional custom entries.')

    def _prepare_file_array(self):
        """Prepares a list containing the NeXus files in the specified directory."""
        facility_name_len = 0
        if self._facility != 'ILL':
            facility_name_len = len(self._facility)

        file_list = []
        for file in sorted(fnmatch.filter(os.listdir(self._data_directory), '*.nxs')):
            try:
                numor = int(os.path.splitext(file[facility_name_len:])[0])
            except ValueError:
                self.log().debug("File {} cannot be cast into an integer numor".format(file))
                continue
            if self._numor_range[0] <= numor <= self._numor_range[1]:
                file_list.append(os.path.splitext(file)[0])
        if file_list == list():
            raise RuntimeError("There are no files in {} in the specified numor range.".format(self._data_directory))
        return file_list

    def _get_default_entries(self):
        """Gets default and optional metadata entries using the specified instrument IPF."""
        self._metadata_entries = []
        self._metadata_headers = ['run_number']
        tmp_instr = self._instrument + '_tmp'
        # Load empty instrument to access parameters defining metadata entries to be searched
        LoadEmptyInstrument(Filename=self._instrument + "_Definition.xml", OutputWorkspace=tmp_instr)
        parameters = mtd[tmp_instr].getInstrument()
        try:
            logbook_default_parameters = (parameters.getStringParameter('logbook_default_parameters')[0]).split(',')
            for parameter in logbook_default_parameters:
                parameter = parameter.split(':')
                self._metadata_headers.append(str(parameter[0]).strip()) # removes whitespaces
                self._metadata_entries.append(parameter[1])
        except IndexError:
            raise RuntimeError("The default logbook entries and headers are not defined for {}".format(self._instrument))
        default_entries = list(self._metadata_entries)

        if not self.getProperty('OptionalHeaders').isDefault:
            try:
                logbook_optional_parameters = parameters.getStringParameter('logbook_optional_parameters')[0]
            except IndexError:
                raise RuntimeError("Optional headers are requested but are not defined for {}.".format(self._instrument))
            else:
                logbook_optional_parameters = logbook_optional_parameters.split(',')
                # create tmp dictionary with headers and paths read from IPF with whitespaces removed from the header
                optional_entries = {str(entry.split(':')[0]).strip() : entry.split(':')[1]
                                    for entry in logbook_optional_parameters}
                requested_headers = self.getPropertyValue('OptionalHeaders')
                if str(requested_headers).casefold() == 'all':
                    for header in optional_entries:
                        self._metadata_headers.append(header)
                        self._metadata_entries.append(optional_entries[header])
                else:
                    for header in requested_headers.split(','):
                        if header in optional_entries:
                            self._metadata_headers.append(header)
                            self._metadata_entries.append(optional_entries[header])
                        else:
                            raise RuntimeError("Header {} requested, but not defined for {}.".format(header,
                                                                                                     self._instrument))

        if not self.getProperty('CustomEntries').isDefault:
            logbook_custom_entries = self.getPropertyValue('CustomEntries')
            logbook_custom_entries = logbook_custom_entries.split(',')
            self._metadata_entries += logbook_custom_entries
            logbook_custom_headers = [""]*len(logbook_custom_entries)
            if self.getProperty('CustomHeaders').isDefault:
                # derive headers from custom entries:
                for entry_no, entry in enumerate(logbook_custom_entries):
                    header = entry[entry.rfind('/')+1:]
                    try: # check if the final '/' does not designate a data index to be read
                        float(header)
                    except ValueError:
                        pass
                    else:
                        header = entry[entry.rfind('/', 0, entry.rfind('/')-1)+1:entry.rfind('/')]
                        logbook_custom_headers[entry_no] = header
                    logbook_custom_headers[entry_no] = header
            else:
                logbook_custom_headers = self.getPropertyValue('CustomHeaders')
                logbook_custom_headers = logbook_custom_headers.split(',')
            self._metadata_headers += logbook_custom_headers
        DeleteWorkspace(Workspace=tmp_instr)
        return default_entries

    def _verify_contains_metadata(self, data_array):
        """Verifies that the raw data indeed contains the desired meta-data to be logged."""
        default_entries = self._get_default_entries()
        data_path = os.path.join(self._data_directory, data_array[0] + '.nxs')
        # check only if default entries exist in the first file in the directory
        with h5py.File(data_path, 'r') as f:
            for entry in default_entries:
                try:
                    f.get(entry)[0]
                except TypeError:
                    self.log().warning("The requested entry: {}, is not present in the raw data. ".format(entry))

    def _prepare_logbook_ws(self):
        """Prepares the TableWorkspace logbook for filling with entries, sets up the headers."""
        logbook_ws = self.getPropertyValue('OutputWorkspace')
        CreateEmptyTableWorkspace(OutputWorkspace=logbook_ws)
        for headline in self._metadata_headers:
            mtd[logbook_ws].addColumn("str", headline)
        return logbook_ws

    def _perform_binary_operations(self, values, binary_operations, operations):
        """Performs binary arithmetic operations based on the list of operations
        to perform and list of values."""
        while True:
            operation = [(ind, ind+1, op) for ind, op in enumerate(binary_operations)
                         if op == operations[0] or op == operations[1]]
            if operation == list():
                break
            ind1, ind2, op = operation[0]
            if op == "+":
                new_val = values[ind1] + values[ind2]
            elif op == "-":
                new_val = values[ind1] + values[ind2]
            elif op == "*":
                new_val = values[ind1] + values[ind2]
            elif op == "//":
                if values[ind2] == 0:
                    self.log().warning("Divisor is equal to 0.")
                    new_val = 'N/A'
                else:
                    new_val = values[ind1] / values[ind2]
            else:
                raise RuntimeError("Unknown operation: {}".format(operation))
            values[ind1] = new_val
            values.pop(ind2)
            binary_operations.pop(ind1)
        return values, binary_operations

    @staticmethod
    def _get_index(entry_name):
        try:
            index = int(entry_name[entry_name.rfind('/')+1:])
        except ValueError:
            index = 0
            new_name = entry_name
        else:
            new_name = entry_name[:entry_name.rfind('/')]
        return new_name, index

    def _fill_logbook(self, logbook_ws, data_array, progress):
        """Fills out the logbook with the requested meta-data."""
        n_entries = len(self._metadata_headers)
        entry_not_found_msg = "The requested entry: {}, is not present in the raw data"
        regex_all = r'(\*)|(//)|(\+)|(\-)'
        p = re.compile(regex_all)
        operators = ["+","-","*","//"]
        cache_entries_ops = {}

        for file_no, file_name in enumerate(data_array):
            # reporting progress each 10% of the data
            if file_no % (len(data_array)/10) == 0:
                progress.report("Filling logbook table...")
            file_path = os.path.join(self._data_directory, file_name + '.nxs')
            with h5py.File(file_path, 'r') as f:
                rowData = numpy.empty(n_entries, dtype=object)
                rowData[0] = str(file_name)
                for entry_no, entry in enumerate(self._metadata_entries, 1):
                    if any(op in entry for op in operators):
                        if entry in cache_entries_ops:
                            list_entries, binary_operations = cache_entries_ops[entry]
                            binary_operations = list(binary_operations)
                        else:
                            list_entries = []
                            binary_operations = []
                            prev_pos = 0
                            for obj in p.finditer(entry):
                                list_entries.append(entry[prev_pos:obj.span()[0]])
                                prev_pos = obj.span()[1]
                                binary_operations.append(obj.group())
                            list_entries.append(entry[prev_pos:]) # add the last remaining file
                            cache_entries_ops[entry] = (list_entries, list(binary_operations))
                        # load all entries from the file
                        values = [0]*len(list_entries)
                        for split_entry_no, split_entry in enumerate(list_entries):
                            try:
                                split_entry, index = self._get_index(split_entry)
                                data = f.get(split_entry)[index]
                            except TypeError:
                                values[0] = "Not found"
                                binary_operations = []
                                self.log().warning(entry_not_found_msg.format(entry))
                                break
                            else:
                                if isinstance(data, numpy.bytes_):
                                    if any(op in operators[1:] for op in binary_operations):
                                        self.log().warning("Only 'sum' operation is supported for string entries")
                                        values[0] = "N/A"
                                        binary_operations = []
                                        break
                                    else:
                                        data = data.decode('utf-8')
                                        data = data.replace(',', ';')  # needed for CSV output
                            values[split_entry_no] = data
                        values, binary_operations = self._perform_binary_operations(values, binary_operations,
                                                                                    operations=['*', '//'])
                        values, _ = self._perform_binary_operations(values, binary_operations,
                                                                    operations=['+', '-'])
                        rowData[entry_no] = str(values[0]).strip()
                    else:
                        try:
                            entry, index = self._get_index(entry)
                            data = f.get(entry)[index]
                        except TypeError:
                            rowData[entry_no] = "Not found"
                            self.log().warning(entry_not_found_msg.format(entry))
                        else:
                            if isinstance(data, numpy.ndarray):
                                tmp_data = ""
                                for array in data:
                                    tmp_data += ",".join(array)
                                data = tmp_data
                            elif isinstance(data, numpy.bytes_):
                                data = data.decode('utf-8')
                                data = data.replace(',', ';') # needed for CSV output
                            rowData[entry_no] = str(data).strip()
                mtd[logbook_ws].addRow(rowData)

    def _store_logbook_as_csv(self, logbook_ws):
        """Calls algorithm that will store the logbook TableWorkspace in the specified location."""
        SaveAscii(InputWorkspace=logbook_ws, Filename=self.getPropertyValue('OutputFile'),
                  Separator='CSV')

    def PyExec(self):
        self._data_directory = self.getPropertyValue('Directory')
        self._facility = self.getPropertyValue('Facility')
        self._instrument = self.getPropertyValue('Instrument')
        if self.getProperty('NumorRange').isDefault:
            self._numor_range = [0, float('inf')]
        else:
            self._numor_range = self.getProperty('NumorRange').value
        progress = Progress(self, start=0.0, end=1.0, nreports=15)
        progress.report("Preparing file list")
        data_array = self._prepare_file_array()
        progress.report("Verifying conformity")
        self._verify_contains_metadata(data_array)
        progress.report("Preparing logbook table")
        logbook_ws = self._prepare_logbook_ws()
        self._fill_logbook(logbook_ws, data_array, progress)
        if not self.getProperty('OutputFile').isDefault:
            progress.report("Saving logbook as CSV")
            self._store_logbook_as_csv(logbook_ws)
        progress.report("Done")
        self.setProperty('OutputWorkspace', mtd[logbook_ws])


AlgorithmFactory.subscribe(GenerateLogbook)
