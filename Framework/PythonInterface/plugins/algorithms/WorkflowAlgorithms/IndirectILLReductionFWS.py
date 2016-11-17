#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)


from mantid.simpleapi import *  # noqa
from mantid.kernel import *  # noqa
from mantid.api import *  # noqa
from mantid import mtd
import numpy as np
import re


class IndirectILLReductionFWS(DataProcessorAlgorithm):

    _sample_files = None
    _background_files = None
    _calibration_files = None
    _observable = None
    _sortX = None
    _red_ws = None
    _back_scaling = None
    _criteria = None
    _progress = None
    _analyser = None
    _reflection = None
    _common_args = {}
    _all_runs = None

    def category(self):
        return 'Workflow\\MIDAS;Inelastic\\Reduction'

    def summary(self):
        return 'Performs complete FWS multiple file reduction (both elastic and inelastic) ' \
               'for ILL indirect geometry data, instrument IN16B.'

    def name(self):
        return "IndirectILLReductionFWS"

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='Run number(s) of sample run(s).')

        self.declareProperty(MultipleFileProperty('BackgroundRun',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Run number(s) of background (empty can) run(s).')

        self.declareProperty(MultipleFileProperty('CalibrationRun',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Run number(s) of vanadium calibration run(s).')

        self.declareProperty(name='Observable',
                             defaultValue='sample.temperature',
                             doc='Scanning observable, a Sample Log entry\n')

        self.declareProperty(name='SortXAxis',
                             defaultValue=False,
                             doc='Whether or not to sort the x-axis\n')

        self.declareProperty(name='BackgroundScalingFactor', defaultValue=1.,
                             validator=FloatBoundedValidator(lower=0),
                             doc='Scaling factor for background subtraction')

        self.declareProperty(FileProperty('MapFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['xml']),
                             doc='Filename of the detector grouping map file to use. \n'
                                 'If left blank the default will be used '
                                 '(i.e. all vertical pixels will be summed in each PSD tube.)')

        self.declareProperty(name='Analyser',
                             defaultValue='silicon',
                             validator=StringListValidator(['silicon']),
                             doc='Analyser crystal.')

        self.declareProperty(name='Reflection',
                             defaultValue='111',
                             validator=StringListValidator(['111', '311']),
                             doc='Analyser reflection.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', 'red',
                                                    direction=Direction.Output),
                             doc='Output workspace group')

    def validateInputs(self):

        issues = dict()

        return issues

    def setUp(self):

        self._sample_files = self.getPropertyValue('Run')
        self._background_files = self.getPropertyValue('BackgroundRun')
        self._calibration_files = self.getPropertyValue('CalibrationRun')
        self._observable = self.getPropertyValue('Observable')
        self._sortX = self.getProperty('SortXAxis').value
        self._back_scaling = self.getProperty('BackgroundScalingFactor').value

        # arguments to pass to IndirectILLEnergyTransfer
        self._common_args['MapFile'] = self.getPropertyValue('MapFile')
        self._common_args['Analyser'] = self.getPropertyValue('Analyser')
        self._common_args['Reflection'] = self.getPropertyValue('Reflection')

        self._red_ws = self.getPropertyValue('OutputWorkspace')

        # Nexus metadata criteria for FWS type of data (both EFWS and IFWS)
        self._criteria = '($/entry0/instrument/Doppler/maximum_delta_energy$ == 0. or ' \
                         '$/entry0/instrument/Doppler/velocity_profile$ == 1)'

        # make sure observable entry also exists (value is not important)
        self._criteria += ' and ($/entry0/' + self._observable.replace('.', '/') + '$ or True)'

        # empty dictionary
        self._all_runs = dict()

    def _filter_files(self, files, label):
        '''
        Filters the given list of files according to nexus criteria
        @param  files :: list of input files (i.e. , and + separated string)
        @param  label :: label of error message if nothing left after filtering
        @throws RuntimeError :: when nothing left after filtering
        @return :: the list of input files that passsed the criteria
        '''

        files = SelectNexusFilesByMetadata(files, self._criteria)

        if not files:
            raise RuntimeError('None of the {0} runs satisfied the FWS, '
                               'MirrorSense and Observable criteria.'.format(label))
        else:
            self.log().information('Filtered {0} runs are: {0} \\n'.format(label, files.replace(',', '\\n')))

        return files

    def _ifws_peak_bins(self, ws):
        '''
        Gives the bin indices of the first and last peaks (of spectra 0) in the IFWS
        @param ws :: input workspace
        return    :: [xmin,xmax]
        '''

        y = mtd[ws].readY(0)
        size = len(y)
        mid = int(size / 2)
        imin = np.nanargmax(y[0:mid])
        imax = np.nanargmax(y[mid:size]) + mid
        return imin, imax

    def _ifws_integrate(self, wsgroup):
        '''
        Integrates IFWS over two peaks at the beginning and end
        @param ws :: input workspace group
        '''

        for index in range(mtd[wsgroup].getNumberOfEntries()):
            ws = mtd[wsgroup].getItem(index).getName()
            size = mtd[ws].blocksize()
            imin, imax = self._ifws_peak_bins(ws)
            x_values = mtd[ws].readX(0)
            int1 = '__int1_' + ws
            int2 = '__int2_' + ws
            Integration(InputWorkspace=ws, OutputWorkspace=int1,
                        RangeLower=x_values[0], RangeUpper=x_values[2*imin])
            Integration(InputWorkspace=ws, OutputWorkspace=int2,
                        RangeLower=x_values[-2*(size-imax)], RangeUpper=x_values[-1])
            Plus(LHSWorkspace=int1, RHSWorkspace=int2, OutputWorkspace=ws)
            DeleteWorkspace(int1)
            DeleteWorkspace(int2)

    def _perform_unmirror(self, groupws):
        '''
        Sums the integrals of left and right for two wings, or returns the integral of one wing
        @param ws :: group workspace containing one ws for one wing, and two ws for two wing data
        '''
        if mtd[groupws].getNumberOfEntries() == 2:  # two wings, sum
            left = mtd[groupws].getItem(0).getName()
            right = mtd[groupws].getItem(1).getName()
            UnGroupWorkspace(groupws)
            summed = '__sum_' + groupws
            Plus(LHSWorkspace=left, RHSWorkspace=right, OutputWorkspace=summed)
            DeleteWorkspace(left)
            DeleteWorkspace(right)
            RenameWorkspace(InputWorkspace=summed, OutputWorkspace=groupws)
        else:
            RenameWorkspace(InputWorkspace=mtd[groupws].getItem(0), OutputWorkspace=groupws)

    def PyExec(self):

        self.setUp()

        # total number of (unsummed) runs
        total = self._sample_files.count(',')+self._background_files.count(',')+self._calibration_files.count(',')

        self._progress = Progress(self, start=0.0, end=1.0, nreports=total)

        self._reduce_multiple_runs(self._sample_files, 'sample')

        if self._background_files:
            self._reduce_multiple_runs(self._background_files, 'background')
            self._interpolate(self._red_ws + '_sample', self._red_ws + '_background', 'backg_interp', 'background')
            Minus(LHSWorkspace=self._red_ws + '_sample', RHSWorkspace='backg_interp', OutputWorkspace=self._red_ws)
            # Rename workspaces
            #for workspace in range(mtd['backg_interp'].getNumberOfEntries()):
            #    RenameWorkspace(InputWorkspace=mtd['backg_interp'].getItem(workspace)getName()..., OutputWorkspace=...)
            DeleteWorkspace('backg_interp')
            DeleteWorkspace(self._red_ws + '_background')
            DeleteWorkspace(self._red_ws + '_sample')

        if self._calibration_files:
            self._reduce_multiple_runs(self._calibration_files, 'calibration')
            # interpolate, divide, delete calibration

        self.log().information('Run files map is :'+str(self._all_runs))

        #RenameWorkspace(InputWorkspace=self._red_ws + '_sample', OutputWorkspace=self._red_ws)

        self.setProperty('OutputWorkspace', self._red_ws)

    def _reduce_multiple_runs(self, files, label):
        '''
        Filters and reduces multiple files
        @param files :: list of run paths
        @param label :: output ws name
        '''

        files = self._filter_files(files, label)

        for run in files.split(','):
            self._reduce_run(run, label)

        self._create_matrices(label)

    def _reduce_run(self, run, label):
        '''
        Reduces the given (single or summed multiple) run
        @param run :: run path
        @param runnumber :: run number
        @param  label :: sample, background or calibration
        '''

        runnumber = os.path.basename(run.split('+')[0]).split('.')[0]

        ws = runnumber + '_' + label

        self._progress.report("Reducing run #" + runnumber)

        IndirectILLEnergyTransfer(Run=run, OutputWorkspace=ws, **self._common_args)

        energy = round(mtd[ws].getItem(0).getRun().getLogData('Doppler.maximum_delta_energy').value, 2)

        if energy == 0.:
            # Elastic, integrate over full energy range
            Integration(InputWorkspace=ws, OutputWorkspace=ws)
        else:
            # Inelastic, do something more complex
            self._ifws_integrate(ws)

        self._perform_unmirror(ws)

        self._subscribe_run(ws, energy, label)

    def _subscribe_run(self, ws, energy, label):
        '''
        Subcribes the given ws name to the list for
        given energy and sample/background or calibration
        @param ws :: workspace name
        @param energy :: energy
        @param label  :: sample,calibration or background
        '''

        if label in self._all_runs:
            if energy in self._all_runs[label]:
                self._all_runs[label][energy].append(ws)
            else:
                self._all_runs[label][energy] = [ws]
        else:
            self._all_runs[label] = dict()
            self._all_runs[label][energy] = [ws]

    def _get_observable_values(self, ws_list):
        '''
        Retrives the needed sample log values for the given list of workspaces
        @param ws_list :: list of workspaces
        @returns :: array of observable values
        '''

        result = []

        for ws in ws_list:
            value = mtd[ws].getRun().getLogData(self._observable).value

            if 'start_time' not in self._observable:
                value = float(value)
            else:
                # start_time is a string like 2016-09-12T09:09:15,
                # find an elegant way to convert to absolute time
                pass

            result.append(value)

        return result

    def _get_energies(self, input_workspace):
        '''
        Returns: energies   a list of unique energies

        '''
        energies = []
        number_of_workspaces = mtd[input_workspace].getNumberOfEntries()

        # Count appearances of energy, this will be the number of GroupWorkspaces needed
        for i in range(number_of_workspaces):
            the_workspace = mtd[input_workspace].getItem(i).getRun()
            if the_workspace.hasProperty('Doppler.maximum_delta_energy'):
                energy = round(the_workspace.getLogData('Doppler.maximum_delta_energy').value, 2)
            else:
                energy = float('nan')
            energies.append(energy)

        # return_counts would be nice to use here but requires numpy version 1.9.0
        energy_values, indices = np.unique(energies, return_inverse=True)
        self.log().information('FWS energies: {0}'.format(energy_values))
        self.log().debug('Corresponding workspaces (index): {0}'.format(indices))

        return [energy_values, indices]

    def _empty_workspace(self, input_workspace, workspace_name):
        n_hists = mtd[input_workspace].getNumberHistograms()

        zero_data = np.zeros(n_hists * len(mtd[input_workspace].readX(0)))

        # Setting up matrix of same size as sample run
        self.log().debug('Creating workspace with {0} spectra and {1} channel(s)'.
                         format(n_hists, len(mtd[input_workspace].readX(0))))

        # ParentWorkspace needed for getting observable (for example)
        CreateWorkspace(OutputWorkspace=workspace_name,
                        DataX=mtd[input_workspace].extractX(),
                        DataY=zero_data,
                        NSpec=mtd[input_workspace].getNumberHistograms(),
                        Distribution=True,
                        ParentWorkspace=mtd[input_workspace])
        self._set_x_label(workspace_name)

    def _interpolate(self, sample_ws, to_be_interpolated_ws, interpolated_ws, label):
        '''
        Create a group workspace containing interpolated to_be_interpolated_ws intensities.
        The individual workspaces must have the same size as the sample input workspaces.
        In case there are more workspaces for the reference or to_be_interpolated_ws that do not have corresponding
        energies in the sample workspace group, those workspaces will not be considered.
        In case there are less workspaces for the to_be_interpolated_ws (missing corresponding energies in the sample
        workspace group), workspaces will be added but will contain only zeros and do not disturb later subtraction
        The output workspace group needs to be sorted

        input sample_ws (group ws)
        input to_be_interpolated_ws (group ws)
        output interpolated_ws (group ws)
        label should be background or ... must be an existing label
        '''

        list_result_workspace = []

        # Add empty workspace in case to_be_interpolated_ws workspace does not contain energy of sample workspace group
        # Skip to_be_interpolated_ws energies that are additional and not represented in sample workspace group

        to_be_interpolated_energies, to_be_interpolated_indices = self._get_energies(to_be_interpolated_ws)
        sample_energies, sample_indices = self._get_energies(sample_ws)
        del to_be_interpolated_energies
        del sample_energies

        missing_to_be_interpolated_indices = set(sample_indices) - set(to_be_interpolated_indices)

        for sample_index in missing_to_be_interpolated_indices:

            energy = round(mtd[sample_ws].getItem(sample_index).getRun().
                           getLogData('Doppler.maximum_delta_energy').value, 2)

            # might be more general here
            result_name = 'interpolated_' + self._red_ws + '_' + label + '_' + str(energy)

            self.log().debug('Add empty workspace {0}'.format(result_name))

            # Add empty workspace to the group
            self._empty_workspace(mtd[sample_ws].getItem(sample_index).getName(), result_name)
            list_result_workspace.append(result_name)

        # Take all remaining to_be_interpolated_ws workspaces into account, the sample_index is a common index of sample
        # and to_be_interpolated_ws workspaces

        for sample_index in set(to_be_interpolated_indices):

            result_name = 'interpolated_' + mtd[to_be_interpolated_ws].getItem(sample_index).getName()

            self.log().debug('Add new workspace {0}, {1}th workspace of the groups'.format(result_name, sample_index))

            # Get observables of the sample workspace
            observables_all = mtd[sample_ws].getItem(sample_index).readX(0)
            self.log().debug('Observables of group {0}, workspace {1}: {2}'.
                             format(sample_ws, mtd[sample_ws].getItem(sample_index).getName(), observables_all))
            # Get observables of the to_be_interpolated_ws runs
            observables_to_be_interpolated = mtd[to_be_interpolated_ws].getItem(sample_index).readX(0)
            self.log().debug('Observables of group {0}, workspace {1}: {2}'.
                             format(to_be_interpolated_ws, mtd[to_be_interpolated_ws].getItem(sample_index).getName(),
                                    observables_to_be_interpolated))

            # Number of histograms in to_be_interpolated_ws and sample files must be compatible
            number_histograms = mtd[sample_ws].getItem(sample_index).getNumberHistograms()
            if mtd[sample_ws].getItem(sample_index).getNumberHistograms() != \
                    mtd[to_be_interpolated_ws].getItem(sample_index).getNumberHistograms():
                self.log().notice('Sample {0} and reference workspace {1} workspace do not have the same number of'
                                  'histograms'.format(sample_ws, to_be_interpolated_ws))

            # Check whether observables of sample input runs are contained in to_be_interpolated_ws runs
            if frozenset(observables_to_be_interpolated).isdisjoint(frozenset(observables_all)):
                self.log().debug('Sample input run(s) and reference run(s) have different observable(s).')
            else:
                # maybe impossible case
                self.log().debug('Sample input run(s) and reference run(s) have common observable(s).')

            # Create a to_be_interpolated_ws workspace
            if len(observables_to_be_interpolated) > 2:
                self.log().notice('Perform spline interpolation')
                # Spline interpolation of the reference workspace
                SplineInterpolation(WorkspaceToMatch=mtd[sample_ws].getItem(sample_index),
                                    WorkspaceToInterpolate=mtd[to_be_interpolated_ws].getItem(sample_index),
                                    OutputWorkspace=result_name)
            else:
                self._empty_workspace(mtd[sample_ws].getItem(sample_index).getName(), result_name)

                # Insert the to_be_interpolated_ws values at the corresponding observable
                if len(observables_to_be_interpolated) == 1:
                    self.log().notice('Set flat workspace')
                    for i in range(number_histograms):
                        mtd[result_name].setY(i, np.ones(len(observables_all)) *
                                              mtd[to_be_interpolated_ws].getItem(sample_index).readY(i))
                        mtd[result_name].setE(i, np.ones(len(observables_all)) *
                                              mtd[to_be_interpolated_ws].getItem(sample_index).readE(i))
                elif len(observables_to_be_interpolated) == 2:
                    self.log().notice('Perform linear interpolation')
                    # Linear interpolation
                    from scipy.interpolate import interp1d
                    # to_be_interpolated_ws x-range must include or be the same as sample x-range (new x-range should
                    # not be above integration range)

                    if (observables_all[0] <= observables_to_be_interpolated[0]) or \
                            (observables_all[-1] >= observables_all[-1]):
                        self.log().debug('New (sample) x-range is not above (reference) integration range')
                    else:
                        self.log().debug('New (sample) x-range is above (reference) integration range')
                        # The reference workspace should not have x-values outside of the x-range of the sample
                        # workspace
                        # Crop all x-values that are outside of the sample x-range
                        if observables_all[0] >= observables_to_be_interpolated[0]:
                            xmin = observables_all[0]
                            CropWorkspace(InputWorkspace=to_be_interpolated_ws, OutputWorkspace=to_be_interpolated_ws,
                                          XMin=xmin)
                        if observables_all[-1] <= observables_all[-1]:
                            xmax = observables_all[-1]
                            CropWorkspace(InputWorkspace=to_be_interpolated_ws, OutputWorkspace=to_be_interpolated_ws,
                                          XMax=xmax)
                    for i in range(number_histograms):
                        # Y values
                        yfunction = interp1d(mtd[to_be_interpolated_ws].getItem(sample_index).readX(i),
                                             mtd[to_be_interpolated_ws].getItem(sample_index).readY(i))
                        # E values
                        efunction = interp1d(mtd[to_be_interpolated_ws].getItem(sample_index).readX(i),
                                             mtd[to_be_interpolated_ws].getItem(sample_index).readE(i))
                        mtd[result_name].setY(i, yfunction(mtd[sample_ws].getItem(sample_index).readX(i)))
                        mtd[result_name].setE(i, efunction(mtd[sample_ws].getItem(sample_index).readX(i)))

            list_result_workspace.append(result_name)

        self.log().debug('List of workspaces to be sorted: {0}'.format(list_result_workspace))

        # Workspaces must be sorted to allow group operations with the sample workspace
        def _key(item):
            set_item = re.findall("\d+\.\d", item)
            return float(set_item[0])

        GroupWorkspaces(InputWorkspaces=sorted(list_result_workspace, key=_key),
                        OutputWorkspace=interpolated_ws)

    def _create_matrices(self, label):
        '''
        For each reduction type concatenates the workspaces putting the given sample log value as x-axis
        Creates a group workspace for the given label, that contains 2D workspaces for each distinct energy value
        @param label :: sample, background or calibration
        '''

        togroup = []
        for energy in sorted(self._all_runs[label]):

            ws_list = self._all_runs[label][energy]
            size = len(self._all_runs[label][energy])
            wsname = self._red_ws + '_' + label + '_' + str(energy)
            togroup.append(wsname)
            nspectra = mtd[ws_list[0]].getNumberHistograms()
            observable_array = self._get_observable_values(self._all_runs[label][energy])

            y_values = np.zeros(size*nspectra)
            e_values = np.zeros(size*nspectra)
            x_values = np.zeros(size*nspectra)

            CreateWorkspace(DataX=x_values, DataY=y_values, DataE=e_values, NSpec=nspectra,
                            WorkspaceTitle=wsname, Distribution=True, ParentWorkspace=mtd[ws_list[0]],
                            OutputWorkspace=wsname)

            # AddSampleLog(Workspace=wsname, LogName='ReducedRuns', LogText=str(run_numbers))

            for spectrum in range(nspectra):

                mtd[wsname].setX(spectrum, np.array(observable_array))

                y_data = np.zeros(size)
                e_data = np.zeros(size)

                for channel in range(size):
                    y_data[channel] = mtd[ws_list[channel]].readY(spectrum)[0]
                    e_data[channel] = mtd[ws_list[channel]].readE(spectrum)[0]

                mtd[wsname].setY(spectrum, y_data)
                mtd[wsname].setE(spectrum, e_data)

            if self._sortX:
                SortXAxis(InputWorkspace=wsname, OutputWorkspace=wsname)

            self._set_x_label(wsname)

        for energy, ws_list in self._all_runs[label].iteritems():
            for ws in ws_list:
                DeleteWorkspace(ws)

        GroupWorkspaces(InputWorkspaces=togroup, OutputWorkspace=self._red_ws+'_'+label)

    def _set_x_label(self, ws):
        '''
        Sets the x-axis label
        @param ws :: input workspace
        '''

        axis = mtd[ws].getAxis(0)
        if self._observable == 'sample.temperature':
            axis.setUnit("Label").setLabel('Temperature', 'K')
        elif self._observable == 'sample.pressure':
            axis.setUnit("Label").setLabel('Pressure', 'P')
        elif self._observable == 'start_time':
            axis.setUnit("Label").setLabel('Time', 'seconds')
        else:
            axis.setUnit("Label").setLabel(self._observable, 'Unknown')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReductionFWS)
