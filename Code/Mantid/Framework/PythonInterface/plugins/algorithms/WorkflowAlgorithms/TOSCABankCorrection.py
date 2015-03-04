#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *


class TOSCABankCorrection(DataProcessorAlgorithm):

    _input_ws = None
    _output_ws = None
    _search_range = None
    _peak_tolerance = None
    _peak_position = None
    _peak_function = None


    def category(self):
        return 'PythonAlgorithms;Inelastic;CorrectionFunctions'


    def summary(self):
        return 'Corrects TOSCA reductions where the peaks across banks are not in alignment.'


    def PyInit(self):
        self.declareProperty(WorkspaceProperty(name='InputWorkspace', defaultValue='',
                             direction=Direction.Input),
                             doc='Input reduced workspace')

        self.declareProperty(FloatArrayProperty(name='SearchRange',
                                                values=[200, 2000]),
                             doc='Range over which to find peaks')

        self.declareProperty(name='PeakPosition', defaultValue='',
                             doc='Specify a particular peak to use')

        self.declareProperty(name='ClosePeakTolerance', defaultValue=20.0,
                             doc='Tolerance under which peaks are considered to be the same')

        self.declareProperty(name='PeakFunction', defaultValue='Lorentzian',
                             validator=StringListValidator(['Lorentzian', 'Gaussian']),
                             doc='Type of peak to search for')

        self.declareProperty(MatrixWorkspaceProperty(name='OutputWorkspace', defaultValue='',
                             direction=Direction.Output),
                             doc='Output corrected workspace')


    def _validate_range(self, name):
        """
        Validates a range property

        @param name Name of the property
        """

        range_prop = self.getProperty(name).value

        if len(range_prop) != 2:
            return 'Range must have two values'

        if range_prop[0] > range_prop[1]:
            return 'Range must be in format "low,high"'

        return ''


    def validateInput(self):
        issues = dict()

        # Validate search range
        search_range_valid = self._validate_range('SearchRange')
        if search_range_valid != '':
            issues['SearchRange'] = search_range_valid

        return issues


    def PyExec(self):
        self._get_properties()

        # Crop the sample workspace to the search range
        CropWorkspace(InputWorkspace=self._input_ws,
                      OutputWorkspace='__search_ws',
                      XMin=self._search_range[0],
                      XMax=self._search_range[1])

        peak = self._get_peak('__search_ws')
        DeleteWorkspace('__search_ws')

        # Ensure there is at least one peak found
        if peak is None:
            raise RuntimeError('Could not find any peaks. Try increasing \
                                width of SearchRange and/or ClosePeakTolerance')

        target_centre = (peak[0] + peak[1]) / 2.0
        bank_1_scale_factor = target_centre / peak[0]
        bank_2_scale_factor = target_centre / peak[1]

        logger.information('Bank 1 scale factor: %f' % bank_1_scale_factor)
        logger.information('Bank 2 scale factor: %f' % bank_2_scale_factor)

        self._apply_correction(bank_1_scale_factor, bank_2_scale_factor)

        self.setPropertyValue('OutputWorkspace', self._output_ws)


    def _get_properties(self):
        self._input_ws = self.getPropertyValue('InputWorkspace')
        self._output_ws = self.getPropertyValue('OutputWorkspace')

        self._search_range = self.getProperty('SearchRange').value
        self._peak_tolerance = self.getProperty('ClosePeakTolerance').value

        self._peak_function = self.getPropertyValue('PeakFunction')

        try:
            self._peak_position = float(self.getPropertyValue('PeakPosition'))
        except:
            self._peak_position = None


    def _get_peak(self, search_ws):
        """
        Finds a matching peak over the two banks.

        @param search_ws Workspace to search
        @return Peak centres for matching peak over both banks
        """

        find_peak_args = dict()
        if self._peak_position is not None:
            find_peak_args['PeakPositions'] = [self._peak_position]

        # Find the peaks in each bank
        FindPeaks(InputWorkspace=search_ws,
                  PeaksList='__bank_1_peaks',
                  WorkspaceIndex=0,
                  PeakFunction=self._peak_function,
                  **find_peak_args)

        FindPeaks(InputWorkspace=search_ws,
                  PeaksList='__bank_2_peaks',
                  WorkspaceIndex=1,
                  PeakFunction=self._peak_function,
                  **find_peak_args)

        # Sort peaks by height, prefer to match tall peaks
        SortTableWorkspace(InputWorkspace='__bank_1_peaks',
                           OutputWorkspace='__bank_1_peaks',
                           Columns='centre',
                           Ascending=False)

        bank_1_ws = mtd['__bank_1_peaks']
        bank_2_ws = mtd['__bank_2_peaks']

        matching_peaks = list()

        # Find the centres of two peaks that are close to each other on both banks
        for peak_idx in range(0, bank_1_ws.rowCount()):
            bank_1_centre = bank_1_ws.cell('centre', peak_idx)

            for other_peak_idx in range(0, bank_2_ws.rowCount()):
                bank_2_centre = bank_2_ws.cell('centre', other_peak_idx)

                if abs(bank_1_centre - bank_2_centre) < self._peak_tolerance:
                    matching_peaks.append((bank_1_centre, bank_2_centre))

        # Remove temporary workspaces
        DeleteWorkspace('__bank_1_peaks')
        DeleteWorkspace('__bank_2_peaks')

        selected_peak = matching_peaks[0]
        logger.debug('Found matching peak: %s' % (str(selected_peak)))

        return selected_peak


    def _apply_correction(self, bank_1_sf, bank_2_sf):
        """
        Applies correction to a copy of the input workspace.

        @param bank_1_sf Bank 1 scale factor
        @param bank_2_sf Bank 2 scale factor
        """

        # Get the spectra for each bank plus sum of all banks
        ExtractSingleSpectrum(InputWorkspace=self._input_ws,
                              OutputWorkspace='__bank_1',
                              WorkspaceIndex=0)

        ExtractSingleSpectrum(InputWorkspace=self._input_ws,
                              OutputWorkspace='__bank_2',
                              WorkspaceIndex=1)

        ExtractSingleSpectrum(InputWorkspace=self._input_ws,
                              OutputWorkspace='__summed',
                              WorkspaceIndex=2)

        # Correct with shift in X
        ConvertAxisByFormula(InputWorkspace='__bank_1',
                             OutputWorkspace='__bank_1',
                             Axis='X', Formula='x=x*%f' % bank_1_sf)

        ConvertAxisByFormula(InputWorkspace='__bank_2',
                             OutputWorkspace='__bank_2',
                             Axis='X', Formula='x=x*%f' % bank_2_sf)

        # Rebin the two corrected spectra to the original workspace binning
        RebinToWorkspace(WorkspaceToRebin='__bank_1',
                         WorkspaceToMatch='__summed',
                         OutputWorkspace='__bank_1')

        RebinToWorkspace(WorkspaceToRebin='__bank_2',
                         WorkspaceToMatch='__summed',
                         OutputWorkspace='__bank_2')

        # Append spectra to get output workspace
        AppendSpectra(InputWorkspace1='__bank_1',
                      InputWorkspace2='__bank_2',
                      OutputWorkspace=self._output_ws)

        AppendSpectra(InputWorkspace1=self._output_ws,
                      InputWorkspace2='__summed',
                      OutputWorkspace=self._output_ws)

        # Remove temporary workspaces
        DeleteWorkspace('__bank_1')
        DeleteWorkspace('__bank_2')
        DeleteWorkspace('__summed')


AlgorithmFactory.subscribe(TOSCABankCorrection)
