# pylint: disable=no-init,too-many-instance-attributes
from __future__ import (absolute_import, division, print_function)

import os

from IndirectReductionCommon import load_files

from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config


class ISISIndirectDiffractionReduction(DataProcessorAlgorithm):
    _workspace_names = None
    _cal_file = None
    _chopped_data = None
    _output_ws = None
    _data_files = None
    _container_workspace = None
    _container_data_files = None
    _container_scale_factor = None
    _load_logs = None
    _instrument_name = None
    _mode = None
    _spectra_range = None
    _grouping_method = None
    _rebin_string = None
    _ipf_filename = None
    _sum_files = None
    _vanadium_ws = None

    # ------------------------------------------------------------------------------

    def category(self):
        return 'Diffraction\\Reduction'

    def summary(self):
        return 'Performs a diffraction reduction for a set of raw run files for an ISIS indirect spectrometer'

    # ------------------------------------------------------------------------------

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files.')

        self.declareProperty(StringArrayProperty(name='ContainerFiles'),
                             doc='Comma separated list of input files for the empty container runs.')

        self.declareProperty('ContainerScaleFactor', 1.0,
                             doc='Factor by which to scale the container runs.')

        self.declareProperty(FileProperty('CalFile', '', action=FileAction.OptionalLoad),
                             doc='Filename of the .cal file to use in the [[AlignDetectors]] and ' +
                                 '[[DiffractionFocussing]] child algorithms.')

        self.declareProperty(FileProperty('InstrumentParFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['.dat', '.par']),
                             doc='PAR file containing instrument definition. For VESUVIO only')

        self.declareProperty(StringArrayProperty(name='VanadiumFiles'),
                             doc='Comma separated array of vanadium runs')

        self.declareProperty(name='SumFiles', defaultValue=False,
                             doc='Enabled to sum spectra from each input file.')

        self.declareProperty(name='LoadLogFiles', defaultValue=True,
                             doc='Load log files when loading runs')

        self.declareProperty(name='Instrument', defaultValue='IRIS',
                             validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA', 'VESUVIO']),
                             doc='Instrument used for run')

        self.declareProperty(name='Mode', defaultValue='diffspec',
                             validator=StringListValidator(['diffspec', 'diffonly']),
                             doc='Diffraction mode used')

        self.declareProperty(IntArrayProperty(name='SpectraRange'),
                             doc='Range of spectra to use.')

        self.declareProperty(name='RebinParam', defaultValue='',
                             doc='Rebin parameters.')

        self.declareProperty(name='GroupingPolicy', defaultValue='All',
                             validator=StringListValidator(['All', 'Individual', 'IPF']),
                             doc='Selects the type of detector grouping to be used.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='Group name for the result workspaces.')

    # ------------------------------------------------------------------------------

    def validateInputs(self):
        """
        Checks for issues with user input.
        """
        issues = dict()

        # Validate input files
        input_files = self.getProperty('InputFiles').value
        if len(input_files) == 0:
            issues['InputFiles'] = 'InputFiles must contain at least one filename'

        # Validate detector range
        detector_range = self.getProperty('SpectraRange').value
        if len(detector_range) != 2:
            issues['SpectraRange'] = 'SpectraRange must be an array of 2 values only'
        else:
            if detector_range[0] > detector_range[1]:
                issues['SpectraRange'] = 'SpectraRange must be in format [lower_index,upper_index]'

        cal_file = self.getProperty('CalFile').value
        inst = self.getProperty('Instrument').value
        mode = self.getProperty('Mode').value
        if cal_file != '':
            if inst != 'OSIRIS':
                logger.warning('NOT OSIRIS, inst = ' + str(inst))
                logger.warning('type = ' + str(type(inst)))
                issues['CalFile'] = 'Cal Files are currently only available for use in OSIRIS diffspec mode'
            if mode != 'diffspec':
                logger.warning('NOT DIFFSPEC, mode = ' + str(mode))
                logger.warning('type = ' + str(type(mode)))
                issues['CalFile'] = 'Cal Files are currently only available for use in OSIRIS diffspec mode'

        num_samples = len(input_files)
        num_vanadium = len(self.getProperty('VanadiumFiles').value)
        if num_samples != num_vanadium and num_vanadium != 0:
            run_num_mismatch = 'You must input the same number of sample and vanadium runs'
            issues['InputFiles'] = run_num_mismatch
            issues['VanadiumFiles'] = run_num_mismatch

        return issues

    # ------------------------------------------------------------------------------

    def PyExec(self):

        from IndirectReductionCommon import (get_multi_frame_rebin,
                                             identify_bad_detectors,
                                             unwrap_monitor,
                                             process_monitor_efficiency,
                                             scale_monitor,
                                             scale_detectors,
                                             rebin_reduction,
                                             group_spectra,
                                             fold_chopped,
                                             rename_reduction)

        self._setup()

        load_opts = dict()
        if self._instrument_name == 'VESUVIO':
            load_opts['InstrumentParFile'] = self._par_filename
            load_opts['Mode'] = 'FoilOut'
            load_opts['LoadMonitors'] = True

        self._workspace_names, self._chopped_data = load_files(self._data_files,
                                                               self._ipf_filename,
                                                               self._spectra_range[0],
                                                               self._spectra_range[1],
                                                               sum_files=self._sum_files,
                                                               load_logs=self._load_logs,
                                                               load_opts=load_opts)

        # applies the changes in the provided calibration file
        self._apply_calibration()
        # Load container if run is given
        self._load_and_scale_container(self._container_scale_factor, load_opts)

        # Load vanadium runs if given
        if self._vanadium_runs:
            self._vanadium_ws, _ = load_files(self._vanadium_runs,
                                              self._ipf_filename,
                                              self._spectra_range[0],
                                              self._spectra_range[1],
                                              sum_files=self._sum_files,
                                              load_logs=self._load_logs,
                                              load_opts=load_opts)

        for index, c_ws_name in enumerate(self._workspace_names):
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

                # Subtract empty container if there is one
                if self._container_workspace is not None:
                    Minus(LHSWorkspace=ws_name,
                          RHSWorkspace=self._container_workspace,
                          OutputWorkspace=ws_name)

                if self._vanadium_ws:
                    van_ws_name = self._vanadium_ws[index]
                    van_ws = mtd[van_ws_name]
                    if self._container_workspace is not None:
                        cont_ws = mtd[self._container_workspace]

                        if van_ws.blocksize() > cont_ws.blocksize():
                            RebinToWorkspace(WorkspaceToRebin=van_ws_name,
                                             WorkspaceToMatch=self._container_workspace,
                                             OutputWorkspace=van_ws_name)
                        elif cont_ws.blocksize() > van_ws.blocksize():
                            RebinToWorkspace(WorkspaceToRebin=self._container_workspace,
                                             WorkspaceToMatch=van_ws_name,
                                             OutputWorkspace=self._container_workspace)

                        Minus(LHSWorkspace=van_ws_name,
                              RHSWorkspace=self._container_workspace,
                              OutputWorkspace=van_ws_name)

                    if mtd[ws_name].blocksize() > van_ws.blocksize():
                        RebinToWorkspace(WorkspaceToRebin=ws_name,
                                         WorkspaceToMatch=van_ws_name,
                                         OutputWorkspace=ws_name)
                    elif van_ws.blocksize() > mtd[ws_name].blocksize():
                        RebinToWorkspace(WorkspaceToRebin=van_ws_name,
                                         WorkspaceToMatch=ws_name,
                                         OutputWorkspace=van_ws_name)

                    Divide(LHSWorkspace=ws_name,
                           RHSWorkspace=van_ws_name,
                           OutputWorkspace=ws_name,
                           AllowDifferentNumberSpectra=True)

                # Process monitor
                if not unwrap_monitor(ws_name):
                    ConvertUnits(InputWorkspace=monitor_ws_name,
                                 OutputWorkspace=monitor_ws_name,
                                 Target='Wavelength',
                                 EMode='Elastic')

                process_monitor_efficiency(ws_name)
                scale_monitor(ws_name)

                # Scale detector data by monitor intensities
                scale_detectors(ws_name, 'Elastic')

                # Remove the no longer needed monitor workspace
                DeleteWorkspace(monitor_ws_name)

                # Convert to dSpacing
                ConvertUnits(InputWorkspace=ws_name,
                             OutputWorkspace=ws_name,
                             Target='dSpacing',
                             EMode='Elastic')

                # Handle rebinning
                rebin_reduction(ws_name,
                                self._rebin_string,
                                rebin_string_2,
                                num_bins)

                # Group spectra
                group_spectra(ws_name,
                              masked_detectors,
                              self._grouping_method)

            if is_multi_frame:
                fold_chopped(c_ws_name)

        # Remove the container workspaces
        if self._container_workspace is not None:
            DeleteWorkspace(self._container_workspace)
            DeleteWorkspace(self._container_workspace + '_mon')

        if self._vanadium_ws:
            for van_ws in self._vanadium_ws:
                DeleteWorkspace(van_ws)
                DeleteWorkspace(van_ws+'_mon')

        # Rename output workspaces
        output_workspace_names = [rename_reduction(ws_name, self._sum_files) for ws_name in self._workspace_names]

        # Group result workspaces
        GroupWorkspaces(InputWorkspaces=output_workspace_names,
                        OutputWorkspace=self._output_ws)

        self.setProperty('OutputWorkspace', self._output_ws)

    # ------------------------------------------------------------------------------

    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._output_ws = self.getPropertyValue('OutputWorkspace')
        self._data_files = self.getProperty('InputFiles').value
        self._container_data_files = self.getProperty('ContainerFiles').value
        self._cal_file = self.getProperty('CalFile').value
        self._par_filename = self.getPropertyValue('InstrumentParFile')
        self._vanadium_runs = self.getProperty('VanadiumFiles').value
        self._container_scale_factor = self.getProperty('ContainerScaleFactor').value
        self._load_logs = self.getProperty('LoadLogFiles').value
        self._instrument_name = self.getPropertyValue('Instrument')
        self._mode = self.getPropertyValue('Mode')
        self._spectra_range = self.getProperty('SpectraRange').value
        self._rebin_string = self.getPropertyValue('RebinParam')
        self._grouping_method = self.getPropertyValue('GroupingPolicy')

        if self._rebin_string == '':
            self._rebin_string = None

        self._container_workspace = None
        if len(self._container_data_files) == 0:
            self._container_data_files = None

        self._vanadium_ws = None
        if len(self._vanadium_runs) == 0:
            self._vanadium_runs = None

        # Get the IPF filename
        self._ipf_filename = self._instrument_name + '_diffraction_' + self._mode + '_Parameters.xml'
        if not os.path.exists(self._ipf_filename):
            self._ipf_filename = os.path.join(config['instrumentDefinition.directory'], self._ipf_filename)
        logger.information('IPF filename is: %s' % self._ipf_filename)

        # Only enable sum files if we actually have more than one file
        sum_files = self.getProperty('SumFiles').value
        self._sum_files = False

        if sum_files:
            num_raw_files = len(self._data_files)
            if num_raw_files > 1:
                self._sum_files = True
                logger.information('Summing files enabled (have %d files)' % num_raw_files)
            else:
                logger.information('SumFiles options is ignored when only one file is provided')

    def _apply_calibration(self):
        """
        Checks to ensure a calibration file has been given
        and if so performs AlignDetectors and DiffractionFocussing.
        """
        if self._cal_file is not '':
            for ws_name in self._workspace_names:
                AlignDetectors(InputWorkspace=ws_name,
                               OutputWorkspace=ws_name,
                               CalibrationFile=self._cal_file)
                DiffractionFocussing(InputWorkspace=ws_name,
                                     OutputWorkspace=ws_name,
                                     GroupingFileName=self._cal_file)

    def _load_and_scale_container(self, scale_factor, load_opts):
        """
        Loads the container file if given
        Applies the scale factor to the container if not 1.
        """
        if self._container_data_files is not None:
            self._container_workspace, _ = load_files(self._container_data_files,
                                                      self._ipf_filename,
                                                      self._spectra_range[0],
                                                      self._spectra_range[1],
                                                      sum_files=True,
                                                      load_logs=self._load_logs,
                                                      load_opts=load_opts)
            self._container_workspace = self._container_workspace[0]

            # Scale container if factor is given
            if scale_factor != 1.0:
                Scale(InputWorkspace=self._container_workspace,
                      OutputWorkspace=self._container_workspace,
                      Factor=scale_factor,
                      Operation='Multiply')


# ------------------------------------------------------------------------------

AlgorithmFactory.subscribe(ISISIndirectDiffractionReduction)
