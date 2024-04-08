# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from copy import copy
import numpy as np
from scipy import ndimage, interpolate
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, IntListValidator, StringListValidator, FloatArrayProperty, RebinParamsValidator
from mantid.simpleapi import CreateWorkspace, Rebin, SplineSmoothing


class FitIncidentSpectrum(PythonAlgorithm):
    _input_ws = None
    _output_ws = None

    def category(self):
        return "Diffraction\\Fitting"

    def name(self):
        return "FitIncidentSpectrum"

    def summary(self):
        return (
            "Calculate a fit for an incident spectrum using different methods. "
            "Outputs a workspace containing the functionalized fit, its first "
            "derivative and (optionally) its second derivative."
        )

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty(
                "InputWorkspace",
                "",
                direction=Direction.Input,
            ),
            doc="Incident spectrum to be fit.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="Output workspace containing the fit and it's first derivative.",
        )

        self.declareProperty(
            name="WorkspaceIndex", defaultValue=0, doc="Workspace index of the spectra to be fitted (Defaults to the first index.)"
        )

        self.declareProperty(
            FloatArrayProperty(name="BinningForCalc", validator=RebinParamsValidator(AllowEmpty=True), direction=Direction.Input),
            doc="Bin range for calculation given as an array of floats in the same format as `Rebin`: "
            "[Start],[Increment],[End]. If empty use default binning. The calculated "
            "spectrum will use this binning",
        )

        self.declareProperty(
            FloatArrayProperty(name="BinningForFit", validator=RebinParamsValidator(AllowEmpty=True), direction=Direction.Input),
            doc="Bin range for fitting given as an array of floats in the same format as `Rebin`: "
            "[Start],[Increment],[End]. If empty use BinningForCalc. The "
            "incident spectrum will be rebined to this range before being fit.",
        )

        self.declareProperty(
            name="FitSpectrumWith",
            defaultValue="GaussConvCubicSpline",
            validator=StringListValidator(["GaussConvCubicSpline", "CubicSpline", "CubicSplineViaMantid"]),
            doc="The method for fitting the incident spectrum.",
        )

        self.declareProperty(
            name="DerivOrder",
            defaultValue=1,
            validator=IntListValidator([1, 2]),
            doc="Whether to return the first or first and second derivative of the fit function",
        )

    def _setup(self):
        self._input_ws = self.getProperty("InputWorkspace").value
        self._output_ws = self.getProperty("OutputWorkspace").valueAsStr
        self._incident_index = self.getProperty("WorkspaceIndex").value
        self._binning_for_calc = self.getProperty("BinningForCalc").value
        self._binning_for_fit = self.getProperty("BinningForFit").value
        self._fit_spectrum_with = self.getProperty("FitSpectrumWith").value
        self._deriv_order = self.getProperty("DerivOrder").value

    def PyExec(self):
        self._setup()
        if self._binning_for_calc.size == 0:
            x = np.array(self._input_ws.readX(self._incident_index))
            self._binning_for_calc = [i for i in [min(x), x[1] - x[0], max(x) + x[1] - x[0]]]
        else:
            x = np.arange(self._binning_for_calc[0], self._binning_for_calc[2], self._binning_for_calc[1])
        if self._binning_for_fit.size == 0:
            x_fit = np.array(self._input_ws.readX(self._incident_index))
            y_fit = np.array(self._input_ws.readY(self._incident_index))
        else:
            rebinned = Rebin(InputWorkspace=self._input_ws, Params=self._binning_for_fit, PreserveEvents=True, StoreInADS=False)
            x_fit = np.array(rebinned.readX(self._incident_index))
            y_fit = np.array(rebinned.readY(self._incident_index))

        rebin_norm = x.size / x_fit.size
        x_bin_centers = 0.5 * (x[:-1] + x[1:])
        if len(x_fit) != len(y_fit):
            x_fit = 0.5 * (x_fit[:-1] + x_fit[1:])

        if self._fit_spectrum_with == "CubicSpline":
            # Fit using cubic spline
            fit, fit_prime, fit_prime_prime = self.fit_cubic_spline(x_fit, y_fit, x_bin_centers, s=1e7)
        elif self._fit_spectrum_with == "CubicSplineViaMantid":
            # Fit using cubic spline via Mantid
            fit, fit_prime, fit_prime_prime = self.fit_cubic_spline_via_mantid_spline_smoothing(
                self._input_ws, params_input=self._binning_for_fit, params_output=self._binning_for_calc, Error=0.0001, MaxNumberOfBreaks=0
            )
        elif self._fit_spectrum_with == "GaussConvCubicSpline":
            # Fit using Gauss conv cubic spline
            fit, fit_prime, fit_prime_prime = self.fit_cubic_spline_with_gauss_conv(x_fit, y_fit, x_bin_centers, sigma=0.5)

        # Create output workspace
        unit = self._input_ws.getAxis(0).getUnit().unitID()
        data_y = fit
        if self._deriv_order >= 1:
            data_y = np.append(data_y, fit_prime)
        if self._deriv_order >= 2:
            data_y = np.append(data_y, fit_prime_prime)
        data_y /= rebin_norm
        output_workspace = CreateWorkspace(
            DataX=x,
            DataY=data_y,
            UnitX=unit,
            NSpec=1 + self._deriv_order,
            Distribution=False,
            ParentWorkspace=self._input_ws,
            StoreInADS=False,
        )
        self.setProperty("OutputWorkspace", output_workspace)

    def fit_cubic_spline_with_gauss_conv(self, x_fit, y_fit, x, n_gouss=39, sigma=3.0):
        # Fit with Cubic Spline using a Gaussian Convolution to get weights
        def moving_average(y, n=n_gouss, sig=sigma):
            from scipy.signal.windows import gaussian

            b = gaussian(n, sig)
            average = ndimage.filters.convolve1d(y, b / b.sum())
            var = ndimage.filters.convolve1d(np.power(y - average, 2), b / b.sum())
            return average, var

        avg, var = moving_average(y_fit)
        spline_fit = interpolate.UnivariateSpline(x_fit, y_fit, w=1.0 / np.sqrt(var))
        fit = spline_fit(x)
        fit_primes = [None, None]
        for i in range(self._deriv_order):
            spline_fit_prime = spline_fit.derivative(i + 1)
            fit_primes[i] = spline_fit_prime(x)
        return fit, fit_primes[0], fit_primes[1]

    def fit_cubic_spline(self, x_fit, y_fit, x, s=1e15):
        # Fit with Cubic Spline
        tck = interpolate.splrep(x_fit, y_fit, s=s)
        fit = interpolate.splev(x, tck, der=0)
        fit_primes = [None, None]
        for i in range(self._deriv_order):
            fit_primes[i] = interpolate.splev(x, tck, der=i + 1)
        return fit, fit_primes[0], fit_primes[1]

    def fit_cubic_spline_via_mantid_spline_smoothing(self, InputWorkspace, params_input, params_output, **kwargs):
        # Fit with Cubic Spline using the mantid SplineSmoothing algorithm
        rebinned = Rebin(InputWorkspace=InputWorkspace, Params=params_input, PreserveEvents=True, StoreInADS=False)
        fit_tuple = SplineSmoothing(
            InputWorkspace=rebinned, OutputWorkspaceDeriv="fit_primes", DerivOrder=self._deriv_order, StoreInADS=False, **kwargs
        )
        fit = Rebin(InputWorkspace=fit_tuple.OutputWorkspace, Params=params_output, PreserveEvents=True, StoreInADS=False)
        fit_primes = Rebin(InputWorkspace=fit_tuple.OutputWorkspaceDeriv[0], Params=params_output, PreserveEvents=True, StoreInADS=False)
        fit_array = copy(fit.readY(0))
        fit_primes_array = [None, None]
        for i in range(self._deriv_order):
            fit_primes_array[i] = copy(fit_primes.readY(i))
        return fit_array, fit_primes_array[0], fit_primes_array[1]


AlgorithmFactory.subscribe(FitIncidentSpectrum)
