#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)


from mantid.simpleapi import *  # noqa
from mantid.kernel import *  # noqa
from mantid.api import *  # noqa
from mantid import mtd
import numpy as np
import re


class IndirectILLReductionFWS(DataProcessorAlgorithm):

    _sample_file = None
    _background_file = None
    _calibration_file = None
    _mirror_sense = None
    _observable = None
    _sortX = None
    _red_ws = None
    _back_scaling = None
    _criteria = None
    _progress = None
    _analyser = None
    _reflection = None
    _common_args = {}
    _runs = []
    _back_runs = []
    _calib_runs = []

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

        self.declareProperty(name='MirrorSense',
                             defaultValue=False,
                             doc='Whether the runs where recorded in mirror sense.')

        self.declareProperty(name='BackgroundScalingFactor', defaultValue=1.,
                             validator=FloatBoundedValidator(lower=0),
                             doc='Scaling factor for background subtraction')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', 'red',
                                                    direction=Direction.Output),
                             doc='Output workspace group')

    def validateInputs(self):

        issues = dict()

        return issues

    def setUp(self):

        self._sample_file = self.getPropertyValue('Run')
        self._background_file = self.getPropertyValue('BackgroundRun')
        self._calibration_file = self.getPropertyValue('CalibrationRun')
        self._mirror_sense = self.getProperty('MirrorSense').value
        self._observable = self.getPropertyValue('Observable')
        self._sortX = self.getProperty('SortXAxis').value
        self._back_scaling =self.getProperty('BackgroundScalingFactor').value

        # arguments to pass to IndirectILLEnergyTransfer
        self._common_args['MapFile'] = self.getPropertyValue('MapFile')
        self._common_args['Analyser'] = self.getPropertyValue('Analyser')
        self._common_args['Reflection'] = self.getPropertyValue('Reflection')

        self._red_ws = self.getPropertyValue('OutputWorkspace')

        # Nexus metadata criteria for FWS type of data (both EFWS and IFWS)
        self._criteria = '($/entry0/instrument/Doppler/maximum_delta_energy$ == 0. or ' \
                         '$/entry0/instrument/Doppler/velocity_profile$ == 1) and ' \
                         '$/entry0/instrument/Doppler/mirror_sense$ == '

        # mirror sense criteria
        self._criteria += '14' if self._mirror_sense else '16'

        # make sure observable entry also exists (value is not important)
        self._criteria += 'and ($/entry0/' + self._observable.replace('.', '/') + '$ or True)'

        # empty the runs list
        del self._runs[:]
        del self._calib_runs[:]
        del self._back_runs[:]

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
            self.log().information('Filtered {0} runs are: {0} \\n'.format(label,files.replace(',','\\n')))

        return files

    def _filter_all_input_files(self):
        '''
        Filters all the lists of input files needed for the reduction.
        '''

        self._sample_file = self._filter_files(self._sample_file,'sample')

        if self._background_file:
            self._background_file = self._filter_files(self._background_file, 'background')

        if self._calibration_file:
            self._calibration_file = self._filter_files(self._calibration_file, 'calibration')

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
            Plus(LHSWorkspace=int1, RHSWorkspace = int2, OutputWorkspace = ws)
            DeleteWorkspace(int1)
            DeleteWorkspace(int2)

    def _perform_unmirror(self, groupws):
        '''
        Sums the integrals of left and right for two wings, or returns the integral of one wing
        @param ws :: group workspace containing one ws for one wing, and two ws for two wing data
        '''
        if self._mirror_sense:
            left = mtd[groupws].getItem(0).getName()
            right = mtd[groupws].getItem(1).getName()
            Sum(LHSWorkspace=left,RHSWorkspace=right,OutputWorkspace=groupws)
            DeleteWorkspace(left)
            DeleteWorkspace(right)
        else:
            RenameWorkspace(InputWorkspace=mtd[groupws].getItem(0),OutputWorkspace=groupws)

    def PyExec(self):

        self.setUp()

        self._filter_all_input_files()

        runs = self._sample_file.split(',')
        back_runs = self._background_file.split(',')
        calib_runs = self._calibration_file.split(',')

        self._progress = Progress(self, start=0.0, end=1.0, nreports=len(runs))

        for run in runs:
            self._progress.report("Reducing run #" + run)
            self._reduce_run(run)
            self._runs.append(run)

        # create matrix here

        if self._background_file:
            for brun in back_runs:
                self._reduce_run(brun)

            # create matrix here
            # interpolate
            # subtract

        if self._calibration_file:
            for crun in calib_runs:
                self._reduce_run(crun)

            # create matrix here
            # interpolate
            # divide

        #self._set_workspace_properties()

    def _reduce_run(self, run):
        '''
        Reduces the given (single or summed multiple) run
        @param run :: run path
        '''

        runnumber = os.path.basename(run.split('+')[0]).split('.')[0]

        ws = runnumber+'_'+self._red_ws

        IndirectILLEnergyTransfer(Run=run,OutputWorkspace=ws,**self._common_args)

        energy = mtd[ws].getItem(0).getRun().getLogData('Doppler.maximum_delta_energy').value

        if energy == 0.:
            # Elastic, integrate over full energy range
            Integration(InputWorkspace=ws, OutputWorkspace=ws)
        else:
            # Inelastic, do something more complex
            self._ifws_integrate(ws)

        self._perform_unmirror(ws)

    def _get_observable(self, input_ws):
        """
        Set list self._observable
        Args:
            input_ws: GroupWorkspace, all reduced workspaces of one energy
        """
        self._observable = []
        for index in range(mtd[input_ws].getNumberOfEntries()):
            entity = mtd[input_ws].getItem(index).getRun().getLogData(self._observable_name).value
            if self._observable_name != 'start_time':
                entity = float(entity)
            else:
                # start_time is a string like 2016-09-12T09:09:15 (date using hyphen T time using colon)
                entity = str(entity)
                # Get all numbers
                entity = re.findall(r'\d+', entity)
                hours = int(entity[3])
                minutes = int(entity[4])
                seconds = int(entity[5])
                entity = seconds + 60 * minutes + 3600 * hours
                # Improvement possible: account for different days!

            self.log().debug('{0}: {1}'.format(self._observable_name, entity))
            self._observable.append(entity)

    def _create_matrix(self, group, output_ws):
        """
        Args:
            group : GroupWorkspace, will be transformed to one MatrixWorkspace, Workspace2D, not a histogram
            output_ws : name of the output workspace

        """
        self._get_observable(group)

        number_hists = mtd[group].getItem(0).getNumberHistograms()
        number_workspaces = mtd[group].getNumberOfEntries()
        length = number_workspaces * number_hists

        self.log().notice('Final post-processing workspace has {0} channel(s) and {1} spectra'.format(number_workspaces,
                                                                                                      number_hists))

        # Initialisation of the new workspace with values of the first workspace
        y_values = np.zeros([1, length])
        e_values = np.zeros([1, length])
        x_values = np.zeros([1, length])
        # Variable that will store one row of the final matrix
        y_row = np.zeros(number_workspaces)
        e_row = np.zeros(number_workspaces)

        CreateWorkspace(DataX=x_values, DataY=y_values, DataE=e_values, NSpec=number_hists,
                        WorkspaceTitle=output_ws, Distribution=True, ParentWorkspace=mtd[group].getItem(0),
                        OutputWorkspace=output_ws)

        # Get and set workspace entries
        for hist in range(number_hists):
            mtd[output_ws].setX(hist, np.array(self._observable))
            for index in range(number_workspaces):
                workspace = mtd[group].getItem(index)
                y_row[index] = workspace.readY(hist)
                e_row[index] = workspace.readE(hist)
            mtd[output_ws].setY(hist, y_row)
            mtd[output_ws].setE(hist, e_row)

        if self._sortX:
            SortXAxis(InputWorkspace=output_ws, OutputWorkspace=output_ws)

        run_numbers = []
        for index in range(number_workspaces):
            run_numbers.append(mtd[group].getItem(index).getRun().getLogData('run_number').value)

        # Save run number for files of selected energy
        AddSampleLog(Workspace=output_ws, LogName='ReducedRuns', LogText=str(run_numbers))

        # Label x-values
        axis = mtd[output_ws].getAxis(0)
        if self._observable_name == 'sample.temperature':
            axis.setUnit("Label").setLabel('Temperature', 'K')
        elif self._observable_name == 'sample.pressure':
            axis.setUnit("Label").setLabel('Pressure', 'P')
        elif self._observable_name == 'start_time':
            axis.setUnit("Label").setLabel('Time', 'seconds')
        else:
            axis.setUnit("Label").setLabel(self._observable_name, 'Unknown')

    def _get_energies(self):
        '''
        Returns: energies   a list of unique energies
                 indices    indices of the energy values

        '''
        energies = []
        number_of_workspaces = mtd[self._out_ws].getNumberOfEntries()

        # Count appearances of energy, this will be the number of GroupWorkspaces needed
        for i in range(number_of_workspaces):
            the_workspace = mtd[self._out_ws].getItem(i).getRun()
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

    def _get_list_of_workspace_names(self):
        '''

        Returns: a list of all workspaces after calling IndirectILLReduction

        '''
        _selected_runs = []
        number_of_workspaces = mtd[self._out_ws].getNumberOfEntries()
        for i in range(number_of_workspaces):
            _selected_runs.append(re.findall(r'\d+', self.selected_runs[i]))
        self.log().debug('List of runs: {0}'.format(_selected_runs))

        return _selected_runs

    def _set_workspace_properties(self):
        """
        Sets the properties of each GroupWorkspace for each elastic and inelastic energy

        """

        number_of_workspaces = mtd[self._out_ws].getNumberOfEntries()

        energy_values, indices = self._get_energies()

        number_groups = len(energy_values)
        self.log().debug('Number of GroupWorkspaces {0}'.format(number_groups))

        _selected_runs = self._get_list_of_workspace_names()

        UnGroupWorkspace(self._out_ws)

        # List of final output workspaces
        matrices = []

        for mygroup in range(number_groups):
            self.log().debug('Group of energy {0} micro eV'.format(energy_values[mygroup]))

            # Create a new list for each new group of workspaces
            group = []
            # Create GroupWorkspace name of the energy group
            group_name = 'group' + str(energy_values[mygroup])

            # Add the workspaces to the group, which belong to the same energy
            for ith_workspace in range(number_of_workspaces):
                if mygroup == indices[ith_workspace]:
                    # Pick workspace with same energy value
                    self.log().debug('Join workspace {0}'.format(_selected_runs[ith_workspace][0] + '_' + self._out_ws))
                    group.append(_selected_runs[ith_workspace][0] + '_' + self._out_ws)

            # Create a GroupWorkspace
            self.log().debug('GroupWorkspaces {0} for grouping {1}'.format(group_name, group))
            GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=group_name)

            # Create an additional workspace containing all workspaces of the group (transversed)
            matrix = self._out_ws + '_' + str(energy_values[mygroup])
            self._create_matrix(group_name, matrix)
            matrices.append(matrix)

            DeleteWorkspace(group_name)

        self.log().notice('Set OutputWorkspace property for grouped workspaces {0}'.format(matrices))
        GroupWorkspaces(InputWorkspaces=matrices, OutputWorkspace=self._out_ws)
        self.setProperty('OutputWorkspace', self._out_ws)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReductionFWS)
