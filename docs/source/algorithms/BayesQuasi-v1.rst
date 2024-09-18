
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

**This algorithm can only be run on windows due to f2py support and the underlying fortran code**

The model that is being fitted is that of a \delta-function (elastic component) of amplitude A(0)
and Lorentzians of amplitude A(j) and HWHM W(j) where j=1,2,3. The whole function is then convolved
with the resolution function. The -function and Lorentzians are intrinsically normalised to unity
so that the amplitudes represent their integrated areas.

For a Lorentzian, the Fourier transform does the conversion:
:math:`1/(x^{2}+\delta^{2}) \Leftrightarrow exp[-2\pi(\delta k)]`.
If x is identified with energy E and :math:`2\pi k` with :math:`t/\hbar` where t is time then:
:math:`1/[E^{2}+(\hbar / \tau)^{2}] \Leftrightarrow exp[-t/\tau]` and :math:`\sigma` is identified with :math:`\hbar / \tau.`
The program estimates the quasielastic components of each of the groups of spectra and requires the resolution
file and optionally the normalisation file created by ResNorm.

For a Stretched Exponential, the choice of several Lorentzians is replaced with a single function with the shape :
:math:`\psi\beta(x) \Leftrightarrow exp[-2\pi(\sigma k)\beta]`. This, in the energy to time FT transformation,
is :math:`\psi\beta(E) \Leftrightarrow exp[-(t/\tau)\beta]`. So :math:`\sigma` is identified with :math:`(2\pi)\beta\hbar/\tau`.
The model that is fitted is that of an elastic component and the stretched exponential and the program gives the best estimate
for the :math:`\beta` parameter and the width for each group of spectra.

Usage
-----

**Example - BayesQuasi**

.. testcode:: BayesQuasiExample

    # Load in test data
    sampleWs = Load('irs26176_graphite002_red.nxs')
    resWs = Load('irs26173_graphite002_red.nxs')

    # Run BayesQuasi algorithm
    fit_ws, result_ws, prob_ws = BayesQuasi(Program='QL', SampleWorkspace=sampleWs, ResolutionWorkspace=resWs,
                                        MinRange=-0.547607, MaxRange=0.543216, SampleBins=1, ResolutionBins=1,
                                        Elastic=False, Background='Sloping', FixedWidth=False, UseResNorm=False,
                                        WidthFile='', Loop=True)

.. categories::

.. sourcelink::
