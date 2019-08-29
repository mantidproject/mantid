.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm fits and functionalizes an incident spectrum and finds its first derivative.
FitIncidentSpectrum is able to fit an incident spectrum using:

1. CubicSpline: A fit using a cubic cline.
2. CubicSplineViaMantid: A fit with cubic spline using the mantid SplineSmoothing algorithm.
3. GaussConvCubicSpline: A fit with Cubic Spline using a Gaussian Convolution to get weights
4. HowellsFunction: A fit with analytical function from Howells et. al.

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
    binning_incident = "%s,%s,%s" % (0.2, 0.01, 4.0)
    binning_for_calc = "%s,%s,%s" % (0.2, 0.2, 4.0)
    binning_for_fit = "%s,%s,%s" % (0.2, 0.01, 4.0)
    incident_wksp = CreateWorkspace(
        OutputWorkspace=incident_wksp_name,
        NSpec=1,
        DataX=[0],
        DataY=[0],
        UnitX='Wavelength',
        VerticalAxisUnit='Text',
        VerticalAxisValues='IncidentSpectrum')
    incident_wksp = Rebin(InputWorkspace=incident_wksp, Params=binning_incident)
    incident_wksp = ConvertToPointData(InputWorkspace=incident_wksp)


    # Spectrum function given in Milder et al. Eq (5)
    def incidentSpectrum(wavelengths, phiMax, phiEpi, alpha, lambda1, lambda2,
                         lamdaT):
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

    # Calculate the efficiency correction for Alpha=0.693
    # and back calculate measured spectrum
    eff_wksp = CalculateEfficiencyCorrection(
        InputWorkspace=incident_wksp, Alpha=0.693)
    measured_wksp = Divide(LHSWorkspace=incident_wksp, RHSWorkspace=eff_wksp)

    # Fit incident spectrum
    prefix = "incident_spectrum_fit_with_"

    fit_gauss_conv_spline = prefix + "_gauss_conv_spline"
    FitIncidentSpectrum(
        InputWorkspace=incident_wksp,
        OutputWorkspace=fit_gauss_conv_spline,
        BinningForCalc=binning_for_calc,
        BinningForFit=binning_for_fit,
        FitSpectrumWith="GaussConvCubicSpline")

    # Retrieve workspaces
    wksp_fit_gauss_conv_spline = AnalysisDataService.retrieve(
        fit_gauss_conv_spline)

    print(wksp_fit_gauss_conv_spline.readY(0))

Output:

.. testoutput:: ExFitIncidentSpectrum

    [ 5328.83700775  2330.08408285  1600.78200105  2543.59379589  3249.78956903
      2797.87138465  2050.3366076   1417.4868309    965.23854845   659.79544224
       456.54322031   320.88688262   229.29830975   166.5536716    122.89703604
        92.0419568     69.89199835    53.75902111    41.84355559]

References
------------

.. [1] W. S. Howells (1983) *On the Choice of Moderator for Liquids Diffractometer on a Pulsed Neutron Source*, Nuclear Instruments and Methods in Physics Research 223 141-146 `doi: 10.1016/0167-5087(84)90256-4 <https://doi.org/10.1016/0167-5087(84)90256-4>`__
.. [2] D. F. R. Mildner, B. C. Boland, R. N. Sinclair, C. G. Windsor, L. J. Bunce, and J. H. Clarke (1977) *A Cooled Polyethylene Moderator on a Pulsed Neutron Source*, Nuclear Instruments and Methods 152 437-446 `doi: 10.1016/0029-554X(78)90043-5 <https://doi.org/10.1016/0029-554X(78)90043-5>`__

.. categories::

.. sourcelink::