#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from mantid.kernel import *
from mantid.api import *


class EnggFitDIFCFromPeaks(PythonAlgorithm):

    def category(self):
        return "Diffraction\\Engineering;Diffraction\\Fitting"

    def name(self):
        return "EnggFitPeaks"

    def summary(self):
        return ("The algorithm fits an expected diffraction pattern to a workpace spectrum by "
                "performing single peak fits.")

    def PyInit(self):

        self.declareProperty(ITableWorkspaceProperty('FittedPeaks', '', Direction.Input),
                             doc = "Information on fitted peaks, in the format produced by EnggFitPeaks. "
                             "The table must contain, for every peak fitted the expected peak value "
                             "(in d-spacing), and the parameters fitted. The expected values are given "
                             "in the column labelled 'dSpacing'. When using the back-to-back exponential "
                             "peak function, the 'X0' column must have the fitted peak center.")

        self.declareProperty('OutParametersTable', '', direction=Direction.Input,
                             doc = 'Name for a table workspace with the fitted values calculated by '
                             'this algorithm (DIFC and TZERO calibration parameters) for GSAS. '
                             'These two parameters are added as two columns in a single row. If not given, '
                             'the table workspace is not created.')

        self.declareProperty('DIFA', 0.0, direction = Direction.Output,
                             doc = 'Fitted DIFA value. This parameter is not effectively considered and it '
                             'is always zero in this version of the algorithm.')

        self.declareProperty('DIFC', 0.0, direction = Direction.Output,
                             doc = "Fitted DIFC calibration parameter")

        self.declareProperty('TZERO', 0.0, direction = Direction.Output,
                             doc = "Fitted TZERO calibration parameter")

    def validateInputs(self):
        errors = dict()

        peaks = self.getProperty('FittedPeaks').value

        # Better than failing to fit the linear function
        if 1 == peaks.rowCount():
            errors['FittedPeaks'] = ('Only one peak was given in the input peaks table. This is not enough '
                                     'to fit the output parameters difc and zero. Please check the list of '
                                     'expected peaks given and if it is appropriate for the workspace')
        return errors

    def PyExec(self):

        peaks = self.getProperty('FittedPeaks').value

        difa, difc, tzero = self._fit_difc_tzero(peaks)

        out_tbl_name = self.getPropertyValue('OutParametersTable')
        self._produce_outputs(difa, difc, tzero, out_tbl_name)

        self.log().information("Fitted {0} peaks in total. DIFA: {1}, DIFC: {2}, TZERO: {3}".
                               format(peaks.rowCount(), difa, difc, tzero))

    def _fit_difc_tzero(self, fitted_peaks_table):
        """
        Fits a linear function to the dSpacing-TOF relationship and
        returns the fitted (DIFA=0), DIFC and TZERO values. If the
        table passed has less than 2 peaks this raises an exception,
        as it is not possible to fit the difc, zero parameters.

        @param fitted_peaks_table :: table with one row per fitted peak, expecting column 'dSpacing'
        as x values and column 'X0' as y values.

        @returns DIFA, DIFC and TZERO parameters as defined in GSAS and GSAS-II. The difc and zero
        parameters are obtained from fitting a linear background (in _fit_dSpacing_to_ToF) to the
        peaks fitted individually that have been passed in the input table

        """

        num_peaks = fitted_peaks_table.rowCount()
        if num_peaks < 2:
            raise ValueError('Cannot fit a linear function with less than two peaks. Got a table of ' +
                             'peaks with ' + str(num_peaks) + ' peaks')

        convert_tbl_alg = self.createChildAlgorithm('ConvertTableToMatrixWorkspace')
        convert_tbl_alg.setProperty('InputWorkspace', fitted_peaks_table)
        convert_tbl_alg.setProperty('ColumnX', 'dSpacing')
        convert_tbl_alg.setProperty('ColumnY', 'X0')
        convert_tbl_alg.execute()
        dSpacingVsTof = convert_tbl_alg.getProperty('OutputWorkspace').value

        # Fit the curve to get linear coefficients of TOF <-> dSpacing relationship for the detector
        fit_alg = self.createChildAlgorithm('Fit')
        fit_alg.setProperty('Function', 'name=LinearBackground')
        fit_alg.setProperty('InputWorkspace', dSpacingVsTof)
        fit_alg.setProperty('WorkspaceIndex', 0)
        fit_alg.setProperty('CreateOutput', True)
        fit_alg.execute()
        param_table = fit_alg.getProperty('OutputParameters').value

        tzero = param_table.cell('Value', 0) # A0
        difc = param_table.cell('Value', 1) # A1
        difa = 0.0 # Not fitted, we may add an option for this later on

        return (difa, difc, tzero)

    def _produce_outputs(self, difa, difc, tzero, tbl_name):
        """
        Fills in the output properties as requested via the input
        properties. Sets the output difz/difc/tzero values. It can
        also produces a table with these parameters if required in the
        inputs.

        @param difa :: the DIFA GSAS parameter as fitted here
        @param difc :: the DIFC GSAS parameter as fitted here
        @param tzero :: the TZERO GSAS parameter as fitted here
        @param tbl_name :: a table name, non-empty if a table workspace
        should be created with the calibration parameters
        """

        import EnggUtils

        # mandatory outputs
        self.setProperty('DIFA', difa)
        self.setProperty('DIFC', difc)
        self.setProperty('TZERO', tzero)

        # optional outputs
        if tbl_name:
            EnggUtils.generateOutputParTable(tbl_name, difa, difc, tzero)
            self.log().information("Output parameters added into a table workspace: %s" % tbl_name)

AlgorithmFactory.subscribe(EnggFitDIFCFromPeaks)
