
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

**Example - QLRun**

.. testcode:: QLRunExample

    # Load in test data
    sampleWs = Load('irs26176_graphite002_red.nxs')
    resWs = Load('irs26173_graphite002_red.nxs')

    # Run QLRun algorithm
    QLRun(Program="QL", SampleWorkspace=sampleWs, ResolutionWorkspace=resWs, MinRange=-0.547607, MaxRange=0.543216, SampleBins=1, ResolutionBins=1, Elastic=False, Background="Sloping", FixedWidth=False, UseResNorm=False, WidthFile="", Loop=True, Save=False, Plot="None")
    outputName = 'samp_QLd_Workspace_0'

    # capture first fitted workspace
    result_ws = mtd[outputName]

    # Print the result
    print "The Y values of the first fitted workspace are:"
    print "data  : %.5f" %(result_ws.readY(0)[0])
    print "fit.1 : %.5f" %(result_ws.readY(1)[0])
    print "diff.1: %.5f" %(result_ws.readY(2)[0])
    print "fit.2 : %.5f" %(result_ws.readY(3)[0])
    print "diff.2: %.5f" %(result_ws.readY(4)[0])

Output:

.. testoutput:: QLRunExample
    :options: +NORMALIZE_WHITESPACE

    The Y values of the first fitted workspace are:
    data  : 0.02540
    fit.1 : 0.01906
    diff.1: -0.00635
    fit.2 : 0.01732
    diff.2: -0.00808

.. categories::

.. sourcelink::