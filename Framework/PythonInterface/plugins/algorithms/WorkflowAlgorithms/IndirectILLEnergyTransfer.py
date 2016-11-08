from __future__ import (absolute_import, division, print_function)

import numpy as np
from mantid.simpleapi import *  # noqa
from mantid.kernel import *  # noqa
from mantid.api import *  # noqa
from mantid import config, mtd, logger


def _ws_or_none(s):
    return mtd[s] if s != '' else None


def extract_workspace(ws, ws_out, x_start, x_end):
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


class IndirectILLEnergyTransfer(DataProcessorAlgorithm):

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

    def category(self):
        return "Workflow\\MIDAS;Inelastic\\Reduction"

    def summary(self):
        return 'Performs energy transfer reduction for ILL indirect geometry data, instrument IN16B.'

    def name(self):
        return "IndirectILLEnergyTransfer"

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run (s).')

        self.declareProperty(FileProperty('MapFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['xml']),
                             doc='Filename of the detector grouping map file to use. \n'
                                 'If left blank the default will be used.')

        self.declareProperty(name='Analyser',
                             defaultValue='silicon',
                             validator=StringListValidator(['silicon']),
                             doc='Analyser crystal.')

        self.declareProperty(name='Reflection',
                             defaultValue='111',
                             validator=StringListValidator(['111', '311']),
                             doc='Analyser reflection.')

        self.declareProperty(name='CropDeadMonitorChannels',defaultValue=False,
                             doc='Whether or not to exclude the first and last few channels'
                                 'with 0 monitor count in the energy transfer formula.')

        self.declareProperty(WorkspaceGroupProperty("OutputWorkspace", "red",
                                                    optional=PropertyMode.Optional,
                                                    direction=Direction.Output),
                             doc="Group name for the reduced workspace(s).")

    def validateInputs(self):

        issues = dict()

        return issues

    def setUp(self):

        self._run_file = self.getPropertyValue('Run').replace(',','+') # automatic summing
        self._analyser = self.getPropertyValue('Analyser')
        self._map_file = self.getPropertyValue('MapFile')
        self._reflection = self.getPropertyValue('Reflection')
        self._dead_channels = self.getProperty('CropDeadMonitorChannels').value
        self._red_ws = self.getPropertyValue('OutputWorkspace')

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

    def _convert_to_energy(self, ws, n_crop):
        """
        Converts the x-axis from raw channel number to energy transfer
        @param ws :: input workspace name
        @param ws :: number of cropped bins
        """

        x = mtd[ws].readX(0)
        size = mtd[ws].blocksize()
        mid = (x[-1] + x[0])/ 2.
        scale = 1000.  # from micro ev to mili ev

        factor = (size + n_crop)/(size - 1)

        if self._doppler_energy != 0:
            formula = '(x/{0} - 1)*{1}'.format(mid, (self._doppler_energy / scale) * factor)
        else:
            # Center the data for elastic fixed window scan, for integration over the elastic peak
            formula = '(x-{0})*{1}'.format(mid-0.5, 1. / scale)
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

        # This is faster than Load, moreover MergeRuns handles the metadata correctly
        for i, item in enumerate(self._run_file.split('+')):
            if i == 0:
                LoadILLIndirect(Filename=item,OutputWorkspace=self._red_ws)
            if i > 0:
                runnumber = '__' + os.path.basename(item).split('.')[0]
                LoadILLIndirect(Filename=item, OutputWorkspace=runnumber)
                MergeRuns(InputWorkspaces=[self._red_ws,runnumber],OutputWorkspace=self._red_ws)
                DeleteWorkspace(runnumber)

        self._instrument = mtd[self._red_ws].getInstrument()

        self._load_map_file()

        run = '{0:06d}'.format(mtd[self._red_ws].getRunNumber())

        self._ws = run + '_' + self._red_ws

        RenameWorkspace(InputWorkspace=self._red_ws, OutputWorkspace=self._ws)

        LoadParameterFile(Workspace=self._ws, Filename=self._parameter_file)

        self._setup_run_properties()

        if self._mirror_sense == 14:      # two wings, extract left and right

            size = mtd[self._ws].blocksize()
            left = self._ws + '_left'
            right = self._ws + '_right'
            extract_workspace(self._ws, left, 0, int(size/2))
            extract_workspace(self._ws, right, int(size/2), size)
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

        GroupDetectors(InputWorkspace=ws, OutputWorkspace=ws, MapFile=self._map_file, Behaviour='Sum')

        xmin, xmax = self._monitor_zero_range(mon)

        self._normalise_to_monitor(ws, mon)

        n_cropped_bins = 0

        if self._reduction_type == 'QENS':
            if self._dead_channels:
                CropWorkspace(InputWorkspace=ws,OutputWorkspace=ws,XMin=xmin,XMax=xmax+1)
                n_cropped_bins = mtd[mon].blocksize() - mtd[ws].blocksize() - 1
            else:
                self._mask(ws, xmin, xmax)

        DeleteWorkspace(mon)

        self._convert_to_energy(ws, n_cropped_bins)

        ConvertSpectrumAxis(InputWorkspace=ws, OutputWorkspace=ws, Target='Theta', EMode='Indirect')

    def _normalise_to_monitor(self, ws, mon):
        """
        Normalises the ws to the monitor dependent on the reduction type
        @param ws :: input workspace name
        @param ws :: ws's monitor
        """
        x = mtd[ws].readX(0)

        if self._reduction_type == 'QENS':
            # Normalise bin-to-bin
            NormaliseToMonitor(InputWorkspace=ws, OutputWorkspace=ws, MonitorWorkspace=mon)

        elif self._reduction_type == 'EFWS':
            # Integrate over the whole range

            int = '__integral1_' + ws
            Integration(InputWorkspace=mon, OutputWorkspace=int,
                        RangeLower=x[0], RangeUpper=x[-1])

            if mtd[int].readY(0)[0] !=0: # this needs to be checked
                Scale(InputWorkspace=ws, OutputWorkspace=ws, Factor=1. / mtd[int].readY(0)[0])

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

            DeleteWorkspace(i1)
            DeleteWorkspace(i2)
            DeleteWorkspace(int)

        ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws,
                             NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLEnergyTransfer)
