from mantid.simpleapi import *
from mantid.kernel import StringListValidator, Direction
from mantid.api import DataProcessorAlgorithm, PropertyMode, AlgorithmFactory, \
                       FileProperty, FileAction, MatrixWorkspaceProperty
from mantid import config, logger, mtd

import numpy as np
import os.path


class IndirectILLReduction(DataProcessorAlgorithm):

    _raw_workspace = None
    _red_workspace = None
    _red_left_workspace = None
    _red_right_workspace = None
    _map_file = None
    _use_mirror_mode = None
    _save = None
    _plot = None
    _instrument_name = None
    _run_number = None
    _analyser = None
    _reflection = None
    _run_name = None

    def category(self):
        return "Workflow\\MIDAS;Inelastic;PythonAlgorithms"


    def PyInit(self):
        #input options
        self.declareProperty(FileProperty('Run', '', action=FileAction.Load, extensions=["nxs"]),
                             doc='File path of run.')

        self.declareProperty(name='Analyser', defaultValue='silicon',
                             validator=StringListValidator(['silicon']),
                             doc='Analyser crystal')

        self.declareProperty(name='Reflection', defaultValue='111',
                             validator=StringListValidator(['111']),
                             doc='Analyser reflection')

        self.declareProperty(FileProperty('MapFile', '',
                             action=FileAction.OptionalLoad, extensions=["xml"]),
                             doc='Filename of the map file to use. If left blank the default will be used.')

        self.declareProperty(name='MirrorMode', defaultValue=False,
                             doc='Whether to use mirror mode')

        #Output workspace properties
        self.declareProperty(MatrixWorkspaceProperty("RawWorkspace", "",
                             direction=Direction.Output),
                             doc="Name for the output raw workspace created.")

        self.declareProperty(MatrixWorkspaceProperty("ReducedWorkspace", "",
                             direction=Direction.Output),
                             doc="Name for the output reduced workspace created. If mirror mode is used this will be the sum of both"
                             "the left and right hand workspaces.")

        self.declareProperty(MatrixWorkspaceProperty("LeftWorkspace", "",
                             optional=PropertyMode.Optional, direction=Direction.Output),
                             doc="Name for the left workspace if mirror mode is used.")

        self.declareProperty(MatrixWorkspaceProperty("RightWorkspace", "",
                             optional=PropertyMode.Optional, direction=Direction.Output),
                             doc="Name for the right workspace if mirror mode is used.")

        # output options
        self.declareProperty(name='Save', defaultValue=False,
                             doc='Switch Save result to nxs file Off/On')
        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Whether to plot the output workspace.')


    def PyExec(self):
        self.log().information('IndirectILLreduction')

        run_path = self.getPropertyValue('Run')
        self._raw_workspace = self.getPropertyValue('RawWorkspace')
        self._red_workspace = self.getPropertyValue('ReducedWorkspace')
        self._red_left_workspace = self.getPropertyValue('LeftWorkspace')
        self._red_right_workspace = self.getPropertyValue('RightWorkspace')
        self._map_file = self.getProperty('MapFile').value

        self._use_mirror_mode = self.getProperty('MirrorMode').value
        self._save = self.getProperty('Save').value
        self._plot = self.getProperty('Plot').value

        if self._use_mirror_mode:
            if self._red_left_workspace == '':
                raise ValueError("Mirror Mode requires the LeftWorkspace property to be set to a value")

            if self._red_right_workspace == '':
                raise ValueError("Mirror Mode requires the RightWorkspace property to be set to a value")

        LoadILLIndirect(FileName=run_path, OutputWorkspace=self._raw_workspace)

        instrument = mtd[self._raw_workspace].getInstrument()
        self._instrument_name = instrument.getName()

        self._run_number = mtd[self._raw_workspace].getRunNumber()
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')
        self._run_name = self._instrument_name + '_' + str(self._run_number)

        AddSampleLog(Workspace=self._raw_workspace, LogName="mirror_sense",
                     LogType="String", LogText=str(self._use_mirror_mode))

        logger.information('Nxs file : %s' % run_path)

        output_workspaces = self._reduction()

        if self._save:
            workdir = config['defaultsave.directory']
            for ws in output_workspaces:
                file_path = os.path.join(workdir, ws + '.nxs')
                SaveNexusProcessed(InputWorkspace=ws, Filename=file_path)
                logger.information('Output file : ' + file_path)

        if self._plot:
            from IndirectImport import import_mantidplot
            mtd_plot = import_mantidplot()
            graph = mtd_plot.newGraph()

            for ws in output_workspaces:
                mtd_plot.plotSpectrum(ws, 0, window=graph)

            layer = graph.activeLayer()
            layer.setAxisTitle(mtd_plot.Layer.Bottom, 'Energy Transfer (meV)')
            layer.setAxisTitle(mtd_plot.Layer.Left, '')
            layer.setTitle('')

        self.setPropertyValue('RawWorkspace', self._raw_workspace)
        self.setPropertyValue('ReducedWorkspace', self._red_workspace)

        if self._use_mirror_mode:
            self.setPropertyValue('LeftWorkspace', self._red_left_workspace)
            self.setPropertyValue('RightWorkspace', self._red_right_workspace)


    def _reduction(self):
        """
        Run energy conversion for IN16B
        """
        logger.information('Input workspace : %s' % self._raw_workspace)

        idf_directory = config['instrumentDefinition.directory']
        ipf_name = self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml'
        ipf_path = os.path.join(idf_directory, ipf_name)

        LoadParameterFile(Workspace=self._raw_workspace, Filename=ipf_path)
        AddSampleLog(Workspace=self._raw_workspace, LogName="facility",
                     LogType="String", LogText="ILL")

        if self._map_file == '':
            # path name for default map file
            instrument = mtd[self._raw_workspace].getInstrument()
            if instrument.hasParameter('Workflow.GroupingFile'):
                grouping_filename = instrument.getStringParameter('Workflow.GroupingFile')[0]
                self._map_file = os.path.join(config['groupingFiles.directory'], grouping_filename)
            else:
                raise ValueError("Failed to find default map file. Contact development team.")

            logger.information('Map file : %s' % self._map_file)

        grouped_ws = self._run_name + '_group'
        GroupDetectors(InputWorkspace=self._raw_workspace, OutputWorkspace=grouped_ws,
                       MapFile=self._map_file, Behaviour='Average')

        monitor_ws = self._run_name + '_mon'
        ExtractSingleSpectrum(InputWorkspace=self._raw_workspace,
                              OutputWorkspace=monitor_ws, WorkspaceIndex=0)

        if self._use_mirror_mode:
            output_workspaces = self._run_mirror_mode(monitor_ws, grouped_ws)
        else:
            logger.information('Mirror sense is OFF')

            self._calculate_energy(monitor_ws, grouped_ws, self._red_workspace)
            output_workspaces = [self._red_workspace]

        return output_workspaces


    def _run_mirror_mode(self, monitor_ws, grouped_ws):
        """
        Runs energy reduction with mirror mode.

        @param monitor_ws :: name of the monitor workspace
        @param grouped_ws :: name of workspace with the detectors grouped
        """
        logger.information('Mirror sense is ON')

        x = mtd[grouped_ws].readX(0)  # energy array
        mid_point = int((len(x) - 1) / 2)

        #left half
        left_ws = self._run_name + '_left'
        left_mon_ws = left_ws + '_left_mon'
        CropWorkspace(InputWorkspace=grouped_ws, OutputWorkspace=left_ws, XMax=x[mid_point - 1])
        CropWorkspace(InputWorkspace=monitor_ws, OutputWorkspace=left_mon_ws, XMax=x[mid_point - 1])

        self._calculate_energy(left_mon_ws, left_ws, self._red_left_workspace)
        xl = mtd[self._red_left_workspace].readX(0)

        logger.information('Energy range, left : %f to %f' % (xl[0], xl[-1]))

        #right half
        right_ws = self._run_name + '_right'
        right_mon_ws = right_ws + '_mon'
        CropWorkspace(InputWorkspace=grouped_ws, OutputWorkspace=right_ws, Xmin=x[mid_point])
        CropWorkspace(InputWorkspace=monitor_ws, OutputWorkspace=right_mon_ws, Xmin=x[mid_point])

        self._calculate_energy(right_mon_ws, right_ws, self._red_right_workspace)
        xr = mtd[self._red_right_workspace].readX(0)

        logger.information('Energy range, right : %f to %f' % (xr[0], xr[-1]))

        xl = mtd[self._red_left_workspace].readX(0)
        yl = mtd[self._red_left_workspace].readY(0)
        nlmax = np.argmax(np.array(yl))
        xlmax = xl[nlmax]
        xr = mtd[self._red_right_workspace].readX(0)
        yr = mtd[self._red_right_workspace].readY(0)
        nrmax = np.argmax(np.array(yr))
        xrmax = xr[nrmax]
        xshift = xlmax - xrmax
        ScaleX(InputWorkspace=self._red_right_workspace, OutputWorkspace=self._red_right_workspace,
               Factor=xshift, Operation='Add')
        RebinToWorkspace(WorkspaceToRebin=self._red_right_workspace,
                         WorkspaceToMatch=self._red_left_workspace,
                         OutputWorkspace=self._red_right_workspace)

        #sum both workspaces together
        Plus(LHSWorkspace=self._red_left_workspace, RHSWorkspace=self._red_right_workspace,
             OutputWorkspace=self._red_workspace)
        Scale(InputWorkspace=self._red_workspace, OutputWorkspace=self._red_workspace,
              Factor=0.5, Operation='Multiply')

        DeleteWorkspace(monitor_ws)
        DeleteWorkspace(grouped_ws)

        return [self._red_left_workspace, self._red_right_workspace, self._red_workspace]


    def _calculate_energy(self, monitor_ws, grouped_ws, red_ws):
        """
        Convert the input run to energy transfer

        @param monitor_ws :: name of the monitor workspace to divide by
        @param grouped_ws :: name of workspace with the detectors grouped
        @param red_ws :: name to call the reduced workspace
        """
        x_range = self._monitor_range(monitor_ws)
        Scale(InputWorkspace=monitor_ws, OutputWorkspace=monitor_ws, Factor=0.001,
              Operation='Multiply')

        CropWorkspace(InputWorkspace=monitor_ws, OutputWorkspace=monitor_ws,
                      Xmin=x_range[0], XMax=x_range[1])
        ScaleX(InputWorkspace=monitor_ws, OutputWorkspace=monitor_ws, Factor=-x_range[0],
               Operation='Add')

        CropWorkspace(InputWorkspace=grouped_ws, OutputWorkspace=grouped_ws, Xmin=x_range[0],
                      XMax=x_range[1])
        ScaleX(InputWorkspace=grouped_ws, OutputWorkspace=grouped_ws, Factor=-x_range[0],
               Operation='Add')

        Divide(LHSWorkspace=grouped_ws, RHSWorkspace=monitor_ws, OutputWorkspace=grouped_ws)
        formula = self._energy_range(grouped_ws)
        ConvertAxisByFormula(InputWorkspace=grouped_ws, OutputWorkspace=red_ws, Axis='X',
                             Formula=formula, AxisTitle='Energy transfer', AxisUnits='meV')

        xnew = mtd[red_ws].readX(0)  # energy array
        logger.information('Energy range : %f to %f' % (xnew[0], xnew[-1]))

        DeleteWorkspace(grouped_ws)
        DeleteWorkspace(monitor_ws)


    def _monitor_range(self, monitor_ws):
        """
        Get sensible values for the min and max cropping range

        @param monitor_ws :: name of the monitor workspace
        @return tuple containing the min and max x values in the range
        """
        x = mtd[monitor_ws].readX(0)  # energy array
        y = mtd[monitor_ws].readY(0)  # energy array
        imin = np.argmax(np.array(y[0:20]))
        nch = len(y)
        im = np.argmax(np.array(y[nch - 21:nch - 1]))
        imax = nch - 21 + im

        logger.information('Cropping range %f to %f' % (x[imin], x[imax]))

        return x[imin], x[imax]


    def _energy_range(self, ws):
        """
        Calculate the energy range for the workspace

        @param ws :: name of the workspace
        @return formula for convert axis by formula to convert to energy transfer
        """
        x = mtd[ws].readX(0)
        npt = len(x)
        imid = float(npt / 2 + 1)
        gRun = mtd[ws].getRun()
        wave = gRun.getLogData('wavelength').value
        freq = gRun.getLogData('Doppler.doppler_frequency').value
        amp = gRun.getLogData('Doppler.doppler_amplitude').value

        logger.information('Wavelength : ' + str(wave))
        logger.information('Doppler frequency : ' + str(freq))
        logger.information('Doppler amplitude : ' + str(amp))

        vmax = 1.2992581918414711e-4 * freq * amp * 2.0 / wave  # max energy
        dele = 2.0 * vmax / npt
        formula = '(x-%f)*%f' % (imid, dele)

        return formula


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReduction)
