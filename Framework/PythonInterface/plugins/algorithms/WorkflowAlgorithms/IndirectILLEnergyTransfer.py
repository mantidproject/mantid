from __future__ import (absolute_import, division, print_function)

import os
import numpy as np
from mantid import config, mtd, logger
from mantid.kernel import StringListValidator, Direction
from mantid.api import PythonAlgorithm, MultipleFileProperty, FileProperty, \
    WorkspaceGroupProperty, FileAction, Progress
from mantid.simpleapi import *  # noqa


def _ws_or_none(s):
    return mtd[s] if s != '' else None


def _extract_workspace(ws, ws_out, x_start, x_end):
    """
    Extracts a part of the workspace and
    shifts the x-axis to start from 0
    @param  ws      :: input workspace name
    @param  ws_out  :: output workspace name
    @param  x_start :: start bin of workspace to be extracted
    @param  x_end   :: end bin of workspace to be extracted
    """
    CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws_out, XMin=x_start, XMax=x_end)
    ScaleX(InputWorkspace=ws_out, OutputWorkspace=ws_out, Factor=-x_start, Operation='Add')


class IndirectILLEnergyTransfer(PythonAlgorithm):

    _run_file = None
    _map_file = None
    _parameter_file = None
    _reduction_type = None
    _mirror_sense = None
    _doppler_energy = None
    _velocity_profile = None
    _instrument_name = None
    _instrument = None
    _analyser = None
    _reflection = None
    _dead_channels = None
    _ws = None
    _red_ws = None
    _psd_int_range = None
    _use_map_file = None
    _spectrum_axis = None
    _efixed = None

    def category(self):
        return "Workflow\\MIDAS;Workflow\\Inelastic;Inelastic\\Indirect;Inelastic\\Reduction;ILL\\Indirect"

    def summary(self):
        return 'Performs initial energy transfer reduction for ILL indirect geometry data, instrument IN16B.'

    def seeAlso(self):
        return [ "IndirectILLReductionQENS","IndirectILLReductionFWS" ]

    def name(self):
        return "IndirectILLEnergyTransfer"

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run (s).')

        self.declareProperty(FileProperty('MapFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['map','xml']),
                             doc='Filename of the detector grouping map file to use. \n'
                                 'By default all the pixels will be summed per each tube. \n'
                                 'Use .map or .xml file (see GroupDetectors documentation) '
                                 'only if different range is needed for each tube.')

        self.declareProperty(name='ManualPSDIntegrationRange',defaultValue=[1,128],
                             doc='Integration range of vertical pixels in each PSD tube. \n'
                                 'By default all the pixels will be summed per each tube. \n'
                                 'Use this option if the same range (other than default) '
                                 'is needed for all the tubes.')

        self.declareProperty(name='Analyser',
                             defaultValue='silicon',
                             validator=StringListValidator(['silicon']),
                             doc='Analyser crystal.')

        self.declareProperty(name='Reflection',
                             defaultValue='111',
                             validator=StringListValidator(['111', '311']),
                             doc='Analyser reflection.')

        self.declareProperty(name='CropDeadMonitorChannels',defaultValue=False,
                             doc='Whether or not to exclude the first and last few channels '
                                 'with 0 monitor count in the energy transfer formula.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='Group name for the reduced workspace(s).')

        self.declareProperty(name='SpectrumAxis', defaultValue='SpectrumNumber',
                             validator=StringListValidator(['SpectrumNumber', '2Theta', 'Q', 'Q2']),
                             doc='The spectrum axis conversion target.')

    def validateInputs(self):

        issues = dict()

        self._psd_int_range = self.getProperty('ManualPSDIntegrationRange').value

        if not self.getPropertyValue('MapFile'):
            if len(self._psd_int_range) != 2:
                issues['ManualPSDIntegrationRange'] = 'Specify comma separated pixel range, e.g. 1,128'
            elif self._psd_int_range[0] < 1 or self._psd_int_range[1] > 128 \
                    or self._psd_int_range[0] >= self._psd_int_range[1]:
                issues['ManualPSDIntegrationRange'] = 'Start or end pixel number out is of range [1-128], or has wrong order'

        return issues

    def setUp(self):

        self._run_file = self.getPropertyValue('Run').replace(',','+') # automatic summing
        self._analyser = self.getPropertyValue('Analyser')
        self._map_file = self.getPropertyValue('MapFile')
        self._reflection = self.getPropertyValue('Reflection')
        self._dead_channels = self.getProperty('CropDeadMonitorChannels').value
        self._red_ws = self.getPropertyValue('OutputWorkspace')
        self._spectrum_axis = self.getPropertyValue('SpectrumAxis')

        if self._map_file or (self._psd_int_range[0] == 1 and self._psd_int_range[1] == 128):
            self._use_map_file = True
        else:
            self._use_map_file = False

    def _load_map_file(self):
        """
        Loads the detector grouping map file
        @throws RuntimeError :: if neither the user defined nor the default file is found
        """

        self._instrument_name = self._instrument.getName()
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')
        idf_directory = config['instrumentDefinition.directory']
        ipf_name = self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml'
        self._parameter_file = os.path.join(idf_directory, ipf_name)
        self.log().information('Set parameter file : {0}'.format(self._parameter_file))

        if self._use_map_file:
            if self._map_file == '':
                # path name for default map file
                if self._instrument.hasParameter('Workflow.GroupingFile'):
                    grouping_filename = self._instrument.getStringParameter('Workflow.GroupingFile')[0]
                    self._map_file = os.path.join(config['groupingFiles.directory'], grouping_filename)
                else:
                    raise RuntimeError("Failed to find default detector grouping file. Please specify manually.")

            self.log().information('Set detector map file : {0}'.format(self._map_file))

    def _mask(self, ws, xstart, xend):
        """
        Masks the first and last bins
        @param   ws           :: input workspace name
        @param   xstart       :: MaskBins between x[0] and x[xstart]
        @param   xend         :: MaskBins between x[xend] and x[-1]
        """
        x_values = mtd[ws].readX(0)

        if xstart > 0:
            logger.debug('Mask bins smaller than {0}'.format(xstart))
            MaskBins(InputWorkspace=ws, OutputWorkspace=ws, XMin=x_values[0], XMax=x_values[xstart])

        if xend < len(x_values) - 1:
            logger.debug('Mask bins larger than {0}'.format(xend))
            MaskBins(InputWorkspace=ws, OutputWorkspace=ws, XMin=x_values[xend + 1], XMax=x_values[-1])

    def _convert_to_energy(self, ws):
        """
        Converts the x-axis from raw channel number to energy transfer
        @param ws :: input workspace name
        """

        x = mtd[ws].readX(0)
        size = mtd[ws].blocksize()
        mid = (x[-1] + x[0])/ 2.
        scale = 0.001  # from micro ev to mili ev

        factor = size / (size - 1)

        # minus sign is needed
        if self._doppler_energy != 0:
            formula = '-(x/{0} - 1)*{1}'.format(mid, self._doppler_energy * scale * factor)
        else:
            # Center the data for elastic fixed window scan, for integration over the elastic peak
            formula = '-(x-{0})*{1}'.format(mid-0.5, 1. / scale)
            self.log().notice('The only energy value is 0 meV. Ignore the x-axis.')

        self.log().information('Energy conversion formula is: {0}'.format(formula))

        ConvertAxisByFormula(InputWorkspace=ws, OutputWorkspace=ws, Axis='X', Formula=formula, AxisUnits = 'DeltaE')

    def _monitor_max_range(self, ws):
        """
        Gives the bin indices of the first and last peaks in the monitor
        @param ws :: input workspace name
        return    :: [xmin,xmax]
        """

        y = mtd[ws].readY(0)
        size = len(y)
        mid = int(size / 2)
        imin = np.nanargmax(y[0:mid])
        imax = np.nanargmax(y[mid:size]) + mid
        return imin, imax

    def _monitor_zero_range(self, ws):
        """
        Gives the bin indices of the first and last non-zero bins in monitor
        @param ws :: input workspace name
        return    :: [start,end]
        """

        y = mtd[ws].readY(0)
        nonzero = np.argwhere(y!=0)
        start = nonzero[0][0] if nonzero.any() else 0
        end = nonzero[-1][0] if nonzero.any() else len(y)
        return start,end

    def _setup_run_properties(self):
        """
        Sets up the doppler properties, and deduces the reduction type
        @throws RuntimeError :: If anyone of the 3 required entries is missing
        """

        run = mtd[self._ws].getRun()

        message = 'is not defined. Check your data.'

        if run.hasProperty('Doppler.mirror_sense'):
            self._mirror_sense = run.getLogData('Doppler.mirror_sense').value
        else:
            raise RuntimeError('Mirror sense '+ message)

        if run.hasProperty('Doppler.maximum_delta_energy'):
            self._doppler_energy = run.getLogData('Doppler.maximum_delta_energy').value
        else:
            raise RuntimeError('Maximum delta energy '+ message)

        if run.hasProperty('Doppler.velocity_profile'):
            self._velocity_profile = run.getLogData('Doppler.velocity_profile').value
        else:
            raise RuntimeError('Velocity profile '+ message)

        if self._doppler_energy == 0.:
            self._reduction_type = 'EFWS'
        else:
            if self._velocity_profile == 0:
                self._reduction_type = 'QENS'
            else:
                self._reduction_type = 'IFWS'

    def PyExec(self):

        self.setUp()

        self._progress = Progress(self, start=0.0, end=1.0, nreports=self._run_file.count('+'))

        LoadAndMerge(Filename=self._run_file, OutputWorkspace=self._red_ws, LoaderName='LoadILLIndirect')

        self._instrument = mtd[self._red_ws].getInstrument()

        self._load_map_file()

        run = '{0:06d}'.format(mtd[self._red_ws].getRunNumber())

        self._ws = self._red_ws + '_' + run

        if self._run_file.count('+') > 0:  # multiple summed files
            self._ws += '_multiple'

        RenameWorkspace(InputWorkspace=self._red_ws, OutputWorkspace=self._ws)

        LoadParameterFile(Workspace=self._ws, Filename=self._parameter_file)

        self._efixed = self._instrument.getNumberParameter('Efixed')[0]

        self._setup_run_properties()

        if self._mirror_sense == 14:      # two wings, extract left and right

            size = mtd[self._ws].blocksize()
            left = self._ws + '_left'
            right = self._ws + '_right'
            _extract_workspace(self._ws, left, 0, int(size/2))
            _extract_workspace(self._ws, right, int(size/2), size)
            DeleteWorkspace(self._ws)
            self._reduce_one_wing(left)
            self._reduce_one_wing(right)
            GroupWorkspaces(InputWorkspaces=[left,right],OutputWorkspace=self._red_ws)

        elif self._mirror_sense == 16:    # one wing

            self._reduce_one_wing(self._ws)
            GroupWorkspaces(InputWorkspaces=[self._ws],OutputWorkspace=self._red_ws)

        self.setProperty('OutputWorkspace',self._red_ws)

    def _reduce_one_wing(self, ws):
        """
        Reduces given workspace assuming it is one wing already
        @param ws :: input workspace name
        """

        mon = '__mon_'+ws

        ExtractSingleSpectrum(InputWorkspace=ws, OutputWorkspace=mon, WorkspaceIndex=0)

        if self._use_map_file:
            GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws, MapFile=self._map_file)
        else:
            self._group_detectors_with_range(ws)

        xmin, xmax = self._monitor_zero_range(mon)

        self._normalise_to_monitor(ws, mon)

        if self._reduction_type == 'QENS':
            if self._dead_channels:
                CropWorkspace(InputWorkspace=ws,OutputWorkspace=ws,XMin=float(xmin),XMax=float(xmax+1.))
                ScaleX(InputWorkspace=ws, OutputWorkspace=ws, Factor=-float(xmin), Operation='Add')
            else:
                self._mask(ws, xmin, xmax)

        DeleteWorkspace(mon)

        self._convert_to_energy(ws)

        target = None
        if self._spectrum_axis == '2Theta':
            target = 'Theta'
        elif self._spectrum_axis == 'Q':
            target = 'ElasticQ'
        elif self._spectrum_axis == 'Q2':
            target = 'ElasticQSquared'

        if self._spectrum_axis != 'SpectrumNumber':
            ConvertSpectrumAxis(InputWorkspace=ws,OutputWorkspace=ws,
                                EMode='Indirect',Target=target,EFixed=self._efixed)

    def _group_detectors_with_range(self, ws):
        """
        Groups (sums) the multi-detector's pixels according to given range
        @param ws :: input workspace name
        """
        pattern = ''

        for tube in range(1,17):
            pattern += str((tube - 1) * 128 + self._psd_int_range[0])
            pattern += '-'
            pattern += str((tube - 1) * 128 + self._psd_int_range[1])
            pattern += ','

        num_single_det = mtd[ws].getNumberHistograms()-16*128-1

        for single_det in range(num_single_det):
            sd_index = 16*128 + single_det + 1
            pattern += str(sd_index)
            pattern += ','

        pattern = pattern.rstrip(',')

        self.log().information("Grouping the detectors with pattern:\n {0}"
                               .format(pattern))

        GroupDetectors(InputWorkspace=ws,OutputWorkspace=ws,GroupingPattern=pattern)

    def _normalise_to_monitor(self, ws, mon):
        """
        Normalises the ws to the monitor dependent on the reduction type
        @param ws :: input workspace name
        @param mon :: ws's monitor
        """
        x = mtd[ws].readX(0)

        if self._reduction_type == 'QENS':
            # Normalise bin-to-bin, do not use NormaliseToMonitor, it uses scaling that we don't want
            Divide(LHSWorkspace=ws,OutputWorkspace=ws,RHSWorkspace=mon)

        elif self._reduction_type == 'EFWS':
            # Integrate over the whole range

            int = '__integral1_' + ws
            Integration(InputWorkspace=mon, OutputWorkspace=int,
                        RangeLower=x[0], RangeUpper=x[-1])

            if mtd[int].readY(0)[0] !=0: # this needs to be checked
                Scale(InputWorkspace=ws, OutputWorkspace=ws, Factor=1. / mtd[int].readY(0)[0])

            # remember the integral of the monitor
            AddSampleLog(Workspace=ws, LogName="MonitorIntegral", LogType="Number",
                         LogText=str(mtd[int].readY(0)[0]), EnableLogging = False)

            DeleteWorkspace(int)

        elif self._reduction_type == 'IFWS':
            # Integrate over the two peaks at the beginning and at the end and sum
            size = mtd[ws].blocksize()
            x_start, x_end = self._monitor_max_range(mon)

            i1 = '__integral1_' + ws
            i2 = '__integral2_' + ws
            int = '__integral_' + ws

            Integration(InputWorkspace=mon, OutputWorkspace=i1,
                        RangeLower=x[0], RangeUpper=x[2*x_start])

            Integration(InputWorkspace=mon, OutputWorkspace=i2,
                        RangeLower=x[-2*(size - x_end)], RangeUpper=x[-1])

            Plus(LHSWorkspace=i1,RHSWorkspace=i2,OutputWorkspace=int)

            if mtd[int].readY(0)[0] != 0: # this needs to be checked
                Scale(InputWorkspace = ws, OutputWorkspace = ws, Factor = 1./mtd[int].readY(0)[0])

            # remember the integral of the monitor
            AddSampleLog(Workspace=ws, LogName="MonitorIntegral", LogType="Number",
                         LogText=str(mtd[int].readY(0)[0]), EnableLogging = False)

            DeleteWorkspace(i1)
            DeleteWorkspace(i2)
            DeleteWorkspace(int)

        ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws,
                             NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLEnergyTransfer)
