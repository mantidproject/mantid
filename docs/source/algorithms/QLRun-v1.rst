.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The model that is being fitted is that of a \delta-function (elastic component) of amplitude A(0) and Lorentzians of amplitude A(j) and HWHM W(j) where j=1,2,3. The whole function is then convolved with the resolution function. The -function and Lorentzians are intrinsically normalised to unity so that the amplitudes represent their integrated areas.

For a Lorentzian, the Fourier transform does the conversion: 1/(x^{2}+\delta^{2}) \Leftrightarrow exp[-2\pi(\delta k)]. If x is identified with energy E and 2\pi k with t/\hbar where t is time then: 1/[E^{2}+(\hbar / \tau)^{2}] \Leftrightarrow exp[-t
/\tau] and \sigma is identified with \hbar / \tau. The program estimates the quasielastic components of each of the groups of spectra and requires the resolution file and optionally the normalisation file created by ResNorm.

For a Stretched Exponential, the choice of several Lorentzians is replaced with a single function with the shape : \psi\beta(x) \Leftrightarrow
exp[-2\pi(\sigma k)\beta]. This, in the energy to time FT transformation, is \psi\beta(E) \Leftrightarrow exp[-(t/\tau)\beta]. So \sigma is identified with (2\pi)\beta\hbar/\tau . The model that is fitted is that of an elastic component and the stretched exponential and the program gives the best estimate for the \beta parameter and the width for each group of spectra.

Usage
-----
**Example - a basic example using QLRun**

    sample = Load('irs26176_graphite002_red.nxs')
    resolution = Load('irs26173_graphite002_red.nxs')

    QLRun(Program="QL", SampleWorkspace=sample, ResolutionWorkspace=resolution, MinRange=-0.547607, MaxRange=0.543216, SampleBins=1, ResolutionBins=1, Elastic=false, Background="Sloping", FixedWidth=false, UseResNorm=false, WidthFile="", Loop=true, Save=false, Plot="None")

.. code-block:: python

.. categories::

.. sourcelink::