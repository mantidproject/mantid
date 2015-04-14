#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import WorkspaceProperty, FileProperty, FileAction, \
                       DataProcessorAlgorithm, AlgorithmFactory, mtd
from mantid.simpleapi import *


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
        return 'Creates a calibration workspace in energy trnasfer for IN16B.'


    def PyInit(self):
        self.declareProperty(FileProperty(name='Run', defaultValue='', action=FileAction.Load),
                             doc='Comma separated list of input files')

        self.declareProperty(name='MirrorMode', defaultValue=False,
                             doc='Data uses mirror mode')

        self.declareProperty(IntArrayProperty(name='SpectraRange', values=[0, 23],
                             validator=IntArrayMandatoryValidator()),
                             doc='Spectra range to use')

        self.declareProperty(FloatArrayProperty(name='PeakRange', values=[0.0, 100.0],
                             validator=FloatArrayMandatoryValidator()),
                             doc='Peak range in energy transfer')

        self.declareProperty(name='ScaleFactor', defaultValue=1.0,
                             doc='Intensity scaling factor')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Output workspace for calibration data')


    def PyExec(self):
        self._setup()

        temp_raw = '__raw'
        temp_left = '__left'
        temp_right = '__right'

        # Do an energy transfer reduction
        IndirectILLReduction(Run=self._input_file,
                             Analyser='silicon',
                             Reflection='111',
                             MirrorMode=self._mirror_mode,
                             RawWorkspace=temp_raw,
                             LeftWorkspace=temp_left,
                             RightWorkspace=temp_right,
                             ReducedWorkspace=self._out_ws)

        # Clean up unused workspaces
        DeleteWorkspace(temp_raw)
        DeleteWorkspace(temp_left)
        DeleteWorkspace(temp_right)

        # Integrate within peak range
        number_historgrams = mtd[self._out_ws].getNumberHistograms()
        Integration(InputWorkspace=self._out_ws,
                    OutputWorkspace=self._out_ws,
                    RangeLower=self._peak_range[0],
                    RangeUpper=self._peak_range[1],
                    StartWorkspaceIndex=self._spec_range[0],
                    EndWorkspaceIndex=self._spec_range[1])

        ws_mask, num_zero_spectra = FindDetectorsOutsideLimits(InputWorkspace=self._out_ws,
                                                               OutputWorkspace='__temp_ws_mask')
        DeleteWorkspace(ws_mask)

        # Process automatic scaling
        temp_sum = '__sum'
        SumSpectra(InputWorkspace=self._out_ws,
                   OutputWorkspace=temp_sum)
        total = mtd[temp_sum].readY(0)[0]
        DeleteWorkspace(temp_sum)

        if self._intensity_scale is None:
            self._intensity_scale = 1 / (total / (number_historgrams - num_zero_spectra))

        # Apply scaling factor
        Scale(InputWorkspace=self._out_ws,
              OutputWorkspace=self._out_ws,
              Factor=self._intensity_scale,
              Operation='Multiply')


        self.setProperty('OutputWorkspace', self._out_ws)


    def _setup(self):
        """
        Gets properties.
        """

        self._input_file = self.getProperty('Run').value
        self._out_ws = self.getPropertyValue('OutputWorkspace')

        self._peak_range = self.getProperty('PeakRange').value
        self._spec_range = self.getProperty('SpectraRange').value
        self._mirror_mode = self.getProperty('MirrorMode').value

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
