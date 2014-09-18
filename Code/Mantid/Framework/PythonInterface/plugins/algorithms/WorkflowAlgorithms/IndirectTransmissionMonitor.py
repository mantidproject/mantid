from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config, logger
from IndirectImport import import_mantidplot
import numpy
import os.path


class IndirectTransmissionMonitor(PythonAlgorithm):

    def category(self):
        return "Workflow\\Inelastic;PythonAlgorithms;Inelastic"

    def summary(self):
        return "Calculates the sample transmission using the raw data files of the sample and its background or container."

    def PyInit(self):
        self.declareProperty(WorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Sample workspace')

        self.declareProperty(WorkspaceProperty('CanWorkspace', '', direction=Direction.Input),
                             doc='Background/can workspace')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', direction=Direction.Output,
                             optional=PropertyMode.Optional),
                             doc='Output workspace group')

        self.declareProperty(name='Verbose', defaultValue=False, doc='Output more verbose message to log')
        self.declareProperty(name='Plot', defaultValue=False, doc='Plot result workspace')
        self.declareProperty(name='Save', defaultValue=False, doc='Save result workspace to nexus file in the default save directory')

    def PyExec(self):
        from IndirectCommon import StartTime, EndTime
        StartTime('IndirectTransmissionMonitor')

        sample_ws_in = self.getPropertyValue("SampleWorkspace")
        can_ws_in = self.getPropertyValue("CanWorkspace")
        out_ws = self.getPropertyValue('OutputWorkspace')
        verbose = self.getProperty("Verbose").value
        plot = self.getProperty("Plot").value
        save = self.getProperty("Save").value

        ws_basename = str(sample_ws_in)

        self._trans_mon(ws_basename, 'Sam', sample_ws_in)
        self._trans_mon(ws_basename, 'Can', can_ws_in)

        # Generate workspace names
        sam_ws = ws_basename + '_Sam'
        can_ws = ws_basename + '_Can'
        trans_ws = ws_basename + '_Trans'

        # Divide sample and can workspaces
        Divide(LHSWorkspace=sam_ws, RHSWorkspace=can_ws, OutputWorkspace=trans_ws)
        trans = numpy.average(mtd[trans_ws].readY(0))

        AddSampleLog(Workspace=trans_ws, LogName='can_workspace', LogType='String', LogText=can_ws_in)

        # Generate an output workspace name if none provided
        if out_ws == '':
            out_ws = ws_basename + '_Transmission'

        # Group workspaces
        group = sam_ws + ',' + can_ws + ',' + trans_ws
        GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=out_ws)

        self.setProperty('OutputWorkspace', out_ws)

        if verbose:
            logger.notice('Transmission : ' + str(trans))

        # Save the tranmissin workspace group to a nexus file
        if save:
            workdir = config['defaultsave.directory']
            path = os.path.join(workdir, out_ws + '.nxs')
            SaveNexusProcessed(InputWorkspace=out_ws, Filename=path)
            if verbose:
                logger.notice('Output file created : ' + path)

        # Plot spectra from transmission workspace
        if plot:
            mtd_plot = import_mantidplot()
            mtd_plot.plotSpectrum(out_ws, 0)

        EndTime('IndirectTransmissionMonitor')

    def _unwrap_mon(self, input_ws):
        out_ws = '_unwrap_mon_out'

        # Unwrap monitor - inWS contains M1,M2,S1 - outWS contains unwrapped Mon
        # Unwrap s1>2 to L of S2 (M2) ie 38.76  Ouput is in wavelength
        _, join = UnwrapMonitor(InputWorkspace=input_ws, OutputWorkspace=out_ws, LRef='37.86')

        # Fill bad (dip) in spectrum
        RemoveBins(InputWorkspace=out_ws, OutputWorkspace=out_ws, Xmin=join - 0.001, Xmax=join + 0.001, Interpolation="Linear")
        FFTSmooth(InputWorkspace=out_ws, OutputWorkspace=out_ws, WorkspaceIndex=0, IgnoreXBins=True)  # Smooth - FFT

        DeleteWorkspace(input_ws)  # delete monWS

        return out_ws

    def _trans_mon(self, ws_basename, file_type, input_ws):
        CropWorkspace(InputWorkspace=input_ws, OutputWorkspace='__m1', StartWorkspaceIndex=0, EndWorkspaceIndex=0)
        CropWorkspace(InputWorkspace=input_ws, OutputWorkspace='__m2', StartWorkspaceIndex=1, EndWorkspaceIndex=1)
        CropWorkspace(InputWorkspace=input_ws, OutputWorkspace='__det', StartWorkspaceIndex=2, EndWorkspaceIndex=2)

        # Check for single or multiple time regimes
        mon_tcb_start = mtd['__m1'].readX(0)[0]
        spec_tcb_start = mtd['__det'].readX(0)[0]

        DeleteWorkspace('__det')  # delete monWS
        mon_ws = '__Mon'

        if spec_tcb_start == mon_tcb_start:
            mon_ws = self._unwrap_mon('__m1')  # unwrap the monitor spectrum and convert to wavelength
            RenameWorkspace(InputWorkspace=mon_ws, OutputWorkspace='__Mon1')
        else:
            ConvertUnits(InputWorkspace='__m1', OutputWorkspace='__Mon1', Target="Wavelength")

        ConvertUnits(InputWorkspace='__m2', OutputWorkspace='__Mon2', Target="Wavelength")

        DeleteWorkspace('__m2')  # delete monWS

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
        mon_ws = ws_basename + '_' + file_type
        Divide(LHSWorkspace='__Mon2', RHSWorkspace='__Mon1', OutputWorkspace=mon_ws)

        DeleteWorkspace('__Mon1')  # delete monWS
        DeleteWorkspace('__Mon2')  # delete monWS


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectTransmissionMonitor)
