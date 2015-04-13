#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import WorkspaceProperty, FileProperty, FileAction, \
                       DataProcessorAlgorithm, AlgorithmFactory, mtd
from mantid.simpleapi import Load, CalculateFlatBackground, DeleteWorkspace, \
                             Integration, SumSpectra, Scale, CropWorkspace, \
                             FindDetectorsOutsideLimits


class ILLIN16BCalibration(DataProcessorAlgorithm):

    _input_file = None
    _out_ws = None
    _peak_range = None
    _back_range = None
    _spec_range = None
    _intensity_scale = None


    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Creates a calibration workspace for IN16B.'


    def PyInit(self):
        self.declareProperty(FileProperty(name='InputFile', defaultValue='', action=FileAction.Load),
                             doc='Comma separated list of input files')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Output workspace for calibration data')

        self.declareProperty(IntArrayProperty(name='SpectraRange', values=[1, 2048],
                             validator=IntArrayMandatoryValidator()),
                             doc='Spectra range to use')

        self.declareProperty(FloatArrayProperty(name='PeakRange', values=[0.0, 100.0],
                             validator=FloatArrayMandatoryValidator()),
                             doc='Peak range in time of flight')

        self.declareProperty(FloatArrayProperty(name='BackgroundRange', values=[0.0, 1000.0],
                             validator=FloatArrayMandatoryValidator()),
                             doc='Background range in time of flight')

        self.declareProperty(name='ScaleFactor', defaultValue=1.0,
                             doc='Intensity scaling factor')


    def PyExec(self):
        self._setup()

        Load(Filename=self._input_file,
             OutputWorkspace=self._out_ws)

        CropWorkspace(InputWorkspace=self._out_ws,
                      OutputWorkspace=self._out_ws,
                      StartWorkspaceIndex=int(self._spec_range[0]),
                      EndWorkspaceIndex=int(self._spec_range[1]))

        CalculateFlatBackground(InputWorkspace=self._out_ws,
                                OutputWorkspace=self._out_ws,
                                StartX=self._back_range[0],
                                EndX=self._back_range[1],
                                Mode='Mean')

        number_historgrams = mtd[self._out_ws].getNumberHistograms()
        Integration(InputWorkspace=self._out_ws,
                    OutputWorkspace=self._out_ws,
                    RangeLower=self._peak_range[0],
                    RangeUpper=self._peak_range[1])

        ws_mask, num_zero_spectra = FindDetectorsOutsideLimits(InputWorkspace=self._out_ws,
                                                               OutputWorkspace='__temp_ws_mask')
        DeleteWorkspace(ws_mask)

        tempSum = SumSpectra(InputWorkspace=self._out_ws,
                             OutputWorkspace='__temp_sum')
        total = tempSum.readY(0)[0]
        DeleteWorkspace(tempSum)

        if self._intensity_scale is None:
            self._intensity_scale = 1 / ( total / (number_historgrams - num_zero_spectra) )

        Scale(InputWorkspace=self._out_ws,
              OutputWorkspace=self._out_ws,
              Factor=self._intensity_scale,
              Operation='Multiply')

        self.setProperty('OutputWorkspace', self._out_ws)


    def _setup(self):
        """
        Gets properties.
        """

        self._input_file = self.getProperty('InputFile').value
        self._out_ws = self.getPropertyValue('OutputWorkspace')

        self._peak_range = self.getProperty('PeakRange').value
        self._back_range = self.getProperty('BackgroundRange').value
        self._spec_range = self.getProperty('SpectraRange').value

        self._intensity_scale = self.getProperty('ScaleFactor').value
        if self._intensity_scale == 1.0:
            self._intensity_scale = None


    def validateInputs(self):
        """
        Validates input ranges.
        """
        issues = dict()

        issues['SpectraRange'] = self._validate_range('SpectraRange')
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


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ILLIN16BCalibration)
