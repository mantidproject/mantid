.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm fits and functionalizes an incident spectrum and finds its first derivative

Usage
-----

**Example: fit an incident spectrum using GaussConvCubicSpline**

.. testcode:: ExFitIncidentSpectrum

    import numpy as np
    import matplotlib.pyplot as plt
    from mantid.simpleapi import \
        AnalysisDataService, \
        CalculateEfficiencyCorrection, \
        ConvertToPointData, \
        CreateWorkspace, \
        Divide, \
        FitIncidentSpectrum, \
        Rebin

    # Create the workspace to hold the already corrected incident spectrum
    incident_wksp_name = 'incident_spectrum_wksp'
    binning = "%s,%s,%s" % (0.2, 0.01, 4.0)
    incident_wksp = CreateWorkspace(
        OutputWorkspace=incident_wksp_name,
        NSpec=1,
        DataX=[0],
        DataY=[0],
        UnitX='Wavelength',
        VerticalAxisUnit='Text',
        VerticalAxisValues='IncidentSpectrum')
    incident_wksp = Rebin(InputWorkspace=incident_wksp, Params=binning)
    incident_wksp = ConvertToPointData(InputWorkspace=incident_wksp)


    # Spectrum function given in Milder et al. Eq (5)
    def incidentSpectrum(wavelengths, phiMax, phiEpi, alpha, lambda1, lambda2,
                         lambdaT):
        deltaTerm = 1. / (1. + np.exp((wavelengths - lambda1) / lambda2))
        term1 = phiMax * (
            lambdaT**4. / wavelengths**5.) * np.exp(-(lambdaT / wavelengths)**2.)
        term2 = phiEpi * deltaTerm / (wavelengths**(1 + 2 * alpha))
        return term1 + term2


    # Variables for polyethlyene moderator at 300K
    phiMax = 6324
    phiEpi = 786
    alpha = 0.099
    lambda1 = 0.67143
    lambda2 = 0.06075
    lambdaT = 1.58

    # Add the incident spectrum to the workspace
    corrected_spectrum = incidentSpectrum(
        incident_wksp.readX(0), phiMax, phiEpi, alpha, lambda1, lambda2, lambdaT)
    incident_wksp.setY(0, corrected_spectrum)

    # Fit incident spectrum
    prefix = "incident_spectrum_fit_with"

    fit_gauss_conv_spline = prefix + "_gauss_conv_spline"
    FitIncidentSpectrum(
        InputWorkspace=incident_wksp,
        OutputWorkspace=fit_gauss_conv_spline,
        BinningForCalc=binning,
        BinningForFit=binning,
        FitSpectrumWith="GaussConvCubicSpline")

    # Retrieve workspaces
    wksp_fit_gauss_conv_spline = AnalysisDataService.retrieve(fit_gauss_conv_spline)

.. testcleanup:: ExFitIncidentSpectrum

    DeleteWorkspace('eff_wksp')
    DeleteWorkspace('incident_wksp')
    DeleteWorkspace('measured_wksp')
    DeleteWorkspace('incident_spectrum_wksp')
    DeleteWorkspace('fit')
    DeleteWorkspace('fit_prime_1')
    DeleteWorkspace('incident_spectrum_fit_with_gauss_conv_spline')

Output:

.. testoutput:: ExFitIncidentSpectrum

   the fitted peak: centre=2.05, sigma=0.70

.. categories::

.. sourcelink::