#pylint: disable=no-init
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config, logger
from IndirectImport import import_mantidplot
import numpy
import os.path


class IndirectTransmissionMonitor(PythonAlgorithm):

    _sample_ws_in = None
    _can_ws_in = None
    _out_ws = None
    _plot = None
    _save = None


    def category(self):
        return "Workflow\\Inelastic;PythonAlgorithms;Inelastic"


    def summary(self):
        return "Calculates the sample transmission using the raw data files of the sample and its background or container."


    def PyInit(self):
        self.declareProperty(WorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Sample workspace')

        self.declareProperty(WorkspaceProperty('CanWorkspace', '', direction=Direction.Input),
                             doc='Background/can workspace')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='Output workspace group')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Plot result workspace')
        self.declareProperty(name='Save', defaultValue=False,
                             doc='Save result workspace to nexus file in the default save directory')

    def PyExec(self):
        self._setup()

        ws_basename = str(self._sample_ws_in)

        self._trans_mon(ws_basename, 'Sam', self._sample_ws_in)
        self._trans_mon(ws_basename, 'Can', self._can_ws_in)

        # Generate workspace names
        sam_ws = ws_basename + '_Sam'
        can_ws = ws_basename + '_Can'
        trans_ws = ws_basename + '_Trans'

        # Divide sample and can workspaces
        Divide(LHSWorkspace=sam_ws, RHSWorkspace=can_ws, OutputWorkspace=trans_ws)

        trans = numpy.average(mtd[trans_ws].readY(0))
        logger.information('Average Transmission: ' + str(trans))

        AddSampleLog(Workspace=trans_ws, LogName='can_workspace', LogType='String', LogText=self._can_ws_in)

        # Group workspaces
        group = sam_ws + ',' + can_ws + ',' + trans_ws
        GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=self._out_ws)

        self.setProperty('OutputWorkspace', self._out_ws)

        # Save the tranmissin workspace group to a nexus file
        if self._save:
            workdir = config['defaultsave.directory']
            path = os.path.join(workdir, self._out_ws + '.nxs')
            SaveNexusProcessed(InputWorkspace=self._out_ws, Filename=path)
            logger.information('Output file created : ' + path)

        # Plot spectra from transmission workspace
        if self._plot:
            mtd_plot = import_mantidplot()
            mtd_plot.plotSpectrum(self._out_ws, 0)


    def _setup(self):
        """
        Get properties.
        """

        self._sample_ws_in = self.getPropertyValue("SampleWorkspace")
        self._can_ws_in = self.getPropertyValue("CanWorkspace")
        self._out_ws = self.getPropertyValue('OutputWorkspace')
        self._plot = self.getProperty("Plot").value
        self._save = self.getProperty("Save").value


    def _get_spectra_index(self, input_ws):
        """
        Gets the index of the two monitors and first detector for the current instrument configurtion.
        Assumes monitors are named monitor1 and monitor2
        """

        instrument = mtd[input_ws].getInstrument()

        try:
            analyser = instrument.getStringParameter('analyser')[0]
            detector_1_idx = instrument.getComponentByName(analyser)[0].getID() - 1
            logger.information('Got index of first detector for analyser %s: %d' % (analyser, detector_1_idx))
        except IndexError:
            detector_1_idx = 2
            logger.warning('Could not determine index of first detetcor, using default value.')

        try:
            monitor_1_idx = self._get_detector_spectrum_index(input_ws, instrument.getComponentByName('monitor1').getID())

            monitor_2 = instrument.getComponentByName('monitor2')
            if monitor_2 is not None:
                monitor_2_idx = self._get_detector_spectrum_index(input_ws, monitor_2.getID())
            else:
                monitor_2_idx = None

            logger.information('Got index of monitors: %d, %s' % (monitor_1_idx, str(monitor_2_idx)))
        except IndexError:
            monitor_1_idx = 0
            monitor_2_idx = 1
            logger.warning('Could not determine index of monitors, using default values.')

        return monitor_1_idx, monitor_2_idx, detector_1_idx


    def _get_detector_spectrum_index(self, workspace, detector_id):
        """
        Returns the spectrum index for a given detector ID in a workspace.

        @param workspace Workspace to find detector in
        @param detector_id Detector ID to search for
        """

        for spec_idx in range(0, mtd[workspace].getNumberHistograms()):
            if mtd[workspace].getDetector(spec_idx).getID() == detector_id:
                return spec_idx

        return None


    def _unwrap_mon(self, input_ws):
        out_ws = '_unwrap_mon_out'

        # Unwrap monitor - inWS contains M1,M2,S1 - outWS contains unwrapped Mon
        # Unwrap s1>2 to L of S2 (M2) ie 38.76  Ouput is in wavelength
        _, join = UnwrapMonitor(InputWorkspace=input_ws, OutputWorkspace=out_ws, LRef='37.86')

        # Fill bad (dip) in spectrum
        RemoveBins(InputWorkspace=out_ws, OutputWorkspace=out_ws, Xmin=join - 0.001, Xmax=join + 0.001, Interpolation="Linear")
        FFTSmooth(InputWorkspace=out_ws, OutputWorkspace=out_ws, WorkspaceIndex=0, IgnoreXBins=True)  # Smooth - FFT

        DeleteWorkspace(input_ws)

        return out_ws


    def _trans_mon(self, ws_basename, file_type, input_ws):
        monitor_1_idx, monitor_2_idx, detector_1_idx = self._get_spectra_index(input_ws)

        CropWorkspace(InputWorkspace=input_ws, OutputWorkspace='__m1',
                      StartWorkspaceIndex=monitor_1_idx, EndWorkspaceIndex=monitor_1_idx)
        if monitor_2_idx is not None:
            CropWorkspace(InputWorkspace=input_ws, OutputWorkspace='__m2',
                          StartWorkspaceIndex=monitor_2_idx, EndWorkspaceIndex=monitor_2_idx)
        CropWorkspace(InputWorkspace=input_ws, OutputWorkspace='__det',
                      StartWorkspaceIndex=detector_1_idx, EndWorkspaceIndex=detector_1_idx)

        # Check for single or multiple time regimes
        mon_tcb_start = mtd['__m1'].readX(0)[0]
        spec_tcb_start = mtd['__det'].readX(0)[0]

        DeleteWorkspace('__det')
        mon_ws = '__Mon'

        if spec_tcb_start == mon_tcb_start:
            mon_ws = self._unwrap_mon('__m1')  # unwrap the monitor spectrum and convert to wavelength
            RenameWorkspace(InputWorkspace=mon_ws, OutputWorkspace='__Mon1')
        else:
            ConvertUnits(InputWorkspace='__m1', OutputWorkspace='__Mon1', Target="Wavelength")

        mon_ws = ws_basename + '_' + file_type

        if monitor_2_idx is not None:
            ConvertUnits(InputWorkspace='__m2', OutputWorkspace='__Mon2', Target="Wavelength")
            DeleteWorkspace('__m2')

            x_in = mtd['__Mon1'].readX(0)
            xmin1 = mtd['__Mon1'].readX(0)[0]
            xmax1 = mtd['__Mon1'].readX(0)[len(x_in) - 1]

            x_in = mtd['__Mon2'].readX(0)
            xmin2 = mtd['__Mon2'].readX(0)[0]
            xmax2 = mtd['__Mon2'].readX(0)[len(x_in) - 1]

            wmin = max(xmin1, xmin2)
            wmax = min(xmax1, xmax2)

            CropWorkspace(InputWorkspace='__Mon1', OutputWorkspace='__Mon1', XMin=wmin, XMax=wmax)
            RebinToWorkspace(WorkspaceToRebin='__Mon2', WorkspaceToMatch='__Mon1', OutputWorkspace='__Mon2')
            Divide(LHSWorkspace='__Mon2', RHSWorkspace='__Mon1', OutputWorkspace=mon_ws)

            DeleteWorkspace('__Mon1')
            DeleteWorkspace('__Mon2')
        else:
            RenameWorkspace(InputWorkspace='__Mon1', OutputWorkspace=mon_ws)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectTransmissionMonitor)
