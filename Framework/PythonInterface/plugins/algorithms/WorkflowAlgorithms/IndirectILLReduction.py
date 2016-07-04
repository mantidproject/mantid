#pylint: disable=no-init,invalid-name
from mantid.simpleapi import *
from mantid.kernel import StringListValidator, IntBoundedValidator, Direction
from mantid.api import DataProcessorAlgorithm, PropertyMode, AlgorithmFactory, \
                       MultipleFileProperty, FileProperty, FileAction, \
                       MatrixWorkspaceProperty
from mantid import config, logger, mtd

import numpy as np
import os.path

#pylint: disable=too-many-instance-attributes
class IndirectILLReduction(DataProcessorAlgorithm):

    #workspaces
    _raw_workspace = None
    _red_workspace = None
    _calibration_workspace = None
    _mnorm_workspace = None
    _vnorm_workspace = None
    _unmirror_workspace = None

    #other properties
    _analyser = None
    _control_mode = None
    _instrument_name = None
    _map_file = None
    _mirror_sense = None
    _plot = None
    _reflection = None
    _save = None
    _unmirror_option = None
    _run_path = None
    _instrument_name = None

    def category(self):
        return "Workflow\\MIDAS;Inelastic\\Reduction"

    def summary(self):
        return 'Performs an energy transfer reduction for ILL indirect inelastic data.'

    def PyInit(self):
        # Input options
        self.declareProperty(MultipleFileProperty('Run', extensions=["nxs"]),
                                                  doc='File path of run (s).')

        self.declareProperty(name='Analyser', defaultValue='silicon',
                             validator=StringListValidator(['silicon']),
                             doc='Analyser crystal.')

        self.declareProperty(name='Reflection', defaultValue='111',
                             validator=StringListValidator(['111']),
                             doc='Analyser reflection.')

        self.declareProperty(FileProperty('MapFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=["xml"]),
                             doc='Filename of the map file to use. If left blank the default will be used.')

        self.declareProperty(name='MirrorSense', defaultValue=True,
                             doc='Whether mirror sense is enabled.')

        self.declareProperty(name='ControlMode', defaultValue=False,
                             doc='Whether to output the workspaces in intermediate steps.')

        self.declareProperty(name='UnmirrorOption', defaultValue=7,
                             validator=IntBoundedValidator(lower=0,upper=7),
                             doc='Unmirroring option.')

        # Output workspace properties
        self.declareProperty(MatrixWorkspaceProperty("RawWorkspace", "raw",
                                                     direction=Direction.Output),
                             doc="Name for the output raw workspace created.")

        self.declareProperty(MatrixWorkspaceProperty("ReducedWorkspace", "red",
                                                     direction=Direction.Output),
                             doc="Name for the output reduced workspace created.")

        self.declareProperty(MatrixWorkspaceProperty("MNormalisedWorkspace", "mnorm",
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc="Name for the workspace normalised to monitor.")

        self.declareProperty(MatrixWorkspaceProperty("VNormalisedWorkspace", "vnorm",
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc="Name for the workspace normalised to vanadium.")

        self.declareProperty(MatrixWorkspaceProperty("UnmirroredWorkspace", "unmirrored",
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Output),
                             doc="Name for the unmirrored workspace.")

        # Optional input calibration workspace
        self.declareProperty(MatrixWorkspaceProperty("CalibrationWorkspace", "",
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc="Workspace containing calibration intensities.")

        # Output options
        self.declareProperty(name='Save', defaultValue=False,
                             doc='Whether to save the reduced workpsace to nxs file.')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Whether to plot the reduced workspace.')

        self.validateInputs()

    def validateInputs(self):

        issues = dict()
        return issues

    def PyExec(self):

        self._run_path = self.getPropertyValue('Run')
        self._calibration_workspace = self.getPropertyValue('CalibrationWorkspace')
        self._raw_workspace = self.getPropertyValue('RawWorkspace')
        self._red_workspace = self.getPropertyValue('ReducedWorkspace')
        self._vnorm_workspace = self.getPropertyValue('VNormalisedWorkspace')
        self._mnorm_workspace = self.getPropertyValue('MNormalisedWorkspace')
        self._unmirror_workspace = self.getPropertyValue('UnmirroredWorkspace')
        self._map_file = self.getPropertyValue('MapFile')
        self._mirror_sense = self.getPropertyValue('MirrorSense')
        self._control_mode = self.getPropertyValue('ControlMode')
        self._unmirror_option = self.getPropertyValue('UnmirrorOption')
        self._save = self.getPropertyValue('Save')
        self._plot = self.getPropertyValue('Plot')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        Load(Filename=self._run_path,OutputWorkspace=self._raw_workspace)
        self.log().information('Loaded .nxs file(s) : %s' % self._run_path)

        self.setPropertyValue('RawWorkspace', self._raw_workspace)

        if type(mtd[self._raw_workspace]) == "GroupWorkspace":
            instrument = mtd[self._raw_workspace].getItem(0).getInstrument()
            self._instrument_name = instrument.getName()
            outWS = self._multifile_reduction()
        else:
            instrument = mtd[self._raw_workspace].getInstrument()
            self._instrument_name = instrument.getName()
            outWS = self._reduction(self._raw_workspace)

        if self._control_mode == False:
            self.setPropertyValue('ReducedWorkspace', outWS)
        else:
            self.setPropertyValue('MNormalisedWorkspace', outWS[0])
            self.setPropertyValue('VNormalisedWorkspace', outWS[1])
            self.setPropertyValue('UnmirroredWorkspace', outWS[2])
            self.setPropertyValue('ReducedWorkspace', outWS[3])

    def _save(self):

        workdir = config['defaultsave.directory']
        file_path = os.path.join(workdir, self._red_workspace + '.nxs')
        SaveNexusProcessed(InputWorkspace=self._red_workspace, Filename=file_path)
        logger.information('Output file : ' + file_path)

    def _plot(self):

        from IndirectImport import import_mantidplot
        mtd_plot = import_mantidplot()
        graph = mtd_plot.newGraph()
        mtd_plot.plotSpectrum(self._red_workspace, 0, window=graph)
        layer = graph.activeLayer()
        layer.setAxisTitle(mtd_plot.Layer.Bottom, 'Energy Transfer (micro eV)')
        layer.setAxisTitle(mtd_plot.Layer.Left, '')
        layer.setTitle('')

    def _multifile_reduction(self):
        #do some stuff here
        return

    def _reduction(self,inWS):
        """
        Run indirect reduction for IN16B
        """
        logger.information('Input workspace : %s' % inWS)

        idf_directory = config['instrumentDefinition.directory']
        ipf_name = self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml'
        ipf_path = os.path.join(idf_directory, ipf_name)

        LoadParameterFile(Workspace=inWS, Filename=ipf_path)
        AddSampleLog(Workspace=inWS,
                     LogName="facility",
                     LogType="String",
                     LogText="ILL")

        if self._map_file == '':
            # path name for default map file
            instrument = mtd[inWS].getInstrument()
            if instrument.hasParameter('Workflow.GroupingFile'):
                grouping_filename = instrument.getStringParameter('Workflow.GroupingFile')[0]
                self._map_file = os.path.join(config['groupingFiles.directory'], grouping_filename)
            else:
                raise ValueError("Failed to find default map file. Contact development team.")

            logger.information('Map file : %s' % self._map_file)

        grouped_ws = inWS+'_group'
        GroupDetectors(InputWorkspace=inWS,
                       OutputWorkspace=grouped_ws,
                       MapFile=self._map_file,
                       Behaviour='Average')

        monitor_ws = inWS+'_mon'
        ExtractSingleSpectrum(InputWorkspace=inWS,
                              OutputWorkspace=monitor_ws,
                              WorkspaceIndex=0)

        if self._control_mode == True:
            output_workspaces = tuple()

        self._mnorm_workspace = self._normalise_to_monitor(monitor_ws, grouped_ws)

        if self._control_mode == True:
            output_workspaces.add(self._mnorm_workspace)

        self._vnorm_workspace = self._normalise_to_vanadium(self._mnorm_workspace, self._calibration_workspace)

        if self._control_mode == True:
            output_workspaces.add(self._vnorm_workspace)

        if self._mirror_sense:
            self._unmirror_workspace = self._unmirror(self._vnorm_workspace)
        else:
            self._unmirror_workspace = self._vnorm_workspace

        if self._control_mode == True:
            output_workspaces.add(self._unmirror_workspace)

        self._red_workspace = self._calculate_energy(self._unmirror_workspace)

        output_workspaces.add(self._red_workspace)

        if self._control_mode == False:
            return self._red_workspace
        else:
            return output_workspaces

    def _normalise_to_monitor(self, mon, ws):
        return

    def _normalise_to_vanadium(self, ws, van):
        return

    def _unmirror(self, monitor_ws, grouped_ws):
        return
        """
        Runs energy reduction with mirror mode.

        @param monitor_ws :: name of the monitor workspace
        @param grouped_ws :: name of workspace with the detectors grouped
        """
        logger.information('Mirror sense is ON')

        x = mtd[grouped_ws].readX(0)  # energy array
        mid_point = int((len(x) - 1) / 2)

        #left half
        left_ws = '_left'
        left_mon_ws = '_left_mon'
        CropWorkspace(InputWorkspace=grouped_ws,
                      OutputWorkspace=left_ws,
                      XMax=x[mid_point - 1])
        CropWorkspace(InputWorkspace=monitor_ws,
                      OutputWorkspace=left_mon_ws,
                      XMax=x[mid_point - 1])

        #self._calculate_energy(left_mon_ws, left_ws, self._red_left_workspace)
        #xl = mtd[self._red_left_workspace].readX(0)

        logger.information('Energy range, left : %f to %f' % (xl[0], xl[-1]))

        #right half
        right_ws = '_right'
        right_mon_ws = right_ws + '_mon'
        CropWorkspace(InputWorkspace=grouped_ws,
                      OutputWorkspace=right_ws,
                      Xmin=x[mid_point])
        CropWorkspace(InputWorkspace=monitor_ws,
                      OutputWorkspace=right_mon_ws,
                      Xmin=x[mid_point])

       # self._calculate_energy(right_mon_ws, right_ws, self._red_right_workspace)
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
        ScaleX(InputWorkspace=self._red_right_workspace,
               OutputWorkspace=self._red_right_workspace,
               Factor=xshift,
               Operation='Add')
        RebinToWorkspace(WorkspaceToRebin=self._red_right_workspace,
                         WorkspaceToMatch=self._red_left_workspace,
                         OutputWorkspace=self._red_right_workspace)

        #sum both workspaces together
        Plus(LHSWorkspace=self._red_left_workspace,
             RHSWorkspace=self._red_right_workspace,
             OutputWorkspace=self._red_workspace)
        Scale(InputWorkspace=self._red_workspace,
              OutputWorkspace=self._red_workspace,
              Factor=0.5, Operation='Multiply')

        DeleteWorkspace(monitor_ws)
        DeleteWorkspace(grouped_ws)

    def _calculate_energy(self, monitor_ws, grouped_ws):
        """
        Convert the input run to energy transfer

        @param monitor_ws :: name of the monitor workspace to divide by
        @param grouped_ws :: name of workspace with the detectors grouped
        """
        x_range = self._monitor_range(monitor_ws)
        Scale(InputWorkspace=monitor_ws,
              OutputWorkspace=monitor_ws,
              Factor=0.001,
              Operation='Multiply')

        CropWorkspace(InputWorkspace=monitor_ws,
                      OutputWorkspace=monitor_ws,
                      Xmin=x_range[0],
                      XMax=x_range[1])
        ScaleX(InputWorkspace=monitor_ws,
               OutputWorkspace=monitor_ws,
               Factor=-x_range[0],
               Operation='Add')

        CropWorkspace(InputWorkspace=grouped_ws,
                      OutputWorkspace=grouped_ws,
                      Xmin=x_range[0],
                      XMax=x_range[1])
        ScaleX(InputWorkspace=grouped_ws,
               OutputWorkspace=grouped_ws,
               Factor=-x_range[0],
               Operation='Add')

        # Apply the detector intensity calibration
        if self._calibration_workspace != '':
            Divide(LHSWorkspace=grouped_ws,
                   RHSWorkspace=self._calibration_workspace,
                   OutputWorkspace=grouped_ws)

        Divide(LHSWorkspace=grouped_ws,
               RHSWorkspace=monitor_ws,
               OutputWorkspace=grouped_ws)
        formula = self._energy_range(grouped_ws)
        ConvertAxisByFormula(InputWorkspace=grouped_ws,
                             OutputWorkspace=self._red_workspace,
                             Axis='X',
                             Formula=formula)

        red_ws_p = mtd[self._red_workspace]
        red_ws_p.getAxis(0).setUnit('DeltaE')

        xnew = red_ws_p.readX(0)  # energy array
        logger.information('Energy range : %f to %f' % (xnew[0], xnew[-1]))

        DeleteWorkspace(grouped_ws)
        DeleteWorkspace(monitor_ws)

        return self._red_workspace

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
        logger.information('Wavelength : %s' % wave)
        if gRun.hasProperty('Doppler.maximum_delta_energy'):
            energy = gRun.getLogData('Doppler.maximum_delta_energy').value
            logger.information('Doppler max energy : %s' % energy)
        elif gRun.hasProperty('Doppler.doppler_speed'):
            speed = gRun.getLogData('Doppler.doppler_speed').value
            amp = gRun.getLogData('Doppler.doppler_amplitude').value
            logger.information('Doppler speed : %s' % speed)
            logger.information('Doppler amplitude : %s' % amp)
            energy = 1.2992581918414711e-4 * speed * amp * 2.0 / wave  # max energy
        elif gRun.hasProperty('Doppler.doppler_freq'):
            speed = gRun.getLogData('Doppler.doppler_freq').value
            amp = gRun.getLogData('Doppler.doppler_amplitude').value
            logger.information('Doppler freq : %s' % freq)
            logger.information('Doppler amplitude : %s' % amp)
            energy = 1.2992581918414711e-4 * freq * amp * 2.0 / wave  # max energy

        dele = 2.0 * energy / npt
        formula = '(x-%f)*%f' % (imid, dele)

        return formula


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReduction)
