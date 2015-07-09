#pylint: disable=invalid-name,attribute-defined-outside-init,too-many-instance-attributes,too-many-branches,no-init,deprecated-module
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
from mantid import config

import os


_str_or_none = lambda s: s if s != '' else None
_float_or_none = lambda i: float(i) if i != '' else None
_elems_or_none = lambda l: l if len(l) != 0 else None


class ISISIndirectEnergyTransfer(DataProcessorAlgorithm):

    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Runs an energy transfer reduction for an inelastic indirect geometry instrument.'


    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files')

        self.declareProperty(name='SumFiles', defaultValue=False,
                             doc='Toggle input file summing or sequential processing')

        self.declareProperty(name='LoadLogFiles', defaultValue=True,
                             doc='Load log files when loading runs')

        self.declareProperty(WorkspaceProperty('CalibrationWorkspace', '', direction=Direction.Input, optional=PropertyMode.Optional),
                             doc='Workspace contining calibration data')

        # Instrument configuration properties
        self.declareProperty(name='Instrument', defaultValue='', doc='Instrument used during run.',
                             validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA', 'TFXA']))
        self.declareProperty(name='Analyser', defaultValue='', doc='Analyser bank used during run.',
                             validator=StringListValidator(['graphite', 'mica', 'fmica']))
        self.declareProperty(name='Reflection', defaultValue='', doc='Reflection number for instrument setup during run.',
                             validator=StringListValidator(['002', '004', '006']))

        self.declareProperty(IntArrayProperty(name='SpectraRange', values=[0, 1], validator=IntArrayMandatoryValidator()),
                             doc='Comma separated range of spectra number to use.')
        self.declareProperty(FloatArrayProperty(name='BackgroundRange'),
                             doc='Range of background to subtact from raw data in time of flight.')
        self.declareProperty(name='RebinString', defaultValue='', doc='Rebin string parameters.')
        self.declareProperty(name='DetailedBalance', defaultValue='', doc='')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0, doc='Factor by which to scale result.')
        self.declareProperty(name='FoldMultipleFrames', defaultValue=True,
                             doc='Folds multiple framed data sets into a single workspace.')

        # Spectra grouping options
        self.declareProperty(name='GroupingMethod', defaultValue='IPF',
                             validator=StringListValidator(['Individual', 'All', 'File', 'Workspace', 'IPF']),
                             doc='Method used to group spectra.')
        self.declareProperty(WorkspaceProperty('GroupingWorkspace', '', direction=Direction.Input, optional=PropertyMode.Optional),
                             doc='Workspace containing spectra grouping.')
        self.declareProperty(FileProperty('MapFile', '', action=FileAction.OptionalLoad, extensions=['.map']),
                             doc='Workspace containing spectra grouping.')

        # Output properties
        self.declareProperty(name='UnitX', defaultValue='DeltaE', doc='X axis units for the result workspace.',
                             validator=StringListValidator(['DeltaE', 'DeltaE_inWavenumber']))
        self.declareProperty(StringArrayProperty(name='SaveFormats'), doc='Comma seperated list of save formats')
        self.declareProperty(name='Plot', defaultValue='None', doc='Type of plot to output after reduction.',
                             validator=StringListValidator(['None', 'Spectra', 'Contour', 'Both']))

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='Workspace group for the resulting workspaces.')


    def PyExec(self):
        from IndirectReductionCommon import (load_files,
                                             get_multi_frame_rebin,
                                             identify_bad_detectors,
                                             unwrap_monitor,
                                             process_monitor_efficiency,
                                             scale_monitor,
                                             scale_detectors,
                                             rebin_reduction,
                                             group_spectra,
                                             fold_chopped,
                                             rename_reduction,
                                             save_reduction,
                                             plot_reduction)

        self._setup()
        self._workspace_names, self._chopped_data = load_files(self._data_files,
                                                              self._ipf_filename,
                                                              self._spectra_range[0],
                                                              self._spectra_range[1],
                                                              self._sum_files,
                                                              self._load_logs)

        for c_ws_name in self._workspace_names:
            is_multi_frame = isinstance(mtd[c_ws_name], WorkspaceGroup)

            # Get list of workspaces
            if is_multi_frame:
                workspaces = mtd[c_ws_name].getNames()
            else:
                workspaces = [c_ws_name]

            # Process rebinning for framed data
            rebin_string_2, num_bins = get_multi_frame_rebin(c_ws_name,
                                                             self._rebin_string)

            masked_detectors = identify_bad_detectors(workspaces[0])

            # Process workspaces
            for ws_name in workspaces:
                monitor_ws_name = ws_name + '_mon'

                # Process monitor
                if not unwrap_monitor(ws_name):
                    ConvertUnits(InputWorkspace=monitor_ws_name,
                                 OutputWorkspace=monitor_ws_name,
                                 Target='Wavelength',
                                 EMode='Elastic')

                process_monitor_efficiency(ws_name)
                scale_monitor(ws_name)

                # Do background removal if a range was provided
                if self._background_range is not None:
                    ConvertToDistribution(Workspace=ws_name)
                    CalculateFlatBackground(InputWorkspace=ws_name,
                                            OutputWorkspace=ws_name,
                                            StartX=self._background_range[0],
                                            EndX=self._background_range[1],
                                            Mode='Mean')
                    ConvertFromDistribution(Workspace=ws_name)

                # Divide by the calibration workspace if one was provided
                if self._calibration_ws is not None:
                    Divide(LHSWorkspace=ws_name,
                           RHSWorkspace=self._calibration_ws,
                           OutputWorkspace=ws_name)

                # Scale detector data by monitor intensities
                scale_detectors(ws_name, 'Indirect')

                # Remove the no longer needed monitor workspace
                DeleteWorkspace(monitor_ws_name)

                # Convert to energy
                ConvertUnits(InputWorkspace=ws_name,
                             OutputWorkspace=ws_name,
                             Target='DeltaE',
                             EMode='Indirect')
                CorrectKiKf(InputWorkspace=ws_name,
                            OutputWorkspace=ws_name,
                            EMode='Indirect')

                # Handle rebinning
                rebin_reduction(ws_name,
                                self._rebin_string,
                                rebin_string_2,
                                num_bins)

                # Detailed balance
                if self._detailed_balance is not None:
                    corr_factor = 11.606 / (2 * self._detailed_balance)
                    ExponentialCorrection(InputWorkspace=ws_name,
                                          OutputWorkspace=ws_name,
                                          C0=1.0,
                                          C1=corr_factor,
                                          Operation='Multiply')

                # Scale
                if self._scale_factor != 1.0:
                    Scale(InputWorkspaces=ws_name,
                          OutputWorkspace=ws_name,
                          Factor=self._scale_factor,
                          Operation='Multiply')

                # Group spectra
                group_spectra(ws_name,
                              masked_detectors,
                              self._grouping_method,
                              self._grouping_map_file,
                              self._grouping_ws)

            if self._fold_multiple_frames and is_multi_frame:
                fold_chopped(c_ws_name)

            # Convert to output units if needed
            if self._output_x_units != 'DeltaE':
                ConvertUnits(InputWorkspace=c_ws_name,
                             OutputWorkspace=c_ws_name,
                             EMode='Indirect',
                             Target=self._output_x_units)

        # Rename output workspaces
        output_workspace_names = [rename_reduction(ws_name, self._sum_files) for ws_name in self._workspace_names]

        # Save result workspaces
        if self._save_formats is not None:
            save_reduction(output_workspace_names,
                           self._save_formats,
                           self._output_x_units)

        # Group result workspaces
        GroupWorkspaces(InputWorkspaces=output_workspace_names,
                        OutputWorkspace=self._output_ws)

        self.setProperty('OutputWorkspace', self._output_ws)

        # Plot result workspaces
        if self._plot_type != 'None':
            for ws_name in mtd[self._output_ws].getNames():
                plot_reduction(ws_name, self._plot_type)


    def validateInputs(self):
        """
        Validates algorithm properties.
        """
        issues = dict()

        # Validate the instrument configuration by checking if a parameter file exists
        instrument_name = self.getPropertyValue('Instrument')
        analyser = self.getPropertyValue('Analyser')
        reflection = self.getPropertyValue('Reflection')

        ipf_filename = os.path.join(config['instrumentDefinition.directory'],
                                    instrument_name + '_' + analyser + '_' + reflection + '_Parameters.xml')

        if not os.path.exists(ipf_filename):
            error_message = 'Invalid instrument configuration'
            issues['Instrument'] = error_message
            issues['Analyser'] = error_message
            issues['Reflection'] = error_message

        # Validate spectra range
        spectra_range = self.getProperty('SpectraRange').value
        if len(spectra_range) != 2:
            issues['SpectraRange'] = 'Range must contain exactly two items'
        elif spectra_range[0] > spectra_range[1]:
            issues['SpectraRange'] = 'Range must be in format: lower,upper'

        # Validate background range
        background_range = _elems_or_none(self.getProperty('BackgroundRange').value)
        if background_range is not None:
            if len(background_range) != 2:
                issues['BackgroundRange'] = 'Range must contain exactly two items'
            elif background_range[0] > background_range[1]:
                issues['BackgroundRange'] = 'Range must be in format: lower,upper'

        # Validate grouping method
        grouping_method = self.getPropertyValue('GroupingMethod')
        grouping_ws = _str_or_none(self.getPropertyValue('GroupingWorkspace'))

        if grouping_method == 'Workspace' and grouping_ws is None:
            issues['GroupingWorkspace'] = 'Must select a grouping workspace for current GroupingWorkspace'

        # Validate save formats
        save_formats = self.getProperty('SaveFormats').value
        valid_formats = ['nxs', 'ascii', 'spe', 'nxspe', 'aclimax', 'davegrp']
        for format_name in save_formats:
            if format_name not in valid_formats:
                issues['SaveFormats'] = '%s is not a valid save format' % format_name
                break

        return issues


    def _setup(self):
        """
        Gets algorithm properties.
        """

        # Get properties
        self._data_files = self.getProperty('InputFiles').value
        self._sum_files = self.getProperty('SumFiles').value
        self._load_logs = self.getProperty('LoadLogFiles').value
        self._calibration_ws = _str_or_none(self.getPropertyValue('CalibrationWorkspace'))

        self._instrument_name = self.getPropertyValue('Instrument')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        self._spectra_range = self.getProperty('SpectraRange').value
        self._background_range = _elems_or_none(self.getProperty('BackgroundRange').value)
        self._rebin_string = _str_or_none(self.getPropertyValue('RebinString'))
        self._detailed_balance = _float_or_none(self.getPropertyValue('DetailedBalance'))
        self._scale_factor = self.getProperty('ScaleFactor').value
        self._fold_multiple_frames = self.getProperty('FoldMultipleFrames').value

        self._grouping_method = self.getPropertyValue('GroupingMethod')
        self._grouping_ws = _str_or_none(self.getPropertyValue('GroupingWorkspace'))
        self._grouping_map_file = _str_or_none(self.getPropertyValue('MapFile'))

        self._output_x_units = self.getPropertyValue('UnitX')
        self._plot_type = self.getPropertyValue('Plot')
        self._save_formats = _elems_or_none(self.getProperty('SaveFormats').value)

        self._output_ws = self.getPropertyValue('OutputWorkspace')

        # Disable sum files if there is only one file
        if len(self._data_files) == 1:
            if self._sum_files:
                logger.warning('SumFiles disabled when only one input file is provided.')
            self._sum_files = False

        # Get the IPF filename
        self._ipf_filename = os.path.join(config['instrumentDefinition.directory'],
                                          self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml')
        logger.information('Instrument parameter file: %s' % self._ipf_filename)

        # Warn when grouping options are to be ignored
        if self._grouping_method != 'Workspace' and self._grouping_ws is not None:
            logger.warning('GroupingWorkspace will be ignored by selected GroupingMethod')

        if self._grouping_method != 'File' and self._grouping_map_file is not None:
            logger.warning('MapFile will be ignored by selected GroupingMethod')

        # The list of workspaces being processed
        self._workspace_names = []


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ISISIndirectEnergyTransfer)
