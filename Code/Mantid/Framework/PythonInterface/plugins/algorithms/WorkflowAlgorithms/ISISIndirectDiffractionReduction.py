from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config

import os.path as path


class ISISIndirectDiffractionReduction(DataProcessorAlgorithm):

    def category(self):
        return 'Diffraction;PythonAlgorithms'


    def summary(self):
        return ''  # TODO


    def PyInit(self):
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files.')

        self.declareProperty(name='SumFiles', defaultValue=False,
                             doc='Enabled to sum spectra from each input file.')

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

        self.declareProperty(name='IndividualGrouping', defaultValue=False,
                             doc='Do not group results into a single spectra.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Group name for the result workspaces.')


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

        return issues


    def PyExec(self):
        self._setup()
        workspace_names = self._load_files()

        self._identify_bad_detectors(workspace_names[0])

        for ws_name in workspace_names:
            # Process monitor
            self._unwrap_monitor(ws_name)
            ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, Target='Wavelength')
            self._process_monitor_efficiency(ws_name)
            self._scale_monitor(ws_name)

            # Scale detector data by monitor intensities
            monitor_ws_name = ws_name + '_mon'
            ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, Target='Wavelength', EMode='Indirect')
            RebinToWorkspace(WorkspaceToRebin=ws_name, WorkspaceToMatch=monitor_ws_name, OutputWorkspace=ws_name)
            Divide(LHSWorkspace=ws_name, RHSWorkspace=monitor_ws_name, OutputWorkspace=ws_name)

            # Remove the no longer needed monitor workspace
            # DeleteWorkspace(monitor_ws_name)

            # Convert to dSpacing
            ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, Target='dSpacing', EMode='Elastic')

            # Rebin in dSpacing
            if self._rebin_params is not None:
                Rebin(InputWorkspace=ws_name, OutputWorkspace=ws_name, Params=self._rebin_params)

        # TODO: spectra grouping
        # TODO: workspace grouping


    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._raw_file_list = self.getProperty('InputFiles').value
        self._instrument_name = self.getPropertyValue('Instrument')
        self._spectra_range = self.getProperty('SpectraRange').value
        self._rebin_params = self.getPropertyValue('RebinParam')

        if self._rebin_params == '':
            self._rebin_params = None


    def _load_files(self):
        """
        Loads a set of files and extracts just the spectra we care about (i.e. detector range and monitor).

        @return List of workspace names
        """

        raw_workspaces = list()

        for filename in self._raw_file_list:
            # The filename without path and extension will be the workspace name
            ws_name = path.splitext(path.basename(filename))[0]

            logger.debug('Loading file %s as workspace %s' % (filename, ws_name))

            if self._instrument_name == 'VESUVIO':
                # Use the bespoke loader for VESUVIO
                LoadVesuvio(Filename=filename, OutputWorkspace=ws_name,
                            SpectrumList='1-198', Mode='FoilOut')
            else:
                # Good ole' Load will do for everything else
                Load(Filename=filename, OutputWorkspace=ws_name)

            raw_workspaces.append(ws_name)

        # Extract monitor spectra and detectors we care about
        for ws_name in raw_workspaces:
            # Get the spectrum number for the monitor
            instrument = mtd[ws_name].getInstrument()
            monitor_index = int(instrument.getNumberParameter('Workflow.Monitor1-SpectrumNumber')[0])
            logger.debug('Workspace %s monitor 1 spectrum number :%d' % (ws_name, monitor_index))

            # Get the monitor spectrum
            monitor_ws_name = ws_name + '_mon'
            ExtractSingleSpectrum(InputWorkspace=ws_name, OutputWorkspace=monitor_ws_name,
                                  WorkspaceIndex=monitor_index)

            # Crop to the detectors required
            CropWorkspace(InputWorkspace=ws_name, OutputWorkspace=ws_name,
                          StartWorkspaceIndex=self._spectra_range[0] - 1,
                          EndWorkspaceIndex=self._spectra_range[1] - 1)

        return raw_workspaces


    def _identify_bad_detectors(self, ws_name):
        """
        Identify detectors which should be masked

        @param ws_name Name of worksapce to use ot get masking detectors
        """

        instrument = mtd[ws_name].getInstrument()

        try:
            masking_type = instrument.getStringParameter('Workflow.Masking')[0]
        except IndexError:
            masking_type = 'None'

        logger.information('Masking type: %s' % (masking_type))

        if masking_type == 'IdentifyNoisyDetectors':
            ws_mask = '__workspace_mask'
            IdentifyNoisyDetectors(InputWorkspace=ws_name, OutputWorkspace=ws_mask)


    def _unwrap_monitor(self, ws_name):
        """
        Unwrap monitor if required based on value of Workflow.UnwrapMonitor parameter

        @param ws_name Name of workspace
        """

        monitor_ws_name = ws_name + '_mon'

        instrument = mtd[monitor_ws_name].getInstrument()
        try:
            unwrap = instrument.getStringParameter('Workflow.UnwrapMonitor')[0]

            if unwrap == 'Always':
                should_unwrap = True
            elif unwrap == 'BaseOnTimeRegime':
                mon_time = mtd[monitor_ws_name].readX(0)[0]
                det_time = mtd[ws_name].readX(0)[0]
                should_unwrap = mon_time == det_time
            else:
                should_unwrap = False

        except IndexError:
            should_unwrap = False

        logger.debug('Need to unwrap monitor for %s: %s' % (ws_name, str(should_unwrap)))

        if not should_unwrap:
            return
        else:
            sample = instrument.getSample()
            sample_to_source = sample.getPos() - instrument.getSource().getPos()
            radius = mtd[ws_name].getDetector(0).getDistance(sample)
            z_dist = sample_to_source.getZ()
            l_ref = z_dist + radius

            logger.debug('For workspace %s: radius=%d, z_dist=%d, l_ref=%d' %
                         (ws_name, radius, z_dist, l_ref))

            unwrapped_ws, join = UnwrapMonitor(InputWorkspace=monitor_ws_name,
                                               OutputWorkspace=monitor_ws_name, LRef=l_ref)

            RemoveBins(InputWorkspace=monitor_ws_name, OutputWorkspace=monitor_ws_name,
                       XMin=join-0.001, XMax=join+0.001,
                       Interpolation='Linear')

            try:
                FFTSmooth(InputWorkspace=monitor_ws_name, OutputWorkspace=monitor_ws_name, WorkspaceIndex=0)
            except ValueError:
                raise ValueError('Uneven bin widths are not supported.')


    def _process_monitor_efficiency(self, ws_name):
        """
        Process monitor efficiency for a given workspace.

        @param ws_name Name of workspace to process monitor for
        """

        monitor_ws_name = ws_name + '_mon'
        instrument = mtd[ws_name].getInstrument()

        try:
            area = instrument.getNumberParameter('Workflow.Monitor1-Area')[0]
            thickness = instrument.getNumberParameter('Workflow.Monitor1-Thickness')[0]
            attenuation = instrument.getNumberParameter('Workflow.Monitor1-Attenuation')[0]
        except IndexError:
            raise ValueError('Cannot get monitor details form parameter file')

        if area == -1 or thickness == -1 or attenuation == -1:
            logger.information('For workspace %s, skipping monitor efficiency' % (ws_name))
            return

        OneMinusExponentialCor(InputWorkspace=monitor_ws_name, OutputWorkspace=monitor_ws_name,
                               C=attenuation * thickness, C1=area)


    def _scale_monitor(self, ws_name):
        """
        Scale monitor intensity by a factor given as the Workflow.MonitorScalingFactor parameter.

        @param ws_name Name of workspace to process monitor for
        """

        monitor_ws_name = ws_name + '_mon'
        instrument = mtd[ws_name].getInstrument()

        try:
            scale_factor = instrument.getNumberParameter('Workflow.MonitorScalingFactor')[0]
        except IndexError:
            logger.information('No monitor scaling factor found for workspace %s' % ws_name)
            return

        if scale_factor != 1.0:
            Scale(InputWorkspace=monitor_ws_name, OutputWorkspace=monitor_ws_name,
                    Factor=1.0 / scale_factor, Operation='Multiply')


AlgorithmFactory.subscribe(ISISIndirectDiffractionReduction)
