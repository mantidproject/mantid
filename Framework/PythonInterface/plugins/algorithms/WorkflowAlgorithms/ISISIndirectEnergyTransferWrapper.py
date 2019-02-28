# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init,deprecated-module
from __future__ import (absolute_import, division, print_function)

from mantid.kernel import (Direction, FloatArrayProperty, FloatBoundedValidator, IntArrayMandatoryValidator,
                           IntArrayProperty, Property, StringArrayProperty, StringListValidator)
from mantid.api import (AlgorithmFactory, AnalysisDataService, DataProcessorAlgorithm,  FileAction, FileProperty,
                        PropertyMode, WorkspaceGroupProperty, WorkspaceProperty)


def exists_in_ads(workspace_name):
    return AnalysisDataService.doesExist(workspace_name)


def get_ads_workspace(workspace_name):
    return AnalysisDataService.retrieve(workspace_name) if exists_in_ads(workspace_name) else None


def string_or_none(string):
    return string if string != '' else None


def workspace_or_none(workspace_name):
    return get_ads_workspace(workspace_name) if workspace_name != '' else None


def elements_or_none(elements):
    return elements if len(elements) != 0 else None


class ISISIndirectEnergyTransferWrapper(DataProcessorAlgorithm):

    def category(self):
        return 'Workflow\\Inelastic;Inelastic\\Indirect'

    def seeAlso(self):
        return ["ISISIndirectEnergyTransfer"]

    def summary(self):
        return 'Runs an energy transfer reduction for an inelastic indirect geometry instrument. It is a wrapper for ' \
               'the ISISIndirectEnergyTransfer algorithm.'

    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files')

        self.declareProperty(name='SumFiles', defaultValue=False,
                             doc='Toggle input file summing or sequential processing')

        self.declareProperty(name='LoadLogFiles', defaultValue=True,
                             doc='Load log files when loading runs')

        self.declareProperty(WorkspaceProperty('CalibrationWorkspace', '',
                                               direction=Direction.Input,
                                               optional=PropertyMode.Optional),
                             doc='Workspace containing calibration data')

        # Instrument configuration properties
        self.declareProperty(name='Instrument', defaultValue='',
                             validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA', 'TFXA']),
                             doc='Instrument used during run.')
        self.declareProperty(name='Analyser', defaultValue='',
                             validator=StringListValidator(['graphite', 'mica', 'fmica']),
                             doc='Analyser bank used during run.')
        self.declareProperty(name='Reflection', defaultValue='',
                             validator=StringListValidator(['002', '004', '006']),
                             doc='Reflection number for instrument setup during run.')

        self.declareProperty(name='Efixed', defaultValue=Property.EMPTY_DBL,
                             validator=FloatBoundedValidator(0.0),
                             doc='Overrides the default Efixed value for the analyser/reflection selection.')

        self.declareProperty(IntArrayProperty(name='SpectraRange', values=[0, 1],
                                              validator=IntArrayMandatoryValidator()),
                             doc='Comma separated range of spectra number to use.')
        self.declareProperty(FloatArrayProperty(name='BackgroundRange'),
                             doc='Range of background to subtract from raw data in time of flight.')
        self.declareProperty(name='RebinString', defaultValue='',
                             doc='Rebin string parameters.')
        self.declareProperty(name='DetailedBalance', defaultValue=Property.EMPTY_DBL,
                             doc='')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0,
                             doc='Factor by which to scale result.')
        self.declareProperty(name='FoldMultipleFrames', defaultValue=True,
                             doc='Folds multiple framed data sets into a single workspace.')

        # Spectra grouping options
        self.declareProperty(name='GroupingMethod', defaultValue='IPF',
                             validator=StringListValidator(['Individual', 'All', 'File', 'Workspace', 'IPF', 'Custom']),
                             doc='Method used to group spectra.')
        self.declareProperty(WorkspaceProperty('GroupingWorkspace', '',
                                               direction=Direction.Input,
                                               optional=PropertyMode.Optional),
                             doc='Workspace containing spectra grouping.')
        self.declareProperty(name='GroupingString', defaultValue='',
                             direction=Direction.Input,
                             doc='Spectra to group as string')
        self.declareProperty(FileProperty('MapFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['.map']),
                             doc='Workspace containing spectra grouping.')

        # Output properties
        self.declareProperty(name='UnitX', defaultValue='DeltaE',
                             validator=StringListValidator(['DeltaE', 'DeltaE_inWavenumber']),
                             doc='X axis units for the result workspace.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='Workspace group for the resulting workspaces.')

    def _setup(self):
        self._data_files = self.getProperty('InputFiles').value
        self._sum_files = self.getProperty('SumFiles').value
        self._load_logs = self.getProperty('LoadLogFiles').value
        self._calibration_workspace = workspace_or_none(self.getPropertyValue('CalibrationWorkspace'))

        self._instrument_name = self.getPropertyValue('Instrument')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')
        self._efixed = self.getProperty('Efixed').value

        self._spectra_range = self.getProperty('SpectraRange').value
        self._background_range = elements_or_none(self.getProperty('BackgroundRange').value)
        self._rebin_string = string_or_none(self.getPropertyValue('RebinString'))
        self._detailed_balance = self.getProperty('DetailedBalance').value
        self._scale_factor = self.getProperty('ScaleFactor').value
        self._fold_multiple_frames = self.getProperty('FoldMultipleFrames').value

        self._grouping_method = self.getPropertyValue('GroupingMethod')
        self._grouping_workspace = workspace_or_none(self.getPropertyValue('GroupingWorkspace'))
        self._grouping_string = string_or_none(self.getPropertyValue('GroupingString'))
        self._grouping_map_file = string_or_none(self.getPropertyValue('MapFile'))

        self._output_x_units = self.getPropertyValue('UnitX')
        self._output_workspace = self.getPropertyValue('OutputWorkspace')

    def PyExec(self):
        self._setup()
        self._reduce_data()

        self.setProperty('OutputWorkspace', get_ads_workspace(self._output_workspace))

    def _reduce_data(self):
        reduction_algorithm = self.createChildAlgorithm(name='ISISIndirectEnergyTransfer', startProgress=0.1,
                                                        endProgress=1.0, enableLogging=False)
        reduction_algorithm.enableHistoryRecordingForChild(False)

        args = {"InputFiles": self._data_files, "SumFiles": self._sum_files, "LoadLogFiles": self._sum_files,
                "CalibrationWorkspace": self._calibration_workspace, "Instrument": self._instrument_name,
                "Analyser": self._analyser, "Reflection": self._reflection, "SpectraRange": self._spectra_range,
                "BackgroundRange": self._background_range, "RebinString": self._rebin_string,
                "ScaleFactor": self._scale_factor, "FoldMultipleFrames": self._fold_multiple_frames,
                "GroupingMethod": self._grouping_method, "GroupingWorkspace": self._grouping_workspace,
                "GroupingString": self._grouping_string, "MapFile": self._grouping_map_file,
                "UnitX": self._output_x_units, "OutputWorkspace": self._output_workspace}

        if self._efixed != Property.EMPTY_DBL:
            args["Efixed"] = self._efixed

        if self._detailed_balance != Property.EMPTY_DBL:
            args["DetailedBalance"] = self._detailed_balance

        for key, value in args.items():
            if value is not None:
                reduction_algorithm.setProperty(key, value)

        reduction_algorithm.execute()


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ISISIndirectEnergyTransferWrapper)
