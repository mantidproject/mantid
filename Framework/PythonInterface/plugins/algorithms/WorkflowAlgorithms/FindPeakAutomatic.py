# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function, unicode_literals)

from mantid.simpleapi import (ConvertToPointData, CreateWorkspace, RenameWorkspace, DeleteWorkspace,
                              CreateEmptyTableWorkspace, FitGaussianPeaks)
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, WorkspaceProperty, Progress)
from mantid.kernel import (Direction, FloatBoundedValidator, IntBoundedValidator)
from mantid import mtd

import numpy as np
import scipy.signal


class FindPeakAutomatic(DataProcessorAlgorithm):
    _acceptance = 0.01
    _bad_peak_to_consider = 50
    _smooth_window = 5
    _use_poisson_cost = False
    _fit_to_baseline = False
    _estimate_peak_sigma = 5
    _min_sigma = 0.0
    _max_sigma = 30.0
    _plot_peaks = None
    _plot_baseline = None

    def category(self):
        return 'Optimization\\PeakFinding'

    def summary(self):
        return 'Locates and estimated parameters for all the peaks in an emission spectra'

    def seeAlso(self):
        return [
            'FitGaussianPeaks', 'FindPeaks', 'FindPeaksMD', 'FindSXPeaks', 'FitPeak', 'FitPeaks'
        ]

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)

    def PyInit(self):
        # Input workspace
        self.declareProperty(
            WorkspaceProperty(name='InputWorkspace', defaultValue='', direction=Direction.Input),
            'Workspace with peaks to be identified')

        # Input parameters
        self.declareProperty('StartXValue', 0.0, doc='Value of X to start the search from')
        self.declareProperty('EndXValue', np.Inf, doc='Value of X to stop the search to')
        self.declareProperty(
            'AcceptanceThreshold',
            0.01,
            doc=
            'Threshold for considering a peak significant, the exact meaning of the value depends '
            'on the cost function used and the data to be fitted. '
            'Good values might be about 1-10 for poisson cost and 0.0001-0.01 for chi2',
            validator=FloatBoundedValidator(lower=0.0))
        self.declareProperty(
            'SmoothWindow',
            5,
            doc='Half size of the window used to find the background values to subtract',
            validator=IntBoundedValidator(lower=0))
        self.declareProperty(
            'BadPeaksToConsider',
            20,
            doc='Number of peaks that do not exceed the acceptance threshold to be searched before '
            'terminating. This is useful because sometimes good peaks can be found after '
            'some bad ones. However setting this value too high will make the search much slower.',
            validator=IntBoundedValidator(lower=0))
        self.declareProperty(
            'UsePoissonCost',
            False,
            doc='Use a probabilistic approach to find the cost of a fit instead of using chi2.')
        self.declareProperty(
            'FitToBaseline',
            False,
            doc='Use a probabilistic approach to find the cost of a fit instead of using chi2.')
        self.declareProperty(
            'EstimatePeakSigma',
            3.0,
            doc='A rough estimate of the standard deviation of the gaussian used to fit the peaks',
            validator=FloatBoundedValidator(lower=0.0))
        self.declareProperty('MinPeakSigma',
                             0.5,
                             doc='Minimum value for the standard deviation of a peak',
                             validator=FloatBoundedValidator(lower=0.0))
        self.declareProperty('MaxPeakSigma',
                             30.0,
                             doc='Maximum value for the standard deviation of a peak',
                             validator=FloatBoundedValidator(lower=0.0))
        self.declareProperty('PlotPeaks', False,
                             'Plot the position of the peaks found by the algorithm')
        self.declareProperty('PlotBaseline', False,
                             'Plot the baseline as calculated by the algorithm')

        # Output table
        self.declareProperty(name='PeakPropertiesTableName',
                             defaultValue='',
                             doc='Name of the table containing the properties of the peaks')
        self.declareProperty(
            name='RefitPeakPropertiesTableName',
            defaultValue='',
            doc=
            'Name of the table containing the properties of the peaks that had to be fitted twice '
            'as the first time the error was unreasonably large')

    def validateInputs(self):
        issues = {}

        self._acceptance = self.getProperty('AcceptanceThreshold').value
        self._smooth_window = self.getProperty('SmoothWindow').value
        self._bad_peak_to_consider = self.getProperty('BadPeaksToConsider').value
        self._use_poisson_cost = self.getProperty('UsePoissonCost').value
        self._fit_to_baseline = self.getProperty('FitToBaseline').value
        self._plot_peaks = self.getProperty('PlotPeaks').value
        self._plot_baseline = self.getProperty('PlotBaseline').value
        self._estimate_peak_sigma = self.getProperty('EstimatePeakSigma').value
        self._min_sigma = self.getProperty('MinPeakSigma').value
        self._max_sigma = self.getProperty('MaxPeakSigma').value

        if self._max_sigma < self._min_sigma:
            issues['MinPeakSigma'] = 'Sigma bounds must be: MinPeakSigma <= MaxPeakSigma'
            issues['MaxPeakSigma'] = 'Sigma bounds must be: MinPeakSigma <= MaxPeakSigma'

        if self._estimate_peak_sigma < self._min_sigma:
            issues['EstimatePeakSigma'] = 'EstimatePeakSigma must be greater than MinPeakSigma'

        if self._estimate_peak_sigma > self._max_sigma:
            issues['EstimatePeakSigma'] = 'EstimatePeakSigma must be greater than MaxPeakSigma'

        return issues

    def PyExec(self):
        # Progress reporter for algorithm initialization
        prog_reporter = Progress(self, start=0.0, end=0.1, nreports=4)

        raw_xvals, raw_yvals, raw_error, raw_data_ws, error_ws = self.load_data(prog_reporter)

        # Convert the data to point data
        prog_reporter.report('Converting to point data')
        raw_data_ws = ConvertToPointData(error_ws)
        raw_xvals = raw_data_ws.readX(0).copy()
        raw_yvals = raw_data_ws.readY(0).copy()

        raw_xvals, raw_yvals, raw_error = self.crop_data(raw_xvals, raw_yvals, raw_error,
                                                         prog_reporter)

        # Find the best peaks
        (peakids, peak_table,
         refit_peak_table), baseline = self.process(raw_xvals,
                                                    raw_yvals,
                                                    raw_error,
                                                    acceptance=self._acceptance,
                                                    average_window=self._smooth_window,
                                                    bad_peak_to_consider=self._bad_peak_to_consider,
                                                    use_poisson=self._use_poisson_cost,
                                                    peak_width_estimate=self._estimate_peak_sigma,
                                                    fit_to_baseline=self._fit_to_baseline,
                                                    prog_reporter=prog_reporter)

        if self._plot_peaks:
            self.plot_peaks(raw_xvals, raw_yvals, baseline, peakids)

        self.set_output_properties(raw_data_ws)

        self.delete_temporary_workspaces()

    def load_data(self, prog_reporter):
        # Load the data and clean from Nans
        raw_data_ws = self.getProperty('InputWorkspace').value
        raw_xvals = raw_data_ws.readX(0).copy()
        raw_yvals = raw_data_ws.readY(0).copy()
        prog_reporter.report('Loaded data')

        # If the data does not have errors use poisson statistics create an workspace with added errors
        raw_error = raw_data_ws.readE(0).copy()
        if len(np.argwhere(raw_error > 0)) == 0:
            raw_error = np.sqrt(raw_yvals)
            error_ws = '{}_with_errors'.format(raw_data_ws.getName())
            CreateWorkspace(DataX=raw_xvals,
                            DataY=raw_yvals,
                            DataE=raw_error,
                            OutputWorkspace=error_ws)
        else:
            error_ws = raw_data_ws

        return raw_xvals, raw_yvals, raw_error, raw_data_ws, error_ws

    def crop_data(self, raw_xvals, raw_yvals, raw_error, prog_reporter):
        # Crop the data as required by the user
        start_index = min(np.argwhere(raw_xvals > self.getProperty('StartXValue').value))[0]
        end_index = max(np.argwhere(raw_xvals < self.getProperty('EndXValue').value))[0]
        raw_xvals = raw_xvals[np.isfinite(raw_yvals)][start_index:end_index]
        raw_error = raw_error[np.isfinite(raw_yvals)][start_index:end_index]
        raw_yvals = raw_yvals[np.isfinite(raw_yvals)][start_index:end_index]
        prog_reporter.report('Cropped data')

        return raw_xvals, raw_yvals, raw_error

    def plot_peaks(self, raw_xvals, raw_yvals, baseline, peakids):
        import matplotlib.pyplot as plt
        plt.plot(raw_xvals, raw_yvals)
        if self._plot_baseline:
            plt.plot(raw_xvals, baseline)
        plt.scatter(raw_xvals[peakids], raw_yvals[peakids], marker='x', c='r')
        plt.show()

    def set_output_properties(self, raw_data_ws):
        if self.getPropertyValue('PeakPropertiesTableName') == '':
            peak_table_name = '{}_{}'.format(raw_data_ws.getName(), 'properties')
        else:
            peak_table_name = self.getPropertyValue('PeakPropertiesTableName')
        if self.getPropertyValue('RefitPeakPropertiesTableName') == '':
            refit_peak_table_name = '{}_{}'.format(raw_data_ws.getName(), 'refit_properties')
        else:
            refit_peak_table_name = self.getPropertyValue('RefitPeakPropertiesTableName')
        RenameWorkspace('peak_table', peak_table_name)
        RenameWorkspace('refit_peak_table', refit_peak_table_name)

    def delete_temporary_workspaces(self):
        self.delete_if_present('ret')
        self.delete_if_present('peak_table')
        self.delete_if_present('refit_peak_table')
        self.delete_if_present('raw_data_ws')
        self.delete_if_present('flat_ws')
        self.delete_if_present('fit_result_NormalisedCovarianceMatrix')
        self.delete_if_present('fit_result_Parameters')
        self.delete_if_present('fit_result_Workspace')
        self.delete_if_present('fit_cost')

    @staticmethod
    def delete_if_present(workspace):
        if workspace in mtd:
            DeleteWorkspace(workspace)

    def _single_erosion(self, yvals, centre, half_window_size):
        if half_window_size == 0:
            return yvals[centre]

        left_id = max(0, centre - half_window_size)
        right_id = min(len(yvals), centre + half_window_size + 1)

        return np.min(yvals[left_id:right_id])

    def _single_dilation(self, yvals, centre, half_window_size):
        if half_window_size == 0:
            return yvals[centre]

        left_id = max(0, centre - half_window_size)
        right_id = min(len(yvals), centre + half_window_size + 1)

        return np.max(yvals[left_id:right_id])

    def erosion(self, yvals, half_window_size):
        new_yvals = yvals.copy()
        for i in range(len(yvals)):
            new_yvals[i] = self._single_erosion(yvals, i, half_window_size)

        return new_yvals

    def dilation(self, yvals, half_window_size):
        new_yvals = yvals.copy()
        for i in range(len(yvals)):
            new_yvals[i] = self._single_dilation(yvals, i, half_window_size)

        return new_yvals

    def opening(self, yvals, half_window_size):
        return self.dilation(self.erosion(yvals, half_window_size), half_window_size)

    def average(self, yvals, half_window_size):
        average = self.dilation(self.opening(yvals, half_window_size), half_window_size)
        average += self.erosion(self.opening(yvals, half_window_size), half_window_size)
        return average / 2

    def generate_peak_guess_table(self, xvals, peakids):
        peak_table = CreateEmptyTableWorkspace()
        peak_table.addColumn(type='float', name='centre')
        for peak_idx in sorted(peakids):
            peak_table.addRow([xvals[peak_idx]])

        return peak_table

    def find_good_peaks(self, xvals, peakids, acceptance, bad_peak_to_consider, use_poisson, fit_ws,
                        peak_width_estimate):
        prog_reporter = Progress(self, start=0.1, end=1.0, nreports=2 + len(peakids))

        actual_peaks = []
        skipped = 0
        cost_idx = 1 if use_poisson else 0
        peak_table, refit_peak_table, cost = FitGaussianPeaks(
            InputWorkspace=fit_ws,
            PeakGuessTable=self.generate_peak_guess_table(xvals, []),
            CentreTolerance=1.0,
            EstimatedPeakSigma=peak_width_estimate,
            MinPeakSigma=self._min_sigma,
            MaxPeakSigma=self._max_sigma,
            GeneralFitTolerance=0.1,
            RefitTolerance=0.001)
        old_cost = cost.column(cost_idx)[0]
        prog_reporter.report('Fitting null hypothesis')

        for idx, peak_idx in enumerate(peakids):
            peak_table, refit_peak_table, cost = FitGaussianPeaks(
                InputWorkspace=fit_ws,
                PeakGuessTable=self.generate_peak_guess_table(xvals, actual_peaks + [peak_idx]),
                CentreTolerance=1.0,
                EstimatedPeakSigma=peak_width_estimate,
                MinPeakSigma=self._min_sigma,
                MaxPeakSigma=self._max_sigma,
                GeneralFitTolerance=0.1,
                RefitTolerance=0.001)
            new_cost = cost.column(cost_idx)[0]
            if use_poisson:
                # if p_new > p_old, but uses logs
                cost_change = new_cost - old_cost
                good_peak_condition = cost_change > np.log(acceptance)
            else:
                cost_change = abs(new_cost - old_cost) / new_cost
                good_peak_condition = (new_cost <= old_cost) and (cost_change > acceptance)

            if skipped > bad_peak_to_consider:
                break
            if good_peak_condition:
                skipped = 0
                actual_peaks.append(peak_idx)
                old_cost = new_cost
            else:
                skipped += 1
            prog_reporter.report('Iteration {}, {} peaks found'.format(idx + 1, len(actual_peaks)))

        peak_table, refit_peak_table, cost = FitGaussianPeaks(
            InputWorkspace=fit_ws,
            PeakGuessTable=self.generate_peak_guess_table(xvals, actual_peaks),
            CentreTolerance=1.0,
            EstimatedPeakSigma=peak_width_estimate,
            MinPeakSigma=self._min_sigma,
            MaxPeakSigma=self._max_sigma,
            GeneralFitTolerance=0.1,
            RefitTolerance=0.001)
        prog_reporter.report('Fitting done')
        return actual_peaks, peak_table, refit_peak_table

    def process(self, raw_xvals, raw_yvals, raw_error, acceptance, average_window,
                bad_peak_to_consider, use_poisson, peak_width_estimate, fit_to_baseline,
                prog_reporter):
        # Remove background
        rough_base = self.average(raw_yvals, average_window)
        baseline = rough_base + self.average(raw_yvals - rough_base, average_window)
        flat_yvals = raw_yvals - baseline
        if fit_to_baseline:
            tmp = baseline.copy()
            baseline = flat_yvals
            flat_yvals = tmp
        flat_ws = CreateWorkspace(DataX=np.concatenate((raw_xvals, raw_xvals)),
                                  DataY=np.concatenate((flat_yvals, baseline)),
                                  DataE=np.concatenate((raw_error, raw_error)),
                                  NSpec=2)
        prog_reporter.report('Removed background')

        # Find all the peaks. find_peaks was introduced in scipy 1.1.0, if using an older version use find_peaks_cwt
        # however this will not do an equally good job as it cannot sort by prominence (also added in 1.1.0)
        major, minor, _ = scipy.__version__.split('.')
        if int(major) >= 1 and int(minor) >= 1:
            raw_peaks, _ = scipy.signal.find_peaks(raw_yvals)
            flat_peaks, params = scipy.signal.find_peaks(flat_yvals, prominence=(None, None))
            prominence = params['prominences']
            flat_peaks = sorted(zip(flat_peaks, prominence), key=lambda x: x[1], reverse=True)
            if fit_to_baseline:
                flat_peaks = [peak_idx for peak_idx, prom in flat_peaks if peak_idx]
            else:
                flat_peaks = [peak_idx for peak_idx, prom in flat_peaks if peak_idx in raw_peaks]
        else:
            flat_peaks = scipy.signal.find_peaks_cwt(flat_yvals, widths=np.array([0.1]))
            flat_peaks = sorted(flat_peaks, key=lambda peak_idx: flat_yvals[peak_idx], reverse=True)

        return self.find_good_peaks(raw_xvals,
                                    flat_peaks,
                                    acceptance=acceptance,
                                    bad_peak_to_consider=bad_peak_to_consider,
                                    use_poisson=use_poisson,
                                    fit_ws=flat_ws,
                                    peak_width_estimate=peak_width_estimate), baseline


AlgorithmFactory.subscribe(FindPeakAutomatic)
