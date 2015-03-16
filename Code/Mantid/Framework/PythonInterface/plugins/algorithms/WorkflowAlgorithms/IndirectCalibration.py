#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import os.path


class IndirectCalibration(DataProcessorAlgorithm):

    _input_files = None
    _out_ws = None
    _peak_range = None
    _back_range = None
    _spec_range = None
    _intensity_scale = None
    _plot = None


    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Creates a calibration workspace from a White-Beam Vanadium run.'


    def PyInit(self):
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',\
                             direction=Direction.Output),
                             doc='Output workspace for calibration data')

        self.declareProperty(IntArrayProperty(name='DetectorRange', values=[0, 1],\
                             validator=IntArrayMandatoryValidator()),
                             doc='Range of detectors')

        self.declareProperty(FloatArrayProperty(name='PeakRange', values=[0.0, 100.0],\
                             validator=FloatArrayMandatoryValidator()),
                             doc='')

        self.declareProperty(FloatArrayProperty(name='BackgroundRange', values=[0.0, 1000.0],\
                             validator=FloatArrayMandatoryValidator()),
                             doc='')

        self.declareProperty(name='ScaleFactor', defaultValue=1.0,
                             doc='')

        self.declareProperty(name='Plot', defaultValue=False, doc='Plot the calibration data')


    def validateInputs(self):
        """
        Validates input ranges.
        """
        issues = dict()

        issues['DetectorRange'] = self._validate_range('DetectorRange')
        issues['PeakRange'] = self._validate_range('PeakRange')
        issues['BackgroundRange'] = self._validate_range('BackgroundRange')

        return issues


    def _validate_range(self, property_name):
        """
        Validates a range property.

        @param property_name Name of the property to validate
        @returns String detailing error, None if no error
        """

        prop_range = self.getProperty(property_name).value
        if len(prop_range) == 2:
            if prop_range[0] > prop_range[1]:
                return 'Invalid range'
        else:
            return 'Incorrect number of values (should be 2)'

        return None


    def PyExec(self):
        from mantid import logger

        self._setup()

        runs = []
        for in_file in self._input_files:
            (_, filename) = os.path.split(in_file)
            (root, _) = os.path.splitext(filename)
            try:
                Load(Filename=in_file, OutputWorkspace=root,\
                    SpectrumMin=int(self._spec_range[0]), SpectrumMax=int(self._spec_range[1]),\
                    LoadLogFiles=False)
                runs.append(root)
            except Exception as exc:
                logger.error('Could not load raw file "%s": %s' % (in_file, str(exc)))

        calib_ws_name = 'calibration'
        if len(runs) > 1:
            MergeRuns(InputWorkspaces=",".join(runs), OutputWorkspace=calib_ws_name)
            factor = 1.0 / len(runs)
            Scale(InputWorkspace=calib_ws_name, OutputWorkspace=calib_ws_name, Factor=factor)
        else:
            calib_ws_name = runs[0]

        CalculateFlatBackground(InputWorkspace=calib_ws_name, OutputWorkspace=calib_ws_name,\
                StartX=self._back_range[0], EndX=self._back_range[1], Mode='Mean')

        from inelastic_indirect_reduction_steps import NormaliseToUnityStep
        ntu = NormaliseToUnityStep()
        ntu.set_factor(self._intensity_scale)
        ntu.set_peak_range(self._peak_range[0], self._peak_range[1])
        ntu.execute(None, calib_ws_name)

        RenameWorkspace(InputWorkspace=calib_ws_name, OutputWorkspace=self._out_ws)

        # Remove old workspaces
        if len(runs) > 1:
            for run in runs:
                DeleteWorkspace(Workspace=run)

        self.setProperty('OutputWorkspace', self._out_ws)
        self._post_process()


    def _setup(self):
        """
        Gets properties.
        """

        self._input_files = self.getProperty('InputFiles').value
        self._out_ws = self.getPropertyValue('OutputWorkspace')

        self._peak_range = self.getProperty('PeakRange').value
        self._back_range = self.getProperty('BackgroundRange').value
        self._spec_range = self.getProperty('DetectorRange').value

        self._intensity_scale = self.getProperty('ScaleFactor').value
        if self._intensity_scale == 1.0:
            self._intensity_scale = None

        self._plot = self.getProperty('Plot').value


    def _post_process(self):
        """
        Handles adding logs and plotting.
        """

        # Add sample logs to output workspace
        if self._intensity_scale is not None:
            AddSampleLog(Workspace=self._out_ws, LogName='Scale Factor', LogType='Number', LogText=str(self._intensity_scale))
        AddSampleLog(Workspace=self._out_ws, LogName='Peak Min', LogType='Number', LogText=str(self._peak_range[0]))
        AddSampleLog(Workspace=self._out_ws, LogName='Peak Max', LogType='Number', LogText=str(self._peak_range[1]))
        AddSampleLog(Workspace=self._out_ws, LogName='Back Min', LogType='Number', LogText=str(self._back_range[0]))
        AddSampleLog(Workspace=self._out_ws, LogName='Back Max', LogType='Number', LogText=str(self._back_range[1]))

        if self._plot:
            from mantidplot import plotBin
            plotBin(mtd[self._out_ws], 0)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectCalibration)
