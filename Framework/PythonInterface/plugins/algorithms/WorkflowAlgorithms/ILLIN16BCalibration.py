#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import (WorkspaceProperty, FileProperty, MultipleFileProperty, FileAction,
                        DataProcessorAlgorithm, AlgorithmFactory)
from mantid.simpleapi import *


class ILLIN16BCalibration(DataProcessorAlgorithm):

    _input_file = None
    _out_ws = None
    _map_file = None
    _peak_range = None
    _intensity_scale = None



    def category(self):
        return 'Workflow\\Inelastic;Inelastic\\Calibration'


    def summary(self):
        return 'Creates a calibration workspace in energy trnasfer for IN16B.'


    def PyInit(self):
        self.declareProperty(MultipleFileProperty(name='Run',extensions=['nxs']),
                             doc='List of input file (s)')

        self.declareProperty(FileProperty(name='MapFile', defaultValue='',
                                          action=FileAction.OptionalLoad,
                                          extensions=['xml']),
                             doc='Detector grouping map file')

        self.declareProperty(FloatArrayProperty(name='PeakRange', values=[-0.01, 0.01],
                                                validator=FloatArrayMandatoryValidator()),
                             doc='Peak range in energy transfer in meV')

        self.declareProperty(name='ScaleFactor', defaultValue=1.0,
                             doc='Intensity scaling factor')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', 'calib',
                                               direction=Direction.Output),
                             doc='Output workspace for calibration data')


    def PyExec(self):
        self._setup()

        __temp = IndirectILLReduction(Run=self._input_file,
                                      MapFile=self._map_file,
                                      SumRuns=True,
                                      UnmirrorOption=1)

        # Integrate within peak range
        __ws_name = __temp.getItem(0).getName()
        __ws_name = Integration(InputWorkspace=__ws_name,
                                RangeLower=float(self._peak_range[0]),
                                RangeUpper=float(self._peak_range[1]))

        # Apply scaling factor
        Scale(InputWorkspace=__ws_name,
              OutputWorkspace=self._out_ws,
              Factor=self._intensity_scale,
              Operation='Multiply')

        DeleteWorkspace(__ws_name)

        self.setProperty('OutputWorkspace', self._out_ws)


    def _setup(self):
        """
        Gets properties.
        """
        self._input_file = self.getPropertyValue('Run')
        self._out_ws = self.getPropertyValue('OutputWorkspace')

        self._map_file = self.getPropertyValue('MapFile')
        self._peak_range = self.getProperty('PeakRange').value

        self._intensity_scale = self.getProperty('ScaleFactor').value

    def validateInputs(self):
        """
        Validates input ranges.
        """
        issues = dict()

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
