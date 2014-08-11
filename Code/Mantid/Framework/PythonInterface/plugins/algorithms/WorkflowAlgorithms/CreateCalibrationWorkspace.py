from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import os.path


class CreateCalibrationWorkspace(DataProcessorAlgorithm):

    def category(self):
        return "Workflow\\Inelastic;PythonAlgorithms;Inelastic"

    def summary(self):
        return "Creates a calibration workspace from a White-Beam Vanadium run."

    def PyInit(self):
        self.declareProperty(FileProperty('InputFiles', '',
            action=FileAction.Load), doc='Comma separated list of input files')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
            direction=Direction.Output), doc='Output workspace for calibration data')

        self.declareProperty(name='DetectorRange', defaultValue='',
                validator=StringMandatoryValidator(), doc='Range of detectors')

        self.declareProperty(name='PeakRange', defaultValue='',
                validator=StringMandatoryValidator(), doc='')

        self.declareProperty(name='BackgroundRange', defaultValue='',
                validator=StringMandatoryValidator(), doc='')

        self.declareProperty(name='ScaleFactor', defaultValue='', doc='')

        self.declareProperty(name='Plot', defaultValue=False, doc='Plot the calibration data')

    def PyExec(self):
        from mantid import logger
        from IndirectCommon import StartTime, EndTime
        StartTime('CreateCalibrationWorkspace')

        input_files = self.getPropertyValue('InputFiles').split(',')
        out_ws = self.getPropertyValue('OutputWorkspace')

        peak_range = self.getPropertyValue('PeakRange').split(',')
        back_range = self.getPropertyValue('BackgroundRange').split(',')
        spec_range = self.getPropertyValue('DetectorRange').split(',')

        intensity_scale = self.getPropertyValue('ScaleFactor')
        if intensity_scale == '':
            intensity_scale = None
        else:
            intensity_scale = float(intensity_scale)

        plot = self.getProperty('Plot').value

        runs = []
        for in_file in input_files:
            (_, filename) = os.path.split(in_file)
            (root, _) = os.path.splitext(filename)
            try:
                Load(Filename=in_file, OutputWorkspace=root,
                    SpectrumMin=int(spec_range[0]), SpectrumMax=int(spec_range[1]),
                    LoadLogFiles=False)
                runs.append(root)
            except:
                logger.error('Indirect: Could not load raw file: ' + in_file)

        calib_ws_name = 'calibration'
        if len(runs) > 1:
            MergeRuns(InputWorkspaces=",".join(runs), OutputWorkspace=calib_ws_name)
            factor = 1.0 / len(runs)
            Scale(InputWorkspace=calib_ws_name, OutputWorkspace=calib_ws_name, Factor=factor)
        else:
            calib_ws_name = runs[0]

        CalculateFlatBackground(InputWorkspace=calib_ws_name, OutputWorkspace=calib_ws_name,
                StartX=float(back_range[0]), EndX=float(back_range[1]), Mode='Mean')

        from inelastic_indirect_reduction_steps import NormaliseToUnityStep
        ntu = NormaliseToUnityStep()
        ntu.set_factor(intensity_scale)
        ntu.set_peak_range(float(peak_range[0]), float(peak_range[1]))
        ntu.execute(None, calib_ws_name)

        RenameWorkspace(InputWorkspace=calib_ws_name, OutputWorkspace=out_ws)

        ## Add sample logs to output workspace
        if intensity_scale:
            AddSampleLog(Workspace=out_ws, LogName='Scale Factor', LogType='Number', LogText=str(intensity_scale))
        AddSampleLog(Workspace=out_ws, LogName='Peak Min', LogType='Number', LogText=peak_range[0])
        AddSampleLog(Workspace=out_ws, LogName='Peak Max', LogType='Number', LogText=peak_range[1])
        AddSampleLog(Workspace=out_ws, LogName='Back Min', LogType='Number', LogText=back_range[0])
        AddSampleLog(Workspace=out_ws, LogName='Back Max', LogType='Number', LogText=back_range[1])

        ## Remove old workspaces
        if len(runs) > 1:
            for run in runs:
                DeleteWorkspace(Workspace=run)

        logger.notice(str(plot))
        if plot:
            from mantidplot import plotTimeBin
            plotTimeBin(out_ws, 0)

        self.setProperty('OutputWorkspace', out_ws)

        EndTime('CreateCalibrationWorkspace')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(CreateCalibrationWorkspace)
