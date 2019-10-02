# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, division, print_function
from copy import copy
import numpy as np
from scipy import signal, ndimage, interpolate
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, StringListValidator, FloatArrayProperty, RebinParamsValidator
from mantid.simpleapi import CreateWorkspace, Rebin, SplineSmoothing


class FitIncidentSpectrum(PythonAlgorithm):
    _input_ws = None
    _output_ws = None
    _scipy_not_old = hasattr(interpolate.UnivariateSpline, "derivative")
    # check if scipy version is greater than 0.12.1 i.e it has the derivative function

    def category(self):
        return 'Diffraction\\Fitting'

    def name(self):
        return 'FitIncidentSpectrum'

    def summary(self):
        return 'Calculate a fit for an incident spectrum using different methods. ' \
               'Outputs a workspace containing the functionalized fit and its first ' \
               'derivative.'

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty('InputWorkspace', '',
                                    direction=Direction.Input,),
            doc='Incident spectrum to be fit.')

        self.declareProperty(
            MatrixWorkspaceProperty('OutputWorkspace', '',
                                    direction=Direction.Output),
            doc='Output workspace containing the fit and it\'s first derivative.')

        self.declareProperty(FloatArrayProperty(name="BinningForCalc",
                                                validator=RebinParamsValidator(AllowEmpty=True),
                                                direction=Direction.Input),
                             doc='Bin range for calculation given as an array of floats in the same format as `Rebin`: '
                                 '[Start],[Increment],[End]. If empty use default binning. The calculated '
                                 'spectrum will use this binning')

        self.declareProperty(FloatArrayProperty(name="BinningForFit",
                                                validator=RebinParamsValidator(AllowEmpty=True),
                                                direction=Direction.Input),
                             doc='Bin range for fitting given as an array of floats in the same format as `Rebin`: '
                                 '[Start],[Increment],[End]. If empty use BinningForCalc. The '
                                 'incident spectrum will be rebined to this range before being fit.')

        self.declareProperty(
            name='FitSpectrumWith',
            defaultValue='GaussConvCubicSpline',
            validator=StringListValidator(['GaussConvCubicSpline', 'CubicSpline', 'CubicSplineViaMantid']),
            doc='The method for fitting the incident spectrum.')

    def _setup(self):
        self._input_ws = self.getProperty('InputWorkspace').value
        self._output_ws = self.getProperty('OutputWorkspace').valueAsStr
        self._binning_for_fit = self.getProperty('BinningForFit').value
        self._fit_spectrum_with = self.getProperty('FitSpectrumWith').value
        self._binning_for_calc = self.getProperty('BinningForCalc').value
        if not self._binning_for_calc.all():
            x = self._input_ws.readX(0)
            self._binning_for_calc = [str(i) for i in [min(x), x[1] - x[0], max(x) + x[1] - x[0]]]

    def PyExec(self):
        self._setup()

        x = np.arange(self._binning_for_calc[0], self._binning_for_calc[2], self._binning_for_calc[1])
        incident_index = 0
        if self._binning_for_fit.size == 0:
            x_fit = np.array(self._input_ws.readX(incident_index))
            y_fit = np.array(self._input_ws.readY(incident_index))
        else:
            rebinned = Rebin(
                self._input_ws,
                Params=self._binning_for_fit,
                PreserveEvents=True,
                StoreInADS=False)
            x_fit = np.array(rebinned.readX(incident_index))
            y_fit = np.array(rebinned.readY(incident_index))

        if len(x_fit) != len(y_fit):
            x_fit = x_fit[:-1]

        if self._fit_spectrum_with == 'CubicSpline':
            # Fit using cubic spline
            fit, fit_prime = self.fit_cubic_spline(x_fit, y_fit, x, s=1e7)
        elif self._fit_spectrum_with == 'CubicSplineViaMantid':
            # Fit using cubic spline via Mantid
            fit, fit_prime = self.fit_cubic_spline_via_mantid_spline_smoothing(
                self._input_ws,
                params_input=self._binning_for_fit,
                params_output=self._binning_for_calc,
                Error=0.0001,
                MaxNumberOfBreaks=0)
        elif self._fit_spectrum_with == 'GaussConvCubicSpline':
            # Fit using Gauss conv cubic spline
            fit, fit_prime = self.fit_cubic_spline_with_gauss_conv(x_fit, y_fit, x, sigma=0.5)

        # Create output workspace
        output_workspace = CreateWorkspace(
            DataX=x,
            DataY=np.append(fit, fit_prime),
            UnitX='Wavelength',
            NSpec=2,
            Distribution=False,
            ParentWorkspace=self._input_ws,
            StoreInADS=False)
        self.setProperty("OutputWorkspace", output_workspace)

    def fit_cubic_spline_with_gauss_conv(self, x_fit, y_fit, x, n_gouss=39, sigma=3):
        # Fit with Cubic Spline using a Gaussian Convolution to get weights
        def moving_average(y, n=n_gouss, sig=sigma):
            b = signal.gaussian(n, sig)
            average = ndimage.filters.convolve1d(y, b / b.sum())
            var = ndimage.filters.convolve1d(np.power(y - average, 2), b / b.sum())
            return average, var

        avg, var = moving_average(y_fit)
        spline_fit = interpolate.UnivariateSpline(x_fit, y_fit, w=1. / np.sqrt(var))
        fit = spline_fit(x)

        if self._scipy_not_old:
            spline_fit_prime = spline_fit.derivative()
            fit_prime = spline_fit_prime(x)
        else:
            index = np.arange(len(x))
            fit_prime = np.empty(len(x))
            for pos in index:
                    dx = (x[1] - x[0])/1000
                    y1 = spline_fit(x[pos] - dx)
                    y2 = spline_fit(x[pos] + dx)
                    fit_prime[pos] = (y2-y1)/dx
        return fit, fit_prime

    def fit_cubic_spline(self, x_fit, y_fit, x, s=1e15):
        # Fit with Cubic Spline
        tck = interpolate.splrep(x_fit, y_fit, s=s)
        fit = interpolate.splev(x, tck, der=0)
        fit_prime = interpolate.splev(x, tck, der=1)
        return fit, fit_prime

    def fit_cubic_spline_via_mantid_spline_smoothing(self, InputWorkspace, params_input, params_output, **kwargs):
        # Fit with Cubic Spline using the mantid SplineSmoothing algorithm
        rebinned = Rebin(
            InputWorkspace=InputWorkspace,
            Params=params_input,
            PreserveEvents=True,
            StoreInADS=False)
        fit_tuple = SplineSmoothing(
            InputWorkspace=rebinned,
            OutputWorkspaceDeriv='fit_prime',
            DerivOrder=1,
            StoreInADS=False,
            **kwargs)
        fit = Rebin(
            InputWorkspace=fit_tuple.OutputWorkspace,
            Params=params_output,
            PreserveEvents=True,
            StoreInADS=False)
        fit_prime = Rebin(
            InputWorkspace=fit_tuple.OutputWorkspaceDeriv[0],
            Params=params_output,
            PreserveEvents=True,
            StoreInADS=False)
        fit_array = copy(fit.readY(0))
        fit_prime_array = copy(fit_prime.readY(0))
        return fit_array, fit_prime_array


AlgorithmFactory.subscribe(FitIncidentSpectrum)
