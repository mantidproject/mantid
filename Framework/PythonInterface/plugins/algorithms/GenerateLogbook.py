# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid import config
from mantid.api import AlgorithmFactory, FileAction, FileProperty, \
    ITableWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, IntArrayOrderedPairsValidator, \
    StringListValidator
from mantid.simpleapi import *

import fnmatch
import h5py
import os
import numpy


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
            facility = self.getPropertyValue('Facility')
            instruments = str(config.getFacility(facility).instruments())
            instrument = self.getPropertyValue('Instrument')
            if instrument not in instruments:
                issues['Instrument'] = 'The instrument {} does not belong to {} facility.'.format(instrument, facility)

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
            if self._numor_range[0] <= numor < self._numor_range[1]:
                file_list.append(os.path.splitext(file)[0])
        return file_list

    def _get_default_entries(self):
        self._metadata_entries = []
        self._metadata_headers = ['run_number']
        tmp_instr = self._instrument + '_tmp'
        LoadEmptyInstrument(InstrumentName=self._instrument, OutputWorkspace=tmp_instr)
        parameters = mtd[tmp_instr].getInstrument()
        try:
            logbook_entries = parameters.getStringParameter('logbook_default_entries')[0]
            logbook_headers = parameters.getStringParameter('logbook_default_headers')[0]
            self._metadata_entries += logbook_entries.split(',')
            self._metadata_headers += logbook_headers.split(',')
        except IndexError:
            raise RuntimeError("The default logbook entries and headers are not defined for {}".format(self._instrument))
        if not self.getProperty('OptionalHeaders').isDefault:
            try:
                logbook_optional_entries = parameters.getStringParameter('logbook_optional_entries')[0]
                logbook_optional_headers = parameters.getStringParameter('logbook_optional_headers')[0]
            except IndexError:
                raise RuntimeError("Optional headers are requested but are not defined for {}.".format(self._instrument))
            else:
                logbook_optional_entries = logbook_optional_entries.split(',')
                logbook_optional_headers = logbook_optional_headers.split(',')
                optional_entries = {header : entry for header, entry in zip(logbook_optional_headers,
                                                                            logbook_optional_entries)}
                requested_headers = self.getPropertyValue('OptionalHeaders').split(',')
                for header in requested_headers:
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
            if self.getProperty('CustomHeaders').isDefault:
                # derive headers from custom entries:
                for entry in logbook_custom_entries:
                    header = entry[entry.rfind('/')+1:]
                    self._metadata_headers.append(header)
            else:
                logbook_custom_headers = self.getPropertyValue('CustomHeaders')
                logbook_custom_headers = logbook_custom_headers.split(',')
                self._metadata_headers += logbook_custom_headers

        DeleteWorkspace(Workspace=tmp_instr)

    def _verify_contains_metadata(self, data_array):
        """Verifies that the rawdata indeed contains the desired meta-data to be logged."""
        # Load empty instrument to access parameters defining metadata entries to be searched
        self._get_default_entries()
        data_path = os.path.join(self._data_directory, data_array[0] + '.nxs')
        with h5py.File(data_path, 'r') as f:
            for entry in self._metadata_entries:
                try:
                    f.get(entry)[0]
                except TypeError:
                    self.log().error("The requested entry: {}, is not present in the raw data. ".format(entry))
                    raise RuntimeError("The requested entry is not present in the raw data.")

    def _prepare_logbook_ws(self):
        logbook_ws = self.getPropertyValue('OutputWorkspace')
        CreateEmptyTableWorkspace(OutputWorkspace=logbook_ws)
        for headline in self._metadata_headers:
            mtd[logbook_ws].addColumn("str", headline)
        return logbook_ws

    def _fill_logbook(self, logbook_ws, data_array):
        """Fills out the logbook with the requested meta-data."""
        n_entries = len(self._metadata_headers)
        for file_name in data_array:
            file_path = os.path.join(self._data_directory, file_name + '.nxs')
            with h5py.File(file_path, 'r') as f:
                rowData = numpy.empty(n_entries, dtype=object)
                rowData[0] = str(file_name)
                for entry_no, entry in enumerate(self._metadata_entries, 1):
                    data = f.get(entry)[0]
                    if isinstance(data, numpy.bytes_):
                        data = data.decode('utf-8')
                        data = data.replace(',', ';') # needed for CSV output
                    rowData[entry_no] = str(data)
                mtd[logbook_ws].addRow(rowData)

    def _store_logbook_as_csv(self, logbook_ws):
        SaveAscii(InputWorkspace=logbook_ws, Filename=self.getProperty('OutputFile'),
                  Separator='CSV')

    def PyExec(self):
        self._data_directory = self.getPropertyValue('Directory')
        self._facility = self.getPropertyValue('Facility')
        self._instrument = self.getPropertyValue('Instrument')
        if self.getProperty('NumorRange').isDefault:
            self._numor_range = [0, float('inf')]
        else:
            self._numor_range = self.getProperty('NumorRange').value
        data_array = self._prepare_file_array()
        self._verify_contains_metadata(data_array)
        logbook_ws = self._prepare_logbook_ws()
        self._fill_logbook(logbook_ws, data_array)
        if not self.getProperty('OutputFile').isDefault:
            self._store_logbook_as_csv(logbook_ws)
        self.setProperty('OutputWorkspace', mtd[logbook_ws])


AlgorithmFactory.subscribe(GenerateLogbook)
