from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import *  # noqa
from mantid.kernel import *  # noqa
from mantid.api import *  # noqa
from mantid import mtd


class IndirectILLReductionQENS(DataProcessorAlgorithm):

    _sample_file = None
    _alignment_file = None
    _background_file = None
    _calibration_file = None
    _sum_all_runs = None
    _mask_bins = None
    _unmirror_option = None
    _mirror_sense = None
    _back_scaling = None
    _criteria = None
    _progress = None
    _common_args = {}
    _peak_range = []
    _runs = []

    def category(self):
        return "Workflow\\MIDAS;Inelastic\\Reduction"

    def summary(self):
        return 'Performs complete QENS multiple file reduction for ILL indirect geometry data, instrument IN16B.'

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

        self.declareProperty(MultipleFileProperty('AlignmentRun',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Run number(s) of vanadium run(s) used for '
                                 'peak alignment for UnmirrorOption=[5, 7]')

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

        self.declareProperty(name='SumRuns',
                             defaultValue=False,
                             doc='Whether to sum all the input runs.')

        self.declareProperty(name='MirrorSense',
                             defaultValue=False,
                             doc='Whether the runs where recorded in mirror sense.')

        self.declareProperty(name='MaskOverflownBins',
                             defaultValue=True,
                             doc='Whether to mask the bins that overflow the x-axis '
                                 'after performing circular shift for peak alignment '
                                 'during unmirroring step.')

        self.declareProperty(name='CropDeadMonitorChannels', defaultValue=False,
                             doc='Whether or not to exclude the first and last few channels '
                                 'with 0 monitor count in the energy transfer formula.')

        self.declareProperty(name='UnmirrorOption', defaultValue=6,
                             validator=IntBoundedValidator(lower=0, upper=7),
                             doc='Unmirroring options (for MirrorSense): \n'
                                 '0 no unmirroring\n'
                                 '1 sum of left and right\n'
                                 '2 left\n'
                                 '3 right\n'
                                 '4 shift right according to left and sum\n'
                                 '5 like 4, but use alignment run for peak positions\n'
                                 '6 center both left and right at zero and sum\n'
                                 '7 like 6, but use alignment run for peak positions')

        self.declareProperty(name='BackgroundScalingFactor', defaultValue=1.,
                             validator=FloatBoundedValidator(lower=0),
                             doc='Scaling factor for background subtraction')

        self.declareProperty(name='CalibrationPeakRange', defaultValue=[-0.003,0.003],
                             validator=FloatArrayMandatoryValidator(),
                             doc='Peak range for integration over calibration file peak (in mev)')

        self.declareProperty(WorkspaceGroupProperty("OutputWorkspace", "red",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the reduced workspace(s).")

    def validateInputs(self):

        issues = dict()

        uo = self.getProperty('UnmirrorOption').value

        ms = self.getProperty('MirrorSense').value

        if (uo == 5 or uo == 7) and ms and not self.getPropertyValue('AlignmentRun'):
            issues['AlignmentRun'] = 'Given UnmirrorOption requires alignment run to be set'

        if (uo > 0 and uo < 6) and not ms:
            issues['UnmirrorOption'] = 'For the given MirrorSense setting only UnmirrorOption 0,6,7 are valid.'

        if ms and uo == 0:
            issues['UnmirrorOption'] = 'For the given MirrorSense setting UnmirrorOption 0 is not valid.'

        return issues

    def setUp(self):

        self._sample_file = self.getPropertyValue('Run')
        self._alignment_file = self.getPropertyValue('AlignmentRun').replace(',', '+') # automatic summing
        self._background_file = self.getPropertyValue('BackgroundRun').replace(',', '+') # automatic summing
        self._calibration_file = self.getPropertyValue('CalibrationRun').replace(',', '+') # automatic summing
        self._sum_all_runs = self.getProperty('SumRuns').value
        self._mask_bins = self.getProperty('MaskOverflownBins').value
        self._unmirror_option = self.getProperty('UnmirrorOption').value
        self._mirror_sense = self.getProperty('MirrorSense').value
        self._back_scaling = self.getProperty('BackgroundScalingFactor').value
        self._peak_range = self.getProperty('CalibrationPeakRange').value
        self._red_ws = self.getPropertyValue('OutputWorkspace')

        # arguments to pass to IndirectILLEnergyTransfer
        self._common_args['MapFile'] = self.getPropertyValue('MapFile')
        self._common_args['Analyser'] = self.getPropertyValue('Analyser')
        self._common_args['Reflection'] = self.getPropertyValue('Reflection')
        self._common_args['CropDeadMonitorChannels'] = self.getProperty('CropDeadMonitorChannels').value

        if self._sum_all_runs is True:
            self.log().notice('All the sample runs will be summed')
            self._sample_file = self._sample_file.replace(',', '+')

        # Nexus metadata criteria for QENS type of data
        self._criteria = '$/entry0/instrument/Doppler/maximum_delta_energy$ != 0. and ' \
                         '$/entry0/instrument/Doppler/velocity_profile$ == 0 and ' \
                         '$/entry0/instrument/Doppler/mirror_sense$ == '

        # mirror sense criteria
        self._criteria += '14' if self._mirror_sense else '16'

        # empty the runs list
        del self._runs[:]

    def _filter_files(self, files, label):
        '''
        Filters the given list of files according to QENS and mirror sense criteria
        @param  files :: list of input files (i.e. , and + separated string)
        @param  label :: label of error message if nothing left after filtering
        @throws RuntimeError :: when nothing left after filtering
        @return :: the list of input files that passsed the criteria
        '''

        files = SelectNexusFilesByMetadata(files, self._criteria)

        if not files:
            raise RuntimeError('None of the {0} runs satisfied the QENS and MirrorSense criteria.'.format(label))
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

        if self._alignment_file:
            self._alignment_file = self._filter_files(self._alignment_file, 'alignment')

    def _warn_negative_integral(self, ws, message):
        '''
        Raises an error if an integral of the given workspace is <= 0
        @param ws :: input workspace name
        @param message :: message suffix for the error
        @throws RuntimeError :: on non-positive integral found
        '''

        tmp_int = '__tmp_int'+ws
        Integration(InputWorkspace=ws,OutputWorkspace=tmp_int)

        for item in range(mtd[tmp_int].getNumberOfEntries()):
            for index in range(mtd[tmp_int].getItem(item).getNumberHistograms()):
                if mtd[tmp_int].getItem(item).readY(index)[0] <= 0:
                    raise RuntimeError('Negative or 0 integral in spectrum #{0} {1}'.format(index,message))

        DeleteWorkspace(tmp_int)

    def PyExec(self):

        self.setUp()

        self._filter_all_input_files()

        if self._background_file:
            IndirectILLEnergyTransfer(Run = self._background_file, OutputWorkspace = 'background', **self._common_args)
            Scale(InputWorkspace='background',Factor=self._back_scaling,OutputWorkspace='background')

        if self._calibration_file:
            IndirectILLEnergyTransfer(Run = self._calibration_file, OutputWorkspace = 'calibration', **self._common_args)
            Integration(InputWorkspace='calibration',RangeLower=self._peak_range[0],RangeUpper=self._peak_range[1],
                        OutputWorkspace='calibration')
            self._warn_negative_integral('calibration','in calibration workspace.')

        if self._unmirror_option == 5 or self._unmirror_option == 7:
            IndirectILLEnergyTransfer(Run = self._alignment_file, OutputWorkspace = 'alignment', **self._common_args)

        runs = self._sample_file.split(',')

        self._progress = Progress(self, start=0.0, end=1.0, nreports=len(runs))

        for run in runs:
            self._reduce_run(run)

        if self._background_file:
            DeleteWorkspace('background')

        if self._calibration_file:
            DeleteWorkspace('calibration')

        if self._unmirror_option == 5 or self._unmirror_option == 7:
            DeleteWorkspace('alignment')

        GroupWorkspaces(InputWorkspaces=self._runs,OutputWorkspace=self._red_ws)
        self.setProperty('OutputWorkspace',self._red_ws)

    def _reduce_run(self,run):
        '''
        Reduces the given (single or summed multiple) run
        @param run :: run path
        '''

        runnumber = os.path.basename(run).split('.')[0]

        self._progress.report("Reducing run #" + run)

        ws = runnumber + '_' + self._red_ws

        IndirectILLEnergyTransfer(Run = run, OutputWorkspace = ws, **self._common_args)

        if self._background_file:
            Minus(LHSWorkspace=ws, RHSWorkspace='background', OutputWorkspace=ws)
            self._warn_negative_integral(ws,'after background subtraction.')

        if self._calibration_file:
            Divide(LHSWorkspace=ws, RHSWorkspace='calibration', OutputWorkspace=ws)

        self._perform_unmirror(ws)

        # register to reduced runs list
        self._runs.append(ws)

    def _perform_unmirror(self, ws):
        '''
        Performs unmirroring, i.e. summing of left and right wings for two-wing data or centering the one wing data
        @param ws :: input workspace
        '''

        outname = mtd[ws].getName()

        self.log().information('Unmirroring workspace {0} with option {1} and mirror sense {2}'
                               .format(outname,self._unmirror_option, self._mirror_sense))

        if not self._mirror_sense:   # one wing

            name = mtd[ws].getItem(0).getName()

            if self._unmirror_option == 0:
                RenameWorkspace(InputWorkspace = name, OutputWorkspace = outname)
            elif self._unmirror_option == 6:
                MatchPeaks(InputWorkspace = name, OutputWorkspace = outname, MaskBins = self._mask_bins)
                DeleteWorkspace(name)
            elif self._unmirror_option == 7:
                MatchPeaks(InputWorkspace = name, InputWorkspace2 = mtd['alignment'].getItem(0).getName(),
                           MatchInput2ToCenter = True, OutputWorkspace = outname, MaskBins = self._mask_bins)
                DeleteWorkspace(name)

        else:  # two wing

            left = mtd[ws].getItem(0).getName()
            right = mtd[ws].getItem(1).getName()
            UnGroupWorkspace(ws)

            if self._unmirror_option == 2:
                RenameWorkspace(InputWorkspace=left, OutputWorkspace=outname)
                DeleteWorkspace(right)
            elif self._unmirror_option == 3:
                RenameWorkspace(InputWorkspace=right, OutputWorkspace=outname)
                DeleteWorkspace(left)
            elif self._unmirror_option == 4:
                MatchPeaks(InputWorkspace=right, InputWorkspace2=left, OutputWorkspace=right, MaskBins = self._mask_bins)
            elif self._unmirror_option == 5:
                MatchPeaks(InputWorkspace=right, InputWorkspace2=mtd['alignment'].getItem(0).getName(),
                           InputWorkspace3=mtd['alignment'].getItem(1).getName(),OutputWorkspace=right)
            elif self._unmirror_option == 6:
                MatchPeaks(InputWorkspace=left, OutputWorkspace=left)
                MatchPeaks(InputWorkspace=right, OutputWorkspace=right)
            elif self._unmirror_option == 7:
                MatchPeaks(InputWorkspace=left, InputWorkspace2=mtd['alignment'].getItem(0).getName(),
                           OutputWorkspace=left,MatchInput2ToCenter=True)
                MatchPeaks(InputWorkspace=right, InputWorkspace2=mtd['alignment'].getItem(1).getName(),
                           OutputWorkspace=right, MatchInput2ToCenter=True)

            if self._unmirror_option > 3 or self._unmirror_option == 1:
                Plus(LHSWorkspace=left, RHSWorkspace=right, OutputWorkspace=outname)
                Scale(InputWorkspace=outname, OutputWorkspace=outname, Factor=0.5)
                DeleteWorkspace(left)
                DeleteWorkspace(right)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReductionQENS)
