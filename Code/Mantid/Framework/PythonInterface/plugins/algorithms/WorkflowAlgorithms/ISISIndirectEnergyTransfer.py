from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import os


_str_or_none = lambda s: s if s != '' else None
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

        self.declareProperty(name='SumFiles', defaultValue=False, doc='Toggle input file summing or sequential processing')

        self.declareProperty(WorkspaceProperty('CalibrationWorkspace', '',
                             direction=Direction.Input, optional=PropertyMode.Optional), doc='Workspace contining calibration data')

        # Instrument configuration properties
        self.declareProperty(name='Instrument', defaultValue='', doc='Instrument used during run',
                             validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA', 'TFXA']))
        self.declareProperty(name='Analyser', defaultValue='', doc='Analyser used during run',
                             validator=StringListValidator(['graphite', 'mica', 'fmica']))
        self.declareProperty(name='Reflection', defaultValue='', doc='Reflection used during run',
                             validator=StringListValidator(['002', '004', '006']))

        self.declareProperty(IntArrayProperty(name='DetectorRange', values=[0, 1],
                             validator=IntArrayMandatoryValidator()),
                             doc='Comma separated range of detectors to use')
        self.declareProperty(FloatArrayProperty(name='BackgroundRange'),
                             doc='')
        self.declareProperty(name='RebinString', defaultValue='', doc='Rebin string parameters')
        self.declareProperty(name='DetailedBalance', defaultValue=-1.0, doc='')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0, doc='')
        self.declareProperty(name='FoldMultipleFrames', defaultValue=False, doc='')

        # Spectra grouping options
        self.declareProperty(name='GroupingMethod', defaultValue='Individual',
                             validator=StringListValidator(['Individual', 'All', 'Map File', 'Workspace', 'IPF']),
                             doc='Method used to group spectra.')
        self.declareProperty(WorkspaceProperty('GroupingWorkspace', '',
                             direction=Direction.Input, optional=PropertyMode.Optional),
                             doc='Workspace containing spectra grouping.')
        self.declareProperty(FileProperty('MapFile', '',
                             action=FileAction.OptionalLoad, extensions=['.map']),
                             doc='Workspace containing spectra grouping.')

        # Output properties
        self.declareProperty(name='Plot', defaultValue='None', doc='Type of plot to output after reduction',
                             validator=StringListValidator(['None', 'Spectra', 'Contour']))

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Workspace group for the resulting workspaces')


    def PyExec(self):
        self._setup()
        self._load_files()

        for ws_name in self._workspace_names:
            masked_detectors = self._identify_bad_detectors(ws_name)

            # Process monitor
            self._unwrap_monitor(ws_name)
            ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, Target='Wavelength', EMode='Elastic')
            self._process_monitor_efficiency(ws_name)
            self._scale_monitor(ws_name)

            # Do background removal if a range was provided
            if self._background_range is not None:
                ConvertToDistribution(Workspace=ws_name)
                CalculateFlatBackground(InputWorkspace=ws_name, OutputWorkspace=ws_name,
                                        StartX=self._background_range[0],
                                        EndX=self._background_range[1],
                                        Mode='Mean')
                ConvertFromDistribution(Workspace=ws_name)

            # Divide by the calibration workspace if one was provided
            if self._calibration_ws is not None:
                Divide(LHSWorkspace=ws_name,
                       RHSWorkspace=self._calibration_ws,
                       Output_workspace=ws_name)

            # Scale detector data by monitor intensities
            monitor_ws_name = ws_name + '_mon'
            ConvertUnits(InputWorkspace=monitor_ws_name, OutputWorkspace=monitor_ws_name, Target='Wavelength', EMode='Elastic')
            RebinToWorkspace(WorkspaceToRebin=ws_name, WorkspaceToMatch=monitor_ws_name, OutputWorkspace=ws_name)
            Divide(LHSWorkspace=ws_name, RHSWorkspace=monitor_ws_name, OutputWorkspace=ws_name)

            # Remove the no longer needed monitor workspace
            DeleteWorkspace(monitor_ws_name)

            # TODO: Convert to energy
            # TODO: Detailed balance
            # TODO: Scale
            # TODO: Group spectra
            # TODO: Fold

        # Rename output workspaces
        output_workspace_names = [self._rename_workspace(ws_name) for ws_name in self._workspace_names]

        # Group result workspaces
        GroupWorkspaces(InputWorkspaces=output_workspace_names, OutputWorkspace=self._output_ws)

        self.setProperty('OutputWorkspace', self._output_ws)

        # TODO: Plot


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
        spectra_range = self.getProperty('DetectorRange').value
        if len(spectra_range) != 2:
            issues['DetectorRange'] = 'Range must contain exactly two items'
        elif spectra_range[0] > spectra_range[1]:
            issues['DetectorRange'] = 'Range must be in format: lower,upper'

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
        grouping_map_file = _str_or_none(self.getPropertyValue('MapFile'))

        if grouping_method == 'Workspace' and grouping_ws is None:
            issues['GroupingWorkspace'] = 'Must select a grouping workspace for current GroupingWorkspace'

        if grouping_method == 'Map File' and grouping_map_file is None:
            issues['MapFile'] = 'Must provide a map file for current GroupingMethod'

        return issues


    def _setup(self):
        """
        Gets algorithm properties.
        """

        # Get properties
        self._data_files = self.getProperty('InputFiles').value
        self._sum_files = self.getProperty('SumFiles').value
        self._calibration_ws = _str_or_none(self.getPropertyValue('CalibrationWorkspace'))

        self._instrument_name = self.getPropertyValue('Instrument')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        self._spectra_range = self.getProperty('DetectorRange').value
        self._background_range = _elems_or_none(self.getProperty('BackgroundRange').value)
        self._rebin_string = _str_or_none(self.getPropertyValue('RebinString'))
        self._detailed_balance = self.getProperty('DetailedBalance').value
        self._scale_factor = self.getProperty('ScaleFactor').value
        self._fold_multiple_frames = self.getProperty('FoldMultipleFrames').value

        self._grouping_method = self.getPropertyValue('GroupingMethod')
        self._grouping_ws = _str_or_none(self.getPropertyValue('GroupingWorkspace'))
        self._grouping_map_file = _str_or_none(self.getPropertyValue('MapFile'))

        self._plot_type = self.getPropertyValue('Plot')
        self._output_ws = self.getPropertyValue('OutputWorkspace')

        # Get the IPF filename
        self._ipf_filename = os.path.join(config['instrumentDefinition.directory'],
                                          self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml')
        logger.information('Instrument parameter file: %s' % self._ipf_filename)

        # Warn when grouping options are to be ignored
        if self._grouping_method != 'Workspace' and self._grouping_ws is not None:
            logger.warning('GroupingWorkspace will be ignored by selected GroupingMethod')

        if self._grouping_method != 'Map File' and self._grouping_map_file is not None:
            logger.warning('MapFile will be ignored by selected GroupingMethod')

        # The list of workspaces being processed
        self._workspace_names = []


    def _load_files(self):
        """
        Loads a set of files and extracts just the spectra we care about (i.e. detector range and monitor).
        """

        for filename in self._data_files:
            # The filename without path and extension will be the workspace name
            ws_name = os.path.splitext(os.path.basename(filename))[0]
            logger.debug('Loading file %s as workspace %s' % (filename, ws_name))

            Load(Filename=filename, OutputWorkspace=ws_name)

            # Load the instrument parameters
            LoadParameterFile(Workspace=ws_name, Filename=self._ipf_filename)

            # Add the workspace to the list of workspaces
            self._workspace_names.append(ws_name)

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

        # Sum files if needed
        if self._sum_files:
            # Use the first workspace name as the result of summation
            summed_detector_ws_name = self._workspace_names[0]
            summed_monitor_ws_name = self._workspace_names[0] + '_mon'

            # Get a list of the run numbers for the original data
            run_numbers = ','.join([str(mtd[ws_name].getRunNumber()) for ws_name in self._workspace_names])

            # Generate lists of the detector and monitor workspaces
            detector_workspaces = ','.join(self._workspace_names)
            monitor_workspaces = ','.join([ws_name + '_mon' for ws_name in self._workspace_names])

            # Merge the raw workspaces
            MergeRuns(InputWorkspaces=detector_workspaces, OutputWorkspace=summed_detector_ws_name)
            MergeRuns(InputWorkspaces=monitor_workspaces, OutputWorkspace=summed_monitor_ws_name)

            # Delete old workspaces
            for idx in range(1, len(self._workspace_names)):
                DeleteWorkspace(self._workspace_names[idx])
                DeleteWorkspace(self._workspace_names[idx] + '_mon')

            # Derive the scale factor based on number of merged workspaces
            scale_factor = 1.0 / len(self._workspace_names)
            logger.information('Scale factor for summed workspaces: %f' % scale_factor)

            # Scale the new detector and monitor workspaces
            Scale(InputWorkspace=summed_detector_ws_name, OutputWorkspace=summed_detector_ws_name,
                  Factor=scale_factor)
            Scale(InputWorkspace=summed_monitor_ws_name, OutputWorkspace=summed_monitor_ws_name,
                  Factor=scale_factor)

            # Add the list of run numbers to the result workspace as a sample log
            AddSampleLog(Workspace=summed_detector_ws_name, LogName='multi_run_numbers',
                         LogType='String', LogText=run_numbers)

            # Only have the one workspace now
            self._workspace_names = [summed_detector_ws_name]


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

        masked_spec = list()

        if masking_type == 'IdentifyNoisyDetectors':
            ws_mask = '__workspace_mask'
            IdentifyNoisyDetectors(InputWorkspace=ws_name, OutputWorkspace=ws_mask)

            # Convert workspace to a list of spectra
            num_spec = mtd[ws_mask].getNumberHistograms()
            masked_spec = [spec for spec in range(0, num_spec) if mtd[ws_mask].readY(spec)[0] == 0.0]

            # Remove the temporary masking workspace
            DeleteWorkspace(ws_mask)

        logger.debug('Masked specta for workspace %s: %s' % (ws_name, str(masked_spec)))

        return masked_spec


    def _unwrap_monitor(self, ws_name):
        """
        Unwrap monitor if required based on value of Workflow.UnwrapMonitor parameter

        @param ws_name Name of workspace
        """

        monitor_ws_name = ws_name + '_mon'
        instrument = mtd[monitor_ws_name].getInstrument()

        # Determine if the monitor should be unwrapped
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

            _, join = UnwrapMonitor(InputWorkspace=monitor_ws_name,
                                    OutputWorkspace=monitor_ws_name, LRef=l_ref)

            RemoveBins(InputWorkspace=monitor_ws_name, OutputWorkspace=monitor_ws_name,
                       XMin=join - 0.001, XMax=join + 0.001,
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
            scale_factor = instrument.getNumberParameter('Workflow.Monitor1-ScalingFactor')[0]
        except IndexError:
            logger.information('No monitor scaling factor found for workspace %s' % ws_name)
            return

        if scale_factor != 1.0:
            Scale(InputWorkspace=monitor_ws_name, OutputWorkspace=monitor_ws_name,
                  Factor=1.0 / scale_factor, Operation='Multiply')


    def _group_spectra(self, ws_name, masked_detectors):
        """
        Groups spectra in a given workspace according to the Workflow.GroupingMethod and
        Workflow.GroupingFile parameters and GrpupingPolicy property.

        @param ws_name Name of workspace to group spectra of
        @param masked_detectors List of spectra numbers to mask
        """

        instrument = mtd[ws_name].getInstrument()

        # If grouping as per he IPF is desired
        if self._grouping_policy == 'IPF':
            # Get the grouping method from the parameter file
            try:
                grouping_method = instrument.getStringParameter('Workflow.GroupingMethod')[0]
            except IndexError:
                grouping_method = 'All'

        else:
            # Otherwise use the value of GroupingPolicy
            grouping_method = self._grouping_policy

        logger.information('Grouping method for workspace %s is %s' % (ws_name, grouping_method))

        if grouping_method == 'Individual':
            # Nothing to do here
            return

        elif grouping_method == 'All':
            # Get a list of all spectra minus those which are masked
            num_spec = mtd[ws_name].getNumberHistograms()
            spectra_list = [spec for spec in range(0, num_spec) if spec not in masked_detectors]

            # Apply the grouping
            GroupDetectors(InputWorkspace=ws_name, OutputWorkspace=ws_name, Behaviour='Average',
                           WorkspaceIndexList=spectra_list)

        elif grouping_method == 'File':
            # Get the filename for the grouping file
            try:
                grouping_file = instrument.getStringParameter('Workflow.GroupingFile')[0]
            except IndexError:
                raise RuntimeError('IPF requests grouping using file but does not specify a filename')

            # If the file is not found assume it is in the grouping files directory
            if not os.path.isfile(grouping_file):
                grouping_file = os.path.join(config.getString('groupingFiles.directory'), grouping_file)

            # If it is still not found just give up
            if not os.path.isfile(grouping_file):
                raise RuntimeError('Cannot find grouping file %s' % grouping_file)

            # Mask detectors if required
            if len(masked_detectors) > 0:
                MaskDetectors(Workspace=ws_name, WorkspaceIndexList=masked_detectors)

            # Apply the grouping
            GroupDetectors(InputWorkspace=ws_name, OutputWorkspace=ws_name, Behaviour='Average',
                           MapFile=grouping_file)

        else:
            raise RuntimeError('Invalid grouping method %s for workspace %s' % (grouping_method, ws_name))


    def _rename_workspace(self, ws_name):
        """
        Renames a worksapce according to the naming policy in the Workflow.NamingConvention parameter.

        @param ws_name Name of workspace
        @return New name of workspace
        """

        # Get the naming convention parameter form the parameter file
        instrument = mtd[ws_name].getInstrument()
        try:
            convention = instrument.getStringParameter('Workflow.NamingConvention')[0]
        except IndexError:
            # Defualt to run title if naming convention parameter not set
            convention = 'RunTitle'
        logger.information('Naming convention for workspace %s is %s' % (ws_name, convention))

        run_number = mtd[ws_name].getRun()['run_number'].value
        logger.information('Run number for workspace %s is %s' % (ws_name, run_number))

        inst_name = instrument.getName()
        for facility in config.getFacilities():
            try:
                short_inst_name = facility.instrument(inst_name).shortName()
                break
            except:
                pass
        logger.information('Short name for instrument %s is %s' % (inst_name, short_inst_name))

        run_title = mtd[ws_name].getRun()['run_number'].value
        if self._sum_files:
            multi_run_marker = '_multi'
        else:
            multi_run_marker = ''

        if convention == 'None':
            new_name = ws_name

        elif convention == 'RunTitle':
            valid = "-_.() %s%s" % (string.ascii_letters, string.digits)
            formatted_title = ''.join(c for c in run_title in c in valid)
            new_name = '%s%s%s-%s' % (short_inst_name.lower(), run_number, multi_run_marker, formatted_title)

        elif convention == 'AnalyserReflection':
            analyser = instrument.getStringParameter('analyser')[0]
            reflection = instrument.getStringParameter('reflection')[0]
            new_name = '%s%s%s_%s%s_red' % (short_inst_name.upper(), run_number, multi_run_marker,
                                            analyser, reflection)

        else:
            raise RuntimeError('No valid naming convention for workspace %s' % ws_name)

        logger.information('New name for %s workspace: %s' % (ws_name, new_name))

        RenameWorkspace(InputWorkspace=ws_name, OutputWorkspace=new_name)
        return new_name


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ISISIndirectEnergyTransfer)
