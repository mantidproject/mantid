from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *


class ISISIndirectEnergyTransfer(DataProcessorAlgorithm):

    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Runs an energy transfer reduction for an inelastic indirect geometry instrument.'


    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files')

        self.declareProperty(name='SumFiles', defaultValue=False, doc='Toggle input file summing or sequential processing')

        self.declareProperty(WorkspaceProperty('CalibrationWorkspace', '',
                             direction=Direction.Input, optional=PropertyMode.Optional), doc='Workspace contining calibration data')

        # Instrument configuration properties
        self.declareProperty(name='Instrument', defaultValue='', doc='Instrument used during run',
                             validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA', 'TFXA']))
        self.declareProperty(name='Analyser', defaultValue='', doc='Analyser used during run',
                             validator=StringListValidator(['graphite', 'mica', 'fmica']))
        self.declareProperty(name='Reflection', defaultValue='', doc='Reflection used during run',
                             validator=StringListValidator(['002', '004', '006']))

        self.declareProperty(IntArrayProperty(name='DetectorRange', values=[0, 1],
                             validator=IntArrayMandatoryValidator()),
                             doc='Comma separated range of detectors to use')
        self.declareProperty(FloatArrayProperty(name='BackgroundRange'),
                             doc='')
        self.declareProperty(name='RebinString', defaultValue='', doc='Rebin string parameters')
        self.declareProperty(name='DetailedBalance', defaultValue=-1.0, doc='')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0, doc='')
        self.declareProperty(name='FoldMultipleFrames', defaultValue=False, doc='')

        # Spectra grouping options
        self.declareProperty(name='GroupingMethod', defaultValue='Individual',
                             validator=StringListValidator(['Individual', 'All', 'Map File', 'Workspace', 'IPF']),
                             doc='Method used to group spectra.')
        self.declareProperty(WorkspaceProperty('GroupingWorkspace', '',
                             direction=Direction.Input, optional=PropertyMode.Optional),
                             doc='Workspace containing spectra grouping.')
        self.declareProperty(FileProperty('MapFile', '',
                             action=FileAction.OptionalLoad, extensions=['.map']),
                             doc='Workspace containing spectra grouping.')

        # Output properties
        self.declareProperty(name='Plot', defaultValue='None', doc='Type of plot to output after reduction',
                             validator=StringListValidator(['None', 'Spectra', 'Contour']))

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Workspace group for the resulting workspaces')


    def PyExec(self):
        self._setup()

        # TODO: Load
        # TODO: Identify bad detetors
        # TODO: Process monitor
        # TODO: Background removal
        # TODO: Apply calibration
        # TODO: Correct by monitor
        # TODO: Convert to energy
        # TODO: Detailed balance
        # TODO: Scale
        # TODO: Group spectra
        # TODO: Fold
        # TODO: Rename
        # TODO: Group workspaces
        # TODO: Plot


    def validateInputs(self):
        """
        Validates algorithm properties.
        """
        issues = dict()

        # TODO

        return issues


    def _setup(self):
        """
        Gets and algorithm properties.
        """

        self._data_files = self.getProperty('InputFiles').value
        self._sum_files = self.getProperty('SumFiles').value
        self._calib_ws_name = self.getPropertyValue('CalibrationWorkspace')

        self._instrument = self.getPropertyValue('Instrument')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        self._detector_range = self.getProperty('DetectorRange').value
        self._background_range = self.getProperty('BackgroundRange').value
        self._rebin_string = self.getPropertyValue('RebinString')
        self._detailed_balance = self.getProperty('DetailedBalance').value
        self._scale_factor = self.getProperty('ScaleFactor').value
        self._fold_multiple_frames = self.getProperty('FoldMultipleFrames').value

        self._grouping_method = self.getPropertyValue('GroupingMethod')
        self._grouping_ws = self.getPropertyValue('GroupingWorkspace')
        self._grouping_map_file = self.getpropertyValue('MapFile')

        self._plot_type = self.getPropertyValue('Plot')
        self._out_ws_group = self.getPropertyValue('OutputWorkspace')

        self._param_file = os.path.join(config['instrumentDefinition.directory'],
                                        self._instrument + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml')
        logger.information('Instrument parameter file: %s' % self._param_file)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ISISIndirectEnergyTransfer)
