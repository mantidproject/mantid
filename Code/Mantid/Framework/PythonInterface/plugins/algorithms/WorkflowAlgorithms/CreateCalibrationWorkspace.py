from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import os.path


class CreateCalibrationWorkspace(DataProcessorAlgorithm):

    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Creates a calibration workspace from a White-Beam Vanadium run.'


    def PyInit(self):
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files.')

        self.declareProperty(IntArrayProperty(name='DetectorRange', values=[0, 1],
                             validator=IntArrayMandatoryValidator()),
                             doc='Range of detectors.')

        self.declareProperty(FloatArrayProperty(name='PeakRange', values=[0.0, 100.0],
                             validator=FloatArrayMandatoryValidator()),
                             doc='Time of flight range over the peak.')

        self.declareProperty(FloatArrayProperty(name='BackgroundRange', values=[0.0, 1000.0],
                             validator=FloatArrayMandatoryValidator()),
                             doc='Time of flight range over the background.')

        self.declareProperty(name='ScaleFactor', defaultValue=1.0,
                             doc='Factor by which to scale the result.')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Plot the calibration data as a spectra plot.')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Output workspace for calibration data.')


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
                Load(Filename=in_file, OutputWorkspace=root,
                    SpectrumMin=int(self._spec_range[0]), SpectrumMax=int(self._spec_range[1]),
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

        CalculateFlatBackground(InputWorkspace=calib_ws_name, OutputWorkspace=calib_ws_name,
                                StartX=self._back_range[0], EndX=self._back_range[1], Mode='Mean')

        number_historgrams = mtd[calib_ws_name].getNumberHistograms()
        ws_mask, num_zero_spectra = FindDetectorsOutsideLimits(InputWorkspace=calib_ws_name, OutputWorkspace='__temp_ws_mask')
        DeleteWorkspace(ws_mask)

        Integration(InputWorkspace=calib_ws_name, OutputWorkspace=calib_ws_name,
                    RangeLower=self._peak_range[0], RangeUpper=self._peak_range[1])

        temp_sum = SumSpectra(InputWorkspace=calib_ws_name, OutputWorkspace='__temp_sum')
        total = temp_sum.readY(0)[0]
        DeleteWorkspace(temp_sum)

        if self._intensity_scale is None:
            self._intensity_scale = 1 / (total / (number_historgrams - num_zero_spectra))

        Scale(InputWorkspace=calib_ws_name, OutputWorkspace=calib_ws_name,
              Factor=self._intensity_scale, Operation='Multiply')

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
AlgorithmFactory.subscribe(CreateCalibrationWorkspace)
