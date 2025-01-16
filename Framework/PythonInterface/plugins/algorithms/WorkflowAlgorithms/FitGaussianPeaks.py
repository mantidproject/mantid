# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateEmptyTableWorkspace, Fit
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, WorkspaceProperty, ITableWorkspaceProperty
from mantid.kernel import Direction, FloatBoundedValidator, IntBoundedValidator

import numpy as np
import scipy.optimize

MINIMUM_NUMBER_OF_DATA_POINTS = 5
MINIMUM_ALLOWED_WIN_SIZE = 2


class FitGaussianPeaks(DataProcessorAlgorithm):
    # Default arguments
    _centre_tolerance = 1.0
    _estimate_sigma = 3.0
    _min_sigma = 0.0
    _max_sigma = 30.0
    _estimate_fit_window = True
    _fit_window_size = 5
    _general_tolerance = 0.1
    _refit_tolerance = 0.001

    def category(self):
        return "Optimization\\PeakFinding"

    def summary(self):
        return "Fits a list of gaussian peaks returning the parameters and the value of a cost function."

    def seeAlso(self):
        return ["FindPeaksAutomatic", "FindPeaks", "FindPeaksMD", "FindSXPeaks", "FitPeak", "FitPeaks"]

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input), "Workspace with peaks to be identified"
        )
        self.declareProperty(
            ITableWorkspaceProperty(name="PeakGuessTable", defaultValue="peak_guess", direction=Direction.Input),
            "Table containing the guess for the peak position",
        )
        self.declareProperty(
            "CentreTolerance", 1.0, doc="Tolerance value used in looking for peak centre", validator=FloatBoundedValidator(lower=0.0)
        )
        self.declareProperty("EstimatedPeakSigma", 3.0, doc="Estimate of the peak half width", validator=FloatBoundedValidator(lower=0.0))
        self.declareProperty(
            "MinPeakSigma", 0.1, doc="Minimum value for the standard deviation of a peak", validator=FloatBoundedValidator(lower=0.0)
        )
        self.declareProperty(
            "MaxPeakSigma", 30.0, doc="Maximum value for the standard deviation of a peak", validator=FloatBoundedValidator(lower=0.0)
        )
        self.declareProperty(
            "EstimateFitWindow",
            True,
            doc="If checked, algorithm attempts to calculate number of data points to use from"
            " EstimatePeakSigma, if unchecked algorithm will use FitWindowSize argument",
        )
        self.declareProperty(
            "FitWindowSize",
            5,
            doc="Number of data point used to fit peaks, minimum allowed value is 5, value must be an odd number ",
            validator=IntBoundedValidator(lower=5),
        )
        self.declareProperty(
            "GeneralFitTolerance", 0.1, doc="Tolerance for the constraint in the general fit", validator=FloatBoundedValidator(lower=0.0)
        )
        self.declareProperty(
            "RefitTolerance", 0.001, doc="Tolerance for the constraint in the refitting", validator=FloatBoundedValidator(lower=0.0)
        )

        # Output table
        self.declareProperty(
            ITableWorkspaceProperty(name="PeakProperties", defaultValue="peak_table", direction=Direction.Output),
            "Table containing the properties of the peaks",
        )
        self.declareProperty(
            ITableWorkspaceProperty(name="RefitPeakProperties", defaultValue="refit_peak_table", direction=Direction.Output),
            "Table containing the properties of the peaks that had to be fitted twice as the first time the error was unreasonably large",
        )
        self.declareProperty(
            ITableWorkspaceProperty(name="FitCost", defaultValue="fit_cost", direction=Direction.Output),
            "Table containing the value of both chi2 and poisson cost functions for the fit",
        )

    def validateInputs(self):
        issues = {}

        self._centre_tolerance = self.getProperty("CentreTolerance").value
        self._estimate_sigma = self.getProperty("EstimatedPeakSigma").value
        self._min_sigma = self.getProperty("MinPeakSigma").value
        self._max_sigma = self.getProperty("MaxPeakSigma").value
        self._fit_window_size = self.getProperty("FitWindowSize").value
        self._general_tolerance = self.getProperty("GeneralFitTolerance").value
        self._refit_tolerance = self.getProperty("RefitTolerance").value

        if self._max_sigma < self._min_sigma:
            issues["MinPeakSigma"] = "Sigma bounds must be: MinPeakSigma <= MaxPeakSigma"
            issues["MaxPeakSigma"] = "Sigma bounds must be: MinPeakSigma <= MaxPeakSigma"

        if self._estimate_sigma < self._min_sigma:
            issues["EstimatePeakSigma"] = "EstimatePeakSigma must be greater than MinPeakSigma"

        if self._estimate_sigma > self._max_sigma:
            issues["EstimatePeakSigma"] = "EstimatePeakSigma must be greater than MaxPeakSigma"
        if self._fit_window_size % 2 != 1:
            issues["FitWindowSize"] = "WindowSize must be an odd number"
        return issues

    def PyExec(self):
        self._estimate_fit_window = self.getProperty("EstimateFitWindow").value
        xvals, flat_yvals, baseline, errors = self._load_data()

        # Find the index of the peaks given as input
        # This is necessary as the FindPeakAlgorithm can introduce offset in the data
        peakids = self._recentre_peak_position(xvals, flat_yvals)

        # Fit all the peaks and find the best parameters
        _, param = self.general_fit(xvals, flat_yvals, peakids)

        # Create table of peak positions
        peak_table = CreateEmptyTableWorkspace(StoreInADS=False)
        refit_peak_table = CreateEmptyTableWorkspace(StoreInADS=False)
        to_refit = self.parse_fit_table(param, peak_table, True)

        # Refit all the bad peaks by adding stronger constraints
        _, refitted_params = self.refit_peaks(to_refit)
        self.parse_fit_table(refitted_params, refit_peak_table, False)

        # Evaluate the total cost
        fit_cost = self._evaluate_final_cost(xvals, flat_yvals, baseline, errors, peak_table, refit_peak_table)

        self.setProperty("PeakProperties", peak_table)
        self.setProperty("RefitPeakProperties", refit_peak_table)
        self.setProperty("FitCost", fit_cost)

    def _get_window_size(self, xvals):
        # method returns win_size used to determine how many data points to use for fit.
        if self._estimate_fit_window:
            win_size = int(self._estimate_sigma * len(xvals) / (max(xvals) - min(xvals)))
            if win_size < MINIMUM_ALLOWED_WIN_SIZE:
                win_size = MINIMUM_ALLOWED_WIN_SIZE
        else:
            win_size = int((self._fit_window_size - 1) / 2)
        return win_size

    def _load_data(self):
        xvals = self.getProperty("InputWorkspace").value.readX(0).copy()
        flat_yvals = self.getProperty("InputWorkspace").value.readY(0).copy()
        baseline = self.getProperty("InputWorkspace").value.readY(1).copy()
        errors = self.getProperty("InputWorkspace").value.readE(0).copy()
        xvals = xvals[np.isfinite(flat_yvals)]
        flat_yvals = flat_yvals[np.isfinite(flat_yvals)]

        return xvals, flat_yvals, baseline, errors

    def _recentre_peak_position(self, xvals, flat_yvals):
        approx_peak_xvals = self.getProperty("PeakGuessTable").value.column(0)[:]
        peakids = []
        for peakx in approx_peak_xvals:
            mask = np.logical_and(peakx - self._centre_tolerance <= xvals, peakx + self._centre_tolerance >= xvals)
            relative_id = np.argmax(flat_yvals[mask])
            offset_id = np.argwhere(mask)[0, 0]
            peakids.append(relative_id + offset_id)

        return peakids

    def _evaluate_final_cost(self, xvals, flat_yvals, baseline, errors, peak_table, refit_peak_table):
        chi2 = self.evaluate_cost(xvals, flat_yvals, baseline, errors, peak_table, refit_peak_table, use_poisson=False)
        poisson = self.evaluate_cost(xvals, flat_yvals, baseline, errors, peak_table, refit_peak_table, use_poisson=True)
        fit_cost = CreateEmptyTableWorkspace(StoreInADS=False)
        fit_cost.addColumn(type="float", name="Chi2")
        fit_cost.addColumn(type="float", name="Poisson")
        fit_cost.addRow([chi2, poisson])

        return fit_cost

    def parse_fit_table(self, param, data_table, refit=False):
        to_refit = []
        data_table.addColumn(type="float", name="centre")
        data_table.addColumn(type="float", name="error centre")
        data_table.addColumn(type="float", name="height")
        data_table.addColumn(type="float", name="error height")
        data_table.addColumn(type="float", name="sigma")
        data_table.addColumn(type="float", name="error sigma")
        data_table.addColumn(type="float", name="area")
        data_table.addColumn(type="float", name="error area")
        if type(param) is type(data_table):
            for i in range(param.rowCount() // 3):
                height = param.row(3 * i)
                centre = param.row(3 * i + 1)
                sigma = param.row(3 * i + 2)

                # Do not include peaks with high errors, they need to be refitted
                if refit and (
                    any(np.isnan([centre["Error"], height["Error"], sigma["Error"]]))
                    or centre["Value"] == 0.0
                    or centre["Error"] / centre["Value"] > 1.0
                    or height["Value"] == 0.0
                    or height["Error"] / height["Value"] > 1.0
                    or sigma["Value"] == 0.0
                    or sigma["Error"] / sigma["Value"] > 1.0
                ):
                    to_refit.append((centre["Value"], height["Value"], sigma["Value"]))
                    continue

                # Area defined by integral of gaussian peak of given height and width
                area = np.sqrt(2 * np.pi) * sigma["Value"] * height["Value"]
                error_area = np.power(sigma["Error"] / sigma["Value"], 2)
                error_area += np.power(height["Error"] / height["Value"], 2)
                error_area = np.sqrt(error_area)

                data_table.addRow(
                    [centre["Value"], centre["Error"], height["Value"], height["Error"], sigma["Value"], sigma["Error"], area, error_area]
                )

        return to_refit

    def gaussian_peak(self, xvals, centre, height, sigma):
        exp_val = (xvals - centre) / (np.sqrt(2) * sigma)

        return height * np.exp(-exp_val * exp_val)

    def gaussian_peak_background(self, params, xvals, yvals):
        A0, A1, centre, height, sigma = tuple(params)
        return A0 + A1 * xvals + self.gaussian_peak(xvals, centre, height, sigma) - yvals

    def multi_peak(self, params, xvals, yvals):
        model = np.zeros(len(xvals))
        for i in range(len(params) // 3):
            centre = params[3 * i]
            height = params[3 * i + 1]
            sigma = params[3 * i + 2]
            model += self.gaussian_peak(xvals, centre, height, sigma)

        return model - yvals

    def function_difference(self, yval1, yval2, errors):
        bad_idx = np.argwhere(errors <= 0)
        for idx in bad_idx:
            if yval1[idx[0]] > 0:
                errors[idx[0]] = np.sqrt(yval1[idx[0]])
            elif yval2[idx[0]] > 0:
                errors[idx[0]] = np.sqrt(yval2[idx[0]])
            else:
                errors[idx[0]] = 1.0

        return np.sum(np.power(np.abs(yval1 - yval2) / errors, 2)) / len(yval1)

    # Return the log of the product of the single probabilities as given by
    # p_i = exp(-model_i * data_i*ln(model_i))
    def poisson_cost(self, y_data, y_fit):
        y_data = y_data[np.nonzero(y_fit)]
        y_fit = y_fit[np.nonzero(y_fit)]
        y_fit = y_fit[np.nonzero(y_data)]
        y_data = y_data[np.nonzero(y_data)]
        if len(y_fit) < 1:
            return -np.inf
        y_log = np.log(np.abs(y_fit))
        return np.sum(-y_fit + y_data * y_log)

    def evaluate_cost(self, xvals, yvals, baseline, errors, peak_param, refit_peak_param, use_poisson):
        params = []
        for i in range(peak_param.rowCount()):
            row = peak_param.row(i)
            params.append(row["centre"])
            params.append(row["height"])
            params.append(row["sigma"])
        for i in range(refit_peak_param.rowCount()):
            row = refit_peak_param.row(i)
            params.append(row["centre"])
            params.append(row["height"])
            params.append(row["sigma"])

        model_yvals = self.multi_peak(params, xvals, np.zeros(len(xvals)))
        if use_poisson:
            # The cost function takes the log of the model. Hence behaves badly with data close to 0.
            # Returning the data to the original form by adding the baseline will move it away from 0 and solve
            #   the problem.
            return self.poisson_cost(yvals + baseline, model_yvals + baseline)
        else:
            return self.function_difference(yvals, model_yvals, errors)

    def estimate_single_parameters(self, xvals, yvals, centre, win_size):
        lside = max(0, centre - win_size)
        rside = min(len(yvals), centre + win_size)
        if rside - lside < MINIMUM_NUMBER_OF_DATA_POINTS:
            rside = MINIMUM_NUMBER_OF_DATA_POINTS + lside
        x_range = xvals[lside:rside]
        y_range = yvals[lside:rside]
        if len(x_range) < MINIMUM_NUMBER_OF_DATA_POINTS:
            self.log().warning(
                f"Peak at {xvals[centre]} cannot be fitted as there are not enough data points to fit"
                f" peak. This could be due to peak being at the end of the dataset"
            )
            return None
        # The function least_squares should not be used as it is not present in scipy < 1.1.0
        # Using the mantid fitting function will not produce an equally good result here
        p_est = scipy.optimize.leastsq(
            self.gaussian_peak_background,
            x0=np.array([np.average(y_range), 0, xvals[centre], yvals[centre], self._estimate_sigma]),
            args=(x_range, y_range),
        )[0]
        params = [p_est[2], p_est[0] + p_est[1] * p_est[2] + p_est[3], p_est[4]]

        return params

    def general_fit(self, xvals, yvals, peakids):
        if not peakids:
            return np.zeros(len(yvals)), None

        fit_func = ""
        fit_constr = ""
        for i, pid in enumerate(peakids):
            win_size = self._get_window_size(xvals)
            params = self.estimate_single_parameters(xvals, yvals, pid, win_size)
            if params is None:
                continue
            centre, height, sigma = params
            if sigma < self._min_sigma:
                centre = xvals[pid]
                height = yvals[pid]
                sigma = self._estimate_sigma
            fit_func += "name=Gaussian,PeakCentre={},Height={},Sigma={};".format(centre, height, sigma)
            fit_constr += "%f<f%d.PeakCentre<%f,%f<f%d.Height<%f,%f<f%d.Sigma<%d," % (
                xvals[pid] * (1 - self._general_tolerance),
                i,
                xvals[pid] * (1 + self._general_tolerance),
                yvals[pid] * (1 - self._general_tolerance),
                i,
                yvals[pid] * (1 + self._general_tolerance),
                self._min_sigma,
                i,
                self._max_sigma,
            )

        if fit_func == "":
            return yvals, self.function_difference(yvals, np.zeros(len(yvals)), np.sqrt(yvals))

        if fit_constr.count("PeakCentre") == 1:
            fit_constr = fit_constr.replace("f0.", "")

        _, _, _, param, fit_result, _, _ = Fit(
            Function=fit_func,
            InputWorkspace=self.getProperty("InputWorkspace").value,
            Output="fit_result",
            Minimizer="Levenberg-MarquardtMD",
            OutputCompositeMembers=True,
            StartX=min(xvals),
            EndX=max(xvals),
            Constraints=fit_constr,
            StoreInADS=False,
        )

        return fit_result.readY(1).copy(), param

    def refit_peaks(self, bad_params):
        xvals = self.getProperty("InputWorkspace").value.readX(0).copy()

        if not bad_params:
            return np.zeros(len(xvals)), None

        fit_func = ""
        fit_constr = ""
        for i, (centre, height, width) in enumerate(bad_params):
            fit_func += "name=Gaussian,PeakCentre=%f,Height=%f,Sigma=%f;" % (centre, height, width)
            fit_constr += "%f<f%d.PeakCentre<%f,%f<f%d.Height<%f,%f<f%d.Sigma<%d," % (
                centre * (1 - self._refit_tolerance),
                i,
                centre * (1 + self._refit_tolerance),
                height * (1 - self._refit_tolerance),
                i,
                height * (1 + self._refit_tolerance),
                self._min_sigma,
                i,
                self._max_sigma,
            )

        if fit_constr.count("PeakCentre") == 1:
            fit_constr = fit_constr.replace("f0.", "")

        _, _, _, param, fit_result, _, _ = Fit(
            Function=fit_func,
            InputWorkspace=self.getProperty("InputWorkspace").value,
            Output="fit_result",
            Minimizer="Levenberg-MarquardtMD",
            OutputCompositeMembers=True,
            StartX=min(xvals),
            EndX=max(xvals),
            Constraints=fit_constr,
            StoreInADS=False,
        )

        return fit_result.readY(1).copy(), param


AlgorithmFactory.subscribe(FitGaussianPeaks)
