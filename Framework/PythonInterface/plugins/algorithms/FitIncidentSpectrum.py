# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, division, print_function
import numpy as np
from scipy import signal, ndimage, interpolate, optimize
from mantid.api import AlgorithmFactory, CommonBinsValidator, MatrixWorkspaceProperty, PythonAlgorithm, PropertyMode
from mantid.kernel import Direction, StringListValidator, StringMandatoryValidator
from mantid.simpleapi import CreateWorkspace, Rebin, SplineSmoothing, AnalysisDataService


class FitIncidentSpectrum(PythonAlgorithm):
    _input_ws = None
    _output_ws = None

    def category(self):
        return 'Diffraction\\Fitting'

    def name(self):
        return 'FitIncidentSpectrum'

    def summary(self):
        return 'Calculate a fit for an incident spectrum using different methods.' \
               'Outputs a workspace containing the functionalized fit and its first' \
               'derivative.'

    def seeAlso(self):
        return [""]

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty('InputWorkspace', '',
                                    direction=Direction.Input,
                                    # TODO find out what validator is best for this
                                    # validator=CommonBinsValidator(),
                                    ),
            doc='Incident spectrum to be fit.')

        self.declareProperty(
            MatrixWorkspaceProperty('OutputWorkspace', '',
                                    direction=Direction.Output),
            doc='Output workspace containing the fit and it\'s first derivative.')

        self.declareProperty(
            name='BinningForFit',
            defaultValue='0.15,0.05,3.2',
            doc='Bin range for fitting given as a comma separated string in the format \"[Start],[Increment],[End]\".')

        self.declareProperty(
            name='BinningForCalc',
            defaultValue='0.15,0.05,3.2',
            validator=StringMandatoryValidator(),
            doc='Bin range for calculation given as a comma separated string in the format '
                '\"[Start],[Increment],[End]\".')

        self.declareProperty(
            name='FitSpectrumWith',
            defaultValue='GaussConvCubicSpline',
            validator=StringListValidator(['GaussConvCubicSpline', 'CubicSpline', 'CubicSplineViaMantid',
                                           'HowellsFunction']),
            doc='The method for fitting the incident spectrum:'
                'GaussConvCubicSpline'
                'CubicSpline'
                'CubicSplineViaMantid'
                'HowellsFunction')

    def _setup(self):
        self._input_ws = self.getProperty('InputWorkspace').value
        self._output_ws = self.getProperty('OutputWorkspace').valueAsStr
        self._binning_for_fit = self.getProperty('BinningForFit').value
        self._fit_spectrum_with = self.getPropertyValue('FitSpectrumWith')
        self._binning_for_calc = self.getPropertyValue('BinningForCalc')
        if self._binning_for_calc is None:
            x = AnalysisDataService.retrieve(self._input_ws).readX(0)
            binning = [str(i) for i in [min(x), x[1] - x[0], max(x) + x[1] - x[0]]]
            self.binning_for_calc = ",".join(binning)

    def PyExec(self):
        self._setup()
        x = self.parse_binning_for_calc(self._binning_for_calc)
        rebinned = Rebin(
            self._input_ws,
            Params=self._binning_for_fit,
            PreserveEvents=True,
            StoreInADS=False)
        incident_index = 0
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
                ParamsInput=self._binning_for_fit,
                ParamsOutput=self._binning_for_calc,
                Error=0.0001,
                MaxNumberOfBreaks=0)
        elif self._fit_spectrum_with == 'HowellsFunction':
            # Fit using Howells function
            fit, fit_prime = self.fit_howells_function(x_fit, y_fit, x)
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
            StoreInADS=False)
        self.setProperty("OutputWorkspace", output_workspace)

    def parse_binning_for_calc(self, binning_for_calc):
        try:
            params = [float(x) for x in binning_for_calc.split(',')]
        except AttributeError:
            params = [float(x) for x in binning_for_calc]
        xlo, binsize, xhi = params
        return np.arange(xlo, xhi, binsize)


    def fit_cubic_spline_with_gauss_conv(self, x_fit, y_fit, x, sigma=3):
        # Fit with Cubic Spline using a Gaussian Convolution to get weights
        def moving_average(y, sig=sigma):
            b = signal.gaussian(39, sig)
            average = ndimage.filters.convolve1d(y, b / b.sum())
            var = ndimage.filters.convolve1d(np.power(y - average, 2), b / b.sum())
            return average, var

        avg, var = moving_average(y_fit)
        spline_fit = interpolate.UnivariateSpline(x_fit, y_fit, w=1. / np.sqrt(var))
        spline_fit_prime = spline_fit.derivative()
        fit = spline_fit(x)
        fit_prime = spline_fit_prime(x)
        return fit, fit_prime

    def fit_cubic_spline(self, x_fit, y_fit, x, s=1e15):
        tck = interpolate.splrep(x_fit, y_fit, s=s)
        fit = interpolate.splev(x, tck, der=0)
        fit_prime = interpolate.splev(x, tck, der=1)
        return fit, fit_prime

    def fit_cubic_spline_via_mantid_spline_smoothing(self, InputWorkspace, ParamsInput, ParamsOutput, **kwargs):
        Rebin(
            InputWorkspace=InputWorkspace,
            OutputWorkspace='fit',
            Params=ParamsInput,
            PreserveEvents=True)
        SplineSmoothing(
            InputWorkspace='fit',
            OutputWorkspace='fit',
            OutputWorkspaceDeriv='fit_prime',
            DerivOrder=1,
            **kwargs)
        Rebin(
            InputWorkspace='fit',
            OutputWorkspace='fit',
            Params=ParamsOutput,
            PreserveEvents=True)
        Rebin(
            InputWorkspace='fit_prime_1',
            OutputWorkspace='fit_prime_1',
            Params=ParamsOutput,
            PreserveEvents=True)
        return AnalysisDataService.retrieve('fit').readY(0), AnalysisDataService.retrieve('fit_prime_1').readY(0)

    def fit_howells_function(self, x_fit, y_fit, x):
        # Fit with analytical function from HowellsEtAl
        def calc_howells_function(lambdas, phi_max, phi_epi, lam_t, lam_1, lam_2, a):
            term1 = phi_max * ((lam_t**4.) / lambdas**5.) * \
                np.exp(-(lam_t / lambdas)**2.)
            term2 = (phi_epi / (lambdas**(1. + 2. * a))) * \
                (1. / (1 + np.exp((lambdas - lam_1) / lam_2)))
            return term1 + term2

        def calc_howells_function_1st_derivative(lambdas, phi_max, phi_epi, lam_t, lam_1, lam_2, a):
            term1 = (((2 * lam_t**2) / lambdas**2) - 5.) * (1. / lambdas) * \
                phi_max * ((lam_t**4.) / lambdas**5.) * np.exp(-(lam_t / lambdas)**2.)
            term2 = ((1 + 2 * a) / lambdas) \
                * (1. / lambdas) * (phi_epi / (lambdas ** (1. + 2. * a))) \
                * (1. / (1 + np.exp((lambdas - lam_1) / lam_2)))
            return term1 + term2

        params = [1., 1., 1., 0., 1., 1.]
        params, convergence = optimize.curve_fit(
            calc_howells_function, x_fit, y_fit, params)
        fit = calc_howells_function(x, *params)
        fit_prime = calc_howells_function_1st_derivative(x, *params)
        return fit, fit_prime


AlgorithmFactory.subscribe(FitIncidentSpectrum)